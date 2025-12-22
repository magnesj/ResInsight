project(AzureOtelClient)

# Minimal library for Azure OpenTelemetry client
# Dependencies: opentelemetry-cpp and Qt6::Core only

set(SOURCE_FILES
    RiaAzureOtelClient.h
    RiaAzureOtelClient.cpp
)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

# Minimal dependencies
target_link_libraries(${PROJECT_NAME}
    PUBLIC
        opentelemetry-cpp::api
        opentelemetry-cpp::sdk
        opentelemetry-cpp::ext
        opentelemetry-cpp::logs
)

# Make headers available to parent projects
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
