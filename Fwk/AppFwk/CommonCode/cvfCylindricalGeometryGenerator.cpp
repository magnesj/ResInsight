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
#include "cvfArray.h"
#include "cvfBase.h"
#include "cvfDrawableGeo.h"
#include "cvfGeometryBuilderDrawableGeo.h"
#include "cvfOutlineEdgeExtractor.h"
#include "cvfPrimitiveSetIndexedUInt.h"
#include "cvfScalarMapper.h"
#include "cvfStructGridGeometryGenerator.h"
#include "cvfStructGridScalarDataAccess.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace cvf
{

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
CylindricalGeometryGenerator::CylindricalGeometryGenerator( const StructGridInterface* grid, bool useOpenMP )
    : GeometryGeneratorInterface( grid, useOpenMP )
    , m_curveSubdivisions( 10 )
{
    m_quadMapper     = new StructGridQuadToCellFaceMapper;
    m_triangleMapper = new StuctGridTriangleToCellFaceMapper( m_quadMapper.p() );
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

    ref<UIntArray> indices            = StructGridGeometryGenerator::lineIndicesFromQuadVertexArray( m_vertices.p() );
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
ref<DrawableGeo> CylindricalGeometryGenerator::createMeshDrawableFromSingleCell( const StructGridInterface* grid,
                                                                                 size_t                     cellIndex )
{
    return createMeshDrawableFromSingleCell( grid, cellIndex, grid->displayModelOffset() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
ref<DrawableGeo> CylindricalGeometryGenerator::createMeshDrawableFromSingleCell( const StructGridInterface* grid,
                                                                                 size_t                     cellIndex,
                                                                                 const cvf::Vec3d& displayModelOffset )
{
    if ( grid->gridGeometryType() != cvf::GridGeometryType::CYLINDRICAL )
    {
        return StructGridGeometryGenerator::createMeshDrawableFromSingleCell( grid, cellIndex, displayModelOffset );
    }

    CylindricalCell cylCell;
    bool            success = grid->getCylindricalCoords( cellIndex,
                                               cylCell.innerRadius,
                                               cylCell.outerRadius,
                                               cylCell.startAngle,
                                               cylCell.endAngle,
                                               cylCell.topZ,
                                               cylCell.bottomZ );
    if ( !success )
    {
        return StructGridGeometryGenerator::createMeshDrawableFromSingleCell( grid, cellIndex, displayModelOffset );
    }

    std::vector<Vec3f> vertices;

    cvf::Vec3d offset            = displayModelOffset;
    int        curveSubdivisions = 10;

    // Create angular subdivisions for smooth curved surfaces
    double angleRange = cylCell.endAngle - cylCell.startAngle;
    double angleStep  = angleRange / curveSubdivisions;

    // Inner radial face - curved surface with angular subdivisions
    for ( int i = 0; i < curveSubdivisions; ++i )
    {
        double angle1 = cylCell.startAngle + i * angleStep;
        double angle2 = cylCell.startAngle + ( i + 1 ) * angleStep;

        cvf::Vec3d innerBot1 = cylindricalToCartesianStatic( cylCell.innerRadius, angle1, cylCell.bottomZ );
        cvf::Vec3d innerBot2 = cylindricalToCartesianStatic( cylCell.innerRadius, angle2, cylCell.bottomZ );
        cvf::Vec3d innerTop1 = cylindricalToCartesianStatic( cylCell.innerRadius, angle1, cylCell.topZ );
        cvf::Vec3d innerTop2 = cylindricalToCartesianStatic( cylCell.innerRadius, angle2, cylCell.topZ );

        vertices.push_back( cvf::Vec3f( innerBot1 - offset ) );
        vertices.push_back( cvf::Vec3f( innerBot2 - offset ) );
        vertices.push_back( cvf::Vec3f( innerTop2 - offset ) );
        vertices.push_back( cvf::Vec3f( innerTop1 - offset ) );
    }

    // Outer radial face - curved surface with angular subdivisions
    for ( int i = 0; i < curveSubdivisions; ++i )
    {
        double angle1 = cylCell.startAngle + i * angleStep;
        double angle2 = cylCell.startAngle + ( i + 1 ) * angleStep;

        cvf::Vec3d outerBot1 = cylindricalToCartesianStatic( cylCell.outerRadius, angle1, cylCell.bottomZ );
        cvf::Vec3d outerBot2 = cylindricalToCartesianStatic( cylCell.outerRadius, angle2, cylCell.bottomZ );
        cvf::Vec3d outerTop1 = cylindricalToCartesianStatic( cylCell.outerRadius, angle1, cylCell.topZ );
        cvf::Vec3d outerTop2 = cylindricalToCartesianStatic( cylCell.outerRadius, angle2, cylCell.topZ );

        vertices.push_back( cvf::Vec3f( outerBot2 - offset ) );
        vertices.push_back( cvf::Vec3f( outerBot1 - offset ) );
        vertices.push_back( cvf::Vec3f( outerTop1 - offset ) );
        vertices.push_back( cvf::Vec3f( outerTop2 - offset ) );
    }

    // Start angle face - straight radial face (no subdivision needed)
    cvf::Vec3d startInner    = cylindricalToCartesianStatic( cylCell.innerRadius, cylCell.startAngle, cylCell.bottomZ );
    cvf::Vec3d startOuter    = cylindricalToCartesianStatic( cylCell.outerRadius, cylCell.startAngle, cylCell.bottomZ );
    cvf::Vec3d startInnerTop = cylindricalToCartesianStatic( cylCell.innerRadius, cylCell.startAngle, cylCell.topZ );
    cvf::Vec3d startOuterTop = cylindricalToCartesianStatic( cylCell.outerRadius, cylCell.startAngle, cylCell.topZ );

    vertices.push_back( cvf::Vec3f( startInner - offset ) );
    vertices.push_back( cvf::Vec3f( startOuter - offset ) );
    vertices.push_back( cvf::Vec3f( startOuterTop - offset ) );
    vertices.push_back( cvf::Vec3f( startInnerTop - offset ) );

    // End angle face - straight radial face (no subdivision needed)
    cvf::Vec3d endInner    = cylindricalToCartesianStatic( cylCell.innerRadius, cylCell.endAngle, cylCell.bottomZ );
    cvf::Vec3d endOuter    = cylindricalToCartesianStatic( cylCell.outerRadius, cylCell.endAngle, cylCell.bottomZ );
    cvf::Vec3d endInnerTop = cylindricalToCartesianStatic( cylCell.innerRadius, cylCell.endAngle, cylCell.topZ );
    cvf::Vec3d endOuterTop = cylindricalToCartesianStatic( cylCell.outerRadius, cylCell.endAngle, cylCell.topZ );

    vertices.push_back( cvf::Vec3f( endOuter - offset ) );
    vertices.push_back( cvf::Vec3f( endInner - offset ) );
    vertices.push_back( cvf::Vec3f( endInnerTop - offset ) );
    vertices.push_back( cvf::Vec3f( endOuterTop - offset ) );

    // Bottom face - curved surface with angular subdivisions
    for ( int i = 0; i < curveSubdivisions; ++i )
    {
        double angle1 = cylCell.startAngle + i * angleStep;
        double angle2 = cylCell.startAngle + ( i + 1 ) * angleStep;

        cvf::Vec3d bottomInner1 = cylindricalToCartesianStatic( cylCell.innerRadius, angle1, cylCell.bottomZ );
        cvf::Vec3d bottomOuter1 = cylindricalToCartesianStatic( cylCell.outerRadius, angle1, cylCell.bottomZ );
        cvf::Vec3d bottomInner2 = cylindricalToCartesianStatic( cylCell.innerRadius, angle2, cylCell.bottomZ );
        cvf::Vec3d bottomOuter2 = cylindricalToCartesianStatic( cylCell.outerRadius, angle2, cylCell.bottomZ );

        vertices.push_back( cvf::Vec3f( bottomInner1 - offset ) );
        vertices.push_back( cvf::Vec3f( bottomOuter1 - offset ) );
        vertices.push_back( cvf::Vec3f( bottomOuter2 - offset ) );
        vertices.push_back( cvf::Vec3f( bottomInner2 - offset ) );
    }

    // Top face - curved surface with angular subdivisions
    for ( int i = 0; i < curveSubdivisions; ++i )
    {
        double angle1 = cylCell.startAngle + i * angleStep;
        double angle2 = cylCell.startAngle + ( i + 1 ) * angleStep;

        cvf::Vec3d topInner1 = cylindricalToCartesianStatic( cylCell.innerRadius, angle1, cylCell.topZ );
        cvf::Vec3d topOuter1 = cylindricalToCartesianStatic( cylCell.outerRadius, angle1, cylCell.topZ );
        cvf::Vec3d topInner2 = cylindricalToCartesianStatic( cylCell.innerRadius, angle2, cylCell.topZ );
        cvf::Vec3d topOuter2 = cylindricalToCartesianStatic( cylCell.outerRadius, angle2, cylCell.topZ );

        vertices.push_back( cvf::Vec3f( topInner2 - offset ) );
        vertices.push_back( cvf::Vec3f( topOuter2 - offset ) );
        vertices.push_back( cvf::Vec3f( topOuter1 - offset ) );
        vertices.push_back( cvf::Vec3f( topInner1 - offset ) );
    }

    if ( vertices.empty() )
    {
        return nullptr;
    }

    cvf::ref<cvf::Vec3fArray> cvfVertices = new cvf::Vec3fArray;
    cvfVertices->assign( vertices );

    ref<DrawableGeo> geo = new DrawableGeo;
    geo->setVertexArray( cvfVertices.p() );

    ref<UIntArray> indices            = StructGridGeometryGenerator::lineIndicesFromQuadVertexArray( cvfVertices.p() );
    ref<PrimitiveSetIndexedUInt> prim = new PrimitiveSetIndexedUInt( PT_LINES );
    prim->setIndices( indices.p() );

    geo->addPrimitiveSet( prim.p() );
    return geo;
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
void CylindricalGeometryGenerator::generateCylindricalQuads( const CylindricalCell& cell,
                                                             size_t                 cellIndex,
                                                             std::vector<Vec3f>&    vertices )
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

    // Create N subdivisions along the angular range for smooth curved surfaces
    double angleRange = cell.endAngle - cell.startAngle;
    double angleStep  = angleRange / m_curveSubdivisions;

    // Inner radial face (NEG_I in radial grid: smaller radius) - curved surface
    for ( int i = 0; i < m_curveSubdivisions; ++i )
    {
        double angle1 = cell.startAngle + i * angleStep;
        double angle2 = cell.startAngle + ( i + 1 ) * angleStep;

        // Create quad for this subdivision
        cvf::Vec3d innerBot1 = cylindricalToCartesianStatic( cell.innerRadius, angle1, cell.bottomZ );
        cvf::Vec3d innerBot2 = cylindricalToCartesianStatic( cell.innerRadius, angle2, cell.bottomZ );
        cvf::Vec3d innerTop1 = cylindricalToCartesianStatic( cell.innerRadius, angle1, cell.topZ );
        cvf::Vec3d innerTop2 = cylindricalToCartesianStatic( cell.innerRadius, angle2, cell.topZ );

        vertices.push_back( cvf::Vec3f( innerBot1 - offset ) );
        vertices.push_back( cvf::Vec3f( innerBot2 - offset ) );
        vertices.push_back( cvf::Vec3f( innerTop2 - offset ) );
        vertices.push_back( cvf::Vec3f( innerTop1 - offset ) );

        m_quadMapper->quadToCellIndexMap().push_back( cellIndex );
        m_quadMapper->quadToCellFaceMap().push_back( StructGridInterface::NEG_I );
    }

    // Outer radial face (POS_I in radial grid: larger radius) - curved surface
    for ( int i = 0; i < m_curveSubdivisions; ++i )
    {
        double angle1 = cell.startAngle + i * angleStep;
        double angle2 = cell.startAngle + ( i + 1 ) * angleStep;

        // Create quad for this subdivision (reverse winding for outward face)
        cvf::Vec3d outerBot1 = cylindricalToCartesianStatic( cell.outerRadius, angle1, cell.bottomZ );
        cvf::Vec3d outerBot2 = cylindricalToCartesianStatic( cell.outerRadius, angle2, cell.bottomZ );
        cvf::Vec3d outerTop1 = cylindricalToCartesianStatic( cell.outerRadius, angle1, cell.topZ );
        cvf::Vec3d outerTop2 = cylindricalToCartesianStatic( cell.outerRadius, angle2, cell.topZ );

        vertices.push_back( cvf::Vec3f( outerBot2 - offset ) );
        vertices.push_back( cvf::Vec3f( outerBot1 - offset ) );
        vertices.push_back( cvf::Vec3f( outerTop1 - offset ) );
        vertices.push_back( cvf::Vec3f( outerTop2 - offset ) );

        m_quadMapper->quadToCellIndexMap().push_back( cellIndex );
        m_quadMapper->quadToCellFaceMap().push_back( StructGridInterface::POS_I );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void CylindricalGeometryGenerator::addCircumferentialFaces( const CylindricalCell& cell,
                                                            size_t                 cellIndex,
                                                            std::vector<Vec3f>&    vertices )
{
    cvf::Vec3d offset = m_grid->displayModelOffset();

    // Create N subdivisions along the radial range for smooth curved surfaces
    double radiusRange = cell.outerRadius - cell.innerRadius;
    double radiusStep  = radiusRange / m_curveSubdivisions;

    // Start angle face (NEG_J in radial grid: smaller theta) - curved surface
    for ( int i = 0; i < m_curveSubdivisions; ++i )
    {
        double radius1 = cell.innerRadius + i * radiusStep;
        double radius2 = cell.innerRadius + ( i + 1 ) * radiusStep;

        // Create quad for this subdivision
        cvf::Vec3d startInner    = cylindricalToCartesianStatic( radius1, cell.startAngle, cell.bottomZ );
        cvf::Vec3d startOuter    = cylindricalToCartesianStatic( radius2, cell.startAngle, cell.bottomZ );
        cvf::Vec3d startInnerTop = cylindricalToCartesianStatic( radius1, cell.startAngle, cell.topZ );
        cvf::Vec3d startOuterTop = cylindricalToCartesianStatic( radius2, cell.startAngle, cell.topZ );

        vertices.push_back( cvf::Vec3f( startInner - offset ) );
        vertices.push_back( cvf::Vec3f( startOuter - offset ) );
        vertices.push_back( cvf::Vec3f( startOuterTop - offset ) );
        vertices.push_back( cvf::Vec3f( startInnerTop - offset ) );

        m_quadMapper->quadToCellIndexMap().push_back( cellIndex );
        m_quadMapper->quadToCellFaceMap().push_back( StructGridInterface::NEG_J );
    }

    // End angle face (POS_J in radial grid: larger theta) - curved surface
    for ( int i = 0; i < m_curveSubdivisions; ++i )
    {
        double radius1 = cell.innerRadius + i * radiusStep;
        double radius2 = cell.innerRadius + ( i + 1 ) * radiusStep;

        // Create quad for this subdivision (reverse winding for outward face)
        cvf::Vec3d endInner    = cylindricalToCartesianStatic( radius1, cell.endAngle, cell.bottomZ );
        cvf::Vec3d endOuter    = cylindricalToCartesianStatic( radius2, cell.endAngle, cell.bottomZ );
        cvf::Vec3d endInnerTop = cylindricalToCartesianStatic( radius1, cell.endAngle, cell.topZ );
        cvf::Vec3d endOuterTop = cylindricalToCartesianStatic( radius2, cell.endAngle, cell.topZ );

        vertices.push_back( cvf::Vec3f( endOuter - offset ) );
        vertices.push_back( cvf::Vec3f( endInner - offset ) );
        vertices.push_back( cvf::Vec3f( endInnerTop - offset ) );
        vertices.push_back( cvf::Vec3f( endOuterTop - offset ) );

        m_quadMapper->quadToCellIndexMap().push_back( cellIndex );
        m_quadMapper->quadToCellFaceMap().push_back( StructGridInterface::POS_J );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void CylindricalGeometryGenerator::addTopBottomFaces( const CylindricalCell& cell,
                                                      size_t                 cellIndex,
                                                      std::vector<Vec3f>&    vertices )
{
    cvf::Vec3d offset = m_grid->displayModelOffset();

    // Create N x N subdivisions for smooth curved surfaces on top/bottom faces
    double radiusRange = cell.outerRadius - cell.innerRadius;
    double angleRange  = cell.endAngle - cell.startAngle;
    double radiusStep  = radiusRange / m_curveSubdivisions;
    double angleStep   = angleRange / m_curveSubdivisions;

    // Bottom face (NEG_K) - subdivided curved surface
    for ( int j = 0; j < m_curveSubdivisions; ++j )
    {
        for ( int i = 0; i < m_curveSubdivisions; ++i )
        {
            double radius1 = cell.innerRadius + i * radiusStep;
            double radius2 = cell.innerRadius + ( i + 1 ) * radiusStep;
            double angle1  = cell.startAngle + j * angleStep;
            double angle2  = cell.startAngle + ( j + 1 ) * angleStep;

            // Create quad for this subdivision
            cvf::Vec3d bottomInnerStart = cylindricalToCartesianStatic( radius1, angle1, cell.bottomZ );
            cvf::Vec3d bottomOuterStart = cylindricalToCartesianStatic( radius2, angle1, cell.bottomZ );
            cvf::Vec3d bottomInnerEnd   = cylindricalToCartesianStatic( radius1, angle2, cell.bottomZ );
            cvf::Vec3d bottomOuterEnd   = cylindricalToCartesianStatic( radius2, angle2, cell.bottomZ );

            vertices.push_back( cvf::Vec3f( bottomInnerStart - offset ) );
            vertices.push_back( cvf::Vec3f( bottomOuterStart - offset ) );
            vertices.push_back( cvf::Vec3f( bottomOuterEnd - offset ) );
            vertices.push_back( cvf::Vec3f( bottomInnerEnd - offset ) );

            m_quadMapper->quadToCellIndexMap().push_back( cellIndex );
            m_quadMapper->quadToCellFaceMap().push_back( StructGridInterface::NEG_K );
        }
    }

    // Top face (POS_K) - subdivided curved surface
    for ( int j = 0; j < m_curveSubdivisions; ++j )
    {
        for ( int i = 0; i < m_curveSubdivisions; ++i )
        {
            double radius1 = cell.innerRadius + i * radiusStep;
            double radius2 = cell.innerRadius + ( i + 1 ) * radiusStep;
            double angle1  = cell.startAngle + j * angleStep;
            double angle2  = cell.startAngle + ( j + 1 ) * angleStep;

            // Create quad for this subdivision (reverse winding for upward face)
            cvf::Vec3d topInnerStart = cylindricalToCartesianStatic( radius1, angle1, cell.topZ );
            cvf::Vec3d topOuterStart = cylindricalToCartesianStatic( radius2, angle1, cell.topZ );
            cvf::Vec3d topInnerEnd   = cylindricalToCartesianStatic( radius1, angle2, cell.topZ );
            cvf::Vec3d topOuterEnd   = cylindricalToCartesianStatic( radius2, angle2, cell.topZ );

            vertices.push_back( cvf::Vec3f( topInnerEnd - offset ) );
            vertices.push_back( cvf::Vec3f( topOuterEnd - offset ) );
            vertices.push_back( cvf::Vec3f( topOuterStart - offset ) );
            vertices.push_back( cvf::Vec3f( topInnerStart - offset ) );

            m_quadMapper->quadToCellIndexMap().push_back( cellIndex );
            m_quadMapper->quadToCellFaceMap().push_back( StructGridInterface::POS_K );
        }
    }
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
    return m_grid->getCylindricalCoords( cellIndex,
                                         cell.innerRadius,
                                         cell.outerRadius,
                                         cell.startAngle,
                                         cell.endAngle,
                                         cell.topZ,
                                         cell.bottomZ );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
cvf::Vec3d CylindricalGeometryGenerator::cylindricalToCartesian( double radius, double angle, double z ) const
{
    // Convert angle from degrees to radians for trigonometric functions
    double angleRadians = angle * M_PI / 180.0;
    double x            = radius * std::cos( angleRadians );
    double y            = radius * std::sin( angleRadians );
    return cvf::Vec3d( x, y, z );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
cvf::Vec3d CylindricalGeometryGenerator::cylindricalToCartesianStatic( double radius, double angle, double z )
{
    // Convert angle from degrees to radians for trigonometric functions
    double angleRadians = angle * M_PI / 180.0;
    double x            = radius * std::cos( angleRadians );
    double y            = radius * std::sin( angleRadians );
    return cvf::Vec3d( x, y, z );
}

} // namespace cvf
