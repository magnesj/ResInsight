project(AzureOtelClient)

# Minimal library for Azure OpenTelemetry client Dependencies: opentelemetry-cpp
# and Qt6::Core only

set(SOURCE_FILES RiaAzureOtelClient.h RiaAzureOtelClient.cpp)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

# Find dependencies
find_package(nlohmann_json CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(opentelemetry-cpp CONFIG REQUIRED)

# Minimal dependencies
target_link_libraries(
  ${PROJECT_NAME}
  PUBLIC opentelemetry-cpp::api
         opentelemetry-cpp::common
         opentelemetry-cpp::trace
         opentelemetry-cpp::logs
         opentelemetry-cpp::resources
         opentelemetry-cpp::otlp_http_exporter
         opentelemetry-cpp::otlp_http_log_record_exporter
         opentelemetry-cpp::http_client_curl
)

# Make headers available to parent projects
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
