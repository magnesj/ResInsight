
#include "gtest/gtest.h"

#include "cvfCellRange.h"

using namespace cvf;

// Test default constructor
TEST( CellRangeTest, DefaultConstructor )
{
    CellRange range;
    Vec3st    min, max;
    range.range( min, max );

    EXPECT_TRUE( min == cvf::Vec3st::UNDEFINED );
    EXPECT_TRUE( max == cvf::Vec3st::UNDEFINED );
}

// Test parameterized constructor with Vec3st
TEST( CellRangeTest, ParameterizedConstructorVec3st )
{
    Vec3st    min( 1, 2, 3 );
    Vec3st    max( 4, 5, 6 );
    CellRange range( min, max );
    Vec3st    minResult, maxResult;
    range.range( minResult, maxResult );

    EXPECT_EQ( minResult.x(), 1 );
    EXPECT_EQ( minResult.y(), 2 );
    EXPECT_EQ( minResult.z(), 3 );
    EXPECT_EQ( maxResult.x(), 4 );
    EXPECT_EQ( maxResult.y(), 5 );
    EXPECT_EQ( maxResult.z(), 6 );
}

// Test parameterized constructor with individual values
TEST( CellRangeTest, ParameterizedConstructorValues )
{
    CellRange range( 1, 2, 3, 4, 5, 6 );
    Vec3st    min, max;
    range.range( min, max );

    EXPECT_EQ( min.x(), 1 );
    EXPECT_EQ( min.y(), 2 );
    EXPECT_EQ( min.z(), 3 );
    EXPECT_EQ( max.x(), 4 );
    EXPECT_EQ( max.y(), 5 );
    EXPECT_EQ( max.z(), 6 );
}

// Test setRange method
TEST( CellRangeTest, SetRange )
{
    CellRange range;
    Vec3st    min( 1, 2, 3 );
    Vec3st    max( 4, 5, 6 );
    range.setRange( min, max );
    Vec3st minResult, maxResult;
    range.range( minResult, maxResult );

    EXPECT_EQ( minResult.x(), 1 );
    EXPECT_EQ( minResult.y(), 2 );
    EXPECT_EQ( minResult.z(), 3 );
    EXPECT_EQ( maxResult.x(), 4 );
    EXPECT_EQ( maxResult.y(), 5 );
    EXPECT_EQ( maxResult.z(), 6 );
}

// Test normalize method
TEST( CellRangeTest, Normalize )
{
    CellRange range( 4, 5, 6, 1, 2, 3 );
    EXPECT_TRUE( range.normalize() );
    Vec3st min, max;
    range.range( min, max );

    EXPECT_EQ( min.x(), 1 );
    EXPECT_EQ( min.y(), 2 );
    EXPECT_EQ( min.z(), 3 );
    EXPECT_EQ( max.x(), 4 );
    EXPECT_EQ( max.y(), 5 );
    EXPECT_EQ( max.z(), 6 );
}

// Test isInRange method
TEST( CellRangeTest, IsInRange )
{
    CellRange range( 1, 2, 3, 4, 5, 6 );

    EXPECT_TRUE( range.isInRange( 2, 3, 4 ) );
    EXPECT_FALSE( range.isInRange( 0, 3, 4 ) );
    EXPECT_FALSE( range.isInRange( 2, 6, 4 ) );
    EXPECT_FALSE( range.isInRange( 2, 3, 7 ) );
}
