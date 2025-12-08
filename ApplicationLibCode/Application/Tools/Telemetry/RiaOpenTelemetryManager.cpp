/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2024- Equinor ASA
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

#ifdef RESINSIGHT_OPENTELEMETRY_ENABLED
#include "opentelemetry/exporters/otlp/otlp_http_exporter_options.h"
#include "opentelemetry/sdk/resource/resource.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/trace/provider.h"
namespace sdk      = opentelemetry::sdk;
namespace resource = opentelemetry::sdk::resource;
#endif

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RiaOpenTelemetryManager::HealthMetrics::getSuccessRate() const
{
    uint64_t total = eventsSent.load() + eventsDropped.load();
    if ( total == 0 ) return 1.0;
    return static_cast<double>( eventsSent.load() ) / total;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaOpenTelemetryManager::HealthMetrics::isHealthy() const
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
#ifdef RESINSIGHT_OPENTELEMETRY_ENABLED
    std::lock_guard<std::mutex> lock( m_configMutex );

    if ( m_initialized.load() )
    {
        return true;
    }

    auto* prefs      = RiaPreferencesOpenTelemetry::current();
    auto  validation = prefs->validate();
    if ( !validation.isValid )
    {
        handleError( TelemetryError::ConfigurationError, QString::fromStdString( validation.errorMessage ) );
        return false;
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
#else
    RiaLogging::warning( "OpenTelemetry not enabled at compile time" );
    return false;
#endif
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

    std::string rawStackTrace       = ss.str();
    std::string sanitizedStackTrace = RiaOpenTelemetryPrivacyFilter::sanitizeStackTrace( rawStackTrace );

    std::map<std::string, std::string> attributes;
    attributes["crash.signal"]      = std::to_string( signalCode );
    attributes["crash.thread_id"]   = std::to_string( std::hash<std::thread::id>{}( std::this_thread::get_id() ) );
    attributes["crash.stack_trace"] = sanitizedStackTrace;
    attributes["service.name"]      = RiaPreferencesOpenTelemetry::current()->serviceName().toStdString();
    attributes["service.version"]   = RiaPreferencesOpenTelemetry::current()->serviceVersion().toStdString();

    // Filter attributes through privacy filter
    RiaOpenTelemetryPrivacyFilter::FilterRules rules;
    rules.filterFilePaths = RiaPreferencesOpenTelemetry::current()->filterFilePaths();
    rules.filterUserData  = RiaPreferencesOpenTelemetry::current()->filterUserData();

    auto filteredAttributes = RiaOpenTelemetryPrivacyFilter::filterAttributes( attributes, rules );

    // Report with high priority (bypass sampling)
    std::unique_lock<std::mutex> lock( m_queueMutex );
    m_eventQueue.emplace( "crash.signal_handler", filteredAttributes );
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

    std::string rawStackTrace       = ss.str();
    std::string sanitizedStackTrace = RiaOpenTelemetryPrivacyFilter::sanitizeStackTrace( rawStackTrace );

    std::map<std::string, std::string> attributes;
    attributes["test.type"]        = "manual_stack_trace";
    attributes["test.thread_id"]   = std::to_string( std::hash<std::thread::id>{}( std::this_thread::get_id() ) );
    attributes["test.stack_trace"] = sanitizedStackTrace;
    attributes["service.name"]     = RiaPreferencesOpenTelemetry::current()->serviceName().toStdString();
    attributes["service.version"]  = RiaPreferencesOpenTelemetry::current()->serviceVersion().toStdString();

    // Filter attributes through privacy filter
    RiaOpenTelemetryPrivacyFilter::FilterRules rules;
    rules.filterFilePaths = RiaPreferencesOpenTelemetry::current()->filterFilePaths();
    rules.filterUserData  = RiaPreferencesOpenTelemetry::current()->filterUserData();

    auto filteredAttributes = RiaOpenTelemetryPrivacyFilter::filterAttributes( attributes, rules );

    reportEventAsync( "test.stack_trace", filteredAttributes );

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
RiaOpenTelemetryManager::HealthMetrics RiaOpenTelemetryManager::getHealthMetrics() const
{
    return m_healthMetrics;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaOpenTelemetryManager::isHealthy() const
{
    return m_healthMetrics.isHealthy() && !isCircuitBreakerOpen();
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
#ifdef RESINSIGHT_OPENTELEMETRY_ENABLED
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
#else
    return false;
#endif
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaOpenTelemetryManager::createExporter()
{
#ifdef RESINSIGHT_OPENTELEMETRY_ENABLED
    try
    {
        auto* prefs = RiaPreferencesOpenTelemetry::current();

        otlp::OtlpHttpExporterOptions opts;

        if ( prefs->activeEnvironment() == "development" )
        {
            opts.url = prefs->localEndpoint().toStdString();
        }
        else
        {
            // Parse Azure Application Insights connection string
            QString connStr = prefs->connectionString();
            if ( connStr.contains( "IngestionEndpoint=" ) )
            {
                QStringList parts = connStr.split( ';' );
                for ( const QString& part : parts )
                {
                    if ( part.startsWith( "IngestionEndpoint=" ) )
                    {
                        QString endpoint = part.mid( 18 ); // Remove "IngestionEndpoint="
                        opts.url         = ( endpoint + "/v1/traces" ).toStdString();
                        break;
                    }
                }
            }

            // Add instrumentation key header for Azure
            if ( connStr.contains( "InstrumentationKey=" ) )
            {
                QStringList parts = connStr.split( ';' );
                for ( const QString& part : parts )
                {
                    if ( part.startsWith( "InstrumentationKey=" ) )
                    {
                        QString key                              = part.mid( 19 ); // Remove "InstrumentationKey="
                        opts.headers["x-ms-instrumentation-key"] = key.toStdString();
                        break;
                    }
                }
            }
        }

        opts.timeout = std::chrono::milliseconds( prefs->connectionTimeoutMs() );

        auto exporter = std::make_unique<otlp::OtlpHttpExporter>( opts );

        // Create batch span processor
        sdk::trace::BatchSpanProcessorOptions processorOpts;
        processorOpts.max_queue_size        = prefs->maxBatchSize();
        processorOpts.schedule_delay_millis = std::chrono::milliseconds( prefs->batchTimeoutMs() );

        auto processor = std::make_unique<sdk::trace::BatchSpanProcessor>( std::move( exporter ), processorOpts );

        // Create tracer provider
        m_provider = std::make_shared<sdk::trace::TracerProvider>( std::move( processor ) );

        // Set global provider
        trace::Provider::SetTracerProvider( m_provider );

        // Get tracer
        m_tracer = trace::Provider::GetTracerProvider()->GetTracer( prefs->serviceName().toStdString(), prefs->serviceVersion().toStdString() );

        return true;
    }
    catch ( const std::exception& e )
    {
        handleError( TelemetryError::NetworkError, QString( "Failed to create exporter: %1" ).arg( e.what() ) );
        return false;
    }
#else
    return false;
#endif
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOpenTelemetryManager::setupResourceAttributes()
{
#ifdef RESINSIGHT_OPENTELEMETRY_ENABLED
    // Resource attributes are typically set during provider creation
    // This would be expanded with system information, process details, etc.
#endif
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
#ifdef RESINSIGHT_OPENTELEMETRY_ENABLED
    if ( !m_tracer )
    {
        updateHealthMetrics( false );
        return;
    }

    try
    {
        auto span = m_tracer->StartSpan( event.name );

        // Set attributes
        for ( const auto& [key, value] : event.attributes )
        {
            span->SetAttribute( key, value );
        }

        // Set timestamp
        span->SetAttribute( "timestamp", std::chrono::duration_cast<std::chrono::milliseconds>( event.timestamp.time_since_epoch() ).count() );

        if ( event.name.find( "crash" ) != std::string::npos )
        {
            span->SetStatus( trace::StatusCode::kError, "Application crashed" );
        }
        else
        {
            span->SetStatus( trace::StatusCode::kOk );
        }

        span->End();

        updateHealthMetrics( true );
        resetCircuitBreaker();
    }
    catch ( const std::exception& e )
    {
        handleError( TelemetryError::InternalError, QString( "Failed to process event: %1" ).arg( e.what() ) );
        updateHealthMetrics( false );
    }
#endif
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
    attributes["health.events_queued"]    = std::to_string( metrics.eventsQueued.load() );
    attributes["health.events_sent"]      = std::to_string( metrics.eventsSent.load() );
    attributes["health.events_dropped"]   = std::to_string( metrics.eventsDropped.load() );
    attributes["health.network_failures"] = std::to_string( metrics.networkFailures.load() );
    attributes["health.success_rate"]     = std::to_string( metrics.getSuccessRate() );
    attributes["health.queue_size"]       = std::to_string( getCurrentQueueSize() );

    reportEventAsync( "health.status", attributes );
}