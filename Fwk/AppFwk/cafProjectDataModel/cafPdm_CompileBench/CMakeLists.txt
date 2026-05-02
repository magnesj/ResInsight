cmake_minimum_required(VERSION 3.15)

project(cafPdm_CompileBench)

find_package(
  Qt6
  COMPONENTS
  REQUIRED Core Widgets
)
qt_standard_project_setup()

set(PROJECT_FILES
    bench_basic_fields.cpp bench_enum_fields.cpp bench_container_fields.cpp
    bench_filepath_fields.cpp bench_mixed_fields.cpp bench_heavy_fields.cpp
    bench_basic_fields_template.cpp bench_enum_fields_template.cpp
    bench_container_fields_template.cpp bench_filepath_fields_template.cpp
    bench_mixed_fields_template.cpp bench_heavy_fields_template.cpp
)

# OBJECT library: compiles each file individually without linking. This isolates
# the compile-time cost per translation unit.
add_library(${PROJECT_NAME} OBJECT ${PROJECT_FILES})

# Disable unity build for this target — each file must compile independently
set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD OFF)

# bench_heavy_fields_template.cpp generates many per-template-instantiation
# sections (one function-local static per (Class, Keyword) pair) and exceeds the
# default COFF section limit. /bigobj raises the limit; impact on compile time
# is negligible.
if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /bigobj)
endif()

target_link_libraries(
  ${PROJECT_NAME} PRIVATE cafProjectDataModel Qt6::Core Qt6::Widgets
)
