#include "cafVecIjk.h"
#include <gtest/gtest.h>

using namespace caf;

// Test the constructor and accessors
TEST( VecIjkTest, ConstructorAndAccessors )
{
    VecIjk vec( 1, 2, 3 );

    EXPECT_EQ( vec.i(), 1 );
    EXPECT_EQ( vec.j(), 2 );
    EXPECT_EQ( vec.k(), 3 );
}

// Test the toString method
TEST( VecIjkTest, ToString )
{
    VecIjk      vec( 1, 2, 3 );
    std::string expectedString = "1, 2, 3";
    EXPECT_EQ( vec.toString(), expectedString );
}

// Test the constructor with zero values
TEST( VecIjkTest, ConstructorWithZeroValues )
{
    VecIjk vec( 0, 0, 0 );

    EXPECT_EQ( vec.i(), 0 );
    EXPECT_EQ( vec.j(), 0 );
    EXPECT_EQ( vec.k(), 0 );
}

// Test the constructor with large values
TEST( VecIjkTest, ConstructorWithLargeValues )
{
    VecIjk vec( 1000000, 2000000, 3000000 );

    EXPECT_EQ( vec.i(), 1000000 );
    EXPECT_EQ( vec.j(), 2000000 );
    EXPECT_EQ( vec.k(), 3000000 );
}
