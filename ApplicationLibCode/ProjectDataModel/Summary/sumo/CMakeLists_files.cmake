set(SOURCE_GROUP_HEADER_FILES ${CMAKE_CURRENT_LIST_DIR}/RimSummaryEnsembleSumo.h
                              ${CMAKE_CURRENT_LIST_DIR}/RimSummaryCaseSumo.h
)

set(SOURCE_GROUP_SOURCE_FILES
    ${CMAKE_CURRENT_LIST_DIR}/RimSummaryEnsembleSumo.cpp
    ${CMAKE_CURRENT_LIST_DIR}/RimSummaryCaseSumo.cpp
)

list(APPEND CODE_HEADER_FILES ${SOURCE_GROUP_HEADER_FILES})

list(APPEND CODE_SOURCE_FILES ${SOURCE_GROUP_SOURCE_FILES})

source_group(
  "ProjectDataModel\\Summary"
  FILES ${SOURCE_GROUP_HEADER_FILES} ${SOURCE_GROUP_SOURCE_FILES}
        ${CMAKE_CURRENT_LIST_DIR}/CMakeLists_files.cmake
)
