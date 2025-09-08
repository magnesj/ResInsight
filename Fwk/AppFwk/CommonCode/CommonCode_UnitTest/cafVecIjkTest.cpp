//##################################################################################################
//
//   Custom Visualization Core library
//   Copyright (C) 2017 Ceetron Solutions AS
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

#include "../cafVecIjk.h"
#include "gtest/gtest.h"

using namespace caf;

TEST( CafVecIjkTest, BasicConstruction )
{
    VecIjk vec( 10, 20, 30 );

    EXPECT_EQ( 10u, vec.i() );
    EXPECT_EQ( 20u, vec.j() );
    EXPECT_EQ( 30u, vec.k() );
}

TEST( CafVecIjkTest, ZeroBasedToOneBased )
{
    VecIjk zeroBased( 0, 5, 10 );
    VecIjk oneBased = zeroBased.toOneBased();

    EXPECT_EQ( 1u, oneBased.i() );
    EXPECT_EQ( 6u, oneBased.j() );
    EXPECT_EQ( 11u, oneBased.k() );
}

TEST( CafVecIjkTest, OneBasedToZeroBased )
{
    VecIjk oneBased( 1, 6, 11 );
    VecIjk zeroBased = oneBased.toZeroBased();

    EXPECT_EQ( 0u, zeroBased.i() );
    EXPECT_EQ( 5u, zeroBased.j() );
    EXPECT_EQ( 10u, zeroBased.k() );
}

TEST( CafVecIjkTest, EqualityComparison )
{
    VecIjk vec1( 1, 2, 3 );
    VecIjk vec2( 1, 2, 3 );
    VecIjk vec3( 1, 2, 4 );

    EXPECT_EQ( vec1.i(), vec2.i() );
    EXPECT_EQ( vec1.j(), vec2.j() );
    EXPECT_EQ( vec1.k(), vec2.k() );

    EXPECT_NE( vec1.k(), vec3.k() );
}

TEST( CafVecIjkTest, StringConversion )
{
    VecIjk      vec( 10, 20, 30 );
    std::string str = vec.toString();

    EXPECT_TRUE( str.find( "10" ) != std::string::npos );
    EXPECT_TRUE( str.find( "20" ) != std::string::npos );
    EXPECT_TRUE( str.find( "30" ) != std::string::npos );
}