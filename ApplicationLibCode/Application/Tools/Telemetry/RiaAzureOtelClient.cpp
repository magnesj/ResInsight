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

#include <QDebug>

#include <regex>
#include <sstream>

// TODO: Full OpenTelemetry SDK integration pending compatibility fixes
// Currently using stub implementation

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaAzureOtelClient::RiaAzureOtelClient()
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
        qWarning() << "RiaAzureOtelClient already initialized";
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
        qCritical() << "RiaAzureOtelClient: endpoint is required";
        return false;
    }

    if ( m_config.serviceName.empty() )
    {
        qCritical() << "RiaAzureOtelClient: serviceName is required";
        return false;
    }

    // Setup resource attributes
    setupResourceAttributes();

    // TODO: Initialize OpenTelemetry providers when SDK compatibility is resolved
    m_initialized = true;

    qInfo() << "RiaAzureOtelClient initialized (stub mode)";
    return m_initialized;
}

//--------------------------------------------------------------------------------------------------
/// Shutdown the client and cleanup resources
//--------------------------------------------------------------------------------------------------
void RiaAzureOtelClient::shutdown()
{
    if ( !m_initialized ) return;

    m_initialized = false;
    qInfo() << "RiaAzureOtelClient shut down";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaAzureOtelClient::isInitialized() const
{
    return m_initialized;
}

//--------------------------------------------------------------------------------------------------
/// Setup resource attributes for the service
//--------------------------------------------------------------------------------------------------
void RiaAzureOtelClient::setupResourceAttributes()
{
    // Add standard resource attributes
    m_resourceAttributes["telemetry.sdk.name"]     = "opentelemetry";
    m_resourceAttributes["telemetry.sdk.language"] = "cpp";
    m_resourceAttributes["service.name"]           = m_config.serviceName;
    m_resourceAttributes["service.version"]        = m_config.serviceVersion;
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
    if ( !m_initialized ) return;

    // TODO: Implement when SDK is available
    qDebug() << "StartSpan:" << QString::fromStdString( spanName );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaAzureOtelClient::endCurrentSpan()
{
    // TODO: Implement when SDK is available
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaAzureOtelClient::addSpanEvent( const std::string& eventName, const std::map<std::string, std::string>& attributes )
{
    // TODO: Implement when SDK is available
}

//--------------------------------------------------------------------------------------------------
/// Log an event with custom severity
//--------------------------------------------------------------------------------------------------
void RiaAzureOtelClient::logEvent( const std::string& message, const std::string& severity, const std::map<std::string, std::string>& attributes )
{
    if ( !m_initialized ) return;

    qInfo() << QString::fromStdString( severity ) << ":" << QString::fromStdString( message );
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
    if ( !m_initialized ) return;

    qDebug() << "TrackEvent:" << QString::fromStdString( eventName );
}

//--------------------------------------------------------------------------------------------------
/// Track an exception
//--------------------------------------------------------------------------------------------------
void RiaAzureOtelClient::trackException( const std::string&                        exceptionType,
                                         const std::string&                        message,
                                         const std::string&                        stackTrace,
                                         const std::map<std::string, std::string>& properties )
{
    if ( !m_initialized ) return;

    qCritical() << "Exception:" << QString::fromStdString( exceptionType ) << QString::fromStdString( message );
}

//--------------------------------------------------------------------------------------------------
/// Force flush all pending telemetry
//--------------------------------------------------------------------------------------------------
RiaAzureOtelClient::ExportResult RiaAzureOtelClient::forceFlush()
{
    if ( !m_initialized ) return ExportResult::InvalidConfig;

    // TODO: Implement when SDK is available
    return ExportResult::Success;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::string RiaAzureOtelClient::getEndpoint() const
{
    return m_config.endpoint;
}
