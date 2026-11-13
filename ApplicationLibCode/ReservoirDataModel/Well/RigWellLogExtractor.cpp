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

#include "RigWellLogExtractor.h"

#include "RigWellPath.h"

#include "cafAssert.h"

#include <algorithm>
#include <cmath>
#include <tuple>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RigWellLogExtractor::RigWellLogExtractor( const RigWellPath* wellpath, const std::string& wellCaseErrorMsgName )
    : m_wellPathGeometry( wellpath )
    , m_wellCaseErrorMsgName( wellCaseErrorMsgName )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RigWellLogExtractor::~RigWellLogExtractor()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const std::vector<double>& RigWellLogExtractor::cellIntersectionMDs() const
{
    return m_intersectionMeasuredDepths;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const std::vector<double>& RigWellLogExtractor::cellIntersectionTVDs() const
{
    return m_intersectionTVDs;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<WellPathCellIntersectionInfo> RigWellLogExtractor::cellIntersectionInfosAlongWellPath() const
{
    std::vector<WellPathCellIntersectionInfo> infoVector;
    if ( m_intersectedCellsGlobIdx.empty() ) return infoVector;

    for ( size_t i = 0; i < m_intersectedCellsGlobIdx.size() - 1; i = i + 2 )
    {
        CAF_ASSERT( m_intersectedCellsGlobIdx[i] == m_intersectedCellsGlobIdx[i + 1] );

        WellPathCellIntersectionInfo cellInfo;

        cellInfo.globCellIndex = m_intersectedCellsGlobIdx[i];
        cellInfo.startPoint    = m_intersections[i];
        cellInfo.endPoint      = m_intersections[i + 1];
        cellInfo.startMD       = m_intersectionMeasuredDepths[i];
        cellInfo.endMD         = m_intersectionMeasuredDepths[i + 1];

        cellInfo.intersectedCellFaceIn  = m_intersectedCellFaces[i];
        cellInfo.intersectedCellFaceOut = m_intersectedCellFaces[i + 1];

        cellInfo.intersectionLengthsInCellCS = calculateLengthInCell( cellInfo.globCellIndex, cellInfo.startPoint, cellInfo.endPoint );

        infoVector.push_back( cellInfo );
    }

    return infoVector;
}

//--------------------------------------------------------------------------------------------------
/// Insert additional intersections if the distance between two intersections is larger than maxDistanceBetweenIntersections
//--------------------------------------------------------------------------------------------------
void RigWellLogExtractor::resampleIntersections( double maxDistanceBetweenIntersections )
{
    if ( maxDistanceBetweenIntersections <= 0.0 ) return;

    std::vector<cvf::Vec3d>                         resampledIntersections;
    std::vector<size_t>                             resampledIntersectedCellsGlobIdx;
    std::vector<cvf::StructGridInterface::FaceType> resampledIntersectedCellFaces;
    std::vector<double>                             resampledIntersectionMeasuredDepths;
    std::vector<double>                             resampledIntersectionTVDs;

    if ( m_intersections.size() > 1 )
    {
        resampledIntersections.push_back( m_intersections[0] );
        resampledIntersectedCellsGlobIdx.push_back( m_intersectedCellsGlobIdx[0] );
        resampledIntersectedCellFaces.push_back( m_intersectedCellFaces[0] );
        resampledIntersectionMeasuredDepths.push_back( m_intersectionMeasuredDepths[0] );
        resampledIntersectionTVDs.push_back( m_intersectionTVDs[0] );

        for ( size_t i = 1; i < m_intersections.size(); i++ )
        {
            const cvf::Vec3d p1       = m_intersections[i - 1];
            const cvf::Vec3d p2       = m_intersections[i];
            const double     distance = ( p2 - p1 ).length();
            if ( distance > maxDistanceBetweenIntersections )
            {
                size_t numberOfPoints = static_cast<size_t>( distance / maxDistanceBetweenIntersections ) + 1;
                for ( size_t j = 1; j < numberOfPoints; j++ )
                {
                    // Insert data twice, as the curve visualization algorithm is based on pairs of intersections

                    resampledIntersectedCellsGlobIdx.push_back( m_intersectedCellsGlobIdx[i] );
                    resampledIntersectedCellsGlobIdx.push_back( m_intersectedCellsGlobIdx[i] );
                    resampledIntersectedCellFaces.push_back( m_intersectedCellFaces[i] );
                    resampledIntersectedCellFaces.push_back( m_intersectedCellFaces[i] );

                    const double     t = static_cast<double>( j ) / static_cast<double>( numberOfPoints );
                    const cvf::Vec3d p = p1 + ( p2 - p1 ) * t;
                    resampledIntersections.push_back( p );
                    resampledIntersections.push_back( p );

                    const double md = m_intersectionMeasuredDepths[i - 1] +
                                      ( m_intersectionMeasuredDepths[i] - m_intersectionMeasuredDepths[i - 1] ) * t;
                    resampledIntersectionMeasuredDepths.push_back( md );
                    resampledIntersectionMeasuredDepths.push_back( md );

                    const double tvd = m_intersectionTVDs[i - 1] + ( m_intersectionTVDs[i] - m_intersectionTVDs[i - 1] ) * t;
                    resampledIntersectionTVDs.push_back( tvd );
                    resampledIntersectionTVDs.push_back( tvd );
                }
            }
            resampledIntersections.push_back( p2 );
            resampledIntersectedCellsGlobIdx.push_back( m_intersectedCellsGlobIdx[i] );
            resampledIntersectedCellFaces.push_back( m_intersectedCellFaces[i] );
            resampledIntersectionMeasuredDepths.push_back( m_intersectionMeasuredDepths[i] );
            resampledIntersectionTVDs.push_back( m_intersectionTVDs[i] );
        }
    }

    m_intersectionMeasuredDepths = resampledIntersectionMeasuredDepths;
    m_intersectionTVDs           = resampledIntersectionTVDs;
    m_intersections              = resampledIntersections;
    m_intersectedCellsGlobIdx    = resampledIntersectedCellsGlobIdx;
    m_intersectedCellFaces       = resampledIntersectedCellFaces;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::optional<double> RigWellLogExtractor::averageMdForCell( size_t cellIndex ) const
{
    std::vector<size_t> indicesForCell;

    for ( size_t i = 0; i < m_intersectedCellsGlobIdx.size(); i++ )
    {
        if ( m_intersectedCellsGlobIdx[i] == cellIndex ) indicesForCell.emplace_back( i );
    }

    if ( indicesForCell.empty() ) return std::nullopt;

    double sum = 0.0;
    for ( const auto& i : indicesForCell )
    {
        sum += m_intersectionMeasuredDepths[i];
    }

    return sum / static_cast<double>( indicesForCell.size() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const std::vector<size_t>& RigWellLogExtractor::intersectedCellsGlobIdx() const
{
    return m_intersectedCellsGlobIdx;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const std::vector<cvf::Vec3d>& RigWellLogExtractor::intersections() const
{
    return m_intersections;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const std::vector<cvf::StructGridInterface::FaceType>& RigWellLogExtractor::intersectedCellFaces() const
{
    return m_intersectedCellFaces;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const RigWellPath* RigWellLogExtractor::wellPathGeometry() const
{
    return m_wellPathGeometry.p();
}

//--------------------------------------------------------------------------------------------------
/// Appends the crossings of the well path segment p1 - p2, with the measured depth of each crossing point
//--------------------------------------------------------------------------------------------------
void RigWellLogExtractor::appendSegmentCrossings( const std::vector<HexIntersectionInfo>& intersections,
                                                  const cvf::Vec3d&                       p1,
                                                  double                                  md1,
                                                  const cvf::Vec3d&                       p2,
                                                  double                                  md2,
                                                  std::vector<WellPathCellCrossing>*      crossings )
{
    for ( const auto& intersection : intersections )
    {
        const double lengthFromP1 = ( intersection.m_intersectionPoint - p1 ).length();
        const double lengthToP2   = ( p2 - intersection.m_intersectionPoint ).length();
        const double lineLength   = lengthFromP1 + lengthToP2;

        double measuredDepth = md1;
        if ( lineLength > 0.00001 ) measuredDepth = md1 + ( md2 - md1 ) * lengthFromP1 / lineLength;

        crossings->push_back( { measuredDepth, intersection } );
    }
}

//--------------------------------------------------------------------------------------------------
/// Splits the well path into intervals at the cell surface crossings and assigns each interval to the cell containing its
/// midpoint. Crossings closer than mergeTolerance to the first crossing of a group are merged into one breakpoint, and
/// crossings closer than mergeTolerance to the well path ends are merged into the end points. Where cells overlap, typically at
/// faults, the lowest cell index wins, so every measured depth belongs to at most one cell and the intervals never overlap.
/// Consecutive intervals in the same cell are merged, so touches of other cells do not split an interval.
///
/// A cell containing the midpoint of an interval either contains the midpoint of the previous interval or has a crossing at the
/// breakpoint between them, so the candidates are the cells containing the previous midpoint and the cells crossed at the
/// breakpoint. candidateCellsAtPoint is only used for the first interval and when no candidate contains the midpoint.
//--------------------------------------------------------------------------------------------------
std::vector<WellPathCellIntersectionInfo> RigWellLogExtractor::assignCellIntervals( const std::vector<cvf::Vec3d>&    wellPathPoints,
                                                                                    const std::vector<double>&        measuredDepths,
                                                                                    std::vector<WellPathCellCrossing> crossings,
                                                                                    double                            mergeTolerance,
                                                                                    const CandidateCellsAtPoint&      candidateCellsAtPoint,
                                                                                    const IsPointInCell&              isPointInCell )
{
    std::vector<WellPathCellIntersectionInfo> cellIntervals;
    if ( wellPathPoints.size() < 2 || wellPathPoints.size() != measuredDepths.size() ) return cellIntervals;

    const double startMd = measuredDepths.front();
    const double endMd   = measuredDepths.back();
    if ( !( endMd > startMd ) ) return cellIntervals;

    // Deterministic order, independent of the order of the candidate cells
    for ( auto& crossing : crossings )
    {
        crossing.measuredDepth = std::clamp( crossing.measuredDepth, startMd, endMd );
    }

    auto sortKey = []( const WellPathCellCrossing& crossing )
    {
        const HexIntersectionInfo& info = crossing.intersection;
        return std::make_tuple( crossing.measuredDepth,
                                info.m_hexIndex,
                                info.m_isIntersectionEntering,
                                static_cast<int>( info.m_face ),
                                info.m_intersectionPoint.x(),
                                info.m_intersectionPoint.y(),
                                info.m_intersectionPoint.z() );
    };
    std::sort( crossings.begin(),
               crossings.end(),
               [&sortKey]( const WellPathCellCrossing& a, const WellPathCellCrossing& b ) { return sortKey( a ) < sortKey( b ); } );

    struct Breakpoint
    {
        double measuredDepth;
        double firstCrossingMd;
        double lastCrossingMd;
        size_t crossingBegin;
        size_t crossingEnd;
    };

    std::vector<Breakpoint> breakpoints;

    size_t crossingIdx = 0;
    while ( crossingIdx < crossings.size() && crossings[crossingIdx].measuredDepth <= startMd + mergeTolerance )
    {
        crossingIdx++;
    }
    breakpoints.push_back( { startMd, startMd, crossingIdx > 0 ? crossings[crossingIdx - 1].measuredDepth : startMd, 0, crossingIdx } );

    while ( crossingIdx < crossings.size() )
    {
        Breakpoint breakpoint;
        breakpoint.firstCrossingMd = crossings[crossingIdx].measuredDepth;
        breakpoint.crossingBegin   = crossingIdx;
        while ( crossingIdx < crossings.size() && crossings[crossingIdx].measuredDepth < breakpoint.firstCrossingMd + mergeTolerance )
        {
            crossingIdx++;
        }
        breakpoint.crossingEnd    = crossingIdx;
        breakpoint.lastCrossingMd = crossings[crossingIdx - 1].measuredDepth;
        breakpoint.measuredDepth  = 0.5 * ( breakpoint.firstCrossingMd + breakpoint.lastCrossingMd );
        breakpoints.push_back( breakpoint );
    }

    if ( breakpoints.size() > 1 && breakpoints.back().firstCrossingMd >= endMd - mergeTolerance )
    {
        breakpoints.back().measuredDepth = endMd;
    }
    else
    {
        breakpoints.push_back( { endMd, endMd, endMd, crossings.size(), crossings.size() } );
    }

    // Points along the well path for increasing measured depths
    auto pointAtMd = [&]( double measuredDepth, size_t* segmentIdx )
    {
        while ( *segmentIdx + 2 < measuredDepths.size() && measuredDepths[*segmentIdx + 1] < measuredDepth )
        {
            ( *segmentIdx )++;
        }

        const double md1 = measuredDepths[*segmentIdx];
        const double md2 = measuredDepths[*segmentIdx + 1];
        const double t   = md2 > md1 ? std::clamp( ( measuredDepth - md1 ) / ( md2 - md1 ), 0.0, 1.0 ) : 0.0;

        const cvf::Vec3d& p1 = wellPathPoints[*segmentIdx];
        const cvf::Vec3d& p2 = wellPathPoints[*segmentIdx + 1];
        return cvf::Vec3d( p1 + t * ( p2 - p1 ) );
    };

    std::vector<cvf::Vec3d> breakpointPoints;
    {
        size_t segmentIdx = 0;
        for ( const auto& breakpoint : breakpoints )
        {
            breakpointPoints.push_back( pointAtMd( breakpoint.measuredDepth, &segmentIdx ) );
        }
        breakpointPoints.front() = wellPathPoints.front();
        breakpointPoints.back()  = wellPathPoints.back();
    }

    // The face of the cell at a breakpoint is taken from a crossing of the cell, preferably entering at the start of an interval
    // and leaving at the end. The well path end points are used as they are.
    auto cellEndpoint = [&]( size_t breakpointIdx, size_t cellIndex, bool preferEntering )
    {
        const Breakpoint&          breakpoint = breakpoints[breakpointIdx];
        const HexIntersectionInfo* found      = nullptr;
        for ( size_t i = breakpoint.crossingBegin; i < breakpoint.crossingEnd; ++i )
        {
            const HexIntersectionInfo& info = crossings[i].intersection;
            if ( info.m_hexIndex != cellIndex ) continue;

            if ( found == nullptr || ( info.m_isIntersectionEntering == preferEntering && found->m_isIntersectionEntering != preferEntering ) )
            {
                found = &info;
            }
        }

        const bool isWellPathEnd = breakpointIdx == 0 || breakpointIdx + 1 == breakpoints.size();
        if ( found == nullptr ) return std::make_pair( breakpointPoints[breakpointIdx], cvf::StructGridInterface::NO_FACE );
        return std::make_pair( isWellPathEnd ? breakpointPoints[breakpointIdx] : found->m_intersectionPoint, found->m_face );
    };

    auto containingCellsFrom = [&isPointInCell]( std::vector<size_t> candidates, const cvf::Vec3d& point )
    {
        std::sort( candidates.begin(), candidates.end() );
        candidates.erase( std::unique( candidates.begin(), candidates.end() ), candidates.end() );

        std::vector<size_t> containingCells;
        for ( size_t cellIndex : candidates )
        {
            if ( isPointInCell( cellIndex, point ) ) containingCells.push_back( cellIndex );
        }
        return containingCells;
    };

    std::vector<size_t> containingCells;
    size_t              midpointSegmentIdx = 0;
    for ( size_t i = 0; i + 1 < breakpoints.size(); ++i )
    {
        const Breakpoint& start = breakpoints[i];
        const Breakpoint& end   = breakpoints[i + 1];

        // The midpoint is between the crossings, also when merged crossings are spread out
        const cvf::Vec3d midpoint = pointAtMd( 0.5 * ( start.lastCrossingMd + end.firstCrossingMd ), &midpointSegmentIdx );

        std::vector<size_t> candidates = containingCells;
        for ( size_t c = start.crossingBegin; c < start.crossingEnd; ++c )
        {
            candidates.push_back( crossings[c].intersection.m_hexIndex );
        }
        if ( i == 0 )
        {
            const std::vector<size_t> cellsAtPoint = candidateCellsAtPoint( midpoint );
            candidates.insert( candidates.end(), cellsAtPoint.begin(), cellsAtPoint.end() );
        }

        containingCells = containingCellsFrom( candidates, midpoint );
        if ( containingCells.empty() && i > 0 )
        {
            containingCells = containingCellsFrom( candidateCellsAtPoint( midpoint ), midpoint );
        }

        if ( containingCells.empty() || !( end.measuredDepth > start.measuredDepth ) ) continue;

        const size_t cellIndex = containingCells.front();

        const auto [startPoint, startFace] = cellEndpoint( i, cellIndex, true );
        const auto [endPoint, endFace]     = cellEndpoint( i + 1, cellIndex, false );

        if ( !cellIntervals.empty() && cellIntervals.back().globCellIndex == cellIndex && cellIntervals.back().endMD == start.measuredDepth )
        {
            cellIntervals.back().endPoint               = endPoint;
            cellIntervals.back().endMD                  = end.measuredDepth;
            cellIntervals.back().intersectedCellFaceOut = endFace;
            continue;
        }

        WellPathCellIntersectionInfo cellInterval;
        cellInterval.globCellIndex               = cellIndex;
        cellInterval.startPoint                  = startPoint;
        cellInterval.endPoint                    = endPoint;
        cellInterval.startMD                     = start.measuredDepth;
        cellInterval.endMD                       = end.measuredDepth;
        cellInterval.intersectionLengthsInCellCS = cvf::Vec3d::ZERO;
        cellInterval.intersectedCellFaceIn       = startFace;
        cellInterval.intersectedCellFaceOut      = endFace;
        cellIntervals.push_back( cellInterval );
    }

    return cellIntervals;
}

//--------------------------------------------------------------------------------------------------
/// Stores the cell intervals as pairs of points, the start and the end point of each interval
//--------------------------------------------------------------------------------------------------
void RigWellLogExtractor::setCellIntervals( const std::vector<WellPathCellIntersectionInfo>& cellIntervals )
{
    m_intersections.clear();
    m_intersectedCellsGlobIdx.clear();
    m_intersectedCellFaces.clear();
    m_intersectionMeasuredDepths.clear();
    m_intersectionTVDs.clear();

    for ( const auto& cellInterval : cellIntervals )
    {
        for ( bool isStart : { true, false } )
        {
            const cvf::Vec3d& point = isStart ? cellInterval.startPoint : cellInterval.endPoint;

            m_intersections.push_back( point );
            m_intersectedCellsGlobIdx.push_back( cellInterval.globCellIndex );
            m_intersectedCellFaces.push_back( isStart ? cellInterval.intersectedCellFaceIn : cellInterval.intersectedCellFaceOut );
            m_intersectionMeasuredDepths.push_back( isStart ? cellInterval.startMD : cellInterval.endMD );
            m_intersectionTVDs.push_back( std::fabs( point.z() ) );
        }
    }
}