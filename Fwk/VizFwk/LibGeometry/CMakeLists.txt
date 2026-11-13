project(LibGeometry)

# CMake Unity Build
if (CVF_ENABLE_UNITY_BUILD)
  message("Cmake Unity build is enabled on : ${PROJECT_NAME}")
  set(CMAKE_UNITY_BUILD true)
endif()

# Use our strict compile flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${CEE_STRICT_CXX_FLAGS}")

# For now, disable warning about unknown pragmas locally here (due to usage of OpenMP)
if (CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unknown-pragmas")
endif()


set(CEE_HEADER_FILES
cvfArrowGenerator.h
cvfBoundingBox.h
cvfBoundingBoxTree.h
cvfBoxGenerator.h
cvfEdgeKey.h
cvfFrustum.h
cvfGeometryBuilder.h
cvfGeometryBuilderFaceList.h
cvfGeometryBuilderTriangles.h
cvfGeometryUtils.h
cvfLibGeometry.h
cvfMeshEdgeExtractor.h
cvfOutlineEdgeExtractor.h
cvfPatchGenerator.h
cvfPrimitiveTests.h
cvfRay.h
cvfRobustPredicates.h
cvfTriangleMeshEdgeExtractor.h
cvfTriangleVertexSplitter.h
cvfVertexCompactor.h
cvfVertexWelder.h
)

set(CEE_SOURCE_FILES
cvfArrowGenerator.cpp
cvfBoundingBox.cpp
cvfBoundingBoxTree.cpp
cvfBoxGenerator.cpp
cvfFrustum.cpp
cvfEdgeKey.cpp
cvfGeometryBuilder.cpp
cvfGeometryBuilderFaceList.cpp
cvfGeometryBuilderTriangles.cpp
cvfGeometryUtils.cpp
cvfMeshEdgeExtractor.cpp
cvfOutlineEdgeExtractor.cpp
cvfPatchGenerator.cpp
cvfPrimitiveTests.cpp
cvfRay.cpp
cvfRobustPredicates.cpp
cvfTriangleMeshEdgeExtractor.cpp
cvfTriangleVertexSplitter.cpp
cvfVertexCompactor.cpp
cvfVertexWelder.cpp
)

# Vendored exact geometric predicates, compiled with its own flags (not subject to strict flags)
if (NOT TARGET ShewchukPredicates)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../ThirdParty/ShewchukPredicates ${CMAKE_CURRENT_BINARY_DIR}/ShewchukPredicates)
endif()

add_library(${PROJECT_NAME} ${CEE_HEADER_FILES} ${CEE_SOURCE_FILES})

target_include_directories(${PROJECT_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries ( ${PROJECT_NAME}
    PUBLIC LibCore
    PRIVATE ShewchukPredicates
)

set(PROJECT_FILES ${CEE_HEADER_FILES} ${CEE_SOURCE_FILES})
source_group("" FILES ${PROJECT_FILES})
