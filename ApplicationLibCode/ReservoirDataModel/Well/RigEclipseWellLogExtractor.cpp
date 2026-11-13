/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) Statoil ASA
//  Copyright (C) Ceetron Solutions AS
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

#include "RigEclipseWellLogExtractor.h"

#include "RiaLogging.h"

#include "RigCaseCellResultsData.h"
#include "RigEclipseCaseData.h"
#include "RigEclipseResultAddress.h"
#include "RigMainGrid.h"
#include "RigResultAccessor.h"
#include "RigWellPath.h"
#include "RigWellPathGeometryTools.h"
#include "RigWellPathIntersectionTools.h"

#include "cvfBoundingBox.h"
#include "cvfGeometryTools.h"

#include "cafAssert.h"

#include <algorithm>
#include <array>
#include <iterator>
#include <limits>
#include <map>
#include <tuple>

//==================================================================================================
///
//==================================================================================================

RigEclipseWellLogExtractor::RigEclipseWellLogExtractor( const RigEclipseCaseData* aCase,
                                                        const RigWellPath*        wellpath,
                                                        const std::string&        wellCaseErrorMsgName )
    : RigWellLogExtractor( wellpath, wellCaseErrorMsgName )
    , m_caseData( aCase )
{
    calculateIntersection();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigEclipseWellLogExtractor::calculateIntersection()
{
    const std::vector<cvf::Vec3d>& wellPathPoints = m_wellPathGeometry->wellPathPoints();
    const std::vector<double>&     measuredDepths = m_wellPathGeometry->measuredDepths();
    if ( wellPathPoints.size() < 2 ) return;

    const RigMainGrid* mainGrid             = m_caseData->mainGrid();
    const bool         isCellFaceNormalsOut = mainGrid->isFaceNormalsOutwards();

    auto isCellUsed = [mainGrid]( size_t globalCellIndex )
    {
        const RigCell& cell = mainGrid->cell( globalCellIndex );
        return !cell.isInvalid() && cell.subGrid() == nullptr;
    };

    std::vector<WellPathCellCrossing> crossings;
    for ( size_t wpp = 0; wpp < wellPathPoints.size() - 1; ++wpp )
    {
        std::vector<HexIntersectionInfo> intersections;
        const cvf::Vec3d&                p1 = wellPathPoints[wpp];
        const cvf::Vec3d&                p2 = wellPathPoints[wpp + 1];

        cvf::BoundingBox bb;
        bb.add( p1 );
        bb.add( p2 );

        for ( const auto& globalCellIndex : findCloseCellIndices( bb ) )
        {
            if ( !isCellUsed( globalCellIndex ) ) continue;

            std::array<cvf::Vec3d, 8> hexCorners = mainGrid->cellCornerVertices( globalCellIndex );
            RigHexIntersectionTools::lineHexCellIntersection( p1, p2, hexCorners, globalCellIndex, &intersections );
        }

        if ( !isCellFaceNormalsOut )
        {
            for ( auto& intersection : intersections )
            {
                intersection.m_isIntersectionEntering = !intersection.m_isIntersectionEntering;
            }
        }

        appendSegmentCrossings( intersections, p1, measuredDepths[wpp], p2, measuredDepths[wpp + 1], &crossings );
    }

    auto candidateCellsAtPoint = [this, &isCellUsed]( const cvf::Vec3d& point )
    {
        cvf::BoundingBox bb;
        bb.add( point );

        std::vector<size_t> cells = findCloseCellIndices( bb );
        cells.erase( std::remove_if( cells.begin(), cells.end(), [&isCellUsed]( size_t cellIndex ) { return !isCellUsed( cellIndex ); } ),
                     cells.end() );
        return cells;
    };

    auto isPointInCell = [mainGrid]( size_t globalCellIndex, const cvf::Vec3d& point )
    { return RigHexIntersectionTools::isPointInCell( point, mainGrid->cellCornerVertices( globalCellIndex ) ); };

    std::vector<WellPathCellIntersectionInfo> cellIntervals =
        assignCellIntervals( wellPathPoints, measuredDepths, std::move( crossings ), computeLengthThreshold(), candidateCellsAtPoint, isPointInCell );

    if ( cellIntervals.empty() && mainGrid->isRadial() )
    {
        cellIntervals = radialGridAxisIntervals();
    }

    setCellIntervals( cellIntervals );
}

//--------------------------------------------------------------------------------------------------
/// A well in a radial grid is located on the grid axis, inside the inner radius or on a face between angular cells, and is not
/// intersected by any cell. Use the innermost cell column and place the cells along the well path by depth. Requires the innermost
/// ring to consist of a single 360 degree cell for every layer, as picking one of several angular sectors would be arbitrary.
//--------------------------------------------------------------------------------------------------
std::vector<WellPathCellIntersectionInfo> RigEclipseWellLogExtractor::radialGridAxisIntervals() const
{
    std::vector<WellPathCellIntersectionInfo> cellIntervals;

    const RigMainGrid* mainGrid = m_caseData->mainGrid();

    // Only the innermost ring can safely represent the well's position when its cells span the full 360 degrees for every layer.
    // If the ring is split into several angular sectors, picking a single sector would be an arbitrary and potentially wrong choice.
    if ( mainGrid->cellCountJ() != 1 ) return cellIntervals;

    const std::vector<double>& wellPathMds  = m_wellPathGeometry->measuredDepths();
    const std::vector<double>  wellPathTvds = m_wellPathGeometry->trueVerticalDepths();
    if ( wellPathMds.size() < 2 ) return cellIntervals;

    const auto [minTvdIt, maxTvdIt] = std::minmax_element( wellPathTvds.begin(), wellPathTvds.end() );
    const double minTvd             = *minTvdIt;
    const double maxTvd             = *maxTvdIt;

    std::vector<std::tuple<double, double, size_t>> cellDepths;
    for ( size_t k = 0; k < mainGrid->cellCountK(); k++ )
    {
        const size_t cellIndex = mainGrid->cellIndexFromIJK( 0, 0, k );
        if ( mainGrid->cell( cellIndex ).isInvalid() ) continue;

        double topTvd    = std::numeric_limits<double>::infinity();
        double bottomTvd = -std::numeric_limits<double>::infinity();
        for ( const auto& corner : mainGrid->cellCornerVertices( cellIndex ) )
        {
            topTvd    = std::min( topTvd, -corner.z() );
            bottomTvd = std::max( bottomTvd, -corner.z() );
        }

        if ( bottomTvd <= minTvd || topTvd >= maxTvd ) continue;

        cellDepths.emplace_back( std::max( topTvd, minTvd ), std::min( bottomTvd, maxTvd ), cellIndex );
    }

    if ( cellDepths.empty() ) return cellIntervals;

    // MD interpolation requires unique TVD values ordered along the well path. K is not guaranteed to increase with depth, and
    // neighbour cells share their boundary TVD.
    std::vector<double> boundaryTvds;
    for ( const auto& [topTvd, bottomTvd, cellIndex] : cellDepths )
    {
        boundaryTvds.push_back( topTvd );
        boundaryTvds.push_back( bottomTvd );
    }
    std::sort( boundaryTvds.begin(), boundaryTvds.end() );
    boundaryTvds.erase( std::unique( boundaryTvds.begin(), boundaryTvds.end() ), boundaryTvds.end() );

    const std::vector<double> boundaryMds = RigWellPathGeometryTools::interpolateMdFromTvd( wellPathMds, wellPathTvds, boundaryTvds );
    if ( boundaryMds.size() != boundaryTvds.size() ) return cellIntervals;

    auto mdAtTvd = [&]( double tvd )
    {
        const auto it = std::lower_bound( boundaryTvds.begin(), boundaryTvds.end(), tvd );
        return boundaryMds[std::distance( boundaryTvds.begin(), it )];
    };

    for ( const auto& [topTvd, bottomTvd, cellIndex] : cellDepths )
    {
        const double startMd = std::min( mdAtTvd( topTvd ), mdAtTvd( bottomTvd ) );
        const double endMd   = std::max( mdAtTvd( topTvd ), mdAtTvd( bottomTvd ) );
        if ( !( endMd > startMd ) ) continue;

        WellPathCellIntersectionInfo cellInterval;
        cellInterval.globCellIndex               = cellIndex;
        cellInterval.startMD                     = startMd;
        cellInterval.endMD                       = endMd;
        cellInterval.startPoint                  = m_wellPathGeometry->interpolatedPointAlongWellPath( startMd );
        cellInterval.endPoint                    = m_wellPathGeometry->interpolatedPointAlongWellPath( endMd );
        cellInterval.intersectionLengthsInCellCS = cvf::Vec3d::ZERO;
        cellInterval.intersectedCellFaceIn       = cvf::StructGridInterface::NO_FACE;
        cellInterval.intersectedCellFaceOut      = cvf::StructGridInterface::NO_FACE;
        cellIntervals.push_back( cellInterval );
    }

    std::sort( cellIntervals.begin(),
               cellIntervals.end(),
               []( const WellPathCellIntersectionInfo& a, const WellPathCellIntersectionInfo& b ) { return a.startMD < b.startMD; } );

    return cellIntervals;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigEclipseWellLogExtractor::curveData( const RigResultAccessor* resultAccessor, std::vector<double>* values )
{
    CAF_ASSERT( values );
    values->resize( intersections().size() );

    for ( size_t cpIdx = 0; cpIdx < intersections().size(); ++cpIdx )
    {
        size_t                             cellIdx  = intersectedCellsGlobIdx()[cpIdx];
        cvf::StructGridInterface::FaceType cellFace = intersectedCellFaces()[cpIdx];
        ( *values )[cpIdx]                          = resultAccessor->cellFaceScalarGlobIdx( cellIdx, cellFace );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<size_t> RigEclipseWellLogExtractor::findCloseCellIndices( const cvf::BoundingBox& bb )
{
    return m_caseData->mainGrid()->findIntersectingCells( bb );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
cvf::Vec3d RigEclipseWellLogExtractor::calculateLengthInCell( size_t cellIndex, const cvf::Vec3d& startPoint, const cvf::Vec3d& endPoint ) const
{
    std::array<cvf::Vec3d, 8> hexCorners = m_caseData->mainGrid()->cellCornerVertices( cellIndex );

    return RigWellPathIntersectionTools::calculateLengthInCell( hexCorners, startPoint, endPoint );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RigEclipseWellLogExtractor::computeLengthThreshold() const
{
    // Default length tolerance for most common grid sizes
    double tolerance = 0.01;

    // For grids with very thin z-layers, reduce the tolerance to be able to find the intersections
    // If not, the intersection will be considered as non-valid cell edge intersection and discarded
    // https://github.com/OPM/ResInsight/issues/9244

    auto gridCellResult = const_cast<RigCaseCellResultsData*>( m_caseData->results( RiaDefines::PorosityModelType::MATRIX_MODEL ) );

    auto resultAdr = RigEclipseResultAddress( RiaDefines::ResultCatType::STATIC_NATIVE, "DZ" );
    if ( gridCellResult && gridCellResult->hasResultEntry( resultAdr ) )
    {
        double averageDZ = 0.1;
        gridCellResult->meanCellScalarValues( resultAdr, averageDZ );

        const double scaleFactor = 0.05;
        tolerance                = std::min( tolerance, averageDZ * scaleFactor );
    }

    return tolerance;
}
