cmake_minimum_required(VERSION 3.26)

# C++26 Reflection Example
# Note: This requires experimental compiler support for C++26 reflection (P2996)

project(Cpp26ReflectionExample LANGUAGES CXX)

# Check compiler support
set(REFLECTION_SUPPORTED FALSE)
set(REFLECTION_FLAGS "")

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Clang with experimental reflection support
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "18.0")
        set(REFLECTION_SUPPORTED TRUE)
        set(REFLECTION_FLAGS "-std=c++2c;-freflection")
        message(STATUS "Clang ${CMAKE_CXX_COMPILER_VERSION} detected - enabling experimental C++26 reflection")
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # GCC - check for future support
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "15.0")
        set(REFLECTION_SUPPORTED TRUE)
        set(REFLECTION_FLAGS "-std=c++2c;-freflection")
        message(STATUS "GCC ${CMAKE_CXX_COMPILER_VERSION} detected - enabling experimental C++26 reflection")
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # MSVC - check for future support
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "19.40")
        set(REFLECTION_SUPPORTED TRUE)
        set(REFLECTION_FLAGS "/std:c++latest")
        message(STATUS "MSVC ${CMAKE_CXX_COMPILER_VERSION} detected - enabling experimental C++26 reflection")
    endif()
endif()

if(REFLECTION_SUPPORTED)
    add_executable(reflection_example reflection_example.cpp)

    target_compile_options(reflection_example PRIVATE ${REFLECTION_FLAGS})

    # Set C++ standard to latest
    set_target_properties(reflection_example PROPERTIES
        CXX_STANDARD 26
        CXX_STANDARD_REQUIRED OFF
        CXX_EXTENSIONS OFF
    )

    message(STATUS "C++26 Reflection example target created: reflection_example")
    message(STATUS "Build with: cmake --build . --target reflection_example")
else()
    message(WARNING "C++26 reflection is not supported by ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    message(WARNING "The reflection example will not be built.")
    message(WARNING "Requires:")
    message(WARNING "  - Clang 18+ with -freflection flag")
    message(WARNING "  - GCC 15+ (when available)")
    message(WARNING "  - MSVC 19.40+ (when available)")
    message(WARNING "See README.md for more information.")

    # Create a dummy target that explains the situation
    add_custom_target(reflection_example
        COMMAND ${CMAKE_COMMAND} -E echo "ERROR: C++26 reflection not supported by current compiler"
        COMMAND ${CMAKE_COMMAND} -E echo "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}"
        COMMAND ${CMAKE_COMMAND} -E echo "See Examples/Cpp26Reflection/README.md for requirements"
        COMMAND ${CMAKE_COMMAND} -E false
        COMMENT "C++26 reflection example (not buildable with current compiler)"
    )
endif()
