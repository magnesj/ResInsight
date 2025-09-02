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

#include "cvfCylindricalGeometryGenerator.h"
#include "cvfStructGridGeometryGenerator.h"
#include "cvfStructGridScalarDataAccess.h"
#include "cvfScalarMapper.h"
#include "cvfGeometryBuilderDrawableGeo.h"
#include "cvfPrimitiveSetIndexedUInt.h"
#include "cvfOutlineEdgeExtractor.h"
#include "cvfArray.h"
#include "cvfBase.h"
#include "cvfDrawableGeo.h"
#include <cmath>

namespace cvf
{

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
CylindricalGeometryGenerator::CylindricalGeometryGenerator( const StructGridInterface* grid, bool useOpenMP )
    : GeometryGeneratorInterface( grid, useOpenMP )
{
    m_quadMapper = new StructGridQuadToCellFaceMapper;
    m_triangleMapper = new StuctGridTriangleToCellFaceMapper( m_quadMapper.p() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
CylindricalGeometryGenerator::~CylindricalGeometryGenerator()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void CylindricalGeometryGenerator::setCellVisibility( const UByteArray* cellVisibility )
{
    m_cellVisibility = cellVisibility;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void CylindricalGeometryGenerator::addFaceVisibilityFilter( const CellFaceVisibilityFilter* cellVisibilityFilter )
{
    m_cellVisibilityFilters.push_back( cellVisibilityFilter );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
ref<DrawableGeo> CylindricalGeometryGenerator::generateSurface()
{
    computeArrays();

    CVF_ASSERT( m_vertices.notNull() );

    if ( m_vertices->size() == 0 ) return nullptr;

    ref<DrawableGeo> geo = new DrawableGeo;
    geo->setFromQuadVertexArray( m_vertices.p() );

    return geo;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
ref<DrawableGeo> CylindricalGeometryGenerator::createMeshDrawable()
{
    if ( !( m_vertices.notNull() && m_vertices->size() != 0 ) ) return nullptr;

    ref<DrawableGeo> geo = new DrawableGeo;
    geo->setVertexArray( m_vertices.p() );

    ref<UIntArray> indices = StructGridGeometryGenerator::lineIndicesFromQuadVertexArray( m_vertices.p() );
    ref<PrimitiveSetIndexedUInt> prim = new PrimitiveSetIndexedUInt( PT_LINES );
    prim->setIndices( indices.p() );

    geo->addPrimitiveSet( prim.p() );
    return geo;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
ref<DrawableGeo> CylindricalGeometryGenerator::createOutlineMeshDrawable( double creaseAngle )
{
    if ( !( m_vertices.notNull() && m_vertices->size() != 0 ) ) return nullptr;

    cvf::OutlineEdgeExtractor ee( creaseAngle, *m_vertices );

    ref<UIntArray> indices = StructGridGeometryGenerator::lineIndicesFromQuadVertexArray( m_vertices.p() );
    ee.addPrimitives( 4, *indices );

    ref<cvf::UIntArray> lineIndices = ee.lineIndices();
    if ( lineIndices->size() == 0 )
    {
        return nullptr;
    }

    ref<PrimitiveSetIndexedUInt> prim = new PrimitiveSetIndexedUInt( PT_LINES );
    prim->setIndices( lineIndices.p() );

    ref<DrawableGeo> geo = new DrawableGeo;
    geo->setVertexArray( m_vertices.p() );
    geo->addPrimitiveSet( prim.p() );

    return geo;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void CylindricalGeometryGenerator::textureCoordinates( Vec2fArray*                       textureCoords,
                                                      const StructGridScalarDataAccess* resultAccessor,
                                                      const ScalarMapper*               mapper ) const
{
    if ( !resultAccessor ) return;

    size_t numVertices = m_quadMapper->quadCount() * 4;

    textureCoords->resize( numVertices );
    cvf::Vec2f* rawPtr = textureCoords->ptr();

    double     cellScalarValue;
    cvf::Vec2f texCoord;

#pragma omp parallel for private( texCoord, cellScalarValue ) if ( m_useOpenMP )
    for ( int i = 0; i < static_cast<int>( m_quadMapper->quadCount() ); i++ )
    {
        cellScalarValue = resultAccessor->cellScalar( m_quadMapper->cellIndex( i ) );
        texCoord        = mapper->mapToTextureCoord( cellScalarValue );
        if ( cellScalarValue == HUGE_VAL || cellScalarValue != cellScalarValue )
        {
            texCoord[1] = 1.0f;
        }

        size_t j;
        for ( j = 0; j < 4; j++ )
        {
            rawPtr[i * 4 + j] = texCoord;
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const StructGridQuadToCellFaceMapper* CylindricalGeometryGenerator::quadToCellFaceMapper() const
{
    return m_quadMapper.p();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const StuctGridTriangleToCellFaceMapper* CylindricalGeometryGenerator::triangleToCellFaceMapper() const
{
    return m_triangleMapper.p();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
ref<DrawableGeo> CylindricalGeometryGenerator::createMeshDrawableFromSingleCell( const StructGridInterface* grid, size_t cellIndex )
{
    return createMeshDrawableFromSingleCell( grid, cellIndex, grid->displayModelOffset() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
ref<DrawableGeo> CylindricalGeometryGenerator::createMeshDrawableFromSingleCell( const StructGridInterface* grid,
                                                                                 size_t                     cellIndex,
                                                                                 const cvf::Vec3d&          displayModelOffset )
{
    // For now, fall back to standard hexahedral representation for single cells
    return StructGridGeometryGenerator::createMeshDrawableFromSingleCell( grid, cellIndex, displayModelOffset );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void CylindricalGeometryGenerator::computeArrays()
{
    std::vector<Vec3f> vertices;
    m_quadMapper->quadToCellIndexMap().clear();
    m_quadMapper->quadToCellFaceMap().clear();

    cvf::Vec3d offset = m_grid->displayModelOffset();

#pragma omp parallel for schedule( dynamic ) if ( m_useOpenMP )
    for ( int k = 0; k < static_cast<int>( m_grid->cellCountK() ); k++ )
    {
        size_t j;
        for ( j = 0; j < m_grid->cellCountJ(); j++ )
        {
            size_t i;
            for ( i = 0; i < m_grid->cellCountI(); i++ )
            {
                size_t cellIndex = m_grid->cellIndexFromIJK( i, j, k );
                if ( m_cellVisibility.notNull() && !( *m_cellVisibility )[cellIndex] )
                {
                    continue;
                }

                CylindricalCell cylCell;
                if ( !extractCylindricalCellData( cellIndex, cylCell ) )
                {
                    continue;
                }

                std::vector<StructGridInterface::FaceType> visibleFaces;
                visibleFaces.reserve( 6 );

                if ( isCellFaceVisible( i, j, k, StructGridInterface::NEG_I ) )
                    visibleFaces.push_back( cvf::StructGridInterface::NEG_I );
                if ( isCellFaceVisible( i, j, k, StructGridInterface::POS_I ) )
                    visibleFaces.push_back( cvf::StructGridInterface::POS_I );
                if ( isCellFaceVisible( i, j, k, StructGridInterface::NEG_J ) )
                    visibleFaces.push_back( cvf::StructGridInterface::NEG_J );
                if ( isCellFaceVisible( i, j, k, StructGridInterface::POS_J ) )
                    visibleFaces.push_back( cvf::StructGridInterface::POS_J );
                if ( isCellFaceVisible( i, j, k, StructGridInterface::NEG_K ) )
                    visibleFaces.push_back( cvf::StructGridInterface::NEG_K );
                if ( isCellFaceVisible( i, j, k, StructGridInterface::POS_K ) )
                    visibleFaces.push_back( cvf::StructGridInterface::POS_K );

                if ( !visibleFaces.empty() )
                {
#pragma omp critical( critical_section_CylindricalGeometryGenerator_computeArrays )
                    {
                        generateCylindricalQuads( cylCell, cellIndex, vertices );
                    }
                }
            }
        }
    }

    m_vertices = new cvf::Vec3fArray;
    m_vertices->assign( vertices );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void CylindricalGeometryGenerator::generateCylindricalQuads( const CylindricalCell& cell, size_t cellIndex, std::vector<Vec3f>& vertices )
{
    addRadialFaces( cell, cellIndex, vertices );
    addCircumferentialFaces( cell, cellIndex, vertices );
    addTopBottomFaces( cell, cellIndex, vertices );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void CylindricalGeometryGenerator::addRadialFaces( const CylindricalCell& cell, size_t cellIndex, std::vector<Vec3f>& vertices )
{
    cvf::Vec3d offset = m_grid->displayModelOffset();

    // Inner radial face (NEG_J in radial grid terms)
    cvf::Vec3d innerStart = cylindricalToCartesian( cell.innerRadius, cell.startAngle, cell.bottomZ, cell.centerPoint );
    cvf::Vec3d innerEnd   = cylindricalToCartesian( cell.innerRadius, cell.endAngle, cell.bottomZ, cell.centerPoint );
    cvf::Vec3d innerStartTop = cylindricalToCartesian( cell.innerRadius, cell.startAngle, cell.topZ, cell.centerPoint );
    cvf::Vec3d innerEndTop   = cylindricalToCartesian( cell.innerRadius, cell.endAngle, cell.topZ, cell.centerPoint );

    vertices.push_back( cvf::Vec3f( innerStart - offset ) );
    vertices.push_back( cvf::Vec3f( innerEnd - offset ) );
    vertices.push_back( cvf::Vec3f( innerEndTop - offset ) );
    vertices.push_back( cvf::Vec3f( innerStartTop - offset ) );

    m_quadMapper->quadToCellIndexMap().push_back( cellIndex );
    m_quadMapper->quadToCellFaceMap().push_back( StructGridInterface::NEG_J );

    // Outer radial face (POS_J in radial grid terms)
    cvf::Vec3d outerStart = cylindricalToCartesian( cell.outerRadius, cell.startAngle, cell.bottomZ, cell.centerPoint );
    cvf::Vec3d outerEnd   = cylindricalToCartesian( cell.outerRadius, cell.endAngle, cell.bottomZ, cell.centerPoint );
    cvf::Vec3d outerStartTop = cylindricalToCartesian( cell.outerRadius, cell.startAngle, cell.topZ, cell.centerPoint );
    cvf::Vec3d outerEndTop   = cylindricalToCartesian( cell.outerRadius, cell.endAngle, cell.topZ, cell.centerPoint );

    vertices.push_back( cvf::Vec3f( outerEnd - offset ) );
    vertices.push_back( cvf::Vec3f( outerStart - offset ) );
    vertices.push_back( cvf::Vec3f( outerStartTop - offset ) );
    vertices.push_back( cvf::Vec3f( outerEndTop - offset ) );

    m_quadMapper->quadToCellIndexMap().push_back( cellIndex );
    m_quadMapper->quadToCellFaceMap().push_back( StructGridInterface::POS_J );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void CylindricalGeometryGenerator::addCircumferentialFaces( const CylindricalCell& cell, size_t cellIndex, std::vector<Vec3f>& vertices )
{
    cvf::Vec3d offset = m_grid->displayModelOffset();

    // Start angle face (NEG_I in radial grid terms)
    cvf::Vec3d startInner = cylindricalToCartesian( cell.innerRadius, cell.startAngle, cell.bottomZ, cell.centerPoint );
    cvf::Vec3d startOuter = cylindricalToCartesian( cell.outerRadius, cell.startAngle, cell.bottomZ, cell.centerPoint );
    cvf::Vec3d startInnerTop = cylindricalToCartesian( cell.innerRadius, cell.startAngle, cell.topZ, cell.centerPoint );
    cvf::Vec3d startOuterTop = cylindricalToCartesian( cell.outerRadius, cell.startAngle, cell.topZ, cell.centerPoint );

    vertices.push_back( cvf::Vec3f( startInner - offset ) );
    vertices.push_back( cvf::Vec3f( startOuter - offset ) );
    vertices.push_back( cvf::Vec3f( startOuterTop - offset ) );
    vertices.push_back( cvf::Vec3f( startInnerTop - offset ) );

    m_quadMapper->quadToCellIndexMap().push_back( cellIndex );
    m_quadMapper->quadToCellFaceMap().push_back( StructGridInterface::NEG_I );

    // End angle face (POS_I in radial grid terms)
    cvf::Vec3d endInner = cylindricalToCartesian( cell.innerRadius, cell.endAngle, cell.bottomZ, cell.centerPoint );
    cvf::Vec3d endOuter = cylindricalToCartesian( cell.outerRadius, cell.endAngle, cell.bottomZ, cell.centerPoint );
    cvf::Vec3d endInnerTop = cylindricalToCartesian( cell.innerRadius, cell.endAngle, cell.topZ, cell.centerPoint );
    cvf::Vec3d endOuterTop = cylindricalToCartesian( cell.outerRadius, cell.endAngle, cell.topZ, cell.centerPoint );

    vertices.push_back( cvf::Vec3f( endOuter - offset ) );
    vertices.push_back( cvf::Vec3f( endInner - offset ) );
    vertices.push_back( cvf::Vec3f( endInnerTop - offset ) );
    vertices.push_back( cvf::Vec3f( endOuterTop - offset ) );

    m_quadMapper->quadToCellIndexMap().push_back( cellIndex );
    m_quadMapper->quadToCellFaceMap().push_back( StructGridInterface::POS_I );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void CylindricalGeometryGenerator::addTopBottomFaces( const CylindricalCell& cell, size_t cellIndex, std::vector<Vec3f>& vertices )
{
    cvf::Vec3d offset = m_grid->displayModelOffset();

    // Bottom face (NEG_K)
    cvf::Vec3d bottomInnerStart = cylindricalToCartesian( cell.innerRadius, cell.startAngle, cell.bottomZ, cell.centerPoint );
    cvf::Vec3d bottomInnerEnd   = cylindricalToCartesian( cell.innerRadius, cell.endAngle, cell.bottomZ, cell.centerPoint );
    cvf::Vec3d bottomOuterStart = cylindricalToCartesian( cell.outerRadius, cell.startAngle, cell.bottomZ, cell.centerPoint );
    cvf::Vec3d bottomOuterEnd   = cylindricalToCartesian( cell.outerRadius, cell.endAngle, cell.bottomZ, cell.centerPoint );

    vertices.push_back( cvf::Vec3f( bottomInnerStart - offset ) );
    vertices.push_back( cvf::Vec3f( bottomOuterStart - offset ) );
    vertices.push_back( cvf::Vec3f( bottomOuterEnd - offset ) );
    vertices.push_back( cvf::Vec3f( bottomInnerEnd - offset ) );

    m_quadMapper->quadToCellIndexMap().push_back( cellIndex );
    m_quadMapper->quadToCellFaceMap().push_back( StructGridInterface::NEG_K );

    // Top face (POS_K)
    cvf::Vec3d topInnerStart = cylindricalToCartesian( cell.innerRadius, cell.startAngle, cell.topZ, cell.centerPoint );
    cvf::Vec3d topInnerEnd   = cylindricalToCartesian( cell.innerRadius, cell.endAngle, cell.topZ, cell.centerPoint );
    cvf::Vec3d topOuterStart = cylindricalToCartesian( cell.outerRadius, cell.startAngle, cell.topZ, cell.centerPoint );
    cvf::Vec3d topOuterEnd   = cylindricalToCartesian( cell.outerRadius, cell.endAngle, cell.topZ, cell.centerPoint );

    vertices.push_back( cvf::Vec3f( topInnerEnd - offset ) );
    vertices.push_back( cvf::Vec3f( topOuterEnd - offset ) );
    vertices.push_back( cvf::Vec3f( topOuterStart - offset ) );
    vertices.push_back( cvf::Vec3f( topInnerStart - offset ) );

    m_quadMapper->quadToCellIndexMap().push_back( cellIndex );
    m_quadMapper->quadToCellFaceMap().push_back( StructGridInterface::POS_K );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool CylindricalGeometryGenerator::isCellFaceVisible( size_t i, size_t j, size_t k, StructGridInterface::FaceType face ) const
{
    size_t idx;
    for ( idx = 0; idx < m_cellVisibilityFilters.size(); idx++ )
    {
        const cvf::CellFaceVisibilityFilter* cellFilter = m_cellVisibilityFilters[idx];
        if ( cellFilter->isFaceVisible( i, j, k, face, m_cellVisibility.p() ) )
        {
            return true;
        }
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool CylindricalGeometryGenerator::extractCylindricalCellData( size_t cellIndex, CylindricalCell& cell ) const
{
    // Try to get cylindrical coordinates from the grid interface
    if ( m_grid->getCylindricalCoords( cellIndex, cell.innerRadius, cell.outerRadius, 
                                     cell.startAngle, cell.endAngle, cell.topZ, cell.bottomZ ) )
    {
        // Get center point from grid
        cell.centerPoint = m_grid->cellCentroid( cellIndex );
        cell.centerPoint.z() = ( cell.topZ + cell.bottomZ ) * 0.5;
        return true;
    }

    // Fallback: extract from corner vertices and approximate cylindrical parameters
    cvf::Vec3d cornerVerts[8];
    m_grid->cellCornerVertices( cellIndex, cornerVerts );

    // For radial grids, approximate center from bottom face center
    cvf::Vec3d bottomCenter = ( cornerVerts[0] + cornerVerts[1] + cornerVerts[2] + cornerVerts[3] ) * 0.25;
    cvf::Vec3d topCenter    = ( cornerVerts[4] + cornerVerts[5] + cornerVerts[6] + cornerVerts[7] ) * 0.25;
    
    cell.centerPoint = ( bottomCenter + topCenter ) * 0.5;
    cell.bottomZ = bottomCenter.z();
    cell.topZ = topCenter.z();

    // Approximate radial extents
    double minRadius = HUGE_VAL;
    double maxRadius = 0.0;
    double minAngle = HUGE_VAL;
    double maxAngle = -HUGE_VAL;

    for ( int idx = 0; idx < 8; idx++ )
    {
        cvf::Vec3d relative = cornerVerts[idx] - cell.centerPoint;
        double radius = std::sqrt( relative.x() * relative.x() + relative.y() * relative.y() );
        double angle = std::atan2( relative.y(), relative.x() );

        minRadius = std::min( minRadius, radius );
        maxRadius = std::max( maxRadius, radius );
        minAngle = std::min( minAngle, angle );
        maxAngle = std::max( maxAngle, angle );
    }

    cell.innerRadius = minRadius;
    cell.outerRadius = maxRadius;
    cell.startAngle = minAngle;
    cell.endAngle = maxAngle;

    return true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
cvf::Vec3d CylindricalGeometryGenerator::cylindricalToCartesian( double radius, double angle, double z, const cvf::Vec3d& center ) const
{
    double x = center.x() + radius * std::cos( angle );
    double y = center.y() + radius * std::sin( angle );
    return cvf::Vec3d( x, y, z );
}

} // namespace cvf