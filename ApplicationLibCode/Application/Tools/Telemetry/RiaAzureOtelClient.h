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

#pragma once

#include <map>
#include <memory>
#include <string>

// TODO: Add OpenTelemetry SDK forward declarations when SDK integration is complete

//==================================================================================================
//
// Azure OpenTelemetry Client
// Handles communication with Azure Application Insights using OpenTelemetry Protocol (OTLP)
//
//==================================================================================================
class RiaAzureOtelClient
{
public:
    struct AzureConfig
    {
        std::string connectionString; // Azure Application Insights connection string
        std::string instrumentationKey; // Azure instrumentation key
        std::string endpoint; // OTLP endpoint URL
        std::string serviceName; // Service name for telemetry
        std::string serviceVersion; // Service version
        std::string serviceInstanceId; // Unique instance identifier
        bool        enableTraces     = true; // Enable trace export
        bool        enableLogs       = true; // Enable log export
        int         exportIntervalMs = 5000; // Export interval in milliseconds
    };

    enum class ExportResult
    {
        Success,
        Failure,
        Timeout,
        InvalidConfig
    };

    RiaAzureOtelClient();
    ~RiaAzureOtelClient();

    // Configuration
    bool initialize( const AzureConfig& config );
    void shutdown();
    bool isInitialized() const;

    // Trace operations
    void startSpan( const std::string& spanName, const std::map<std::string, std::string>& attributes );
    void endCurrentSpan();
    void addSpanEvent( const std::string& eventName, const std::map<std::string, std::string>& attributes );

    // Log operations
    void logEvent( const std::string& message, const std::string& severity, const std::map<std::string, std::string>& attributes );
    void logError( const std::string& message, const std::map<std::string, std::string>& attributes );
    void logWarning( const std::string& message, const std::map<std::string, std::string>& attributes );
    void logInfo( const std::string& message, const std::map<std::string, std::string>& attributes );

    // Custom event tracking (Application Insights style)
    void trackEvent( const std::string& eventName, const std::map<std::string, std::string>& properties );
    void trackException( const std::string&                        exceptionType,
                         const std::string&                        message,
                         const std::string&                        stackTrace,
                         const std::map<std::string, std::string>& properties );

    // Force flush
    ExportResult forceFlush();

    // Configuration getters
    const AzureConfig& getConfig() const { return m_config; }
    std::string        getEndpoint() const;

private:
    // Initialization helpers
    bool        initializeTracerProvider();
    bool        initializeLoggerProvider();
    void        setupResourceAttributes();
    std::string parseEndpointFromConnectionString( const std::string& connectionString );
    std::string parseInstrumentationKey( const std::string& connectionString );

    // TODO: Add OpenTelemetry SDK provider members when SDK integration is complete

    // Configuration
    AzureConfig m_config;
    bool        m_initialized = false;

    // Resource attributes
    std::map<std::string, std::string> m_resourceAttributes;
};
