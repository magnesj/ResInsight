/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2025- Equinor ASA
//
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
//
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////

#include "RiaOpenTelemetryManager.h"

#include "RiaLogging.h"
#include "RiaPreferencesOpenTelemetry.h"

#include <QString>

#include <algorithm>
#include <random>
#include <sstream>
#include <thread>

// Windows socket headers must be included before CURL
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <curl/curl.h>
#include <nlohmann/json.hpp>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RiaOpenTelemetryManager::HealthSnapshot::getSuccessRate() const
{
    uint64_t total = eventsSent + eventsDropped;
    if ( total == 0 ) return 1.0;
    return static_cast<double>( eventsSent ) / total;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaOpenTelemetryManager::HealthSnapshot::isHealthy() const
{
    // Consider healthy if success rate > 90% and we've had recent successful sends
    const auto now                  = std::chrono::steady_clock::now();
    const auto timeSinceLastSuccess = now - lastSuccessfulSend;

    return getSuccessRate() > 0.9 && timeSinceLastSuccess < std::chrono::minutes( 5 );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaOpenTelemetryManager& RiaOpenTelemetryManager::instance()
{
    static RiaOpenTelemetryManager instance;
    return instance;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaOpenTelemetryManager::RiaOpenTelemetryManager()
{
    m_healthMetrics.systemStartTime    = std::chrono::steady_clock::now();
    m_healthMetrics.lastSuccessfulSend = m_healthMetrics.systemStartTime;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaOpenTelemetryManager::~RiaOpenTelemetryManager()
{
    shutdown();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaOpenTelemetryManager::initialize()
{
    std::lock_guard<std::mutex> lock( m_configMutex );

    if ( m_initialized.load() )
    {
        return true;
    }

    if ( !initializeProvider() )
    {
        return false;
    }

    // Start worker thread
    m_isShuttingDown = false;
    m_workerThread   = std::make_unique<std::thread>( &RiaOpenTelemetryManager::workerThread, this );

    m_initialized = true;
    m_enabled     = true;

    RiaLogging::info( "OpenTelemetry initialized successfully" );

    // Send initial health span
    if ( m_healthMonitoringEnabled )
    {
        sendHealthSpan();
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOpenTelemetryManager::shutdown( std::chrono::seconds timeout )
{
    if ( !m_initialized.load() )
    {
        return;
    }

    RiaLogging::info( "Shutting down OpenTelemetry" );

    m_isShuttingDown = true;
    m_enabled        = false;

    // Flush pending events
    flushPendingEvents();

    // Wake up worker thread
    m_queueCondition.notify_all();

    // Wait for worker thread to finish with timeout
    if ( m_workerThread && m_workerThread->joinable() )
    {
        auto deadline = std::chrono::steady_clock::now() + timeout;

        // Try to join with timeout
        std::unique_lock<std::mutex> lock( m_queueMutex );
        bool                         finished = m_queueCondition.wait_until( lock, deadline, [this]() { return m_eventQueue.empty(); } );
        lock.unlock();

        m_workerThread->join();

        if ( !finished && !m_eventQueue.empty() )
        {
            RiaLogging::warning( QString( "OpenTelemetry shutdown timeout: %1 events lost" ).arg( m_eventQueue.size() ) );
        }
    }

    m_initialized = false;
    RiaLogging::info( "OpenTelemetry shutdown complete" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOpenTelemetryManager::reportEventAsync( const std::string& eventName, const std::map<std::string, std::string>& attributes )
{
    if ( !isEnabled() || isCircuitBreakerOpen() || !shouldSampleEvent() )
    {
        return;
    }

    std::unique_lock<std::mutex> lock( m_queueMutex );

    // Check queue size and apply backpressure
    if ( m_backpressureEnabled && m_eventQueue.size() >= m_maxQueueSize )
    {
        m_healthMetrics.eventsDropped++;
        return;
    }

    m_eventQueue.emplace( eventName, attributes );
    m_healthMetrics.eventsQueued++;

    lock.unlock();
    m_queueCondition.notify_one();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOpenTelemetryManager::reportCrash( int signalCode, const std::stacktrace& trace )
{
    if ( !isEnabled() )
    {
        return;
    }

    // Format stack trace using existing ResInsight formatter
    std::stringstream ss;
    int               frame = 0;
    for ( const auto& entry : trace )
    {
        ss << "  [" << frame++ << "] " << entry.description() << " at " << entry.source_file() << ":" << entry.source_line() << "\n";
    }

    std::string rawStackTrace = ss.str();

    std::map<std::string, std::string> attributes;
    attributes["crash.signal"]      = std::to_string( signalCode );
    attributes["crash.thread_id"]   = std::to_string( std::hash<std::thread::id>{}( std::this_thread::get_id() ) );
    attributes["crash.stack_trace"] = rawStackTrace;
    attributes["service.name"]      = RiaPreferencesOpenTelemetry::current()->serviceName().toStdString();
    attributes["service.version"]   = RiaPreferencesOpenTelemetry::current()->serviceVersion().toStdString();

    // Report with high priority (bypass sampling)
    std::unique_lock<std::mutex> lock( m_queueMutex );
    m_eventQueue.emplace( "crash.signal_handler", attributes );
    m_healthMetrics.eventsQueued++;
    lock.unlock();
    m_queueCondition.notify_one();

    RiaLogging::error( QString( "Crash reported to OpenTelemetry (signal: %1)" ).arg( signalCode ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOpenTelemetryManager::reportTestCrash( const std::stacktrace& trace )
{
    if ( !isEnabled() )
    {
        return;
    }

    // Format stack trace
    std::stringstream ss;
    int               frame = 0;
    for ( const auto& entry : trace )
    {
        ss << "  [" << frame++ << "] " << entry.description() << " at " << entry.source_file() << ":" << entry.source_line() << "\n";
    }

    std::string rawStackTrace = ss.str();

    std::map<std::string, std::string> attributes;
    attributes["test.type"]        = "manual_stack_trace";
    attributes["test.thread_id"]   = std::to_string( std::hash<std::thread::id>{}( std::this_thread::get_id() ) );
    attributes["test.stack_trace"] = rawStackTrace;
    attributes["service.name"]     = RiaPreferencesOpenTelemetry::current()->serviceName().toStdString();
    attributes["service.version"]  = RiaPreferencesOpenTelemetry::current()->serviceVersion().toStdString();

    reportEventAsync( "test.stack_trace", attributes );

    RiaLogging::info( "Test stack trace reported to OpenTelemetry" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaOpenTelemetryManager::isEnabled() const
{
    return m_enabled.load() && m_initialized.load();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaOpenTelemetryManager::isInitialized() const
{
    return m_initialized.load();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOpenTelemetryManager::setErrorCallback( ErrorCallback callback )
{
    std::lock_guard<std::mutex> lock( m_configMutex );
    m_errorCallback = callback;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOpenTelemetryManager::setMaxQueueSize( size_t maxEvents )
{
    std::lock_guard<std::mutex> lock( m_configMutex );
    m_maxQueueSize = maxEvents;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOpenTelemetryManager::enableBackpressure( bool enable )
{
    std::lock_guard<std::mutex> lock( m_configMutex );
    m_backpressureEnabled = enable;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOpenTelemetryManager::setMemoryThreshold( size_t maxMemoryMB )
{
    std::lock_guard<std::mutex> lock( m_configMutex );
    m_memoryThresholdMB = maxMemoryMB;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOpenTelemetryManager::setSamplingRate( double rate )
{
    std::lock_guard<std::mutex> lock( m_configMutex );
    m_samplingRate = std::clamp( rate, 0.0, 1.0 );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
size_t RiaOpenTelemetryManager::getCurrentQueueSize() const
{
    std::lock_guard<std::mutex> lock( m_queueMutex );
    return m_eventQueue.size();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaOpenTelemetryManager::HealthSnapshot RiaOpenTelemetryManager::getHealthMetrics() const
{
    HealthSnapshot result;
    result.eventsQueued       = m_healthMetrics.eventsQueued.load();
    result.eventsSent         = m_healthMetrics.eventsSent.load();
    result.eventsDropped      = m_healthMetrics.eventsDropped.load();
    result.networkFailures    = m_healthMetrics.networkFailures.load();
    result.lastSuccessfulSend = m_healthMetrics.lastSuccessfulSend;
    result.systemStartTime    = m_healthMetrics.systemStartTime;
    return result;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaOpenTelemetryManager::isHealthy() const
{
    return getHealthMetrics().isHealthy() && !isCircuitBreakerOpen();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOpenTelemetryManager::enableHealthMonitoring( bool enable )
{
    std::lock_guard<std::mutex> lock( m_configMutex );
    m_healthMonitoringEnabled = enable;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaOpenTelemetryManager::initializeProvider()
{
    try
    {
        if ( !createExporter() )
        {
            return false;
        }

        setupResourceAttributes();
        return true;
    }
    catch ( const std::exception& e )
    {
        handleError( TelemetryError::InternalError, QString( "Failed to initialize provider: %1" ).arg( e.what() ) );
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
/// Parse Azure Application Insights connection string
/// Format: InstrumentationKey=<key>;IngestionEndpoint=<endpoint>;...
//--------------------------------------------------------------------------------------------------
static std::map<QString, QString> parseAzureConnectionString( const QString& connectionString )
{
    std::map<QString, QString> result;
    QStringList                parts = connectionString.split( ';', Qt::SkipEmptyParts );

    for ( const QString& part : parts )
    {
        int equalPos = part.indexOf( '=' );
        if ( equalPos > 0 )
        {
            QString key   = part.left( equalPos ).trimmed();
            QString value = part.mid( equalPos + 1 ).trimmed();
            result[key]   = value;
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/// CURL write callback
//--------------------------------------------------------------------------------------------------
static size_t curlWriteCallback( void* contents, size_t size, size_t nmemb, std::string* userp )
{
    userp->append( (char*)contents, size * nmemb );
    return size * nmemb;
}

//--------------------------------------------------------------------------------------------------
/// Send telemetry to Azure Application Insights using REST API
//--------------------------------------------------------------------------------------------------
static bool sendToApplicationInsights( const std::string& jsonPayload, const QString& endpoint, int timeoutMs, std::string& errorMessage )
{
    CURL*       curl;
    CURLcode    res;
    std::string readBuffer;

    curl = curl_easy_init();
    if ( !curl )
    {
        errorMessage = "Failed to initialize CURL";
        return false;
    }

    std::string url = endpoint.toStdString() + "/v2/track";

    struct curl_slist* headers = nullptr;
    headers                    = curl_slist_append( headers, "Content-Type: application/json" );
    headers                    = curl_slist_append( headers, "Accept: application/json" );

    curl_easy_setopt( curl, CURLOPT_URL, url.c_str() );
    curl_easy_setopt( curl, CURLOPT_POSTFIELDS, jsonPayload.c_str() );
    curl_easy_setopt( curl, CURLOPT_HTTPHEADER, headers );
    curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, curlWriteCallback );
    curl_easy_setopt( curl, CURLOPT_WRITEDATA, &readBuffer );
    curl_easy_setopt( curl, CURLOPT_TIMEOUT_MS, timeoutMs );

    res = curl_easy_perform( curl );

    long response_code;
    curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &response_code );

    curl_slist_free_all( headers );
    curl_easy_cleanup( curl );

    if ( res != CURLE_OK )
    {
        errorMessage = std::string( "CURL failed: " ) + curl_easy_strerror( res );
        return false;
    }

    if ( response_code != 200 )
    {
        errorMessage = "HTTP error: " + std::to_string( response_code ) + " - " + readBuffer;
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaOpenTelemetryManager::createExporter()
{
    try
    {
        auto* prefs = RiaPreferencesOpenTelemetry::current();

        // Parse and validate connection string
        auto connectionParams = parseAzureConnectionString( prefs->connectionString() );
        if ( !connectionParams.contains( "InstrumentationKey" ) )
        {
            handleError( TelemetryError::ConfigurationError, "InstrumentationKey not found in connection string" );
            return false;
        }

        if ( !connectionParams.contains( "IngestionEndpoint" ) )
        {
            handleError( TelemetryError::ConfigurationError, "IngestionEndpoint not found in connection string" );
            return false;
        }

        // Using Application Insights REST API for telemetry
        RiaLogging::info( QString( "Application Insights REST API configured for production environment" ) );

        return true;
    }
    catch ( const std::exception& e )
    {
        handleError( TelemetryError::NetworkError, QString( "Failed to create exporter: %1" ).arg( e.what() ) );
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOpenTelemetryManager::setupResourceAttributes()
{
    // Resource attributes are typically set during provider creation
    // This would be expanded with system information, process details, etc.
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOpenTelemetryManager::workerThread()
{
    while ( !m_isShuttingDown.load() )
    {
        processEvents();

        // Health monitoring
        if ( m_healthMonitoringEnabled )
        {
            static auto lastHealthCheck = std::chrono::steady_clock::now();
            auto        now             = std::chrono::steady_clock::now();
            if ( now - lastHealthCheck > std::chrono::minutes( 5 ) )
            {
                sendHealthSpan();
                lastHealthCheck = now;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOpenTelemetryManager::processEvents()
{
    std::unique_lock<std::mutex> lock( m_queueMutex );

    // Wait for events or shutdown signal
    m_queueCondition.wait( lock, [this]() { return !m_eventQueue.empty() || m_isShuttingDown.load(); } );

    if ( m_eventQueue.empty() )
    {
        return;
    }

    // Process a batch of events
    std::queue<Event> batch;
    auto*             prefs        = RiaPreferencesOpenTelemetry::current();
    int               maxBatchSize = prefs ? prefs->maxBatchSize() : 100;

    for ( int i = 0; i < maxBatchSize && !m_eventQueue.empty(); ++i )
    {
        batch.push( m_eventQueue.front() );
        m_eventQueue.pop();
    }

    lock.unlock();

    // Process events outside of lock
    while ( !batch.empty() )
    {
        processEvent( batch.front() );
        batch.pop();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOpenTelemetryManager::processEvent( const Event& event )
{
    try
    {
        auto* prefs = RiaPreferencesOpenTelemetry::current();

        // Use Application Insights REST API
        auto connectionParams = parseAzureConnectionString( prefs->connectionString() );

        if ( !connectionParams.contains( "InstrumentationKey" ) || !connectionParams.contains( "IngestionEndpoint" ) )
        {
            updateHealthMetrics( false );
            return;
        }

        // Format timestamp - must match Application Insights format exactly
        auto time_t = std::chrono::system_clock::to_time_t( event.timestamp );
        auto ms     = std::chrono::duration_cast<std::chrono::milliseconds>( event.timestamp.time_since_epoch() ) % 1000;

        char timeBuffer[100];
        std::strftime( timeBuffer, sizeof( timeBuffer ), "%Y-%m-%dT%H:%M:%S", std::gmtime( &time_t ) );

        // Pad milliseconds to 3 digits
        char msBuffer[8];
        std::snprintf( msBuffer, sizeof( msBuffer ), ".%03dZ", static_cast<int>( ms.count() ) );

        std::string timestamp = std::string( timeBuffer ) + msBuffer;

        // Convert attributes to JSON properties
        nlohmann::json properties = nlohmann::json::object();
        for ( const auto& [key, value] : event.attributes )
        {
            properties[key] = value;
        }

        // Create Application Insights telemetry item
        nlohmann::json telemetryItem = { { "time", timestamp },
                                         { "iKey", connectionParams["InstrumentationKey"].toStdString() },
                                         { "name", "Microsoft.ApplicationInsights.Event" },
                                         { "data",
                                           { { "baseType", "EventData" },
                                             { "baseData", { { "name", event.name }, { "properties", properties } } } } } };

        // Send to Application Insights
        std::string errorMessage;
        bool        success =
            sendToApplicationInsights( telemetryItem.dump(), connectionParams["IngestionEndpoint"], prefs->connectionTimeoutMs(), errorMessage );

        if ( success )
        {
            updateHealthMetrics( true );
            resetCircuitBreaker();
        }
        else
        {
            handleError( TelemetryError::NetworkError, QString( "Failed to send telemetry: %1" ).arg( QString::fromStdString( errorMessage ) ) );
            updateHealthMetrics( false );
        }
    }
    catch ( const std::exception& e )
    {
        handleError( TelemetryError::InternalError, QString( "Failed to process event: %1" ).arg( e.what() ) );
        updateHealthMetrics( false );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaOpenTelemetryManager::shouldSampleEvent() const
{
    if ( m_samplingRate >= 1.0 )
    {
        return true;
    }

    static thread_local std::mt19937                           gen( std::random_device{}() );
    static thread_local std::uniform_real_distribution<double> dis( 0.0, 1.0 );

    return dis( gen ) < m_samplingRate;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOpenTelemetryManager::flushPendingEvents()
{
    // Process remaining events in the queue
    processEvents();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOpenTelemetryManager::handleError( TelemetryError error, const QString& context )
{
    m_consecutiveFailures++;

    if ( m_consecutiveFailures >= 3 )
    {
        m_circuitBreakerOpen = true;
        RiaLogging::warning( "OpenTelemetry circuit breaker opened due to consecutive failures" );
    }

    if ( m_errorCallback )
    {
        m_errorCallback( error, context );
    }

    RiaLogging::warning( QString( "OpenTelemetry error: %1" ).arg( context ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOpenTelemetryManager::escalateError( TelemetryError error, int severity )
{
    // Could implement escalation logic here
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOpenTelemetryManager::switchToOfflineMode()
{
    // Switch to local logging fallback
    m_enabled = false;
    RiaLogging::info( "Switched to offline mode - telemetry will be logged locally" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOpenTelemetryManager::attemptReconnection()
{
    auto now = std::chrono::steady_clock::now();
    if ( now - m_lastReconnectAttempt < std::chrono::minutes( 5 ) )
    {
        return; // Don't retry too frequently
    }

    m_lastReconnectAttempt = now;

    // Try to reinitialize connection
    if ( createExporter() )
    {
        resetCircuitBreaker();
        RiaLogging::info( "OpenTelemetry reconnection successful" );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaOpenTelemetryManager::isCircuitBreakerOpen() const
{
    return m_circuitBreakerOpen.load();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOpenTelemetryManager::resetCircuitBreaker()
{
    m_consecutiveFailures = 0;
    m_circuitBreakerOpen  = false;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOpenTelemetryManager::updateHealthMetrics( bool success )
{
    if ( success )
    {
        m_healthMetrics.eventsSent++;
        m_healthMetrics.lastSuccessfulSend = std::chrono::steady_clock::now();
    }
    else
    {
        m_healthMetrics.networkFailures++;
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOpenTelemetryManager::sendHealthSpan()
{
    if ( !isEnabled() )
    {
        return;
    }

    auto                               metrics = getHealthMetrics();
    std::map<std::string, std::string> attributes;
    attributes["health.events_queued"]    = std::to_string( metrics.eventsQueued );
    attributes["health.events_sent"]      = std::to_string( metrics.eventsSent );
    attributes["health.events_dropped"]   = std::to_string( metrics.eventsDropped );
    attributes["health.network_failures"] = std::to_string( metrics.networkFailures );
    attributes["health.success_rate"]     = std::to_string( metrics.getSuccessRate() );
    attributes["health.queue_size"]       = std::to_string( getCurrentQueueSize() );

    reportEventAsync( "health.status", attributes );
}