# ResInsight OpenTelemetry Integration Plan

## Overview
Add OpenTelemetry logging capability to ResInsight for crash reporting and telemetry, with privacy protection, async operation, and JSON configuration following the OSDU config pattern.

## 1. Configuration System Design

### JSON Configuration File
**Location:** `~/.resinsight/opentelemetry_config.json`

```json
{
  "enabled": true,
  "connection_string": "InstrumentationKey=1056617c-eab1-4c4c-b413-f379270e4502;IngestionEndpoint=https://swedencentral-0.in.applicationinsights.azure.com/;LiveEndpoint=https://swedencentral.livediagnostics.monitor.azure.com/;ApplicationId=096a4759-08db-4e41-9309-255911863ae9",
  "batch_timeout_ms": 5000,
  "max_batch_size": 512,
  "connection_timeout_ms": 10000,
  "privacy": {
    "filter_file_paths": true,
    "filter_user_data": true,
    "allowed_attributes": ["crash_type", "stack_trace", "signal", "thread_id"]
  }
}
```

### Configuration Class
```cpp
// RiaPreferencesOpenTelemetry.h
class RiaPreferencesOpenTelemetry : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;
public:
    static RiaPreferencesOpenTelemetry* current();
    void setData(const std::map<QString, QString>& keyValuePairs);
    
    // Service name and version are hardcoded, not configurable
    QString serviceName() const { return "ResInsight"; }
    QString serviceVersion() const; // Read from ResInsightVersion.cmake
    
private:
    caf::PdmField<bool> m_enabled;
    caf::PdmField<QString> m_connectionString;  // Azure Application Insights connection string
    caf::PdmField<int> m_batchTimeoutMs;
    caf::PdmField<int> m_maxBatchSize;
    caf::PdmField<int> m_connectionTimeoutMs;
    // Privacy settings...
};
```

## 2. Data Filtering/Sanitization System

### Privacy Filter Design
```cpp
class RiaOpenTelemetryPrivacyFilter
{
public:
    struct FilterRules {
        bool filterFilePaths = true;
        bool filterUserData = true;
        std::set<std::string> allowedAttributes;
    };
    
    static std::string sanitizeStackTrace(const std::string& stackTrace);
    static std::map<std::string, std::string> filterAttributes(
        const std::map<std::string, std::string>& attributes);
    
private:
    static std::string removePersonalPaths(const std::string& path);
    static bool isSensitiveAttribute(const std::string& key);
};
```

### Filtering Logic
- **File Paths**: Replace user home directory with `<HOME>`, remove usernames
- **Stack Traces**: Keep function names, line numbers, remove absolute paths  
- **Sensitive Keywords**: Filter environment variables, credentials, personal data
- **Allowlist Approach**: Only explicitly allowed attributes are transmitted

## 3. Span Organization Strategy

### Hierarchical Span Structure
```
Application Root Span (ResInsight Session)
├── Feature Spans (File Operations, Visualization, Simulation)
│   ├── Operation Spans (Load Project, Generate Plot, Run Analysis)
│   │   └── Detail Spans (File I/O, Computation, Rendering)
│   └── Error Spans (Crashes, Exceptions, Failures)
└── Background Spans (Auto-save, Cache Updates)
```

### Span Categories
1. **Session Spans**: Application lifecycle, startup/shutdown
2. **Feature Spans**: Major functional areas (Eclipse data, visualization, wells)
3. **Operation Spans**: User actions (load file, create plot, export data)
4. **Error Spans**: Crash traces, exceptions, validation errors
5. **Performance Spans**: Long-running operations, memory usage

### Critical Spans for Crash Reporting
```cpp
void RiaOpenTelemetryTracer::reportCrash(int signalCode, const std::stacktrace& trace)
{
    auto span = tracer->StartSpan("crash.signal_handler");
    span->SetAttribute("crash.signal", signalCode);
    span->SetAttribute("crash.thread_id", std::this_thread::get_id());
    span->SetAttribute("crash.timestamp", std::chrono::system_clock::now());
    span->SetAttribute("crash.stack_trace", formatFilteredStackTrace(trace));
    span->SetStatus(opentelemetry::trace::StatusCode::kError, "Application crashed");
    span->End();
}
```

## 4. Async/Non-Blocking Implementation

### Thread Safety Design
```cpp
class RiaOpenTelemetryManager
{
public:
    static RiaOpenTelemetryManager& instance();
    void initializeAsync();
    void reportEventAsync(const std::string& event, 
                         const std::map<std::string, std::string>& attributes);
    void shutdown();
    
private:
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_connected{false};
    std::unique_ptr<std::thread> m_workerThread;
    std::queue<Event> m_eventQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCondition;
};
```

### Resilience Strategy
- **Circuit Breaker**: Disable after 3 consecutive connection failures
- **Exponential Backoff**: Retry with increasing delays (1s, 2s, 4s, 8s)
- **Local Queuing**: Buffer events in memory when server unavailable
- **Graceful Degradation**: Continue normal operation if telemetry fails
- **Timeout Management**: 10s connection timeout, 5s send timeout

### Background Processing
- Dedicated worker thread for all OpenTelemetry operations
- Non-blocking event queueing from main thread
- Batch processing to reduce network overhead
- Automatic retry mechanism with exponential backoff

## 5. Code Organization

### File Structure
```
ApplicationLibCode/
├── Application/Tools/Telemetry/
│   ├── RiaOpenTelemetryManager.h/.cpp      # Main manager class
│   ├── RiaOpenTelemetryTracer.h/.cpp       # Span creation and management
│   ├── RiaOpenTelemetryPrivacyFilter.h/.cpp # Data sanitization
│   └── RiaOpenTelemetryExporter.h/.cpp     # Custom exporter implementation
├── Application/
│   └── RiaPreferencesOpenTelemetry.h/.cpp  # Configuration management
└── ApplicationExeCode/
    └── RiaMainTools.cpp                     # Integrate with crash handler
```

### Integration Points
1. **Crash Handler** (`RiaMainTools.cpp:manageSegFailure`): Report crash spans
2. **Logging System** (`RiaLogging.cpp`): Optional telemetry forwarding 
3. **Application Startup** (`RiaGuiApplication.cpp`): Initialize telemetry manager
4. **Configuration Loading** (`RiaConnectorTools.cpp`): Load OpenTelemetry config
5. **Test Menu** (`RiuMainWindow.cpp`): Add test telemetry action

## 6. Implementation Steps

### Phase 1: Core Infrastructure
1. Add opentelemetry-cpp to vcpkg dependencies
2. Create configuration classes following OSDU pattern
3. Implement privacy filter with unit tests
4. Create async manager with circuit breaker

### Phase 2: Crash Reporting
1. Integrate with existing signal handler
2. Add stack trace sanitization
3. Create crash-specific span attributes
4. Add Test menu action for telemetry testing
5. Test with controlled crashes

### Phase 3: Extended Telemetry
1. Add span creation to major operations
2. Implement performance tracking spans
3. Add optional error forwarding from logging system
4. Create telemetry dashboard integration

### Phase 4: Production Readiness
1. Add comprehensive error handling
2. Implement configuration validation
3. Add telemetry for telemetry system health
4. Performance optimization and memory management

## 7. CMake Integration

### vcpkg Dependencies
```cmake
# Add to ThirdParty/CMakeLists.txt
find_package(opentelemetry-cpp CONFIG REQUIRED)
target_link_libraries(ApplicationLibCode PRIVATE 
    opentelemetry-cpp::opentelemetry_trace
    opentelemetry-cpp::opentelemetry_otlp_http_exporter)  # HTTP for Azure Application Insights
```

### Build Configuration
```cmake
option(RESINSIGHT_ENABLE_OPENTELEMETRY "Enable OpenTelemetry integration" OFF)
if(RESINSIGHT_ENABLE_OPENTELEMETRY)
    target_compile_definitions(ApplicationLibCode PRIVATE RESINSIGHT_OPENTELEMETRY_ENABLED)
endif()
```

## 8. Azure Application Insights Integration Notes

### Connection String Format
The configuration uses Azure Application Insights connection string format:
- **InstrumentationKey**: Legacy key for backwards compatibility
- **IngestionEndpoint**: HTTPS endpoint for telemetry data (Sweden Central region)
- **LiveEndpoint**: Real-time diagnostics endpoint
- **ApplicationId**: Unique application identifier

### Exporter Configuration
```cpp
// Initialize Azure Application Insights exporter
auto opts = opentelemetry::exporter::otlp::OtlpHttpExporterOptions();
opts.url = extractIngestionEndpoint(connectionString);  // Extract from connection string
opts.headers["x-ms-instrumentation-key"] = extractInstrumentationKey(connectionString);

auto exporter = std::make_unique<opentelemetry::exporter::otlp::OtlpHttpExporter>(opts);
```

### Azure-Specific Features
- **Application Map**: Automatic service dependency mapping
- **Live Metrics**: Real-time performance monitoring
- **Smart Detection**: AI-powered anomaly detection
- **Retention**: 90-day default retention for telemetry data

### Regional Deployment
- **Current Region**: Sweden Central (`swedencentral`)
- **Ingestion Endpoint**: `https://swedencentral-0.in.applicationinsights.azure.com/`
- **Benefits**: Reduced latency for Nordic/European users, GDPR compliance

## 9. Test Menu Integration

### Test Telemetry Action
Add a new action to the existing Test menu for sending test telemetry data:

```cpp
// In RiuMainWindow.cpp - createTestActions()
auto* testTelemetryAction = new QAction("Send Test Telemetry", this);
testTelemetryAction->setToolTip("Send test logging and stack trace to OpenTelemetry endpoint");
connect(testTelemetryAction, &QAction::triggered, this, &RiuMainWindow::slotSendTestTelemetry);
testMenu->addAction(testTelemetryAction);
```

### Test Implementation
```cpp
void RiuMainWindow::slotSendTestTelemetry()
{
    if (!RiaOpenTelemetryManager::instance().isEnabled()) {
        RiaLogging::warning("OpenTelemetry is not enabled or configured");
        return;
    }
    
    // Send test logging events
    RiaOpenTelemetryManager::instance().reportEventAsync("test.logging", {
        {"test_type", "manual_test"},
        {"timestamp", QDateTime::currentDateTime().toString(Qt::ISODate).toStdString()},
        {"user_action", "test_menu_triggered"}
    });
    
    // Generate and send test stack trace
    auto testStackTrace = std::stacktrace::current();
    RiaOpenTelemetryManager::instance().reportTestCrash(testStackTrace);
    
    RiaLogging::info("Test telemetry data sent to OpenTelemetry endpoint");
}
```

### Test Stack Trace Method
```cpp
// In RiaOpenTelemetryManager
void RiaOpenTelemetryManager::reportTestCrash(const std::stacktrace& trace)
{
    auto span = m_tracer->StartSpan("test.stack_trace");
    span->SetAttribute("test.type", "manual_stack_trace");
    span->SetAttribute("test.thread_id", std::this_thread::get_id());
    span->SetAttribute("test.timestamp", std::chrono::system_clock::now());
    span->SetAttribute("test.stack_trace", RiaOpenTelemetryPrivacyFilter::sanitizeStackTrace(
        internal::formatStacktrace(trace)));
    span->SetStatus(opentelemetry::trace::StatusCode::kOk, "Test stack trace generated");
    span->End();
}
```

### Menu Location
The test action will be added to the existing **Test** menu alongside other testing utilities, making it easily accessible for developers and QA testing.

## 10. Stack Trace Analysis on Azure Application Insights

### Data Location and Access
Stack traces sent via OpenTelemetry will appear in Azure Application Insights under several locations:

#### 1. Traces Table
- **Location**: Logs > Tables > `traces`
- **Query**: `traces | where message contains "crash" or message contains "stack_trace"`
- **Fields**: `timestamp`, `message`, `customDimensions`, `operation_Name`

#### 2. Custom Events
- **Location**: Logs > Tables > `customEvents`  
- **Query**: `customEvents | where name == "crash.signal_handler"`
- **Fields**: `timestamp`, `name`, `customDimensions` (contains stack trace)

### KQL Queries for Stack Trace Analysis

#### Basic Crash Investigation
```kusto
// Find all crashes in last 24 hours
traces
| where timestamp > ago(24h)
| where customDimensions.["crash.signal"] != ""
| project timestamp, 
          signal = customDimensions.["crash.signal"],
          thread_id = customDimensions.["crash.thread_id"],
          stack_trace = customDimensions.["crash.stack_trace"]
| order by timestamp desc
```

#### Crash Frequency Analysis
```kusto
// Group crashes by signal type
traces
| where timestamp > ago(7d)
| where customDimensions.["crash.signal"] != ""
| summarize count() by signal = tostring(customDimensions.["crash.signal"])
| order by count_ desc
```

#### Stack Trace Pattern Analysis
```kusto
// Find common crash patterns
traces
| where customDimensions.["crash.stack_trace"] != ""
| extend stack_trace = tostring(customDimensions.["crash.stack_trace"])
| extend top_function = extract(@"#0\s+([^\s]+)", 1, stack_trace)
| summarize count() by top_function
| order by count_ desc
```

### Analysis Capabilities

#### 1. **Crash Hotspots**
- **Function-level analysis**: Identify which functions appear most frequently in crashes
- **Module analysis**: Track crashes by source file or library
- **Thread analysis**: Correlate crashes with specific thread IDs

#### 2. **Time-based Analysis**
- **Crash trends**: Daily/weekly crash patterns
- **Version correlation**: Compare crash rates between ResInsight versions
- **User session impact**: Link crashes to user workflows

#### 3. **Environment Correlation**
```kusto
// Correlate crashes with system information
traces
| where customDimensions.["crash.signal"] != ""
| join kind=inner (
    customEvents
    | where name == "session.startup"
    | project session_id = operation_Id, 
              os_version = customDimensions.["os.version"],
              qt_version = customDimensions.["qt.version"]
) on $left.operation_Id == $right.session_id
| summarize crashes = count() by os_version, qt_version
```

### Azure Application Insights Features

#### 1. **Application Map**
- **Service dependencies**: Visualize ResInsight's interaction with external services
- **Failure rates**: See crash impact on overall application health
- **Performance impact**: Correlate crashes with response times

#### 2. **Smart Detection**
- **Anomaly detection**: Automatic alerts for unusual crash patterns
- **Failure rate increases**: Notifications when crash rates spike
- **Performance degradation**: Detect when crashes affect user experience

#### 3. **Live Metrics**
- **Real-time monitoring**: See crashes as they happen
- **Immediate investigation**: Quick access to recent crash data
- **System health**: Monitor application stability in real-time

### Workbook Templates

#### Crash Analysis Dashboard
```json
{
  "version": "Notebook/1.0",
  "items": [
    {
      "type": 3,
      "content": {
        "version": "KqlItem/1.0",
        "query": "traces | where customDimensions.['crash.signal'] != '' | summarize count() by bin(timestamp, 1h)",
        "title": "Crashes per Hour"
      }
    },
    {
      "type": 3,
      "content": {
        "version": "KqlItem/1.0",
        "query": "traces | where customDimensions.['crash.signal'] != '' | extend stack_trace = tostring(customDimensions.['crash.stack_trace']) | extend top_function = extract(@'#0\\s+([^\\s]+)', 1, stack_trace) | summarize count() by top_function",
        "title": "Top Crash Functions"
      }
    }
  ]
}
```

### Alert Configuration

#### Critical Crash Alert
- **Condition**: More than 5 crashes in 15 minutes
- **Action**: Email notification to development team
- **Query**: `traces | where customDimensions.["crash.signal"] != "" | summarize count()`

#### New Crash Pattern Alert
- **Condition**: New crash signature detected
- **Action**: Teams notification with stack trace details
- **Query**: `traces | where customDimensions.["crash.signal"] != "" | extend signature = hash_sha1(tostring(customDimensions.["crash.stack_trace"]))`

### Privacy and Compliance

#### Data Retention
- **Default**: 90 days for telemetry data
- **Compliance**: GDPR-compliant with data anonymization
- **Export**: Raw data can be exported for deeper analysis

#### Sensitive Data Handling
- **Filtered paths**: Personal directories replaced with `<HOME>`
- **Sanitized traces**: Function names preserved, personal info removed
- **Audit trail**: All queries and access logged for compliance

### Integration with Development Workflow

#### 1. **CI/CD Integration**
- **Automated alerts**: Notify on crash rate increases after deployments
- **Regression testing**: Compare crash patterns between versions
- **Quality gates**: Block releases if crash rates exceed thresholds

#### 2. **Issue Tracking**
- **Azure DevOps integration**: Automatically create work items for new crash patterns
- **GitHub integration**: Link crashes to code commits and pull requests
- **JIRA integration**: Create bugs with full stack trace context

This comprehensive analysis capability enables the development team to proactively identify, investigate, and resolve crashes in ResInsight deployments worldwide.

This plan provides a comprehensive, privacy-aware, and resilient OpenTelemetry integration that prioritizes crash reporting while maintaining ResInsight's performance and reliability.