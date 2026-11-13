/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2018-     Equinor ASA
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

#include "RiaTestDataDirectory.h"

#include "RifReaderMockModel.h"
#include "RifReaderOpmCommon.h"

#include "RigActiveCellInfo.h"
#include "RigEclipseCaseData.h"
#include "RigGridManager.h"
#include "RigMainGrid.h"
#include "Well/RigEclipseWellLogExtractor.h"
#include "Well/RigWellLogExtractor.h"
#include "Well/RigWellPath.h"

#include <QDir>

#include <chrono>
#include <cmath>
#include <iostream>
#include <tuple>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RigEclipseWellLogExtractor, ShortWellPathInsideOneCell )
{
    cvf::ref<RigEclipseCaseData> reservoir = new RigEclipseCaseData( nullptr );

    {
        cvf::ref<RifReaderMockModel> mockFileInterface = new RifReaderMockModel;

        mockFileInterface->setWorldCoordinates( cvf::Vec3d( 10, 10, 10 ), cvf::Vec3d( 20, 20, 20 ) );
        mockFileInterface->setCellCounts( cvf::Vec3st( 5, 6, 7 ) );
        mockFileInterface->enableWellData( false );

        mockFileInterface->open( "", reservoir.p() );

        reservoir->mainGrid()->computeCachedData();
    }

    EXPECT_FALSE( reservoir->mainGrid()->totalCellCount() == 0 );

    auto firstCell = reservoir->mainGrid()->cell( 0 );
    auto center    = firstCell.center();

    cvf::ref<RigWellPath> wellPathGeometry = new RigWellPath;
    {
        std::vector<cvf::Vec3d> wellPathPoints;
        std::vector<double>     mdValues;

        {
            double offset = 0.0;
            wellPathPoints.push_back( center );
            mdValues.push_back( offset );
        }

        {
            double offset = 0.1;
            wellPathPoints.push_back( center + cvf::Vec3d( 0, 0, offset ) );
            mdValues.push_back( offset );
        }

        wellPathGeometry->setWellPathPoints( wellPathPoints, mdValues );
    }

    cvf::ref<RigEclipseWellLogExtractor> e = new RigEclipseWellLogExtractor( reservoir.p(), wellPathGeometry.p(), "" );

    auto intersections = e->cellIntersectionInfosAlongWellPath();
    EXPECT_FALSE( intersections.empty() );
}

//--------------------------------------------------------------------------------------------------
/// Reproduces https://github.com/OPM/ResInsight/issues/13967
/// A horizontal well crossing several cells must report all the cells it passes through, regardless
/// of which combination of Flip X / Flip Y has been applied to the grid.
//--------------------------------------------------------------------------------------------------
TEST( RigEclipseWellLogExtractor, HorizontalWellAcrossFlippedGrid )
{
    auto buildReservoir = []( bool flipX, bool flipY )
    {
        cvf::ref<RigEclipseCaseData> reservoir         = new RigEclipseCaseData( nullptr );
        cvf::ref<RifReaderMockModel> mockFileInterface = new RifReaderMockModel;

        mockFileInterface->setWorldCoordinates( cvf::Vec3d( 10, 10, 10 ), cvf::Vec3d( 20, 20, 20 ) );
        mockFileInterface->setCellCounts( cvf::Vec3st( 5, 6, 7 ) );
        mockFileInterface->enableWellData( false );
        mockFileInterface->open( "", reservoir.p() );

        if ( flipX || flipY ) reservoir->mainGrid()->setFlipAxis( flipX, flipY );

        reservoir->mainGrid()->computeCachedData();
        return reservoir;
    };

    auto runWellPath = []( cvf::ref<RigEclipseCaseData> reservoir, bool flipX, bool flipY )
    {
        const double            xSign            = flipX ? -1.0 : 1.0;
        const double            ySign            = flipY ? -1.0 : 1.0;
        cvf::ref<RigWellPath>   wellPathGeometry = new RigWellPath;
        std::vector<cvf::Vec3d> wellPathPoints   = { cvf::Vec3d( xSign * 10.5, ySign * 13.0, 15.0 ),
                                                     cvf::Vec3d( xSign * 19.5, ySign * 13.0, 15.0 ) };
        std::vector<double>     mdValues         = { 0.0, 9.0 };
        wellPathGeometry->setWellPathPoints( wellPathPoints, mdValues );

        cvf::ref<RigEclipseWellLogExtractor> e = new RigEclipseWellLogExtractor( reservoir.p(), wellPathGeometry.p(), "" );
        return e->cellIntersectionInfosAlongWellPath();
    };

    auto baseline = runWellPath( buildReservoir( false, false ), false, false );
    EXPECT_GT( baseline.size(), 2u );

    struct FlipCase
    {
        bool flipX;
        bool flipY;
    };
    const FlipCase cases[] = { { false, false }, { true, false }, { false, true }, { true, true } };

    for ( const auto& c : cases )
    {
        // Freshly-built grid with the requested flip applied before the first lazy cache populates.
        auto fresh = runWellPath( buildReservoir( c.flipX, c.flipY ), c.flipX, c.flipY );
        EXPECT_EQ( baseline.size(), fresh.size() ) << "fresh: flipX=" << c.flipX << " flipY=" << c.flipY;

        // Interactive toggle: load un-flipped, populate the face-normal-direction cache, then flip.
        // The intersection result must still match the freshly-built flipped grid.
        auto toggled = buildReservoir( false, false );
        (void)toggled->mainGrid()->isFaceNormalsOutwards();
        toggled->mainGrid()->setFlipAxis( c.flipX, c.flipY );
        toggled->mainGrid()->computeCachedData();

        auto toggledCells = runWellPath( toggled, c.flipX, c.flipY );
        EXPECT_EQ( baseline.size(), toggledCells.size() ) << "toggled: flipX=" << c.flipX << " flipY=" << c.flipY;
    }
}

namespace
{
//--------------------------------------------------------------------------------------------------
/// 3x1x4 grid with TVD 1000-1004, located at x = [1, 4]. A vertical well at x = 0 is outside all cells, as a well on the axis of a
/// radial grid is.
//--------------------------------------------------------------------------------------------------
cvf::ref<RigEclipseCaseData> createRadialAxisTestReservoir( bool isRadial, bool kIncreasesUpwards )
{
    cvf::ref<RigEclipseCaseData> reservoir         = new RigEclipseCaseData( nullptr );
    cvf::ref<RifReaderMockModel> mockFileInterface = new RifReaderMockModel;

    const double topZ    = -1000.0;
    const double bottomZ = -1004.0;
    if ( kIncreasesUpwards )
    {
        mockFileInterface->setWorldCoordinates( cvf::Vec3d( 1, 0, bottomZ ), cvf::Vec3d( 4, 1, topZ ) );
    }
    else
    {
        mockFileInterface->setWorldCoordinates( cvf::Vec3d( 1, 0, topZ ), cvf::Vec3d( 4, 1, bottomZ ) );
    }
    mockFileInterface->setCellCounts( cvf::Vec3st( 3, 1, 4 ) );
    mockFileInterface->enableWellData( false );
    mockFileInterface->open( "", reservoir.p() );

    reservoir->mainGrid()->setIsRadial( isRadial );
    reservoir->mainGrid()->computeCachedData();

    return reservoir;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<WellPathCellIntersectionInfo> radialAxisIntersections( RigEclipseCaseData* reservoir, double topTvd, double bottomTvd, double topMd )
{
    cvf::ref<RigWellPath> wellPathGeometry = new RigWellPath;
    wellPathGeometry->setWellPathPoints( { cvf::Vec3d( 0, 0.5, -topTvd ), cvf::Vec3d( 0, 0.5, -bottomTvd ) },
                                         { topMd, topMd + bottomTvd - topTvd } );

    cvf::ref<RigEclipseWellLogExtractor> e = new RigEclipseWellLogExtractor( reservoir, wellPathGeometry.p(), "" );
    return e->cellIntersectionInfosAlongWellPath();
}
} // namespace

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RigEclipseWellLogExtractor, RadialGridWellOnAxisUsesInnermostColumn )
{
    for ( bool kIncreasesUpwards : { false, true } )
    {
        auto reservoir = createRadialAxisTestReservoir( true, kIncreasesUpwards );

        auto intersections = radialAxisIntersections( reservoir.p(), 1000.0, 1004.0, 100.0 );
        ASSERT_EQ( 4u, intersections.size() ) << "kIncreasesUpwards=" << kIncreasesUpwards;

        for ( size_t i = 0; i < intersections.size(); i++ )
        {
            const size_t k = kIncreasesUpwards ? 3 - i : i;
            EXPECT_EQ( reservoir->mainGrid()->cellIndexFromIJK( 0, 0, k ), intersections[i].globCellIndex );
            EXPECT_NEAR( 100.0 + i, intersections[i].startMD, 1e-6 );
            EXPECT_NEAR( 101.0 + i, intersections[i].endMD, 1e-6 );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RigEclipseWellLogExtractor, RadialGridWellOnAxisClipsCellsToWellPathDepth )
{
    auto reservoir = createRadialAxisTestReservoir( true, false );

    auto intersections = radialAxisIntersections( reservoir.p(), 1001.5, 1003.0, 0.0 );
    ASSERT_EQ( 2u, intersections.size() );

    EXPECT_EQ( reservoir->mainGrid()->cellIndexFromIJK( 0, 0, 1 ), intersections[0].globCellIndex );
    EXPECT_NEAR( 0.0, intersections[0].startMD, 1e-6 );
    EXPECT_NEAR( 0.5, intersections[0].endMD, 1e-6 );

    EXPECT_EQ( reservoir->mainGrid()->cellIndexFromIJK( 0, 0, 2 ), intersections[1].globCellIndex );
    EXPECT_NEAR( 0.5, intersections[1].startMD, 1e-6 );
    EXPECT_NEAR( 1.5, intersections[1].endMD, 1e-6 );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RigEclipseWellLogExtractor, NonRadialGridWellOutsideGridHasNoIntersections )
{
    auto reservoir = createRadialAxisTestReservoir( false, false );

    EXPECT_TRUE( radialAxisIntersections( reservoir.p(), 1000.0, 1004.0, 100.0 ).empty() );
}

//--------------------------------------------------------------------------------------------------
/// The innermost ring fallback only applies when it is a single 360 degree cell for every layer. When the ring is split into
/// several angular sectors, picking a single sector would be arbitrary, so no fallback intersections should be inserted.
//--------------------------------------------------------------------------------------------------
TEST( RigEclipseWellLogExtractor, RadialGridWellOnAxisWithMultipleAngularSectorsHasNoFallback )
{
    cvf::ref<RigEclipseCaseData> reservoir         = new RigEclipseCaseData( nullptr );
    cvf::ref<RifReaderMockModel> mockFileInterface = new RifReaderMockModel;

    mockFileInterface->setWorldCoordinates( cvf::Vec3d( 1, 0, -1004.0 ), cvf::Vec3d( 4, 1, -1000.0 ) );
    mockFileInterface->setCellCounts( cvf::Vec3st( 3, 2, 4 ) );
    mockFileInterface->enableWellData( false );
    mockFileInterface->open( "", reservoir.p() );

    reservoir->mainGrid()->setIsRadial( true );
    reservoir->mainGrid()->computeCachedData();

    EXPECT_TRUE( radialAxisIntersections( reservoir.p(), 1000.0, 1004.0, 100.0 ).empty() );
}

//--------------------------------------------------------------------------------------------------
/// setFlipAxis must actually mirror the grid node coordinates for every combination of Flip X /
/// Flip Y. Covers the data-level part of https://github.com/OPM/ResInsight/issues/13967 where a
/// saved flip flag could appear active while the grid was still un-flipped.
//--------------------------------------------------------------------------------------------------
TEST( RigMainGrid, SetFlipAxisMirrorsNodeCoordinates )
{
    auto checkFlip = []( bool flipX, bool flipY )
    {
        cvf::ref<RigEclipseCaseData> reservoir         = new RigEclipseCaseData( nullptr );
        cvf::ref<RifReaderMockModel> mockFileInterface = new RifReaderMockModel;

        mockFileInterface->setWorldCoordinates( cvf::Vec3d( 100, 200, 300 ), cvf::Vec3d( 110, 210, 310 ) );
        mockFileInterface->setCellCounts( cvf::Vec3st( 2, 2, 2 ) );
        mockFileInterface->enableWellData( false );
        mockFileInterface->open( "", reservoir.p() );

        auto*                         mainGrid = reservoir->mainGrid();
        const std::vector<cvf::Vec3d> originalNodes( mainGrid->nodes().begin(), mainGrid->nodes().end() );
        ASSERT_FALSE( originalNodes.empty() );

        mainGrid->setFlipAxis( flipX, flipY );

        const auto flippedNodes = mainGrid->nodes();
        ASSERT_EQ( originalNodes.size(), flippedNodes.size() );

        const double xSign = flipX ? -1.0 : 1.0;
        const double ySign = flipY ? -1.0 : 1.0;
        for ( size_t i = 0; i < originalNodes.size(); i++ )
        {
            EXPECT_DOUBLE_EQ( xSign * originalNodes[i].x(), flippedNodes[i].x() ) << "flipX=" << flipX << " flipY=" << flipY;
            EXPECT_DOUBLE_EQ( ySign * originalNodes[i].y(), flippedNodes[i].y() ) << "flipX=" << flipX << " flipY=" << flipY;
            EXPECT_DOUBLE_EQ( originalNodes[i].z(), flippedNodes[i].z() ) << "flipX=" << flipX << " flipY=" << flipY;
        }
    };

    checkFlip( false, false );
    checkFlip( true, false );
    checkFlip( false, true );
    checkFlip( true, true );
}

namespace
{
struct TestCell
{
    size_t index;
    double start;
    double end;
};

using CellInterval = std::tuple<size_t, double, double>; // cell index, start MD, end MD

//--------------------------------------------------------------------------------------------------
/// Assigns cell intervals for a well path along the x axis, with MD = x, through one-dimensional cells [start, end]. The
/// crossings are reported per well path segment, so a crossing at a well path vertex is reported twice.
//--------------------------------------------------------------------------------------------------
std::vector<CellInterval> assignIntervals( const std::vector<TestCell>& cells, const std::vector<double>& wellMds, double tolerance = 0.01 )
{
    std::vector<cvf::Vec3d> wellPathPoints;
    for ( double md : wellMds )
    {
        wellPathPoints.emplace_back( md, 0.0, 0.0 );
    }

    std::vector<WellPathCellCrossing> crossings;
    for ( size_t segment = 0; segment + 1 < wellMds.size(); segment++ )
    {
        for ( const auto& cell : cells )
        {
            for ( bool isEntering : { true, false } )
            {
                const double x = isEntering ? cell.start : cell.end;
                if ( x < wellMds[segment] || x > wellMds[segment + 1] ) continue;

                crossings.push_back(
                    { x, HexIntersectionInfo( cvf::Vec3d( x, 0.0, 0.0 ), isEntering, cvf::StructGridInterface::NO_FACE, cell.index ) } );
            }
        }
    }

    auto candidateCellsAtPoint = [&cells]( const cvf::Vec3d& )
    {
        std::vector<size_t> indices;
        for ( const auto& cell : cells )
        {
            indices.push_back( cell.index );
        }
        return indices;
    };

    auto isPointInCell = [&cells]( size_t cellIndex, const cvf::Vec3d& point )
    {
        for ( const auto& cell : cells )
        {
            if ( cell.index == cellIndex ) return cell.start <= point.x() && point.x() <= cell.end;
        }
        return false;
    };

    auto infos =
        RigWellLogExtractor::assignCellIntervals( wellPathPoints, wellMds, crossings, tolerance, candidateCellsAtPoint, isPointInCell );

    std::vector<CellInterval> intervals;
    for ( size_t i = 0; i < infos.size(); i++ )
    {
        EXPECT_LT( infos[i].startMD, infos[i].endMD );
        if ( i > 0 ) EXPECT_LE( infos[i - 1].endMD, infos[i].startMD );

        intervals.emplace_back( infos[i].globCellIndex, infos[i].startMD, infos[i].endMD );
    }
    return intervals;
}
} // namespace

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RigWellLogExtractor, AssignsCellChain )
{
    std::vector<CellInterval> expected = { { 1, 0, 10 }, { 2, 10, 20 }, { 3, 20, 30 } };
    EXPECT_EQ( expected, assignIntervals( { { 1, 0, 10 }, { 2, 10, 20 }, { 3, 20, 30 } }, { 0, 30 } ) );
}

//--------------------------------------------------------------------------------------------------
/// Crossings at a well path vertex are reported by both segments
//--------------------------------------------------------------------------------------------------
TEST( RigWellLogExtractor, AssignsCellChainWithCrossingAtWellPathVertex )
{
    std::vector<CellInterval> expected = { { 1, 0, 10 }, { 2, 10, 20 }, { 3, 20, 30 } };
    EXPECT_EQ( expected, assignIntervals( { { 1, 0, 10 }, { 2, 10, 20 }, { 3, 20, 30 } }, { 0, 10, 20, 30 } ) );
}

//--------------------------------------------------------------------------------------------------
/// Partly overlapping cells, typically at a fault. The lowest cell index wins in the overlap
//--------------------------------------------------------------------------------------------------
TEST( RigWellLogExtractor, AssignsOverlapToLowestCellIndex )
{
    {
        std::vector<CellInterval> expected = { { 1, 0, 20 }, { 2, 20, 30 } };
        EXPECT_EQ( expected, assignIntervals( { { 1, 0, 20 }, { 2, 10, 30 } }, { 0, 30 } ) );
    }
    {
        std::vector<CellInterval> expected = { { 2, 0, 10 }, { 1, 10, 30 } };
        EXPECT_EQ( expected, assignIntervals( { { 2, 0, 20 }, { 1, 10, 30 } }, { 0, 30 } ) );
    }
}

//--------------------------------------------------------------------------------------------------
/// A cell inside another cell, as an LGR cell inside its host cell
//--------------------------------------------------------------------------------------------------
TEST( RigWellLogExtractor, AssignsNestedCells )
{
    {
        std::vector<CellInterval> expected = { { 1, 0, 40 } };
        EXPECT_EQ( expected, assignIntervals( { { 1, 0, 40 }, { 2, 10, 20 } }, { 0, 40 } ) );
    }
    {
        std::vector<CellInterval> expected = { { 5, 0, 10 }, { 2, 10, 20 }, { 5, 20, 40 } };
        EXPECT_EQ( expected, assignIntervals( { { 5, 0, 40 }, { 2, 10, 20 } }, { 0, 40 } ) );
    }
}

//--------------------------------------------------------------------------------------------------
/// A cell touched at a single point does not split the interval of the cell around it
//--------------------------------------------------------------------------------------------------
TEST( RigWellLogExtractor, IgnoresTouchedCell )
{
    std::vector<CellInterval> expected = { { 3, 0, 30 } };
    EXPECT_EQ( expected, assignIntervals( { { 3, 0, 30 }, { 1, 15, 15 } }, { 0, 30 } ) );
}

//--------------------------------------------------------------------------------------------------
/// The well path starts and ends inside cells, with no crossings at the well path ends
//--------------------------------------------------------------------------------------------------
TEST( RigWellLogExtractor, AssignsCellsAtWellPathEnds )
{
    std::vector<CellInterval> expected = { { 1, 5, 10 }, { 2, 10, 25 } };
    EXPECT_EQ( expected, assignIntervals( { { 1, 0, 10 }, { 2, 10, 30 } }, { 5, 25 } ) );

    std::vector<CellInterval> insideOneCell = { { 2, 12, 13 } };
    EXPECT_EQ( insideOneCell, assignIntervals( { { 1, 0, 10 }, { 2, 10, 30 } }, { 12, 13 } ) );
}

//--------------------------------------------------------------------------------------------------
/// Crossings closer than the tolerance are merged into one breakpoint, also next to the well path ends
//--------------------------------------------------------------------------------------------------
TEST( RigWellLogExtractor, MergesCrossingsWithinTolerance )
{
    auto intervals = assignIntervals( { { 1, 0.01, 10 }, { 2, 10.004, 20 }, { 3, 20, 29.995 } }, { 0.005, 30 } );
    ASSERT_EQ( 3u, intervals.size() );

    EXPECT_EQ( 1u, std::get<0>( intervals[0] ) );
    EXPECT_DOUBLE_EQ( 0.005, std::get<1>( intervals[0] ) );
    EXPECT_DOUBLE_EQ( 10.002, std::get<2>( intervals[0] ) );

    EXPECT_EQ( 2u, std::get<0>( intervals[1] ) );
    EXPECT_DOUBLE_EQ( 10.002, std::get<1>( intervals[1] ) );
    EXPECT_DOUBLE_EQ( 20.0, std::get<2>( intervals[1] ) );

    EXPECT_EQ( CellInterval( 3, 20, 30 ), intervals[2] );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RigWellLogExtractor, SkipsGapBetweenCells )
{
    std::vector<CellInterval> expected = { { 1, 0, 10 }, { 2, 15, 20 } };
    EXPECT_EQ( expected, assignIntervals( { { 1, 0, 10 }, { 2, 15, 20 } }, { 0, 20 } ) );

    EXPECT_TRUE( assignIntervals( { { 1, 0, 10 } }, { 20, 30 } ).empty() );
}

//--------------------------------------------------------------------------------------------------
/// A well path vertex exactly on the faces between cells gives contiguous cell intervals without gaps or overlaps
//--------------------------------------------------------------------------------------------------
TEST( RigEclipseWellLogExtractor, WellPathVertexOnCellFace )
{
    cvf::ref<RigEclipseCaseData> reservoir         = new RigEclipseCaseData( nullptr );
    cvf::ref<RifReaderMockModel> mockFileInterface = new RifReaderMockModel;

    mockFileInterface->setWorldCoordinates( cvf::Vec3d( 10, 10, 10 ), cvf::Vec3d( 20, 20, 20 ) );
    mockFileInterface->setCellCounts( cvf::Vec3st( 5, 5, 5 ) );
    mockFileInterface->enableWellData( false );
    mockFileInterface->open( "", reservoir.p() );
    reservoir->mainGrid()->computeCachedData();

    // Horizontal, then bending downwards at x = 14 on the face between two cells, which is also the face between two layers
    cvf::ref<RigWellPath> wellPathGeometry = new RigWellPath;
    wellPathGeometry->setWellPathPoints( { cvf::Vec3d( 10.5, 13.0, 14.0 ), cvf::Vec3d( 14.0, 13.0, 14.0 ), cvf::Vec3d( 19.5, 13.0, 19.5 ) },
                                         { 0.0, 3.5, 3.5 + std::sqrt( 2.0 * 5.5 * 5.5 ) } );

    cvf::ref<RigEclipseWellLogExtractor> e = new RigEclipseWellLogExtractor( reservoir.p(), wellPathGeometry.p(), "" );

    auto intersections = e->cellIntersectionInfosAlongWellPath();
    ASSERT_FALSE( intersections.empty() );

    EXPECT_DOUBLE_EQ( 0.0, intersections.front().startMD );
    EXPECT_DOUBLE_EQ( wellPathGeometry->measuredDepths().back(), intersections.back().endMD );
    for ( size_t i = 1; i < intersections.size(); i++ )
    {
        EXPECT_DOUBLE_EQ( intersections[i - 1].endMD, intersections[i].startMD );
        EXPECT_NE( intersections[i - 1].globCellIndex, intersections[i].globCellIndex );
    }
}

//--------------------------------------------------------------------------------------------------
/// Timing of well log extraction for long horizontal, deviated and vertical wells in the faulted TEST10K_FLT_LGR_NNC
/// model. Disabled by default, run with --gtest_also_run_disabled_tests --gtest_filter=*ExtractionPerformance*
//--------------------------------------------------------------------------------------------------
TEST( RigEclipseWellLogExtractor, DISABLED_ExtractionPerformance )
{
    QDir    baseFolder( TEST_MODEL_DIR );
    QString filename = baseFolder.absoluteFilePath( "TEST10K_FLT_LGR_NNC/TEST10K_FLT_LGR_NNC.EGRID" );
    ASSERT_TRUE( QFile::exists( filename ) );

    cvf::ref<RigEclipseCaseData> caseData = new RigEclipseCaseData( nullptr );
    cvf::ref<RifReaderOpmCommon> reader   = new RifReaderOpmCommon;
    ASSERT_TRUE( reader->open( filename, caseData.p() ) );

    RigMainGrid* mainGrid = caseData->mainGrid();
    mainGrid->computeCachedData();
    {
        auto                activeCellInfo = caseData->activeCellInfo( RiaDefines::PorosityModelType::MATRIX_MODEL );
        std::vector<size_t> activeCells;
        for ( size_t i = 0; i < mainGrid->cellCount(); i++ )
        {
            if ( activeCellInfo->isActive( ReservoirCellIndex( i ) ) ) activeCells.push_back( i );
        }
        mainGrid->computeCharacteristicCellSize( activeCells );
        mainGrid->computeFaceNormalsDirection( activeCells );
    }

    const RigActiveCellInfo* activeCellInfo = caseData->activeCellInfo( RiaDefines::PorosityModelType::MATRIX_MODEL );

    // Span the active cells, as the full grid bounding box includes inactive cells with corners far outside the reservoir
    cvf::BoundingBox bb;
    for ( size_t cellIndex = 0; cellIndex < mainGrid->totalCellCount(); cellIndex++ )
    {
        if ( activeCellInfo->isActive( ReservoirCellIndex( cellIndex ) ) )
        {
            for ( const auto& corner : mainGrid->cellCornerVertices( cellIndex ) )
            {
                bb.add( corner );
            }
        }
    }
    const cvf::Vec3d minP = bb.min();
    const cvf::Vec3d maxP = bb.max();
    const cvf::Vec3d size = maxP - minP;

    // Well paths sampled every 5 m, as imported well paths typically are
    auto createWellPath = []( const cvf::Vec3d& from, const cvf::Vec3d& to )
    {
        const double            length       = ( to - from ).length();
        const size_t            segmentCount = std::max( size_t( 1 ), static_cast<size_t>( length / 5.0 ) );
        std::vector<cvf::Vec3d> points;
        std::vector<double>     mds;
        for ( size_t i = 0; i <= segmentCount; i++ )
        {
            const double t = static_cast<double>( i ) / static_cast<double>( segmentCount );
            points.push_back( from + t * ( to - from ) );
            mds.push_back( 1000.0 + t * length );
        }
        cvf::ref<RigWellPath> wellPath = new RigWellPath;
        wellPath->setWellPathPoints( points, mds );
        return wellPath;
    };

    std::vector<cvf::ref<RigWellPath>> wellPaths;
    const int                          wellCountPerType = 8;
    for ( int i = 0; i < wellCountPerType; i++ )
    {
        const double f = ( i + 0.5 ) / wellCountPerType;

        // Horizontal along x and y
        const double z = minP.z() + ( 0.2 + 0.6 * f ) * size.z();
        wellPaths.push_back( createWellPath( cvf::Vec3d( minP.x() - 10, minP.y() + f * size.y(), z ),
                                             cvf::Vec3d( maxP.x() + 10, minP.y() + f * size.y(), z ) ) );
        wellPaths.push_back( createWellPath( cvf::Vec3d( minP.x() + f * size.x(), minP.y() - 10, z ),
                                             cvf::Vec3d( minP.x() + f * size.x(), maxP.y() + 10, z ) ) );

        // Deviated through the full depth
        wellPaths.push_back( createWellPath( cvf::Vec3d( minP.x() + f * size.x(), minP.y(), maxP.z() + 10 ),
                                             cvf::Vec3d( maxP.x() - f * size.x(), maxP.y(), minP.z() - 10 ) ) );

        // Vertical
        wellPaths.push_back( createWellPath( cvf::Vec3d( minP.x() + f * size.x(), minP.y() + ( 1.0 - f ) * size.y(), maxP.z() + 10 ),
                                             cvf::Vec3d( minP.x() + f * size.x(), minP.y() + ( 1.0 - f ) * size.y(), minP.z() - 10 ) ) );
    }

    // The search tree candidates per segment include degenerate inactive cells that collapse towards a point
    size_t segmentCount   = 0;
    size_t candidateCount = 0;
    for ( const auto& wellPath : wellPaths )
    {
        const auto& points = wellPath->wellPathPoints();
        for ( size_t i = 0; i + 1 < points.size(); i++ )
        {
            cvf::BoundingBox segmentBB;
            segmentBB.add( points[i] );
            segmentBB.add( points[i + 1] );
            candidateCount += mainGrid->findIntersectingCells( segmentBB ).size();
            segmentCount++;
        }
    }

    const int runCount        = 3;
    double    bestMs          = std::numeric_limits<double>::infinity();
    size_t    cellCount       = 0;
    double    mdInCells       = 0.0;
    double    mdInActiveCells = 0.0;
    for ( int run = 0; run < runCount; run++ )
    {
        cellCount       = 0;
        mdInCells       = 0.0;
        mdInActiveCells = 0.0;

        auto start = std::chrono::steady_clock::now();
        for ( const auto& wellPath : wellPaths )
        {
            cvf::ref<RigEclipseWellLogExtractor> extractor = new RigEclipseWellLogExtractor( caseData.p(), wellPath.p(), "" );
            for ( const auto& info : extractor->cellIntersectionInfosAlongWellPath() )
            {
                cellCount++;
                mdInCells += info.endMD - info.startMD;
                if ( activeCellInfo->isActive( ReservoirCellIndex( info.globCellIndex ) ) ) mdInActiveCells += info.endMD - info.startMD;
            }
        }
        auto stop = std::chrono::steady_clock::now();
        bestMs    = std::min( bestMs, std::chrono::duration<double, std::milli>( stop - start ).count() );
    }

    double totalLength = 0.0;
    for ( const auto& wellPath : wellPaths )
    {
        totalLength += wellPath->measuredDepths().back() - wellPath->measuredDepths().front();
    }

    std::cout << "Well log extraction, " << wellPaths.size() << " wells, " << totalLength << " m, " << segmentCount << " segments, "
              << candidateCount << " candidate cells: " << bestMs << " ms, " << cellCount << " cell intervals, " << mdInCells
              << " m in cells, " << mdInActiveCells << " m in active cells" << std::endl;
}
