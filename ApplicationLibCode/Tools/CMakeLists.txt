project(Tools)

find_package(
  Qt6
  COMPONENTS Core
  REQUIRED
)

set(SOURCE_FILES
    RiaHashTools.h
    RiaImageCompareReporter.h
    RiaImageCompareReporter.cpp
    RiaImageFileCompare.h
    RiaImageFileCompare.cpp
    RiaOpenMPTools.h
    RiaOpenMPTools.cpp
    RiaRegressionTextTools.h
    RiaRegressionTextTools.cpp
    RiaStringEncodingTools.h
    RiaStringEncodingTools.cpp
    RiaTimeTTools.h
    RiaTimeTTools.cpp
    RiaWeightedGeometricMeanCalculator.h
    RiaWeightedGeometricMeanCalculator.cpp
    RiaWeightedHarmonicMeanCalculator.h
    RiaWeightedHarmonicMeanCalculator.cpp
    RiaWeightedMeanCalculator.h
    RiaWeightedMeanCalculator.inl
    enum_bitmask.hpp
)

add_library(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} PUBLIC Qt6::Core regression-analysis)
