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

#pragma once

#include "RiaOpenTelemetryPrivacyFilter.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#ifdef RESINSIGHT_OPENTELEMETRY_ENABLED
#include "opentelemetry/exporters/otlp/otlp_http_exporter.h"
#include "opentelemetry/sdk/trace/batch_span_processor.h"
#include "opentelemetry/sdk/trace/simple_processor.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/trace/tracer.h"
#include <stacktrace>
namespace trace = opentelemetry::trace;
namespace otlp  = opentelemetry::exporter::otlp;
#endif

class QString;

//==================================================================================================
//
// OpenTelemetry Manager for ResInsight
// Handles async telemetry reporting with privacy filtering and resilience
//
//==================================================================================================
class RiaOpenTelemetryManager
{
public:
    // Error handling
    enum class TelemetryError
    {
        ConfigurationError,
        NetworkError,
        AuthenticationError,
        QuotaExceeded,
        PrivacyViolation,
        InternalError
    };
    using ErrorCallback = std::function<void( TelemetryError, const QString& details )>;

    // Health monitoring
    struct HealthMetrics
    {
        std::atomic<uint64_t>                 eventsQueued{ 0 };
        std::atomic<uint64_t>                 eventsSent{ 0 };
        std::atomic<uint64_t>                 eventsDropped{ 0 };
        std::atomic<uint64_t>                 networkFailures{ 0 };
        std::chrono::steady_clock::time_point lastSuccessfulSend;
        std::chrono::steady_clock::time_point systemStartTime;

        double getSuccessRate() const;
        bool   isHealthy() const;
    };

    static RiaOpenTelemetryManager& instance();

    bool initialize();
    void shutdown( std::chrono::seconds timeout = std::chrono::seconds( 30 ) );

    // Event reporting
    void reportEventAsync( const std::string& eventName, const std::map<std::string, std::string>& attributes );
    void reportCrash( int signalCode, const std::stacktrace& trace );
    void reportTestCrash( const std::stacktrace& trace );

    // Configuration
    bool isEnabled() const;
    bool isInitialized() const;
    void setErrorCallback( ErrorCallback callback );

    // Performance and memory management
    void   setMaxQueueSize( size_t maxEvents );
    void   enableBackpressure( bool enable );
    void   setMemoryThreshold( size_t maxMemoryMB );
    void   setSamplingRate( double rate );
    size_t getCurrentQueueSize() const;

    // Health monitoring
    HealthMetrics getHealthMetrics() const;
    bool          isHealthy() const;
    void          enableHealthMonitoring( bool enable );

private:
    RiaOpenTelemetryManager();
    ~RiaOpenTelemetryManager();

    RiaOpenTelemetryManager( const RiaOpenTelemetryManager& )            = delete;
    RiaOpenTelemetryManager& operator=( const RiaOpenTelemetryManager& ) = delete;

    struct Event
    {
        std::string                           name;
        std::map<std::string, std::string>    attributes;
        std::chrono::system_clock::time_point timestamp;

        Event( const std::string& eventName, const std::map<std::string, std::string>& attrs )
            : name( eventName )
            , attributes( attrs )
            , timestamp( std::chrono::system_clock::now() )
        {
        }
    };

    // Initialization
    bool initializeProvider();
    bool createExporter();
    void setupResourceAttributes();

    // Event processing
    void workerThread();
    void processEvents();
    void processEvent( const Event& event );
    bool shouldSampleEvent() const;
    void flushPendingEvents();

    // Circuit breaker and resilience
    void handleError( TelemetryError error, const QString& context );
    void escalateError( TelemetryError error, int severity );
    void switchToOfflineMode();
    void attemptReconnection();
    bool isCircuitBreakerOpen() const;
    void resetCircuitBreaker();

    // Health monitoring
    void updateHealthMetrics( bool success );
    void sendHealthSpan();

    // Thread safety
    mutable std::mutex      m_configMutex;
    std::mutex              m_queueMutex;
    std::condition_variable m_queueCondition;
    std::queue<Event>       m_eventQueue;

    // State
    std::atomic<bool> m_initialized{ false };
    std::atomic<bool> m_enabled{ false };
    std::atomic<bool> m_isShuttingDown{ false };
    std::atomic<bool> m_circuitBreakerOpen{ false };

    // Worker thread
    std::unique_ptr<std::thread> m_workerThread;

    // Configuration
    size_t m_maxQueueSize{ 10000 };
    bool   m_backpressureEnabled{ true };
    size_t m_memoryThresholdMB{ 50 };
    double m_samplingRate{ 1.0 };

    // Error handling
    ErrorCallback                         m_errorCallback;
    std::atomic<int>                      m_consecutiveFailures{ 0 };
    std::chrono::steady_clock::time_point m_lastReconnectAttempt;

    // Health monitoring
    mutable HealthMetrics m_healthMetrics;
    bool                  m_healthMonitoringEnabled{ true };

#ifdef RESINSIGHT_OPENTELEMETRY_ENABLED
    // OpenTelemetry objects
    std::shared_ptr<trace::TracerProvider> m_provider;
    std::shared_ptr<trace::Tracer>         m_tracer;
#endif
};