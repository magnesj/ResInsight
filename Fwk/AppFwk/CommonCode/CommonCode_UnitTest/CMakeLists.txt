project(CommonCode_UnitTests)

if(MSVC AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 19.11))
  # VS 2017 : Disable warnings from from gtest code, using deprecated code
  # related to TR1
  add_definitions(-D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)
  message(
    STATUS
      "Add flag to disable warings from gtest -  _SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING"
  )
endif()

set(VIZ_MODULES_FOLDER_NAME ../../../../Fwk/VizFwk)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} # required for gtest-all.cpp
)

set(PROJECT_FILES cafAngleUtilsTest.cpp
                  cafVecIjkTest.cpp
                  cvfCellRangeTest.cpp
                  cvfStructGridTest.cpp
                  CommonCode_UnitTests_main.cpp
)

# add the executable
add_executable(${PROJECT_NAME} ${PROJECT_FILES} gtest/gtest-all.cpp)

target_link_libraries(${PROJECT_NAME} LibCore CommonCode)

# Qt
find_package(
  Qt6
  COMPONENTS
  REQUIRED Core Gui Widgets OpenGL
)
set(QT_LIBRARIES Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL)

target_link_libraries(${PROJECT_NAME} ${QT_LIBRARIES})