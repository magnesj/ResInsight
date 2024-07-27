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

#pragma once

#include "cvfObject.h"
#include "cvfStructGridDefines.h"
#include "cvfVector3.h"

#include "cafAppEnum.h"

#include <cstddef>

namespace cvf
{
class CellFilterBase;

// Navneforslag
//    StructGridGeometryGeneratorInterface

// Main purpose of this class is to define the interface to be used by geometry generators
class StructGridInterface : public Object
{
public:
public:
    StructGridInterface();

    virtual size_t cellCountI() const = 0;
    virtual size_t cellCountJ() const = 0;
    virtual size_t cellCountK() const = 0;

    virtual bool isCellValid( size_t i, size_t j, size_t k ) const = 0;

    virtual cvf::Vec3d minCoordinate() const = 0;
    virtual cvf::Vec3d maxCoordinate() const = 0;
    void               characteristicCellSizes( double* iSize, double* jSize, double* kSize ) const;

    bool hasValidCharacteristicCellSizes() const;
    void computeCharacteristicCellSize( const std::vector<size_t>& globalCellIndices ) const;

    virtual cvf::Vec3d displayModelOffset() const;

    virtual bool
        cellIJKNeighbor( size_t i, size_t j, size_t k, StructGridDefines::FaceType face, size_t* neighborCellIndex ) const = 0;

    virtual size_t cellIndexFromIJK( size_t i, size_t j, size_t k ) const                      = 0;
    virtual bool   ijkFromCellIndex( size_t cellIndex, size_t* i, size_t* j, size_t* k ) const = 0;

    virtual bool cellIJKFromCoordinate( const cvf::Vec3d& coord, size_t* i, size_t* j, size_t* k ) const = 0;

    virtual void       cellCornerVertices( size_t cellIndex, cvf::Vec3d vertices[8] ) const = 0;
    virtual cvf::Vec3d cellCentroid( size_t cellIndex ) const                               = 0;
    virtual void cellMinMaxCordinates( size_t cellIndex, cvf::Vec3d* minCoordinate, cvf::Vec3d* maxCoordinate ) const = 0;

    virtual size_t     gridPointIndexFromIJK( size_t i, size_t j, size_t k ) const = 0;
    virtual cvf::Vec3d gridPointCoordinate( size_t i, size_t j, size_t k ) const   = 0;

public:
    static void cellFaceVertexIndices( StructGridDefines::FaceType face, cvf::ubyte vertexIndices[4] );
    static StructGridDefines::FaceType oppositeFace( StructGridDefines::FaceType face );
    static void                        neighborIJKAtCellFace( size_t                      i,
                                                              size_t                      j,
                                                              size_t                      k,
                                                              StructGridDefines::FaceType face,
                                                              size_t*                     ni,
                                                              size_t*                     nj,
                                                              size_t*                     nk );

    static StructGridDefines::GridAxisType gridAxisFromFace( StructGridDefines::FaceType face );

    static std::pair<ubyte, ubyte>                  edgeVertexIndices( cvf::StructGridDefines::FaceType face1,
                                                                       cvf::StructGridDefines::FaceType face2 );
    static std::vector<StructGridDefines::FaceType> validFaceTypes();

private:
    mutable double m_characteristicCellSizeI;
    mutable double m_characteristicCellSizeJ;
    mutable double m_characteristicCellSizeK;
};

} // namespace cvf
