cmake_minimum_required(VERSION 3.15)

project(cafPdmInitPrototype)

# Prototype for issue #1202: replace the CAF_PDM_Init* macros with functions.
# Nothing in this folder is used by the application.

find_package(
  Qt6
  COMPONENTS
  REQUIRED Core Gui Widgets
)
qt_standard_project_setup()

# Library with the prototype functions
add_library(
  ${PROJECT_NAME}
  cafProtoFieldInit.cpp cafProtoFieldInit.h cafProtoFixedString.h
  cafProtoKeyword.h cafProtoScripting.h cafProtoStaticText.h
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC cafProjectDataModel Qt6::Core)

# Compile-time benchmark. OBJECT library with unity build disabled, each file is
# one isolated translation unit. Compare with cafPdm_CompileBench.
add_library(
  ${PROJECT_NAME}_CompileBench OBJECT
  compile_bench/bench_mixed_function.cpp
  compile_bench/bench_mixed_function_positional.cpp
  compile_bench/bench_mixed_fixedstring.cpp
  compile_bench/bench_scriptable_macro.cpp
  compile_bench/bench_scriptable_function.cpp
  compile_bench/bench_scriptable_fixedstring.cpp
  compile_bench/bench_include_macro.cpp
  compile_bench/bench_include_function.cpp
)
set_target_properties(
  ${PROJECT_NAME}_CompileBench PROPERTIES UNITY_BUILD OFF
)
target_link_libraries(
  ${PROJECT_NAME}_CompileBench PRIVATE ${PROJECT_NAME} cafPdmScripting
                                       Qt6::Core Qt6::Widgets
)

# Parity tests and runtime benchmark
if(MSVC)
  add_definitions(-D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)
endif()

set(GTEST_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/../../cafPdmScripting/cafPdmScripting_UnitTests
)

qt_add_executable(
  ${PROJECT_NAME}_UnitTests
  tests/cafProtoUnitTests.cpp
  tests/cafProtoParityTest.cpp
  tests/cafProtoRuntimeBenchmark.cpp
  tests/cafProtoTestObjects.cpp
  tests/cafProtoTestObjects.h
  ${GTEST_DIR}/gtest/gtest-all.cpp
)
set_target_properties(${PROJECT_NAME}_UnitTests PROPERTIES UNITY_BUILD OFF)
target_include_directories(${PROJECT_NAME}_UnitTests PRIVATE ${GTEST_DIR})
target_link_libraries(
  ${PROJECT_NAME}_UnitTests PRIVATE ${PROJECT_NAME} cafPdmScripting Qt6::Core
                                    Qt6::Gui Qt6::Widgets ${THREAD_LIBRARY}
)
