//##################################################################################################
//
//   Custom Visualization Core library
//   Copyright (C) 2011-2013 Ceetron AS
//
//   This library may be used under the terms of either the GNU General Public License or
//   the GNU Lesser General Public License as follows:
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
//   See the GNU General Public License at <<http://www.gnu.org/licenses/gpl.html>>
//   for more details.
//
//   GNU Lesser General Public License Usage
//   This library is free software; you can redistribute it and/or modify
//   it under the terms of the GNU Lesser General Public License as published by
//   the Free Software Foundation; either version 2.1 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//   See the GNU Lesser General Public License at <<http://www.gnu.org/licenses/lgpl-2.1.html>>
//   for more details.
//
//##################################################################################################

#include "cvfStructGridDefines.h"

void cvf::StructGridDefines::FaceEnum::setUp()
{
    addItem( cvf::StructGridDefines::POS_I, "POS I", "" );
    addItem( cvf::StructGridDefines::NEG_I, "NEG I", "" );
    addItem( cvf::StructGridDefines::POS_J, "POS J", "" );
    addItem( cvf::StructGridDefines::NEG_J, "NEG J", "" );
    addItem( cvf::StructGridDefines::POS_K, "POS K", "" );
    addItem( cvf::StructGridDefines::NEG_K, "NEG K", "" );
    addItem( cvf::StructGridDefines::NO_FACE, "UnDef", "" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void cvf::StructGridDefines::cellFaceVertexIndices( StructGridDefines::FaceType face, cvf::ubyte vertexIndices[4] )
{
    //
    //     7---------6
    //    /|        /|     |k
    //   / |       / |     | /j
    //  4---------5  |     |/
    //  |  3------|--2     *---i
    //  | /       | /
    //  |/        |/
    //  0---------1

    if ( face == StructGridDefines::NEG_K )
    {
        vertexIndices[0] = 0;
        vertexIndices[1] = 3;
        vertexIndices[2] = 2;
        vertexIndices[3] = 1;
    }
    else if ( face == StructGridDefines::POS_K )
    {
        vertexIndices[0] = 4;
        vertexIndices[1] = 5;
        vertexIndices[2] = 6;
        vertexIndices[3] = 7;
    }
    else if ( face == StructGridDefines::NEG_J )
    {
        vertexIndices[0] = 0;
        vertexIndices[1] = 1;
        vertexIndices[2] = 5;
        vertexIndices[3] = 4;
    }
    else if ( face == StructGridDefines::POS_I )
    {
        vertexIndices[0] = 1;
        vertexIndices[1] = 2;
        vertexIndices[2] = 6;
        vertexIndices[3] = 5;
    }
    else if ( face == StructGridDefines::POS_J )
    {
        vertexIndices[0] = 3;
        vertexIndices[1] = 7;
        vertexIndices[2] = 6;
        vertexIndices[3] = 2;
    }
    else if ( face == StructGridDefines::NEG_I )
    {
        vertexIndices[0] = 0;
        vertexIndices[1] = 4;
        vertexIndices[2] = 7;
        vertexIndices[3] = 3;
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
cvf::StructGridDefines::FaceType cvf::StructGridDefines::oppositeFace( StructGridDefines::FaceType face )
{
    StructGridDefines::FaceType opposite;

    switch ( face )
    {
        case StructGridDefines::NEG_I:
            opposite = StructGridDefines::POS_I;
            break;
        case StructGridDefines::POS_I:
            opposite = StructGridDefines::NEG_I;
            break;
        case StructGridDefines::NEG_J:
            opposite = StructGridDefines::POS_J;
            break;
        case StructGridDefines::POS_J:
            opposite = StructGridDefines::NEG_J;
            break;
        case StructGridDefines::NEG_K:
            opposite = StructGridDefines::POS_K;
            break;
        case StructGridDefines::POS_K:
            opposite = StructGridDefines::NEG_K;
            break;
        default:
            opposite = StructGridDefines::POS_I;
            CVF_ASSERT( false );
    }

    return opposite;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void cvf::StructGridDefines::neighborIJKAtCellFace( size_t                      i,
                                                    size_t                      j,
                                                    size_t                      k,
                                                    StructGridDefines::FaceType face,
                                                    size_t*                     ni,
                                                    size_t*                     nj,
                                                    size_t*                     nk )
{
    *ni = i;
    *nj = j;
    *nk = k;

    switch ( face )
    {
        case StructGridDefines::POS_I:
            ( *ni )++;
            break;
        case StructGridDefines::NEG_I:
            if ( i > 0 )
                ( *ni )--;
            else
                ( *ni ) = cvf::UNDEFINED_SIZE_T;
            break;
        case StructGridDefines::POS_J:
            ( *nj )++;
            break;
        case StructGridDefines::NEG_J:
            if ( j > 0 )
                ( *nj )--;
            else
                ( *nj ) = cvf::UNDEFINED_SIZE_T;
            break;
        case StructGridDefines::POS_K:
            ( *nk )++;
            break;
        case StructGridDefines::NEG_K:
            if ( k > 0 )
                ( *nk )--;
            else
                ( *nk ) = cvf::UNDEFINED_SIZE_T;
            break;
        default:
            break;
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
cvf::StructGridDefines::GridAxisType cvf::StructGridDefines::gridAxisFromFace( StructGridDefines::FaceType face )
{
    StructGridDefines::GridAxisType axis = StructGridDefines::GridAxisType::NO_AXIS;

    if ( face == cvf::StructGridDefines::POS_I || face == cvf::StructGridDefines::NEG_I )
    {
        axis = StructGridDefines::GridAxisType::AXIS_I;
    }
    else if ( face == cvf::StructGridDefines::POS_J || face == cvf::StructGridDefines::NEG_J )
    {
        axis = StructGridDefines::GridAxisType::AXIS_J;
    }
    else if ( face == cvf::StructGridDefines::POS_K || face == cvf::StructGridDefines::NEG_K )
    {
        axis = StructGridDefines::GridAxisType::AXIS_K;
    }

    return axis;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::pair<cvf::ubyte, cvf::ubyte> cvf::StructGridDefines::edgeVertexIndices( FaceType face1, FaceType face2 )
{
    // Return the two shared vertex indices between two faces
    // The ordering is identical to the ordering in StructGridDefines::cellFaceVertexIndices

    // Ensure face1 has the largest enum value
    if ( face2 > face1 ) std::swap( face1, face2 );

    if ( face1 == StructGridDefines::NEG_K )
    {
        if ( face2 == StructGridDefines::NEG_I ) return { 0, 3 };
        if ( face2 == StructGridDefines::POS_I ) return { 2, 1 };
        if ( face2 == StructGridDefines::NEG_J ) return { 1, 0 };
        if ( face2 == StructGridDefines::POS_J ) return { 3, 2 };
    }

    if ( face1 == StructGridDefines::POS_K )
    {
        if ( face2 == StructGridDefines::NEG_I ) return { 7, 4 };
        if ( face2 == StructGridDefines::POS_I ) return { 5, 6 };
        if ( face2 == StructGridDefines::NEG_J ) return { 4, 5 };
        if ( face2 == StructGridDefines::POS_J ) return { 6, 7 };
    }

    if ( face1 == StructGridDefines::NEG_J )
    {
        if ( face2 == StructGridDefines::NEG_I ) return { 4, 0 };
        if ( face2 == StructGridDefines::POS_I ) return { 1, 5 };
    }

    if ( face1 == StructGridDefines::POS_J )
    {
        if ( face2 == StructGridDefines::NEG_I ) return { 3, 7 };
        if ( face2 == StructGridDefines::POS_I ) return { 6, 2 };
    }

    return {};
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<cvf::StructGridDefines::FaceType> cvf::StructGridDefines::validFaceTypes()
{
    return {
        cvf::StructGridDefines::FaceType::NEG_I,
        cvf::StructGridDefines::FaceType::POS_I,
        cvf::StructGridDefines::FaceType::NEG_J,
        cvf::StructGridDefines::FaceType::POS_J,
        cvf::StructGridDefines::FaceType::NEG_K,
        cvf::StructGridDefines::FaceType::POS_K,
    };
}
