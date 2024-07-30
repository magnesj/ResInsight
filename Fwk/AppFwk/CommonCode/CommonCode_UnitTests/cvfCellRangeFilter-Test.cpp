
#include "cvfCellRangeFilter.h"
#include <gtest/gtest.h>

using namespace cvf;

// Test default constructor
TEST( CellRangeFilterTest, DefaultConstructor )
{
    CellRangeFilter filter;
    EXPECT_FALSE( filter.hasIncludeRanges() );
}

// Test addCellIncludeRange and hasIncludeRanges
TEST( CellRangeFilterTest, AddCellIncludeRange )
{
    CellRangeFilter filter;
    filter.addCellIncludeRange( 1, 2, 3, 4, 5, 6, true );
    EXPECT_TRUE( filter.hasIncludeRanges() );
}

// Test addCellInclude
TEST( CellRangeFilterTest, AddCellInclude )
{
    CellRangeFilter filter;
    filter.addCellInclude( 1, 2, 3, true );
    EXPECT_TRUE( filter.hasIncludeRanges() );
}

// Test addCellExcludeRange
TEST( CellRangeFilterTest, AddCellExcludeRange )
{
    CellRangeFilter filter;
    filter.addCellExcludeRange( 1, 2, 3, 4, 5, 6, true );
    EXPECT_TRUE( filter.isCellExcluded( 2, 3, 4, true ) );
    EXPECT_FALSE( filter.isCellExcluded( 0, 0, 0, true ) );
}

// Test addCellExclude
TEST( CellRangeFilterTest, AddCellExclude )
{
    CellRangeFilter filter;
    filter.addCellExclude( 1, 2, 3, true );
    EXPECT_TRUE( filter.isCellExcluded( 1, 2, 3, true ) );
    EXPECT_FALSE( filter.isCellExcluded( 0, 0, 0, true ) );
}

// Test isCellVisible
TEST( CellRangeFilterTest, IsCellVisible )
{
    CellRangeFilter filter;
    filter.addCellIncludeRange( 1, 2, 3, 4, 5, 6, true );
    filter.addCellExclude( 2, 3, 4, true );

    EXPECT_TRUE( filter.isCellVisible( 1, 2, 3, true ) );
    EXPECT_FALSE( filter.isCellVisible( 2, 3, 4, true ) );
    EXPECT_FALSE( filter.isCellVisible( 0, 0, 0, true ) );
}
