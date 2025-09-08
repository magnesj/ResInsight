//##################################################################################################
//
//   Custom Visualization Core library
//   Copyright (C) 2011-2013 Ceetron AS
//
//   GNU General Public License Usage
//   This library is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//##################################################################################################

#include "../cvfCellRange.h"
#include "cvfVector3.h"
#include "gtest/gtest.h"

using namespace cvf;

TEST( CvfCellRangeTest, DefaultConstruction )
{
    CellRange range;

    Vec3st min, max;
    range.range( min, max );

    EXPECT_EQ( Vec3st::UNDEFINED, min );
    EXPECT_EQ( Vec3st::UNDEFINED, max );
}

TEST( CvfCellRangeTest, ExplicitConstruction )
{
    CellRange range( 1, 2, 3, 5, 6, 7 );

    Vec3st min, max;
    range.range( min, max );

    EXPECT_EQ( 1u, min.x() );
    EXPECT_EQ( 2u, min.y() );
    EXPECT_EQ( 3u, min.z() );

    EXPECT_EQ( 5u, max.x() );
    EXPECT_EQ( 6u, max.y() );
    EXPECT_EQ( 7u, max.z() );
}

TEST( CvfCellRangeTest, Vec3stConstruction )
{
    Vec3st    minVec( 10, 20, 30 );
    Vec3st    maxVec( 40, 50, 60 );
    CellRange range( minVec, maxVec );

    Vec3st min, max;
    range.range( min, max );

    EXPECT_EQ( minVec, min );
    EXPECT_EQ( maxVec, max );
}

TEST( CvfCellRangeTest, SetRange )
{
    CellRange range;
    Vec3st    minVec( 5, 10, 15 );
    Vec3st    maxVec( 25, 30, 35 );

    range.setRange( minVec, maxVec );

    Vec3st min, max;
    range.range( min, max );

    EXPECT_EQ( minVec, min );
    EXPECT_EQ( maxVec, max );
}

TEST( CvfCellRangeTest, NormalizationSwapsMinMax )
{
    // Create range with max < min to test normalization
    CellRange range( 10, 20, 30, 5, 15, 25 );

    Vec3st min, max;
    range.range( min, max );

    // Should be normalized so min <= max
    EXPECT_EQ( 5u, min.x() );
    EXPECT_EQ( 15u, min.y() );
    EXPECT_EQ( 25u, min.z() );

    EXPECT_EQ( 10u, max.x() );
    EXPECT_EQ( 20u, max.y() );
    EXPECT_EQ( 30u, max.z() );
}

TEST( CvfCellRangeTest, IsInRangeBasic )
{
    CellRange range( 1, 2, 3, 5, 6, 7 );

    // Points inside range
    EXPECT_TRUE( range.isInRange( 1, 2, 3 ) );
    EXPECT_TRUE( range.isInRange( 3, 4, 5 ) );
    EXPECT_TRUE( range.isInRange( 4, 5, 6 ) );

    // Points on boundaries (inclusive min, exclusive max)
    EXPECT_TRUE( range.isInRange( 1, 2, 3 ) ); // min boundary
    EXPECT_FALSE( range.isInRange( 5, 6, 7 ) ); // max boundary (exclusive)

    // Points outside range
    EXPECT_FALSE( range.isInRange( 0, 2, 3 ) ); // below min in i
    EXPECT_FALSE( range.isInRange( 1, 1, 3 ) ); // below min in j
    EXPECT_FALSE( range.isInRange( 1, 2, 2 ) ); // below min in k
    EXPECT_FALSE( range.isInRange( 6, 6, 7 ) ); // above max in i
    EXPECT_FALSE( range.isInRange( 5, 7, 7 ) ); // above max in j
    EXPECT_FALSE( range.isInRange( 5, 6, 8 ) ); // above max in k
}

TEST( CvfCellRangeTest, IsInRangeSingleCell )
{
    CellRange range( 5, 10, 15, 6, 11, 16 );

    // Only one cell should be in range: [5,10,15]
    EXPECT_TRUE( range.isInRange( 5, 10, 15 ) );

    // Adjacent cells should be outside
    EXPECT_FALSE( range.isInRange( 4, 10, 15 ) );
    EXPECT_FALSE( range.isInRange( 6, 10, 15 ) );
    EXPECT_FALSE( range.isInRange( 5, 9, 15 ) );
    EXPECT_FALSE( range.isInRange( 5, 11, 15 ) );
    EXPECT_FALSE( range.isInRange( 5, 10, 14 ) );
    EXPECT_FALSE( range.isInRange( 5, 10, 16 ) );
}

TEST( CvfCellRangeTest, EmptyRangeNormalization )
{
    CellRange emptyRange;

    EXPECT_FALSE( emptyRange.normalize() );
}