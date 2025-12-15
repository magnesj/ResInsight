project(ResultStatisticsCache)

add_library(
  ${PROJECT_NAME}
  RigStatisticsCalculator.h RigStatisticsCalculator.cpp
  RigStatisticsDataCache.h RigStatisticsDataCache.cpp RigStatisticsMath.h
  RigStatisticsMath.cpp
)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE $<TARGET_PROPERTY:ApplicationLibCode,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(${PROJECT_NAME} PRIVATE LibCore)
# ApplicationLibCode removed - linked with WHOLE_ARCHIVE in executables
