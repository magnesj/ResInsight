cmake_minimum_required(VERSION 3.15)

project(CommonCode_UnitTests)

# Qt
if(CEE_USE_QT6)
  find_package(
    Qt6
    COMPONENTS
    REQUIRED Core Widgets Gui
  )
  qt_standard_project_setup()
else()
  find_package(
    Qt5
    COMPONENTS
    REQUIRED Core Gui Widgets
  )
  set(QT_LIBRARIES Qt5::Core Qt5::Widgets Qt5::Gui)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR} # required for gtest-all.cpp
)

find_package(OpenGL)


set(PROJECT_FILES
    CommonCode_UnitTests.cpp
    gtest/gtest-all.cpp
    cvfCellRange-Test.cpp
    cvfCellRangeFilter-Test.cpp
    cvfStructGridDefines-Test.cpp
)

if(CEE_USE_QT6)
  qt_add_executable(${PROJECT_NAME} ${PROJECT_FILES})
else()
  add_executable(${PROJECT_NAME} ${PROJECT_FILES})
endif(CEE_USE_QT6)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

  target_compile_options(
    CommonCode_UnitTests PRIVATE -Wno-ambiguous-reversed-operator
                                 -Wno-invalid-source-encoding
  )
endif()

source_group("" FILES ${PROJECT_FILES})

if(Qt5Core_FOUND)
  set(QT_LIBRARIES Qt5::Core Qt5::Gui Qt5::Widgets)
endif()

target_link_libraries(
  ${PROJECT_NAME} PRIVATE CommonCode cafPdmCore LibCore LibRender ${QT_LIBRARIES} ${OPENGL_LIBRARIES} ${THREAD_LIBRARY}
)

# Copy Qt Dlls
foreach(qtlib ${QT_LIBRARIES})
  add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${qtlib}>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
  )
endforeach(qtlib)

# Install
install(
  TARGETS ${PROJECT_NAME}
  BUNDLE DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(CEE_USE_QT6)
  qt_generate_deploy_app_script(
    TARGET ${PROJECT_NAME} OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR NO_TRANSLATIONS
  )
  install(SCRIPT ${deploy_script})
endif(CEE_USE_QT6)
