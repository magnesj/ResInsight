cmake_minimum_required(VERSION 2.8.12)

project(cafPdmScripting_UnitTests)

find_package(
  Qt5
  COMPONENTS
  REQUIRED Core Xml Gui)
set(QT_LIBRARIES Qt5::Core Qt5::Xml Qt5::Gui)

if(MSVC AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 19.11))
  # VS 2017 : Disable warnings from from gtest code, using deprecated code
  # related to TR1
  add_definitions(-D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)
  message(
    STATUS
      "Add flag to disable warings from gtest -  _SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING"
  )
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR} # required for gtest-all.cpp
)

# add the executable
add_executable(${PROJECT_NAME} cafPdmScripting_UnitTests.cpp
                               gtest/gtest-all.cpp cafPdmScriptingBasicTest.cpp)

target_link_libraries(${PROJECT_NAME} cafPdmScripting ${QT_LIBRARIES}
                      ${THREAD_LIBRARY})

source_group("" FILES ${PROJECT_FILES})

# Copy Qt Dlls
if(Qt5Core_FOUND)
  foreach(qtlib ${QT_LIBRARIES})
    add_custom_command(
      TARGET ${PROJECT_NAME}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${qtlib}>
              $<TARGET_FILE_DIR:${PROJECT_NAME}>)
  endforeach(qtlib)
endif(Qt5Core_FOUND)
