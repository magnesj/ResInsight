/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2021-     Equinor ASA
//
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
//
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////

#include "gtest/gtest.h"

#include "cvfStructGrid.h"

using namespace cvf;

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( StructGridInterfaceTest, TestEdgeVertices )
{
    {
        // Baseline

        StructGridDefines::FaceType face1 = StructGridDefines::NEG_K;
        StructGridDefines::FaceType face2 = StructGridDefines::POS_I;

        auto indices = StructGridInterface::edgeVertexIndices( face1, face2 );
        EXPECT_EQ( indices.first, 2 );
        EXPECT_EQ( indices.second, 1 );
    }

    {
        // Opposite order as baseline test

        StructGridDefines::FaceType face1 = StructGridDefines::POS_I;
        StructGridDefines::FaceType face2 = StructGridDefines::NEG_K;

        auto indices = StructGridInterface::edgeVertexIndices( face1, face2 );
        EXPECT_EQ( indices.first, 2 );
        EXPECT_EQ( indices.second, 1 );
    }
}
