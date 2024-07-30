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
#include "cvfMath.h"

#include <array>
#include <map>

template <>
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

namespace cvf
{
namespace StructGridDefines
{
    //--------------------------------------------------------------------------------------------------
    /// Helper function to create a map of face type to vertex indices. Use of std::array and std::pair allows for
    /// consteval keyword.
    //--------------------------------------------------------------------------------------------------
    consteval auto cubeFaceIndices()
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

        return std::array<std::pair<cvf::StructGridDefines::FaceType, std::array<cvf::ubyte, 4>>, 6>{
            { { FaceType::NEG_K, { 0, 3, 2, 1 } },
              { FaceType::POS_K, { 4, 5, 6, 7 } },
              { FaceType::NEG_J, { 0, 1, 5, 4 } },
              { FaceType::POS_J, { 3, 7, 6, 2 } },
              { FaceType::NEG_I, { 0, 4, 7, 3 } },
              { FaceType::POS_I, { 1, 2, 6, 5 } } } };
    }

    //--------------------------------------------------------------------------------------------------
    ///
    //--------------------------------------------------------------------------------------------------
    auto cvf::StructGridDefines::createFaceIndicesMap()
    {
        std::map<cvf::StructGridDefines::FaceType, std::array<cvf::ubyte, 4>> faultFaceToFaceIdxs;

        for ( const auto& [key, value] : cubeFaceIndices() )
        {
            faultFaceToFaceIdxs[key] = value;
        }

        return faultFaceToFaceIdxs;
    }

    //--------------------------------------------------------------------------------------------------
    ///
    //--------------------------------------------------------------------------------------------------
    void cvf::StructGridDefines::cellFaceVertexIndices( StructGridDefines::FaceType face, cvf::ubyte vertexIndices[4] )
    {
        static const auto faceIndicesMap = createFaceIndicesMap();

        const auto& faceIndices = faceIndicesMap.at( face );
        vertexIndices[0]        = faceIndices[0];
        vertexIndices[1]        = faceIndices[1];
        vertexIndices[2]        = faceIndices[2];
        vertexIndices[3]        = faceIndices[3];
    }

    //--------------------------------------------------------------------------------------------------
    ///
    //--------------------------------------------------------------------------------------------------
    cvf::StructGridDefines::FaceType cvf::StructGridDefines::oppositeFace( StructGridDefines::FaceType face )
    {
        switch ( face )
        {
            case NEG_I:
                return POS_I;
            case POS_I:
                return NEG_I;
            case NEG_J:
                return POS_J;
            case POS_J:
                return NEG_J;
            case NEG_K:
                return POS_K;
            case POS_K:
                return NEG_K;
            default:
                CVF_ASSERT( false );
                return NO_FACE; // Return a default value to satisfy the compiler
        }
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
            case POS_I:
                ++( *ni );
                break;
            case NEG_I:
                *ni = ( i > 0 ) ? i - 1 : cvf::UNDEFINED_SIZE_T;
                break;
            case POS_J:
                ++( *nj );
                break;
            case NEG_J:
                *nj = ( j > 0 ) ? j - 1 : cvf::UNDEFINED_SIZE_T;
                break;
            case POS_K:
                ++( *nk );
                break;
            case NEG_K:
                *nk = ( k > 0 ) ? k - 1 : cvf::UNDEFINED_SIZE_T;
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
        GridAxisType axis = GridAxisType::NO_AXIS;

        if ( face == POS_I || face == NEG_I )
        {
            axis = GridAxisType::AXIS_I;
        }
        else if ( face == POS_J || face == NEG_J )
        {
            axis = GridAxisType::AXIS_J;
        }
        else if ( face == POS_K || face == NEG_K )
        {
            axis = GridAxisType::AXIS_K;
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

        // Define a map for the shared vertex indices between two faces
        static const std::map<std::pair<FaceType, FaceType>, std::pair<cvf::ubyte, cvf::ubyte>> edgeMap =
            { { { NEG_K, NEG_I }, { 0, 3 } },
              { { NEG_K, POS_I }, { 2, 1 } },
              { { NEG_K, NEG_J }, { 1, 0 } },
              { { NEG_K, POS_J }, { 3, 2 } },
              { { POS_K, NEG_I }, { 7, 4 } },
              { { POS_K, POS_I }, { 5, 6 } },
              { { POS_K, NEG_J }, { 4, 5 } },
              { { POS_K, POS_J }, { 6, 7 } },
              { { NEG_J, NEG_I }, { 4, 0 } },
              { { NEG_J, POS_I }, { 1, 5 } },
              { { POS_J, NEG_I }, { 3, 7 } },
              { { POS_J, POS_I }, { 6, 2 } } };

        auto it = edgeMap.find( { face1, face2 } );
        if ( it != edgeMap.end() )
        {
            return it->second;
        }
        return {};
    }

    //--------------------------------------------------------------------------------------------------
    ///
    //--------------------------------------------------------------------------------------------------
    std::vector<cvf::StructGridDefines::FaceType> cvf::StructGridDefines::validFaceTypes()
    {
        return {
            FaceType::NEG_I,
            FaceType::POS_I,
            FaceType::NEG_J,
            FaceType::POS_J,
            FaceType::NEG_K,
            FaceType::POS_K,
        };
    }

} //namespace StructGridDefines
} //namespace cvf
