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
    : m_characteristicCellSizeI( cvf::UNDEFINED_DOUBLE )
    , m_characteristicCellSizeJ( cvf::UNDEFINED_DOUBLE )
    , m_characteristicCellSizeK( cvf::UNDEFINED_DOUBLE )
{
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
