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

#include "cvfStructGrid.h"
#include "cvfBase.h"
#include "cvfBoundingBox.h"

#include <algorithm>

namespace cvf
{
//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
StructGridInterface::StructGridInterface()
{
    m_characteristicCellSizeI = cvf::UNDEFINED_DOUBLE;
    m_characteristicCellSizeJ = cvf::UNDEFINED_DOUBLE;
    m_characteristicCellSizeK = cvf::UNDEFINED_DOUBLE;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void StructGridInterface::cellFaceVertexIndices( StructGridDefines::FaceType face, cvf::ubyte vertexIndices[4] )
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
std::pair<ubyte, ubyte> StructGridInterface::edgeVertexIndices( StructGridDefines::FaceType face1,
                                                                StructGridDefines::FaceType face2 )
{
    // Return the two shared vertex indices between two faces
    // The ordering is identical to the ordering in StructGridInterface::cellFaceVertexIndices

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
StructGridDefines::FaceType StructGridInterface::oppositeFace( StructGridDefines::FaceType face )
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
std::vector<StructGridDefines::FaceType> StructGridInterface::validFaceTypes()
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

//--------------------------------------------------------------------------------------------------
/// Return values are set to cvf::UNDEFINED_SIZE_T if the neighbor is in the negative area
//--------------------------------------------------------------------------------------------------
void StructGridInterface::neighborIJKAtCellFace( size_t                      i,
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
StructGridDefines::GridAxisType StructGridInterface::gridAxisFromFace( StructGridDefines::FaceType face )
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
/// Models with large absolute values for coordinate scalars will often end up with z-fighting due
/// to numerical limits in float used by OpenGL. displayModelOffset() is intended
//  to be subtracted from a domain model coordinate when building geometry
//
//  Used in StructGridGeometryGenerator::computeArrays()
//
//  Vec3d domainModelCoord = ...
//  Vec3d vizCoord = domainModelCoord - displayModelOffset();
//--------------------------------------------------------------------------------------------------
cvf::Vec3d StructGridInterface::displayModelOffset() const
{
    return cvf::Vec3d::ZERO;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void StructGridInterface::characteristicCellSizes( double* iSize, double* jSize, double* kSize ) const
{
    CVF_ASSERT( iSize && jSize && kSize );

    if ( !hasValidCharacteristicCellSizes() )
    {
        std::vector<size_t> reservoirCellIndices;
        reservoirCellIndices.resize( cellCountI() * cellCountJ() * cellCountK() );
        std::iota( reservoirCellIndices.begin(), reservoirCellIndices.end(), 0 );

        computeCharacteristicCellSize( reservoirCellIndices );
    }

    *iSize = m_characteristicCellSizeI;
    *jSize = m_characteristicCellSizeJ;
    *kSize = m_characteristicCellSizeK;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool StructGridInterface::hasValidCharacteristicCellSizes() const
{
    if ( m_characteristicCellSizeI == cvf::UNDEFINED_DOUBLE || m_characteristicCellSizeJ == cvf::UNDEFINED_DOUBLE ||
         m_characteristicCellSizeK == cvf::UNDEFINED_DOUBLE )
        return false;

    return true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void StructGridInterface::computeCharacteristicCellSize( const std::vector<size_t>& globalCellIndices ) const
{
    if ( globalCellIndices.empty() ) return;

    ubyte faceConnPosI[4];
    cellFaceVertexIndices( StructGridDefines::POS_I, faceConnPosI );

    ubyte faceConnNegI[4];
    cellFaceVertexIndices( StructGridDefines::NEG_I, faceConnNegI );

    ubyte faceConnPosJ[4];
    cellFaceVertexIndices( StructGridDefines::POS_J, faceConnPosJ );

    ubyte faceConnNegJ[4];
    cellFaceVertexIndices( StructGridDefines::NEG_J, faceConnNegJ );

    ubyte faceConnPosK[4];
    cellFaceVertexIndices( StructGridDefines::POS_K, faceConnPosK );

    ubyte faceConnNegK[4];
    cellFaceVertexIndices( StructGridDefines::NEG_K, faceConnNegK );

    double    tolerance         = 0.2;
    int       iterationIndex    = 0;
    const int iterationMaxCount = 3;
    while ( iterationIndex < iterationMaxCount )
    {
        if ( iterationIndex > 0 )
        {
            // Divide tolerance by a factor for each iteration
            tolerance = tolerance / 10.0;
        }

        double iLengthAccumulated = 0.0;
        double jLengthAccumulated = 0.0;
        double kLengthAccumulated = 0.0;

        cvf::Vec3d cornerVerts[8];
        size_t     evaluatedCellCount = 0;

        // Evaluate N-th cells, compute the stride between each index
        size_t stride = std::max( size_t( 1 ), globalCellIndices.size() / 100 );

        size_t i, j, k = 0;
        size_t index = 0;
        while ( index < globalCellIndices.size() )
        {
            size_t cellIndex = globalCellIndices[index];
            ijkFromCellIndex( cellIndex, &i, &j, &k );
            if ( isCellValid( i, j, k ) )
            {
                cellCornerVertices( cellIndex, cornerVerts );

                cvf::BoundingBox bb;
                for ( const auto& v : cornerVerts )
                {
                    bb.add( v );
                }

                // Exclude cells with very small volumes
                if ( bb.extent().z() > tolerance )
                {
                    iLengthAccumulated += ( cornerVerts[faceConnPosI[0]] - cornerVerts[faceConnNegI[0]] ).lengthSquared();
                    iLengthAccumulated += ( cornerVerts[faceConnPosI[1]] - cornerVerts[faceConnNegI[3]] ).lengthSquared();
                    iLengthAccumulated += ( cornerVerts[faceConnPosI[2]] - cornerVerts[faceConnNegI[2]] ).lengthSquared();
                    iLengthAccumulated += ( cornerVerts[faceConnPosI[3]] - cornerVerts[faceConnNegI[1]] ).lengthSquared();

                    jLengthAccumulated += ( cornerVerts[faceConnPosJ[0]] - cornerVerts[faceConnNegJ[0]] ).lengthSquared();
                    jLengthAccumulated += ( cornerVerts[faceConnPosJ[1]] - cornerVerts[faceConnNegJ[3]] ).lengthSquared();
                    jLengthAccumulated += ( cornerVerts[faceConnPosJ[2]] - cornerVerts[faceConnNegJ[2]] ).lengthSquared();
                    jLengthAccumulated += ( cornerVerts[faceConnPosJ[3]] - cornerVerts[faceConnNegJ[1]] ).lengthSquared();

                    kLengthAccumulated += ( cornerVerts[faceConnPosK[0]] - cornerVerts[faceConnNegK[0]] ).lengthSquared();
                    kLengthAccumulated += ( cornerVerts[faceConnPosK[1]] - cornerVerts[faceConnNegK[3]] ).lengthSquared();
                    kLengthAccumulated += ( cornerVerts[faceConnPosK[2]] - cornerVerts[faceConnNegK[2]] ).lengthSquared();
                    kLengthAccumulated += ( cornerVerts[faceConnPosK[3]] - cornerVerts[faceConnNegK[1]] ).lengthSquared();

                    evaluatedCellCount++;
                }
            }

            index += stride;
        }

        iterationIndex++;

        if ( evaluatedCellCount > 10 || iterationIndex == iterationMaxCount )
        {
            double divisor = evaluatedCellCount * 4.0;

            if ( divisor > 0.0 )
            {
                m_characteristicCellSizeI = cvf::Math::sqrt( iLengthAccumulated / divisor );
                m_characteristicCellSizeJ = cvf::Math::sqrt( jLengthAccumulated / divisor );
                m_characteristicCellSizeK = cvf::Math::sqrt( kLengthAccumulated / divisor );

                return;
            }
        }
    }
}

} // namespace cvf
