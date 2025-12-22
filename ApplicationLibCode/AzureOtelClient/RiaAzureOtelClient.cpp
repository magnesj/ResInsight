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

#include "RiaAzureOtelClient.h"

// Standard includes needed before OpenTelemetry headers
#include <cstring>
#include <memory>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>

// Define OpenTelemetry ABI version before including headers
#ifndef OPENTELEMETRY_ABI_VERSION_NO
#define OPENTELEMETRY_ABI_VERSION_NO 1
#endif

// OpenTelemetry SDK includes
#include <opentelemetry/exporters/otlp/otlp_http_exporter_factory.h>
#include <opentelemetry/exporters/otlp/otlp_http_log_record_exporter_factory.h>
#include <opentelemetry/logs/provider.h>
#include <opentelemetry/logs/severity.h>
#include <opentelemetry/sdk/logs/logger_provider_factory.h>
#include <opentelemetry/sdk/logs/simple_log_record_processor_factory.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/sdk/trace/simple_processor_factory.h>
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include <opentelemetry/trace/provider.h>

namespace trace_api = opentelemetry::trace;
namespace logs_api  = opentelemetry::logs;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace logs_sdk  = opentelemetry::sdk::logs;
namespace resource  = opentelemetry::sdk::resource;
namespace otlp      = opentelemetry::exporter::otlp;
namespace common    = opentelemetry::common;

// Pimpl implementation holds all OpenTelemetry SDK types
// Note: OpenTelemetry uses nostd::shared_ptr, not std::shared_ptr
struct RiaAzureOtelClient::Impl
{
    opentelemetry::nostd::shared_ptr<trace_sdk::TracerProvider> tracerProvider;
    opentelemetry::nostd::shared_ptr<logs_sdk::LoggerProvider>  loggerProvider;
    opentelemetry::nostd::shared_ptr<trace_api::Tracer>         tracer;
    opentelemetry::nostd::shared_ptr<logs_api::Logger>          logger;
};

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaAzureOtelClient::RiaAzureOtelClient()
    : m_impl( std::make_unique<Impl>() )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaAzureOtelClient::~RiaAzureOtelClient()
{
    shutdown();
}

//--------------------------------------------------------------------------------------------------
/// Initialize the Azure OTEL client with configuration
//--------------------------------------------------------------------------------------------------
bool RiaAzureOtelClient::initialize( const AzureConfig& config )
{
    if ( m_initialized )
    {
        log( LogLevel::Warning, "RiaAzureOtelClient already initialized" );
        return true;
    }

    m_config = config;

    // Parse connection string if provided
    if ( !m_config.connectionString.empty() )
    {
        if ( m_config.endpoint.empty() )
        {
            m_config.endpoint = parseEndpointFromConnectionString( m_config.connectionString );
        }
        if ( m_config.instrumentationKey.empty() )
        {
            m_config.instrumentationKey = parseInstrumentationKey( m_config.connectionString );
        }
    }

    // Validate configuration
    if ( m_config.endpoint.empty() )
    {
        log( LogLevel::Error, "RiaAzureOtelClient: endpoint is required" );
        return false;
    }

    if ( m_config.serviceName.empty() )
    {
        log( LogLevel::Error, "RiaAzureOtelClient: serviceName is required" );
        return false;
    }

    // Setup resource attributes
    setupResourceAttributes();

    // Initialize providers
    bool success = true;
    try
    {
        if ( m_config.enableTraces )
        {
            success &= initializeTracerProvider();
        }
        if ( m_config.enableLogs )
        {
            success &= initializeLoggerProvider();
        }
    }
    catch ( const std::exception& e )
    {
        log( LogLevel::Error, std::string( "Failed to initialize OpenTelemetry providers: " ) + e.what() );
        success = false;
    }

    m_initialized = success;

    if ( m_initialized )
    {
        log( LogLevel::Info, "RiaAzureOtelClient initialized successfully" );
    }
    else
    {
        log( LogLevel::Error, "RiaAzureOtelClient initialization failed" );
    }

    return m_initialized;
}

//--------------------------------------------------------------------------------------------------
/// Shutdown the client and cleanup resources
//--------------------------------------------------------------------------------------------------
void RiaAzureOtelClient::shutdown()
{
    if ( !m_initialized ) return;

    // Force flush before shutdown
    forceFlush();

    if ( m_impl )
    {
        m_impl->tracer         = nullptr;
        m_impl->logger         = nullptr;
        m_impl->tracerProvider = nullptr;
        m_impl->loggerProvider = nullptr;
    }

    m_initialized = false;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaAzureOtelClient::isInitialized() const
{
    return m_initialized;
}

//--------------------------------------------------------------------------------------------------
/// Initialize the tracer provider for distributed tracing
//--------------------------------------------------------------------------------------------------
bool RiaAzureOtelClient::initializeTracerProvider()
{
    try
    {
        // Create OTLP HTTP exporter options
        otlp::OtlpHttpExporterOptions exporterOptions;
        exporterOptions.url = m_config.endpoint + "/v1/traces";

        // Azure Application Insights OTLP requires specific headers
        // The connection string should be passed in the Authorization header
        if ( !m_config.connectionString.empty() )
        {
            // Azure Monitor OTLP expects the connection string in a specific format
            exporterOptions.http_headers.insert( { "Authorization", "Bearer " + m_config.connectionString } );
        }
        else if ( !m_config.instrumentationKey.empty() )
        {
            // Fallback: try with instrumentation key
            exporterOptions.http_headers.insert( { "x-ms-instrumentation-key", m_config.instrumentationKey } );
        }

        // Enable console debug for OTLP exporter (helps diagnose issues)
        exporterOptions.console_debug = true;

        log( LogLevel::Info, std::string( "Initializing tracer with endpoint: " ) + exporterOptions.url );

        // Create the exporter
        auto exporter = otlp::OtlpHttpExporterFactory::Create( exporterOptions );

        // Create resource with attributes
        auto resourceAttributes = resource::ResourceAttributes{
            { "service.name", m_config.serviceName },
            { "service.version", m_config.serviceVersion },
            { "service.instance.id", m_config.serviceInstanceId },
        };

        // Add custom resource attributes
        for ( const auto& [key, value] : m_resourceAttributes )
        {
            resourceAttributes[key] = value;
        }

        auto resourcePtr = resource::Resource::Create( resourceAttributes );

        // Create simple span processor
        auto processor = trace_sdk::SimpleSpanProcessorFactory::Create( std::move( exporter ) );

        // Create tracer provider (returns std::unique_ptr)
        auto tracerProviderUnique = trace_sdk::TracerProviderFactory::Create( std::move( processor ), resourcePtr );

        // Convert to std::shared_ptr (for storage and use)
        std::shared_ptr<trace_sdk::TracerProvider> tracerProviderStd( std::move( tracerProviderUnique ) );

        // Store provider
        m_impl->tracerProvider = opentelemetry::nostd::shared_ptr<trace_sdk::TracerProvider>( tracerProviderStd );

        // Set as global tracer provider (converts to base type automatically)
        std::shared_ptr<trace_api::TracerProvider> tracerProviderBase =
            std::static_pointer_cast<trace_api::TracerProvider>( tracerProviderStd );
        trace_api::Provider::SetTracerProvider( opentelemetry::nostd::shared_ptr<trace_api::TracerProvider>( tracerProviderBase ) );

        // Get tracer instance
        m_impl->tracer = m_impl->tracerProvider->GetTracer( m_config.serviceName, m_config.serviceVersion );

        log( LogLevel::Info, "Tracer provider initialized" );
        return true;
    }
    catch ( const std::exception& e )
    {
        log( LogLevel::Error, std::string( "Failed to initialize tracer provider: " ) + e.what() );
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
/// Initialize the logger provider for log export
//--------------------------------------------------------------------------------------------------
bool RiaAzureOtelClient::initializeLoggerProvider()
{
    try
    {
        // Create OTLP HTTP log exporter options
        otlp::OtlpHttpLogRecordExporterOptions exporterOptions;
        exporterOptions.url = m_config.endpoint + "/v1/logs";

        // Azure Application Insights OTLP requires specific headers
        if ( !m_config.connectionString.empty() )
        {
            exporterOptions.http_headers.insert( { "Authorization", "Bearer " + m_config.connectionString } );
        }
        else if ( !m_config.instrumentationKey.empty() )
        {
            exporterOptions.http_headers.insert( { "x-ms-instrumentation-key", m_config.instrumentationKey } );
        }

        // Enable console debug
        exporterOptions.console_debug = true;

        log( LogLevel::Info, std::string( "Initializing logger with endpoint: " ) + exporterOptions.url );

        // Create the log exporter
        auto exporter = otlp::OtlpHttpLogRecordExporterFactory::Create( exporterOptions );

        // Create resource with attributes
        auto resourceAttributes = resource::ResourceAttributes{
            { "service.name", m_config.serviceName },
            { "service.version", m_config.serviceVersion },
            { "service.instance.id", m_config.serviceInstanceId },
        };

        // Add custom resource attributes
        for ( const auto& [key, value] : m_resourceAttributes )
        {
            resourceAttributes[key] = value;
        }

        auto resourcePtr = resource::Resource::Create( resourceAttributes );

        // Create simple log processor
        auto processor = logs_sdk::SimpleLogRecordProcessorFactory::Create( std::move( exporter ) );

        // Create logger provider (returns std::unique_ptr)
        auto loggerProviderUnique = logs_sdk::LoggerProviderFactory::Create( std::move( processor ), resourcePtr );

        // Convert to std::shared_ptr (for storage and use)
        std::shared_ptr<logs_sdk::LoggerProvider> loggerProviderStd( std::move( loggerProviderUnique ) );

        // Store provider
        m_impl->loggerProvider = opentelemetry::nostd::shared_ptr<logs_sdk::LoggerProvider>( loggerProviderStd );

        // Set as global logger provider (converts to base type automatically)
        std::shared_ptr<logs_api::LoggerProvider> loggerProviderBase = std::static_pointer_cast<logs_api::LoggerProvider>( loggerProviderStd );
        logs_api::Provider::SetLoggerProvider( opentelemetry::nostd::shared_ptr<logs_api::LoggerProvider>( loggerProviderBase ) );

        // Get logger instance
        m_impl->logger = m_impl->loggerProvider->GetLogger( m_config.serviceName, m_config.serviceName, m_config.serviceVersion );

        log( LogLevel::Info, "Logger provider initialized" );
        return true;
    }
    catch ( const std::exception& e )
    {
        log( LogLevel::Error, std::string( "Failed to initialize logger provider: " ) + e.what() );
        return false;
    }
}

//--------------------------------------------------------------------------------------------------
/// Setup resource attributes for the service
//--------------------------------------------------------------------------------------------------
void RiaAzureOtelClient::setupResourceAttributes()
{
    // Add standard resource attributes
    m_resourceAttributes["telemetry.sdk.name"]     = "opentelemetry";
    m_resourceAttributes["telemetry.sdk.language"] = "cpp";
    m_resourceAttributes["telemetry.sdk.version"]  = "1.18.0";
}

//--------------------------------------------------------------------------------------------------
/// Parse OTLP endpoint from Azure connection string
/// Format: InstrumentationKey=xxx;IngestionEndpoint=https://xxx.applicationinsights.azure.com/
//--------------------------------------------------------------------------------------------------
std::string RiaAzureOtelClient::parseEndpointFromConnectionString( const std::string& connectionString )
{
    std::regex  endpointRegex( "IngestionEndpoint=([^;]+)" );
    std::smatch match;

    if ( std::regex_search( connectionString, match, endpointRegex ) && match.size() > 1 )
    {
        std::string endpoint = match[1].str();
        // Remove trailing slash if present
        if ( !endpoint.empty() && endpoint.back() == '/' )
        {
            endpoint.pop_back();
        }
        return endpoint;
    }

    return "";
}

//--------------------------------------------------------------------------------------------------
/// Parse instrumentation key from Azure connection string
//--------------------------------------------------------------------------------------------------
std::string RiaAzureOtelClient::parseInstrumentationKey( const std::string& connectionString )
{
    std::regex  keyRegex( "InstrumentationKey=([^;]+)" );
    std::smatch match;

    if ( std::regex_search( connectionString, match, keyRegex ) && match.size() > 1 )
    {
        return match[1].str();
    }

    return "";
}

//--------------------------------------------------------------------------------------------------
/// Start a new span for distributed tracing
//--------------------------------------------------------------------------------------------------
void RiaAzureOtelClient::startSpan( const std::string& spanName, const std::map<std::string, std::string>& attributes )
{
    if ( !m_initialized || !m_impl || !m_impl->tracer ) return;

    auto span = m_impl->tracer->StartSpan( spanName );

    // Add attributes to span
    for ( const auto& [key, value] : attributes )
    {
        span->SetAttribute( key, value );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaAzureOtelClient::endCurrentSpan()
{
    // Note: In a real implementation, we would need to manage a span stack
    // This is a simplified version
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaAzureOtelClient::addSpanEvent( const std::string& eventName, const std::map<std::string, std::string>& attributes )
{
    // Note: Would need current span context
}

//--------------------------------------------------------------------------------------------------
/// Convert severity string to OpenTelemetry severity level
//--------------------------------------------------------------------------------------------------
static logs_api::Severity getSeverityLevel( const std::string& severity )
{
    if ( severity == "TRACE" ) return logs_api::Severity::kTrace;
    if ( severity == "DEBUG" ) return logs_api::Severity::kDebug;
    if ( severity == "INFO" ) return logs_api::Severity::kInfo;
    if ( severity == "WARN" || severity == "WARNING" ) return logs_api::Severity::kWarn;
    if ( severity == "ERROR" ) return logs_api::Severity::kError;
    if ( severity == "FATAL" ) return logs_api::Severity::kFatal;

    // Default to info
    return logs_api::Severity::kInfo;
}

//--------------------------------------------------------------------------------------------------
/// Log an event with custom severity
//--------------------------------------------------------------------------------------------------
void RiaAzureOtelClient::logEvent( const std::string& message, const std::string& severity, const std::map<std::string, std::string>& attributes )
{
    if ( !m_initialized || !m_impl || !m_impl->logger ) return;

    log( LogLevel::Debug, std::string( "Logging event [" ) + severity + "]: " + message );

    // Convert severity string to enum
    auto severityLevel = getSeverityLevel( severity );

    // Emit log record using OpenTelemetry API
    // Note: The Logger::EmitLogRecord API uses a variadic template
    m_impl->logger->EmitLogRecord( severityLevel, message );

    // Force immediate flush
    log( LogLevel::Debug, "Flushing logger after event" );
    if ( m_impl->loggerProvider )
    {
        bool flushed = m_impl->loggerProvider->ForceFlush();
        log( LogLevel::Debug, flushed ? "Log flush succeeded" : "Log flush failed" );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaAzureOtelClient::logError( const std::string& message, const std::map<std::string, std::string>& attributes )
{
    logEvent( message, "ERROR", attributes );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaAzureOtelClient::logWarning( const std::string& message, const std::map<std::string, std::string>& attributes )
{
    logEvent( message, "WARN", attributes );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaAzureOtelClient::logInfo( const std::string& message, const std::map<std::string, std::string>& attributes )
{
    logEvent( message, "INFO", attributes );
}

//--------------------------------------------------------------------------------------------------
/// Track a custom event (Application Insights style)
//--------------------------------------------------------------------------------------------------
void RiaAzureOtelClient::trackEvent( const std::string& eventName, const std::map<std::string, std::string>& properties )
{
    if ( !m_initialized || !m_impl || !m_impl->tracer ) return;

    log( LogLevel::Debug, std::string( "Tracking event: " ) + eventName );

    // Create a span for the event
    auto span = m_impl->tracer->StartSpan( eventName );

    // Add properties as attributes
    for ( const auto& [key, value] : properties )
    {
        span->SetAttribute( key, value );
    }

    // Mark as event type
    span->SetAttribute( "event.type", "custom" );
    span->SetAttribute( "event.name", eventName );

    // End span immediately for event tracking
    span->End();

    // Force immediate flush to ensure data is sent
    log( LogLevel::Debug, "Flushing tracer after event" );
    if ( m_impl->tracerProvider )
    {
        bool flushed = m_impl->tracerProvider->ForceFlush();
        log( LogLevel::Debug, flushed ? "Flush succeeded" : "Flush failed" );
    }
}

//--------------------------------------------------------------------------------------------------
/// Track an exception
//--------------------------------------------------------------------------------------------------
void RiaAzureOtelClient::trackException( const std::string&                        exceptionType,
                                         const std::string&                        message,
                                         const std::string&                        stackTrace,
                                         const std::map<std::string, std::string>& properties )
{
    if ( !m_initialized || !m_impl || !m_impl->tracer ) return;

    // Create a span for the exception
    auto span = m_impl->tracer->StartSpan( "Exception" );

    span->SetAttribute( "exception.type", exceptionType );
    span->SetAttribute( "exception.message", message );
    span->SetAttribute( "exception.stacktrace", stackTrace );

    // Add additional properties
    for ( const auto& [key, value] : properties )
    {
        span->SetAttribute( key, value );
    }

    // Mark span as error
    span->SetStatus( trace_api::StatusCode::kError, message );

    span->End();
}

//--------------------------------------------------------------------------------------------------
/// Force flush all pending telemetry
//--------------------------------------------------------------------------------------------------
RiaAzureOtelClient::ExportResult RiaAzureOtelClient::forceFlush()
{
    if ( !m_initialized ) return ExportResult::InvalidConfig;

    try
    {
        bool success = true;

        if ( m_impl && m_impl->tracerProvider )
        {
            success &= m_impl->tracerProvider->ForceFlush();
        }

        if ( m_impl && m_impl->loggerProvider )
        {
            success &= m_impl->loggerProvider->ForceFlush();
        }

        return success ? ExportResult::Success : ExportResult::Failure;
    }
    catch ( const std::exception& e )
    {
        log( LogLevel::Error, std::string( "Failed to flush telemetry: " ) + e.what() );
        return ExportResult::Failure;
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::string RiaAzureOtelClient::getEndpoint() const
{
    return m_config.endpoint;
}

//--------------------------------------------------------------------------------------------------
/// Log a message using the callback if configured
//--------------------------------------------------------------------------------------------------
void RiaAzureOtelClient::log( LogLevel level, const std::string& message ) const
{
    if ( m_config.logCallback )
    {
        m_config.logCallback( level, message );
    }
}
