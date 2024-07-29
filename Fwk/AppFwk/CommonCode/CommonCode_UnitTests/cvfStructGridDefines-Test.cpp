
#include "gtest/gtest.h"

#include "../cvfStructGridDefines.h"
#include "cvfMath.h"

using namespace cvf;
using namespace cvf::StructGridDefines;

// Test for cellFaceVertexIndices
TEST( StructGridDefinesTest, CellFaceVertexIndices )
{
    ubyte vertexIndices[4];

    cellFaceVertexIndices( POS_I, vertexIndices );
    EXPECT_EQ( vertexIndices[0], 1 );
    EXPECT_EQ( vertexIndices[1], 2 );
    EXPECT_EQ( vertexIndices[2], 6 );
    EXPECT_EQ( vertexIndices[3], 5 );

    cellFaceVertexIndices( NEG_I, vertexIndices );
    EXPECT_EQ( vertexIndices[0], 0 );
    EXPECT_EQ( vertexIndices[1], 4 );
    EXPECT_EQ( vertexIndices[2], 7 );
    EXPECT_EQ( vertexIndices[3], 3 );

    // Add more tests for other faces...
}

// Test for oppositeFace
TEST( StructGridDefinesTest, OppositeFace )
{
    EXPECT_EQ( oppositeFace( POS_I ), NEG_I );
    EXPECT_EQ( oppositeFace( NEG_I ), POS_I );
    EXPECT_EQ( oppositeFace( POS_J ), NEG_J );
    EXPECT_EQ( oppositeFace( NEG_J ), POS_J );
    EXPECT_EQ( oppositeFace( POS_K ), NEG_K );
    EXPECT_EQ( oppositeFace( NEG_K ), POS_K );
}

// Test for neighborIJKAtCellFace
TEST( StructGridDefinesTest, NeighborIJKAtCellFace )
{
    size_t ni, nj, nk;

    neighborIJKAtCellFace( 1, 1, 1, POS_I, &ni, &nj, &nk );
    EXPECT_EQ( ni, 2 );
    EXPECT_EQ( nj, 1 );
    EXPECT_EQ( nk, 1 );

    neighborIJKAtCellFace( 1, 1, 1, NEG_I, &ni, &nj, &nk );
    EXPECT_EQ( ni, 0 );
    EXPECT_EQ( nj, 1 );
    EXPECT_EQ( nk, 1 );

    neighborIJKAtCellFace( 0, 1, 1, NEG_I, &ni, &nj, &nk );
    EXPECT_EQ( ni, cvf::UNDEFINED_SIZE_T );
    EXPECT_EQ( nj, 1 );
    EXPECT_EQ( nk, 1 );

    // Add more tests for other faces...
}

// Test for gridAxisFromFace
TEST( StructGridDefinesTest, GridAxisFromFace )
{
    EXPECT_EQ( gridAxisFromFace( POS_I ), GridAxisType::AXIS_I );
    EXPECT_EQ( gridAxisFromFace( NEG_I ), GridAxisType::AXIS_I );
    EXPECT_EQ( gridAxisFromFace( POS_J ), GridAxisType::AXIS_J );
    EXPECT_EQ( gridAxisFromFace( NEG_J ), GridAxisType::AXIS_J );
    EXPECT_EQ( gridAxisFromFace( POS_K ), GridAxisType::AXIS_K );
    EXPECT_EQ( gridAxisFromFace( NEG_K ), GridAxisType::AXIS_K );
}

// Test for edgeVertexIndices
TEST( StructGridDefinesTest, EdgeVertexIndices )
{
    auto result = edgeVertexIndices( NEG_K, NEG_I );
    EXPECT_EQ( result.first, 0 );
    EXPECT_EQ( result.second, 3 );

    result = edgeVertexIndices( POS_K, POS_I );
    EXPECT_EQ( result.first, 5 );
    EXPECT_EQ( result.second, 6 );

    // Add more tests for other face pairs...
}

// Test for validFaceTypes
TEST( StructGridDefinesTest, ValidFaceTypes )
{
    auto faces = validFaceTypes();
    EXPECT_EQ( faces.size(), 6 );
    EXPECT_NE( std::find( faces.begin(), faces.end(), POS_I ), faces.end() );
    EXPECT_NE( std::find( faces.begin(), faces.end(), NEG_I ), faces.end() );
    EXPECT_NE( std::find( faces.begin(), faces.end(), POS_J ), faces.end() );
    EXPECT_NE( std::find( faces.begin(), faces.end(), NEG_J ), faces.end() );
    EXPECT_NE( std::find( faces.begin(), faces.end(), POS_K ), faces.end() );
    EXPECT_NE( std::find( faces.begin(), faces.end(), NEG_K ), faces.end() );
}
