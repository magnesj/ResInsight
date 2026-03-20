/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2025 Equinor ASA
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

#include "RicWellPathExportMswTableData.h"

#include "RiaLogging.h"

#include "RicExportCompletionDataSettingsUi.h"
#include "RicExportFractureCompletionsImpl.h"
#include "RicMswCompletions.h"
#include "RicMswExportInfo.h"
#include "RicMswTableDataTools.h"
#include "RicMswValveAccumulators.h"

#include "CompletionsMsw/RigMswSegment.h"
#include "CompletionsMsw/RigMswTableData.h"
#include "RigActiveCellInfo.h"
#include "RigEclipseCaseData.h"
#include "RigGridBase.h"
#include "RigMainGrid.h"
#include "Well/RigWellLogExtractor.h"
#include "Well/RigWellPath.h"
#include "Well/RigWellPathIntersectionTools.h"

#include "RimEclipseCase.h"
#include "RimFishbones.h"
#include "RimFishbonesCollection.h"
#include "RimFractureTemplate.h"
#include "RimMswCompletionParameters.h"
#include "RimPerforationCollection.h"
#include "RimPerforationInterval.h"
#include "RimProject.h"
#include "RimValveCollection.h"
#include "RimWellPath.h"
#include "RimWellPathCompletions.h"
#include "RimWellPathFracture.h"
#include "RimWellPathFractureCollection.h"
#include "RimWellPathTieIn.h"
#include "RimWellPathValve.h"

#include <algorithm>

namespace internal
{
constexpr double VALVE_SEGMENT_LENGTH = 0.1;

//--------------------------------------------------------------------------------------------------
/// Convert a tree segment's cell intersections to flat RigMswCellIntersection objects.
//--------------------------------------------------------------------------------------------------
static std::vector<RigMswCellIntersection> collectCellIntersections( const RicMswSegment* seg )
{
    std::vector<RigMswCellIntersection> result;
    for ( const auto& inter : seg->intersections() )
    {
        RigMswCellIntersection ci;
        auto                   ijk = inter->gridLocalCellIJK().toOneBased();
        ci.i                       = ijk.i();
        ci.j                       = ijk.j();
        ci.k                       = ijk.k();
        ci.distanceStart           = seg->startMD();
        ci.distanceEnd             = seg->endMD();
        ci.gridName                = inter->gridName().toStdString();
        result.push_back( ci );
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/// Add sub-segments for a completion (non-valve, e.g. fishbones lateral, fracture) to the flat list.
//--------------------------------------------------------------------------------------------------
static void addCompletionSubSegments( std::vector<RigMswSegment>&                   result,
                                      int&                                          segmentNumber,
                                      gsl::not_null<RicMswCompletion*>              completion,
                                      int                                           outletNumber,
                                      RicMswExportInfo&                             exportInfo,
                                      double                                        maxSegmentLength,
                                      const std::vector<std::pair<double, double>>& customSegmentIntervals )
{
    bool isDescriptionAdded = false;
    int  localOutletNumber  = outletNumber;

    for ( auto* treeSegment : completion->segments() )
    {
        treeSegment->setSegmentNumber( segmentNumber );

        double startMD  = treeSegment->startMD();
        double endMD    = treeSegment->endMD();
        double startTVD = treeSegment->startTVD();
        double endTVD   = treeSegment->endTVD();

        auto splitSegments = RicMswTableDataTools::createSubSegmentMDPairs( startMD, endMD, maxSegmentLength, customSegmentIntervals );
        for ( const auto& [subStartMD, subEndMD] : splitSegments )
        {
            int subSegNum = segmentNumber++;

            double subStartTVD = RicMswTableDataTools::tvdFromMeasuredDepth( completion->wellPath(), subStartMD );
            double subEndTVD   = RicMswTableDataTools::tvdFromMeasuredDepth( completion->wellPath(), subEndMD );

            if ( completion->completionType() == RigCompletionData::CompletionType::FISHBONES )
            {
                // Use linear interpolation based on start/end TVD for fishbones
                auto normalizedStart = ( subStartMD - startMD ) / ( endMD - startMD );
                subStartTVD          = startTVD * ( 1.0 - normalizedStart ) + endTVD * normalizedStart;
                auto normalizedEnd   = ( subEndMD - startMD ) / ( endMD - startMD );
                subEndTVD            = startTVD * ( 1.0 - normalizedEnd ) + endTVD * normalizedEnd;
            }

            double depth  = 0;
            double length = 0;
            if ( exportInfo.lengthAndDepthText() == "INC" )
            {
                depth  = subEndTVD - subStartTVD;
                length = subEndMD - subStartMD;
            }
            else
            {
                depth  = subEndTVD;
                length = subEndMD;
            }

            double diameter = treeSegment->equivalentDiameter();
            if ( treeSegment->effectiveDiameter() > 0.0 ) diameter = treeSegment->effectiveDiameter();

            RigMswSegment seg;
            seg.segmentNumber       = subSegNum;
            seg.branchNumber        = completion->branchNumber();
            seg.outletSegmentNumber = localOutletNumber;
            seg.length              = length;
            seg.depth               = depth;
            seg.diameter            = diameter;
            seg.roughness           = treeSegment->openHoleRoughnessFactor();
            seg.sourceWellName      = completion->wellPath()->name().toStdString();
            if ( !isDescriptionAdded )
            {
                seg.description    = completion->label().toStdString();
                isDescriptionAdded = true;
            }
            seg.intersections = collectCellIntersections( treeSegment );

            result.push_back( std::move( seg ) );
            localOutletNumber = subSegNum;
        }

        // Recurse into sub-completions
        for ( auto* subComp : treeSegment->completions() )
        {
            auto* subCompletion = dynamic_cast<RicMswCompletion*>( subComp );
            if ( subCompletion )
                addCompletionSubSegments( result,
                                          segmentNumber,
                                          subCompletion,
                                          treeSegment->segmentNumber(),
                                          exportInfo,
                                          maxSegmentLength,
                                          customSegmentIntervals );
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// Add WELSEGS segments for a valve completion to the flat list.
//--------------------------------------------------------------------------------------------------
static void addValveSubSegments( std::vector<RigMswSegment>&                   result,
                                 int&                                          segmentNumber,
                                 RicMswValve*                                  valve,
                                 int                                           outletNumber,
                                 RicMswExportInfo&                             exportInfo,
                                 double                                        maxSegmentLength,
                                 const std::vector<std::pair<double, double>>& customSegmentIntervals )
{
    if ( !valve ) return;
    if ( !valve->isValid() ) return;
    if ( !valve->wellPath() ) return;

    auto segments = valve->segments();

    double startMD = 0.0;
    double endMD   = 0.0;

    if ( valve->completionType() == RigCompletionData::CompletionType::PERFORATION_ICD ||
         valve->completionType() == RigCompletionData::CompletionType::PERFORATION_AICD ||
         valve->completionType() == RigCompletionData::CompletionType::PERFORATION_SICD )
    {
        CVF_ASSERT( segments.size() > 1 );
        auto subSegment = segments[0];
        subSegment->setSegmentNumber( segmentNumber );
        double midPointMD = subSegment->outputMD();
        startMD           = midPointMD;
        endMD             = startMD + 0.1;
    }
    else
    {
        auto subSegment = segments.front();
        subSegment->setSegmentNumber( segmentNumber );
        startMD = subSegment->startMD();
        endMD   = subSegment->endMD();
    }

    auto splitSegments = RicMswTableDataTools::createSubSegmentMDPairs( startMD, endMD, maxSegmentLength, customSegmentIntervals );

    int        localOutletNumber = outletNumber;
    const auto linerDiameter     = valve->wellPath()->mswCompletionParameters()->linerDiameter( exportInfo.unitSystem() );
    const auto roughnessFactor   = valve->wellPath()->mswCompletionParameters()->roughnessFactor( exportInfo.unitSystem() );

    bool isDescriptionAdded = false;
    for ( const auto& [subStartMD, subEndMD] : splitSegments )
    {
        const double subStartTVD = RicMswTableDataTools::tvdFromMeasuredDepth( valve->wellPath(), subStartMD );
        const double subEndTVD   = RicMswTableDataTools::tvdFromMeasuredDepth( valve->wellPath(), subEndMD );

        double depth  = 0;
        double length = 0;
        if ( exportInfo.lengthAndDepthText() == QString( "INC" ) )
        {
            depth  = subEndTVD - subStartTVD;
            length = subEndMD - subStartMD;
        }
        else
        {
            depth  = subEndTVD;
            length = subEndMD;
        }

        RigMswSegment seg;
        seg.segmentNumber       = segmentNumber;
        seg.branchNumber        = valve->branchNumber();
        seg.outletSegmentNumber = localOutletNumber;
        seg.length              = length;
        seg.depth               = depth;
        seg.diameter            = linerDiameter;
        seg.roughness           = roughnessFactor;
        seg.sourceWellName      = valve->wellPath()->name().toStdString();
        if ( !isDescriptionAdded )
        {
            seg.description    = valve->label().toStdString();
            isDescriptionAdded = true;
        }

        result.push_back( std::move( seg ) );

        localOutletNumber = segmentNumber;
        segmentNumber++;
    }
}

//--------------------------------------------------------------------------------------------------
/// Recursively traverse the RicMswBranch tree and write RigMswSegment objects directly to result.
/// Mirrors RicMswTableDataTools::collectWelsegsDataRecursively but writes flat segments instead of rows.
//--------------------------------------------------------------------------------------------------
static void buildFlatSegmentsRecursive( std::vector<RigMswSegment>&                   result,
                                        RicMswExportInfo&                             exportInfo,
                                        gsl::not_null<RicMswBranch*>                  branch,
                                        gsl::not_null<int*>                           segmentNumber,
                                        double                                        prevOutMD,
                                        double                                        prevOutTVD,
                                        int                                           outletSegmentNumber,
                                        double                                        maxSegmentLength,
                                        const std::vector<std::pair<double, double>>& customSegmentIntervals,
                                        bool                                          exportCompletionsAfterMainBore,
                                        const std::optional<QDateTime>&               exportDate );

//--------------------------------------------------------------------------------------------------
/// Process completions on a segment and add them to the flat list.
//--------------------------------------------------------------------------------------------------
static void processCompletionsForSegment( std::vector<RigMswSegment>&                   result,
                                          RicMswExportInfo&                             exportInfo,
                                          RicMswSegment*                                treeSegment,
                                          RicMswValve**                                 outletValve,
                                          int&                                          segmentNumber,
                                          double                                        maxSegmentLength,
                                          const std::vector<std::pair<double, double>>& customSegmentIntervals )
{
    for ( auto* comp : treeSegment->completions() )
    {
        // Skip RicMswPerforation — not exported as WELSEGS rows
        if ( dynamic_cast<RicMswPerforation*>( comp ) ) continue;

        auto* fishboneIcd  = dynamic_cast<RicMswFishbonesICD*>( comp );
        auto* segmentValve = dynamic_cast<RicMswValve*>( comp );

        if ( !fishboneIcd && segmentValve != nullptr )
        {
            // Valve that is not a fishbones ICD
            addValveSubSegments( result, segmentNumber, segmentValve, treeSegment->segmentNumber(), exportInfo, maxSegmentLength, customSegmentIntervals );
            *outletValve = segmentValve;
        }
        else if ( dynamic_cast<RicMswTieInICV*>( comp ) )
        {
            // Special handling for Tie-in ICVs
            int   outletForCompletion = ( *outletValve && ( *outletValve )->segmentCount() > 0 )
                                            ? ( *outletValve )->segments().front()->segmentNumber()
                                            : treeSegment->segmentNumber();
            auto* completion          = dynamic_cast<RicMswCompletion*>( comp );
            if ( completion )
                addCompletionSubSegments( result, segmentNumber, completion, outletForCompletion, exportInfo, maxSegmentLength, customSegmentIntervals );
        }
        else
        {
            auto* completion = dynamic_cast<RicMswCompletion*>( comp );
            if ( completion )
                addCompletionSubSegments( result,
                                          segmentNumber,
                                          completion,
                                          treeSegment->segmentNumber(),
                                          exportInfo,
                                          maxSegmentLength,
                                          customSegmentIntervals );
        }
    }
}

static void buildFlatSegmentsRecursive( std::vector<RigMswSegment>&                   result,
                                        RicMswExportInfo&                             exportInfo,
                                        gsl::not_null<RicMswBranch*>                  branch,
                                        gsl::not_null<int*>                           segmentNumber,
                                        double                                        prevOutMD,
                                        double                                        prevOutTVD,
                                        int                                           outletSegmentNumber,
                                        double                                        maxSegmentLength,
                                        const std::vector<std::pair<double, double>>& customSegmentIntervals,
                                        bool                                          exportCompletionsAfterMainBore,
                                        const std::optional<QDateTime>&               exportDate )
{
    RicMswSegment* lastTreeSegment = nullptr;
    RicMswValve*   outletValve     = nullptr;

    auto branchSegments = branch->segments();
    auto it             = branchSegments.begin();

    // Handle tie-in ICV at branch start
    if ( auto* tieInValve = dynamic_cast<RicMswTieInICV*>( branch.get() ); tieInValve != nullptr )
    {
        // Delegate to old helper via a temporary RigMswTableData to reuse the logic for assigning segment numbers
        // Instead, replicate: assign segmentNumber, create valve WELSEGS segment
        auto valveSegments = tieInValve->segments();
        if ( !valveSegments.empty() )
        {
            auto* valveTreeSeg = valveSegments.front();
            valveTreeSeg->setSegmentNumber( *segmentNumber );

            double startMD   = valveTreeSeg->startMD();
            double endMD     = valveTreeSeg->endMD();
            auto   splitSegs = RicMswTableDataTools::createSubSegmentMDPairs( startMD, endMD, maxSegmentLength, customSegmentIntervals );

            const auto linerDiameter   = tieInValve->wellPath()->mswCompletionParameters()->linerDiameter( exportInfo.unitSystem() );
            const auto roughnessFactor = tieInValve->wellPath()->mswCompletionParameters()->roughnessFactor( exportInfo.unitSystem() );

            int  localOutletNumber  = outletSegmentNumber;
            bool descriptionWritten = false;
            for ( const auto& [subStartMD, subEndMD] : splitSegs )
            {
                const double subStartTVD = RicMswTableDataTools::tvdFromMeasuredDepth( tieInValve->wellPath(), subStartMD );
                const double subEndTVD   = RicMswTableDataTools::tvdFromMeasuredDepth( tieInValve->wellPath(), subEndMD );

                double depth  = 0;
                double length = 0;
                if ( exportInfo.lengthAndDepthText() == "INC" )
                {
                    depth  = subEndTVD - subStartTVD;
                    length = subEndMD - subStartMD;
                }
                else
                {
                    depth  = subEndTVD;
                    length = subEndMD;
                }

                RigMswSegment seg;
                seg.segmentNumber       = *segmentNumber;
                seg.branchNumber        = tieInValve->branchNumber();
                seg.outletSegmentNumber = localOutletNumber;
                seg.length              = length;
                seg.depth               = depth;
                seg.diameter            = linerDiameter;
                seg.roughness           = roughnessFactor;
                seg.sourceWellName      = tieInValve->wellPath()->name().toStdString();
                if ( !descriptionWritten )
                {
                    seg.description    = tieInValve->label().toStdString();
                    descriptionWritten = true;
                }
                result.push_back( std::move( seg ) );

                localOutletNumber = *segmentNumber;
                ( *segmentNumber )++;
            }

            outletSegmentNumber = valveTreeSeg->segmentNumber();
            ++it; // skip the segment below tie-in
        }
    }

    auto branchStartIt = it;
    for ( ; it != branchSegments.end(); ++it )
    {
        auto* treeSegment = *it;
        treeSegment->setSegmentNumber( *segmentNumber );

        bool   isFirstSegment = ( it == branchSegments.begin() );
        double curPrevOutMD   = prevOutMD;
        double curPrevOutTVD  = prevOutTVD;
        if ( lastTreeSegment )
        {
            curPrevOutMD  = lastTreeSegment->outputMD();
            curPrevOutTVD = lastTreeSegment->outputTVD();
        }

        double startMD = treeSegment->startMD();
        double endMD   = treeSegment->endMD();

        auto splitSegs = RicMswTableDataTools::createSubSegmentMDPairs( startMD, endMD, maxSegmentLength, customSegmentIntervals );

        bool isDescriptionAdded    = false;
        int  previousSegmentNumber = outletSegmentNumber;

        for ( const auto& [subStartMD, subEndMD] : splitSegs )
        {
            double midPointMD  = 0.5 * ( subStartMD + subEndMD );
            double midPointTVD = RicMswTableDataTools::tvdFromMeasuredDepth( branch->wellPath(), midPointMD );

            if ( midPointMD < curPrevOutMD )
            {
                curPrevOutMD  = branch->startMD();
                curPrevOutTVD = branch->startTVD();
            }

            double depth  = 0;
            double length = 0;
            if ( exportInfo.lengthAndDepthText() == "INC" )
            {
                depth  = midPointTVD - curPrevOutTVD;
                length = midPointMD - curPrevOutMD;
            }
            else
            {
                depth  = midPointTVD;
                length = midPointMD;
            }

            double linerDiameter   = 0.0;
            double roughnessFactor = 0.0;
            if ( exportDate.has_value() )
            {
                linerDiameter =
                    branch->wellPath()->mswCompletionParameters()->getDiameterAtMD( midPointMD, exportInfo.unitSystem(), *exportDate );
                roughnessFactor =
                    branch->wellPath()->mswCompletionParameters()->getRoughnessAtMD( midPointMD, exportInfo.unitSystem(), *exportDate );
            }
            else
            {
                linerDiameter   = branch->wellPath()->mswCompletionParameters()->getDiameterAtMD( midPointMD, exportInfo.unitSystem() );
                roughnessFactor = branch->wellPath()->mswCompletionParameters()->getRoughnessAtMD( midPointMD, exportInfo.unitSystem() );
            }

            RigMswSegment seg;
            seg.segmentNumber       = *segmentNumber;
            seg.branchNumber        = branch->branchNumber();
            seg.outletSegmentNumber = previousSegmentNumber;
            seg.length              = length;
            seg.depth               = depth;
            seg.diameter            = linerDiameter;
            seg.roughness           = roughnessFactor;
            seg.sourceWellName      = branch->wellPath()->name().toStdString();

            if ( !isDescriptionAdded && isFirstSegment )
            {
                seg.description    = QString( "Segments on branch %1" ).arg( branch->label() ).toStdString();
                isDescriptionAdded = true;
            }

            seg.intersections = collectCellIntersections( treeSegment );
            result.push_back( std::move( seg ) );

            previousSegmentNumber = *segmentNumber;

            if ( splitSegs.size() > 1 )
            {
                ( *segmentNumber )++;
                treeSegment->setSegmentNumber( *segmentNumber );
            }

            treeSegment->setOutputMD( midPointMD );
            treeSegment->setOutputTVD( midPointTVD );
            treeSegment->setSegmentNumber( *segmentNumber );

            curPrevOutMD  = midPointMD;
            curPrevOutTVD = midPointTVD;
        }

        if ( splitSegs.size() <= 1 ) ( *segmentNumber )++;

        lastTreeSegment = treeSegment;

        if ( !exportCompletionsAfterMainBore )
        {
            processCompletionsForSegment( result, exportInfo, treeSegment, &outletValve, *segmentNumber, maxSegmentLength, customSegmentIntervals );
        }
    }

    if ( exportCompletionsAfterMainBore )
    {
        for ( it = branchStartIt; it != branchSegments.end(); ++it )
        {
            processCompletionsForSegment( result, exportInfo, *it, &outletValve, *segmentNumber, maxSegmentLength, customSegmentIntervals );
        }
    }

    for ( auto* childBranch : branch->branches() )
    {
        RicMswSegment* outletForChild = lastTreeSegment;

        RicMswSegment* tieInSegment = branch->findClosestSegmentWithLowerMD( childBranch->startMD() );
        if ( tieInSegment ) outletForChild = tieInSegment;

        int    childOutletSegNum = outletForChild ? outletForChild->segmentNumber() : outletSegmentNumber;
        double childPrevMD       = outletForChild ? outletForChild->outputMD() : prevOutMD;
        double childPrevTVD      = outletForChild ? outletForChild->outputTVD() : prevOutTVD;

        buildFlatSegmentsRecursive( result,
                                    exportInfo,
                                    childBranch,
                                    segmentNumber,
                                    childPrevMD,
                                    childPrevTVD,
                                    childOutletSegNum,
                                    maxSegmentLength,
                                    customSegmentIntervals,
                                    exportCompletionsAfterMainBore,
                                    exportDate );
    }
}

//--------------------------------------------------------------------------------------------------
/// Collect standalone WSEGVALV rows (from the well's valve collection, not from completions)
/// into a map keyed by segment number.
//--------------------------------------------------------------------------------------------------
static void collectStandaloneWsegvalvBySegmentRecursive( std::map<int, WsegvalvRow>&  result,
                                                         gsl::not_null<RicMswBranch*> branch,
                                                         const std::string&           wellName )
{
    auto* valveColl = branch->wellPath()->valveCollection();
    for ( auto* valve : valveColl->activeValves() )
    {
        auto* seg    = branch->findClosestSegmentWithLowerMD( valve->startMD() );
        int   segNum = seg ? seg->segmentNumber() : 1;
        if ( segNum > 1 && seg && valve->startMD() < seg->startMD() ) segNum--;

        WsegvalvRow row;
        row.well                  = wellName;
        row.segmentNumber         = segNum;
        row.cv                    = valve->flowCoefficient();
        row.area                  = valve->area( branch->wellPath()->unitSystem() );
        row.status                = "OPEN";
        row.description           = valve->name().toStdString();
        result[row.segmentNumber] = row;
    }

    for ( auto* subBranch : branch->branches() )
        collectStandaloneWsegvalvBySegmentRecursive( result, subBranch, wellName );
}

//--------------------------------------------------------------------------------------------------
/// Collect WSEGVALV rows from completion-based valves (ICD/ICV) into a map keyed by segment number.
//--------------------------------------------------------------------------------------------------
static void collectWsegvalvBySegmentRecursive( std::map<int, WsegvalvRow>&     result,
                                               gsl::not_null<RicMswBranch*>    branch,
                                               const std::string&              wellName,
                                               const std::optional<QDateTime>& exportDate )
{
    // Tie-in ICV at branch level
    if ( auto* tieInValve = dynamic_cast<RicMswTieInICV*>( branch.get() ) )
    {
        bool isActiveOnDate = !exportDate.has_value() ||
                              ( tieInValve->wellPathValve() && tieInValve->wellPathValve()->isActiveOnDate( *exportDate ) );
        if ( isActiveOnDate && !tieInValve->segments().empty() )
        {
            auto*       firstSeg = tieInValve->segments().front();
            WsegvalvRow row;
            row.well                  = wellName;
            row.segmentNumber         = firstSeg->segmentNumber();
            row.cv                    = tieInValve->flowCoefficient();
            row.area                  = tieInValve->area();
            row.status                = tieInValve->isOpen() ? "OPEN" : "SHUT";
            row.description           = tieInValve->label().toStdString();
            result[row.segmentNumber] = row;
        }
    }

    // Completion-based WSEGVALV entries (ICD / ICV completions on branch segments)
    for ( auto* segment : branch->segments() )
    {
        for ( auto* comp : segment->completions() )
        {
            if ( !RigCompletionData::isWsegValveTypes( comp->completionType() ) ) continue;

            auto* wsegValve = static_cast<RicMswWsegValve*>( comp );
            if ( exportDate.has_value() && wsegValve->wellPathValve() && !wsegValve->wellPathValve()->isActiveOnDate( *exportDate ) )
                continue;

            int segNum = -1;
            for ( auto* valveSeg : wsegValve->segments() )
            {
                if ( valveSeg->segmentNumber() > -1 ) segNum = valveSeg->segmentNumber();
                if ( valveSeg->intersections().empty() ) continue;

                WsegvalvRow row;
                row.well                  = wellName;
                row.segmentNumber         = segNum;
                row.cv                    = wsegValve->flowCoefficient();
                row.area                  = wsegValve->area();
                row.status                = wsegValve->isOpen() ? "OPEN" : "SHUT";
                row.description           = wsegValve->label().toStdString();
                result[row.segmentNumber] = row;
            }
        }
    }

    for ( auto* childBranch : branch->branches() )
        collectWsegvalvBySegmentRecursive( result, childBranch, wellName, exportDate );
}

//--------------------------------------------------------------------------------------------------
/// Convert a WellPathCellIntersectionInfo global-cell index to a RigMswCellIntersection (1-based i,j,k).
/// Returns std::nullopt for gap-segments (globCellIndex >= totalCellCount).
//--------------------------------------------------------------------------------------------------
static std::optional<RigMswCellIntersection>
    toMswCellIntersection( const WellPathCellIntersectionInfo& cellInfo, const RigMainGrid* mainGrid, double distanceStart, double distanceEnd )
{
    if ( cellInfo.globCellIndex >= mainGrid->totalCellCount() ) return std::nullopt;

    size_t             localIdx  = 0;
    const RigGridBase* localGrid = mainGrid->gridAndGridLocalIdxFromGlobalCellIdx( cellInfo.globCellIndex, &localIdx );

    size_t i, j, k;
    localGrid->ijkFromCellIndex( localIdx, &i, &j, &k );
    if ( mainGrid->isDualPorosity() ) k += mainGrid->cellCountK();

    RigMswCellIntersection ci;
    ci.i             = i + 1; // 1-based
    ci.j             = j + 1;
    ci.k             = k + 1;
    ci.distanceStart = distanceStart;
    ci.distanceEnd   = distanceEnd;
    if ( localGrid != mainGrid ) ci.gridName = localGrid->gridName();
    return ci;
}

// Maps the last main-bore sub-segment number for each filtered cell to the cell's MD range.
// Used by valve builders to find the outlet segment for a valve at a given MD.
struct CellSegmentEntry
{
    double cellStartMD;
    double cellEndMD;
    int    lastSubSegmentNumber;
};

//--------------------------------------------------------------------------------------------------
/// Build main-bore WELSEGS segments directly from well-path geometry — no RicMswItem types used.
/// For each grid-cell intersection that overlaps an active perforation interval that has NO active
/// valve, a COMPSEGS entry is embedded.  Cells whose perforations are covered by a valve are
/// skipped here; the valve builder will attach those COMPSEGS to the valve segment instead.
/// Also populates cellSegMap (one entry per non-trivial cell) for valve outlet-segment lookups.
//--------------------------------------------------------------------------------------------------
static std::vector<RigMswSegment> buildMainBoreSegmentsFromGeometry( const RimWellPath*                               wellPath,
                                                                     const std::vector<WellPathCellIntersectionInfo>& filteredIntersections,
                                                                     const RigMainGrid*                               mainGrid,
                                                                     const std::vector<const RimPerforationInterval*>& perforationIntervals,
                                                                     const std::set<const RimPerforationInterval*>&    valvedIntervals,
                                                                     const std::string&                                infoType,
                                                                     double                                            heelMD,
                                                                     double                                            heelTVD,
                                                                     int                                               branchNumber,
                                                                     int&                                              segmentNumber,
                                                                     int                                               outletSegmentNumber,
                                                                     double                                            maxSegmentLength,
                                                                     const std::vector<std::pair<double, double>>& customSegmentIntervals,
                                                                     const std::optional<QDateTime>&               exportDate,
                                                                     RiaDefines::EclipseUnitSystem                 unitSystem,
                                                                     std::vector<CellSegmentEntry>*                cellSegMap )
{
    std::vector<RigMswSegment> result;

    const double segmentLengthThreshold = 1.0e-3;

    double prevOutMD  = heelMD;
    double prevOutTVD = heelTVD;
    int    prevSegNum = outletSegmentNumber;
    bool   firstSeg   = true;

    for ( const auto& cellInfo : filteredIntersections )
    {
        const double cellLength = std::fabs( cellInfo.endMD - cellInfo.startMD );
        if ( cellLength <= segmentLengthThreshold )
        {
            RiaLogging::info( QString( "Skipping segment, threshold = %1, length = %2" ).arg( segmentLengthThreshold ).arg( cellLength ) );
            continue;
        }

        // Collect COMPSEGS only for bare perforations (no active valve) on the main bore.
        // Valved perforations get their COMPSEGS on the valve segment.
        std::vector<RigMswCellIntersection> cellCompsegs;
        for ( const auto* perf : perforationIntervals )
        {
            if ( valvedIntervals.count( perf ) ) continue; // valve handles this interval
            if ( exportDate.has_value() && !perf->isActiveOnDate( exportDate.value() ) ) continue;

            const double overlapStart = std::max( perf->startMD(), cellInfo.startMD );
            const double overlapEnd   = std::min( perf->endMD(), cellInfo.endMD );
            if ( overlapEnd > overlapStart )
            {
                if ( auto ci = toMswCellIntersection( cellInfo, mainGrid, overlapStart, overlapEnd ) ) cellCompsegs.push_back( *ci );
            }
        }

        const auto subSegs =
            RicMswTableDataTools::createSubSegmentMDPairs( cellInfo.startMD, cellInfo.endMD, maxSegmentLength, customSegmentIntervals );

        bool firstSubSeg   = true;
        int  lastSubSegNum = prevSegNum;
        for ( const auto& [subStartMD, subEndMD] : subSegs )
        {
            const double midPointMD  = 0.5 * ( subStartMD + subEndMD );
            const double midPointTVD = RicMswTableDataTools::tvdFromMeasuredDepth( wellPath, midPointMD );

            double curPrevOutMD  = prevOutMD;
            double curPrevOutTVD = prevOutTVD;
            if ( midPointMD < curPrevOutMD )
            {
                curPrevOutMD  = heelMD;
                curPrevOutTVD = heelTVD;
            }

            double length = 0.0;
            double depth  = 0.0;
            if ( infoType == "INC" )
            {
                length = midPointMD - curPrevOutMD;
                depth  = midPointTVD - curPrevOutTVD;
            }
            else
            {
                length = midPointMD;
                depth  = midPointTVD;
            }

            double diameter  = 0.0;
            double roughness = 0.0;
            if ( exportDate.has_value() )
            {
                diameter  = wellPath->mswCompletionParameters()->getDiameterAtMD( midPointMD, unitSystem, *exportDate );
                roughness = wellPath->mswCompletionParameters()->getRoughnessAtMD( midPointMD, unitSystem, *exportDate );
            }
            else
            {
                diameter  = wellPath->mswCompletionParameters()->getDiameterAtMD( midPointMD, unitSystem );
                roughness = wellPath->mswCompletionParameters()->getRoughnessAtMD( midPointMD, unitSystem );
            }

            RigMswSegment seg;
            seg.segmentNumber       = segmentNumber;
            seg.branchNumber        = branchNumber;
            seg.outletSegmentNumber = prevSegNum;
            seg.length              = length;
            seg.depth               = depth;
            seg.diameter            = diameter;
            seg.roughness           = roughness;
            seg.sourceWellName      = wellPath->name().toStdString();
            if ( firstSeg && firstSubSeg )
            {
                seg.description = "Segments on main bore";
                firstSeg        = false;
            }
            if ( firstSubSeg ) seg.intersections = cellCompsegs;

            lastSubSegNum = segmentNumber;
            prevSegNum    = segmentNumber++;
            prevOutMD     = midPointMD;
            prevOutTVD    = midPointTVD;
            firstSubSeg   = false;
            result.push_back( std::move( seg ) );
        }

        if ( cellSegMap ) cellSegMap->push_back( { cellInfo.startMD, cellInfo.endMD, lastSubSegNum } );
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/// Find the main-bore outlet segment number for a given measured depth.
/// Returns 1 (heel) if not found.
//--------------------------------------------------------------------------------------------------
static int findOutletSegmentForMD( const std::vector<CellSegmentEntry>& cellSegMap, double md )
{
    for ( const auto& entry : cellSegMap )
    {
        if ( md >= entry.cellStartMD && md < entry.cellEndMD ) return entry.lastSubSegmentNumber;
    }
    return cellSegMap.empty() ? 1 : cellSegMap.back().lastSubSegmentNumber;
}

//--------------------------------------------------------------------------------------------------
/// Build WELSEGS + COMPSEGS segments for ICD and ICV valve completions directly from
/// RimPerforationInterval / RimWellPathValve — no RicMswItem types used.
/// Each valve location produces one WELSEGS segment at valveMD..valveMD+0.1.
/// COMPSEGS entries are attached covering cells that overlap the valve's segment range.
/// WSEGVALV data (Cv, area) is populated for ICD and ICV valves.
/// TODO: WSEGAICD / WSEGSICD accumulation for AICD and SICD valves.
//--------------------------------------------------------------------------------------------------
static std::vector<RigMswSegment> buildValveSegmentsFromGeometry( const RimWellPath*                                wellPath,
                                                                  const std::vector<WellPathCellIntersectionInfo>&  filteredIntersections,
                                                                  const RigMainGrid*                                mainGrid,
                                                                  const std::vector<const RimPerforationInterval*>& perforationIntervals,
                                                                  const std::vector<CellSegmentEntry>&              cellSegMap,
                                                                  const std::string&                                infoType,
                                                                  const std::string&                                wellNameForExport,
                                                                  int&                                              segmentNumber,
                                                                  int&                                              branchNumber,
                                                                  double                                            maxSegmentLength,
                                                                  const std::vector<std::pair<double, double>>&     customSegmentIntervals,
                                                                  const std::optional<QDateTime>&                   exportDate,
                                                                  RiaDefines::EclipseUnitSystem                     unitSystem )
{
    std::vector<RigMswSegment> result;

    const auto linerDiameter   = wellPath->mswCompletionParameters()->linerDiameter( unitSystem );
    const auto roughnessFactor = wellPath->mswCompletionParameters()->roughnessFactor( unitSystem );

    for ( const auto* perf : perforationIntervals )
    {
        if ( exportDate.has_value() && !perf->isActiveOnDate( exportDate.value() ) ) continue;

        auto valves = perf->descendantsIncludingThisOfType<RimWellPathValve>();
        for ( const auto* valve : valves )
        {
            if ( !valve->isChecked() ) continue;
            if ( exportDate.has_value() && !valve->isActiveOnDate( exportDate.value() ) ) continue;

            const auto valveType = valve->componentType();
            const bool isIcdOrIcv =
                ( valveType == RiaDefines::WellPathComponentType::ICD || valveType == RiaDefines::WellPathComponentType::ICV );
            const bool isAicdOrSicd =
                ( valveType == RiaDefines::WellPathComponentType::AICD || valveType == RiaDefines::WellPathComponentType::SICD );
            if ( !isIcdOrIcv && !isAicdOrSicd ) continue;

            const int valveBranch = ++branchNumber;

            const auto& valveLocations = valve->valveLocations();
            const auto& valveSegs      = valve->valveSegments(); // one (startMD, endMD) per location

            for ( size_t vi = 0; vi < valveLocations.size(); ++vi )
            {
                const double valveMD       = valveLocations[vi];
                const double valveEndMD    = valveMD + VALVE_SEGMENT_LENGTH;
                const double valveSegStart = valveSegs[vi].first;
                const double valveSegEnd   = valveSegs[vi].second;

                const int outletSeg = findOutletSegmentForMD( cellSegMap, valveMD );

                const double valveStartTVD = RicMswTableDataTools::tvdFromMeasuredDepth( wellPath, valveMD );
                const double valveEndTVD   = RicMswTableDataTools::tvdFromMeasuredDepth( wellPath, valveEndMD );

                double length = 0.0;
                double depth  = 0.0;
                if ( infoType == "INC" )
                {
                    length = valveEndMD - valveMD;
                    depth  = valveEndTVD - valveStartTVD;
                }
                else
                {
                    length = valveEndMD;
                    depth  = valveEndTVD;
                }

                RigMswSegment valveSeg;
                valveSeg.segmentNumber       = segmentNumber;
                valveSeg.branchNumber        = valveBranch;
                valveSeg.outletSegmentNumber = outletSeg;
                valveSeg.length              = length;
                valveSeg.depth               = depth;
                valveSeg.diameter            = linerDiameter;
                valveSeg.roughness           = roughnessFactor;
                valveSeg.sourceWellName      = wellPath->name().toStdString();
                valveSeg.description         = QString( "%1 #%2" ).arg( valve->name() ).arg( vi + 1 ).toStdString();

                // COMPSEGS: cells that overlap this valve's coverage range (valveSegStart..valveSegEnd)
                for ( const auto& cellInfo : filteredIntersections )
                {
                    const double overlapStart = std::max( valveSegStart, cellInfo.startMD );
                    const double overlapEnd   = std::min( valveSegEnd, cellInfo.endMD );
                    if ( overlapEnd > overlapStart )
                    {
                        if ( auto ci = toMswCellIntersection( cellInfo, mainGrid, overlapStart, overlapEnd ) )
                            valveSeg.intersections.push_back( *ci );
                    }
                }

                // WSEGVALV for ICD/ICV
                if ( isIcdOrIcv )
                {
                    WsegvalvRow wv;
                    wv.well               = wellNameForExport;
                    wv.segmentNumber      = segmentNumber;
                    wv.cv                 = valve->flowCoefficient();
                    wv.area               = valve->area( unitSystem );
                    wv.status             = valve->isOpen() ? "OPEN" : "SHUT";
                    wv.description        = valve->name().toStdString();
                    valveSeg.wsegvalvData = wv;
                }
                // TODO: WSEGAICD for AICD valves (requires flow-scaling-factor accumulation)
                // TODO: WSEGSICD for SICD valves (requires flow-scaling-factor accumulation)

                segmentNumber++;
                result.push_back( std::move( valveSeg ) );
            }
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/// Build WELSEGS + COMPSEGS segments for fracture completions directly from
/// RimWellPath / RimWellPathFracture — no RicMswItem types used.
/// Each fracture produces one WELSEGS segment branching off the nearest main-bore segment.
/// COMPSEGS entries are attached from the fracture's generateCompdatValues() result.
//--------------------------------------------------------------------------------------------------
static std::vector<RigMswSegment> buildFractureSegmentsFromGeometry( RimEclipseCase*                      eclipseCase,
                                                                     const RimWellPath*                   wellPath,
                                                                     const RigMainGrid*                   mainGrid,
                                                                     const std::vector<CellSegmentEntry>& cellSegMap,
                                                                     const std::string&                   infoType,
                                                                     int&                                 segmentNumber,
                                                                     int&                                 branchNumber )
{
    std::vector<RigMswSegment> result;

    const auto linerDiameter   = wellPath->mswCompletionParameters()->linerDiameter( eclipseCase->eclipseCaseData()->unitsType() );
    const auto roughnessFactor = wellPath->mswCompletionParameters()->roughnessFactor( eclipseCase->eclipseCaseData()->unitsType() );

    const QString wellNameForExport = wellPath->completionSettings()->wellNameForExport();

    for ( RimWellPathFracture* fracture : wellPath->fractureCollection()->activeFractures() )
    {
        fracture->ensureValidNonDarcyProperties();

        double position = fracture->fractureMD();
        double width    = fracture->fractureTemplate()->computeFractureWidth( fracture );

        if ( fracture->fractureTemplate()->orientationType() == RimFractureTemplate::ALONG_WELL_PATH )
        {
            double perforationLength = fracture->fractureTemplate()->perforationLength();
            position -= 0.5 * perforationLength;
            width = perforationLength;
        }

        const double endMD    = position + width;
        const double startTVD = RicMswTableDataTools::tvdFromMeasuredDepth( wellPath, position );
        const double endTVD   = RicMswTableDataTools::tvdFromMeasuredDepth( wellPath, endMD );

        double length = 0.0;
        double depth  = 0.0;
        if ( infoType == "INC" )
        {
            length = width;
            depth  = endTVD - startTVD;
        }
        else
        {
            length = endMD;
            depth  = endTVD;
        }

        const int outletSeg  = findOutletSegmentForMD( cellSegMap, position );
        const int fracBranch = ++branchNumber;

        // Get COMPSEGS cell intersections from fracture completion data
        std::vector<RigCompletionData> completionData = RicExportFractureCompletionsImpl::generateCompdatValues( eclipseCase,
                                                                                                                 wellNameForExport,
                                                                                                                 wellPath->wellPathGeometry(),
                                                                                                                 { fracture },
                                                                                                                 nullptr,
                                                                                                                 nullptr );

        std::vector<RigMswCellIntersection> compsegs;
        for ( const auto& compData : completionData )
        {
            const auto& cell = compData.completionDataGridCell();

            // 0-based -> 1-based
            size_t i = cell.localCellIndexI() + 1;
            size_t j = cell.localCellIndexJ() + 1;
            size_t k = cell.localCellIndexK() + 1;

            // Shift K for dual porosity models
            if ( mainGrid->isDualPorosity() ) k += mainGrid->cellCountK();

            RigMswCellIntersection ci;
            ci.i             = i;
            ci.j             = j;
            ci.k             = k;
            ci.distanceStart = position;
            ci.distanceEnd   = endMD;
            ci.gridName      = cell.lgrName().toStdString();
            compsegs.push_back( ci );
        }

        RigMswSegment seg;
        seg.segmentNumber       = segmentNumber++;
        seg.branchNumber        = fracBranch;
        seg.outletSegmentNumber = outletSeg;
        seg.length              = length;
        seg.depth               = depth;
        seg.diameter            = linerDiameter;
        seg.roughness           = roughnessFactor;
        seg.sourceWellName      = wellPath->name().toStdString();
        seg.description         = fracture->name().toStdString();
        seg.intersections       = std::move( compsegs );

        result.push_back( std::move( seg ) );
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/// Build WELSEGS + COMPSEGS + WSEGVALV segments for fishbones completions directly from
/// RimFishbones — no RicMswItem types used.
/// For each sub location:
///   - One ICD WELSEGS segment branching off the nearest main-bore segment.
///     COMPSEGS = cells closest to the sub. WSEGVALV = ICD Cv/area.
///   - For each lateral: one WELSEGS sub-segment per cell intersection along the lateral,
///     all on a new branch, with COMPSEGS per cell.
//--------------------------------------------------------------------------------------------------
static std::vector<RigMswSegment> buildFishbonesSegmentsFromGeometry( const RimEclipseCase* eclipseCase,
                                                                      const RimWellPath*    wellPath,
                                                                      const RigMainGrid*    mainGrid,
                                                                      const std::vector<WellPathCellIntersectionInfo>& filteredIntersections,
                                                                      const std::vector<CellSegmentEntry>& cellSegMap,
                                                                      const std::string&                   infoType,
                                                                      const std::string&                   wellNameForExport,
                                                                      int&                                 segmentNumber,
                                                                      int&                                 branchNumber,
                                                                      RiaDefines::EclipseUnitSystem        unitSystem )
{
    std::vector<RigMswSegment> result;

    const auto linerDiameter   = wellPath->mswCompletionParameters()->linerDiameter( unitSystem );
    const auto roughnessFactor = wellPath->mswCompletionParameters()->roughnessFactor( unitSystem );

    auto fishbonesSubs = wellPath->completions()->fishbonesCollection()->activeFishbonesSubs();
    for ( const RimFishbones* subs : fishbonesSubs )
    {
        // Group laterals by sub index
        std::map<size_t, std::vector<size_t>> subAndLateralIndices;
        for ( const auto& [subIndex, lateralIndex] : subs->installedLateralIndices() )
            subAndLateralIndices[subIndex].push_back( lateralIndex );

        // Identify which filtered intersections are closest to each sub, and which
        // intersection contains the sub's MD.  This mirrors appendFishbonesMswExportInfo.
        const double fishboneSectionStart = subs->startMD();
        const double fishboneSectionEnd   = subs->endMD();

        std::map<size_t, std::vector<size_t>> closestSubForCellIntersections;
        std::map<size_t, size_t>              cellIntersectionContainingSubIndex;

        for ( size_t ii = 0; ii < filteredIntersections.size(); ++ii )
        {
            const auto& ci = filteredIntersections[ii];
            if ( fishboneSectionEnd < ci.startMD || fishboneSectionStart > ci.endMD ) continue;

            const double midpoint      = 0.5 * ( ci.startMD + ci.endMD );
            size_t       closestSubIdx = 0;
            double       closestDist   = std::numeric_limits<double>::infinity();

            for ( const auto& [subIdx, lats] : subAndLateralIndices )
            {
                const double subMD = subs->measuredDepth( subIdx );
                if ( ci.startMD <= subMD && subMD <= ci.endMD ) cellIntersectionContainingSubIndex[subIdx] = ii;

                const double dist = std::abs( subMD - midpoint );
                if ( dist < closestDist )
                {
                    closestDist   = dist;
                    closestSubIdx = subIdx;
                }
            }
            closestSubForCellIntersections[closestSubIdx].push_back( ii );
        }

        // ICD parameters
        const double icdOrificeRadius = subs->icdOrificeDiameter( unitSystem ) / 2.0;
        const double icdArea          = icdOrificeRadius * icdOrificeRadius * cvf::PI_D * static_cast<double>( subs->icdCount() );
        const double icdCv            = subs->icdFlowCoefficient();

        for ( const auto& [subIndex, lateralIndices] : subAndLateralIndices )
        {
            const double subEndMd      = subs->measuredDepth( subIndex );
            const double startValveMd  = subEndMd - VALVE_SEGMENT_LENGTH;
            const double startValveTVD = RicMswTableDataTools::tvdFromMeasuredDepth( wellPath, startValveMd );
            const double endValveTVD   = RicMswTableDataTools::tvdFromMeasuredDepth( wellPath, subEndMd );

            const int outletSeg = findOutletSegmentForMD( cellSegMap, subEndMd );
            const int icdBranch = ++branchNumber;
            const int icdSegNum = segmentNumber++;

            // COMPSEGS for the ICD segment: cells closest to this sub
            std::set<size_t> icdCellIndices;
            if ( closestSubForCellIntersections.count( subIndex ) )
            {
                for ( auto idx : closestSubForCellIntersections[subIndex] )
                    icdCellIndices.insert( idx );
            }
            if ( cellIntersectionContainingSubIndex.count( subIndex ) )
                icdCellIndices.insert( cellIntersectionContainingSubIndex[subIndex] );

            std::vector<RigMswCellIntersection> icdCompsegs;
            for ( auto idx : icdCellIndices )
            {
                const auto& ci = filteredIntersections[idx];
                if ( auto mci = toMswCellIntersection( ci, mainGrid, ci.startMD, ci.endMD ) ) icdCompsegs.push_back( *mci );
            }

            double icdLength = 0.0;
            double icdDepth  = 0.0;
            if ( infoType == "INC" )
            {
                icdLength = subEndMd - startValveMd;
                icdDepth  = endValveTVD - startValveTVD;
            }
            else
            {
                icdLength = subEndMd;
                icdDepth  = endValveTVD;
            }

            RigMswSegment icdSeg;
            icdSeg.segmentNumber       = icdSegNum;
            icdSeg.branchNumber        = icdBranch;
            icdSeg.outletSegmentNumber = outletSeg;
            icdSeg.length              = icdLength;
            icdSeg.depth               = icdDepth;
            icdSeg.diameter            = linerDiameter;
            icdSeg.roughness           = roughnessFactor;
            icdSeg.sourceWellName      = wellPath->name().toStdString();
            icdSeg.description         = QString( "ICD sub %1" ).arg( subIndex + 1 ).toStdString();
            icdSeg.intersections       = std::move( icdCompsegs );

            WsegvalvRow wv;
            wv.well             = wellNameForExport;
            wv.segmentNumber    = icdSegNum;
            wv.cv               = icdCv;
            wv.area             = icdArea;
            wv.status           = "OPEN";
            wv.description      = QString( "ICD sub %1" ).arg( subIndex + 1 ).toStdString();
            icdSeg.wsegvalvData = wv;

            result.push_back( std::move( icdSeg ) );

            // Lateral sub-segments
            for ( size_t lateralIndex : lateralIndices )
            {
                auto lateralCoordMDPairs = subs->coordsAndMDForLateral( subIndex, lateralIndex );
                if ( lateralCoordMDPairs.empty() ) continue;

                std::vector<cvf::Vec3d> lateralCoords;
                std::vector<double>     lateralMDs;
                for ( auto& [coord, md] : lateralCoordMDPairs )
                {
                    lateralCoords.push_back( coord );
                    lateralMDs.push_back( md );
                }

                auto lateralIntersections = RigWellPathIntersectionTools::findCellIntersectionInfosAlongPath( eclipseCase->eclipseCaseData(),
                                                                                                              wellPath->name(),
                                                                                                              lateralCoords,
                                                                                                              lateralMDs );

                if ( lateralIntersections.empty() ) continue;

                const int latBranch    = ++branchNumber;
                double    prevMD       = lateralMDs.front();
                double    prevTVD      = -lateralCoords.front().z();
                int       latOutletSeg = icdSegNum;

                for ( const auto& cellIntInfo : lateralIntersections )
                {
                    if ( auto mci = toMswCellIntersection( cellIntInfo, mainGrid, cellIntInfo.startMD, cellIntInfo.endMD ) )
                    {
                        double len = 0.0;
                        double dep = 0.0;
                        if ( infoType == "INC" )
                        {
                            len = cellIntInfo.endMD - prevMD;
                            dep = cellIntInfo.endTVD() - prevTVD;
                        }
                        else
                        {
                            len = cellIntInfo.endMD;
                            dep = cellIntInfo.endTVD();
                        }

                        RigMswSegment latSeg;
                        latSeg.segmentNumber       = segmentNumber++;
                        latSeg.branchNumber        = latBranch;
                        latSeg.outletSegmentNumber = latOutletSeg;
                        latSeg.length              = len;
                        latSeg.depth               = dep;
                        latSeg.diameter            = subs->equivalentDiameter( unitSystem );
                        latSeg.roughness           = subs->openHoleRoughnessFactor( unitSystem );
                        latSeg.sourceWellName      = wellPath->name().toStdString();
                        latSeg.intersections       = { *mci };

                        latOutletSeg = latSeg.segmentNumber;
                        prevMD       = cellIntInfo.endMD;
                        prevTVD      = cellIntInfo.endTVD();
                        result.push_back( std::move( latSeg ) );
                    }
                }
            }
        }
    }

    return result;
}

}; // namespace internal

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::expected<RigMswTableData, std::string>
    RicWellPathExportMswTableData::extractSingleWellMswData( RimEclipseCase*                 eclipseCase,
                                                             RimWellPath*                    wellPath,
                                                             bool                            exportCompletionsAfterMainBoreSegments,
                                                             CompletionType                  completionType,
                                                             const std::optional<QDateTime>& exportDate )
{
    auto flatData = buildFlatMswSegments( eclipseCase, wellPath, exportCompletionsAfterMainBoreSegments, completionType, exportDate );
    if ( !flatData ) return std::unexpected( flatData.error() );

    auto            unitSystem = eclipseCase->eclipseCaseData()->unitsType();
    RigMswTableData tableData( wellPath->completionSettings()->wellNameForExport().toStdString(), unitSystem );
    RicMswTableDataTools::collectDataFromFlatList( tableData, *flatData );

    return tableData;
}

//--------------------------------------------------------------------------------------------------
/// Build the flat RigMswSegment list directly, without exposing the tree to the caller.
/// Internally builds the tree, assigns branch/segment numbers, runs all collection functions,
/// and returns the complete WELSEGS header + flat ordered segment list.
//--------------------------------------------------------------------------------------------------
std::expected<RigMswFlatExportData, std::string>
    RicWellPathExportMswTableData::buildFlatMswSegments( RimEclipseCase*                 eclipseCase,
                                                         RimWellPath*                    wellPath,
                                                         bool                            exportCompletionsAfterMainBoreSegments,
                                                         CompletionType                  completionType,
                                                         const std::optional<QDateTime>& exportDate )
{
    if ( !eclipseCase || !wellPath || eclipseCase->eclipseCaseData() == nullptr )
        return std::unexpected( "Invalid eclipse case or well path provided" );

    auto mswParameters = wellPath->mswCompletionParameters();
    if ( !mswParameters ) return std::unexpected( "Missing MSW completion parameters" );

    auto   cellIntersections = generateCellSegments( eclipseCase, wellPath );
    double initialMD         = computeIntitialMeasuredDepth( eclipseCase, wellPath, mswParameters, cellIntersections );

    RiaDefines::EclipseUnitSystem unitSystem = eclipseCase->eclipseCaseData()->unitsType();
    RicMswExportInfo exportInfo( wellPath, unitSystem, initialMD, mswParameters->lengthAndDepth().text(), mswParameters->pressureDrop().text() );

    const bool createSegmentsForPerforations = ( completionType & CompletionType::PERFORATIONS ) == CompletionType::PERFORATIONS;
    if ( !generateWellSegmentsForMswExportInfo( eclipseCase,
                                                wellPath,
                                                createSegmentsForPerforations,
                                                exportDate,
                                                initialMD,
                                                cellIntersections,
                                                &exportInfo,
                                                exportInfo.mainBoreBranch() ) )
        return std::unexpected( "Failed to generate perforations MSW export info" );

    if ( ( completionType & CompletionType::FISHBONES ) == CompletionType::FISHBONES )
        appendFishbonesMswExportInfo( eclipseCase, wellPath, initialMD, cellIntersections, &exportInfo, exportInfo.mainBoreBranch() );

    if ( ( completionType & CompletionType::FRACTURES ) == CompletionType::FRACTURES )
        appendFracturesMswExportInfo( eclipseCase, wellPath, initialMD, cellIntersections, &exportInfo, exportInfo.mainBoreBranch() );

    updateDataForMultipleItemsInSameGridCell( exportInfo.mainBoreBranch() );

    int branchNumber = 1;
    assignBranchNumbersToBranch( eclipseCase, &exportInfo, exportInfo.mainBoreBranch(), &branchNumber );

    const std::vector<std::pair<double, double>> customSegmentIntervals = mswParameters->getSegmentIntervals();

    // Switch between three paths:
    //   useLegacyPath=true  → old collectWelsegsData / buildFlatMswSegmentList (keep for reference)
    //   useGeometryPath=true → buildMswFromGeometry: builds directly from well-path geometry and
    //                          Rim completions, no RicMswBranch/RicMswItem tree created at all.
    //   default             → buildFlatSegmentsDirect: traverses the RicMswBranch tree inline,
    //                          writing RigMswSegment objects without the old collectWelsegsData path.
    constexpr bool useLegacyPath   = false;
    constexpr bool useGeometryPath = false;

    if ( useGeometryPath )
    {
        return buildMswFromGeometry( eclipseCase,
                                     wellPath,
                                     mswParameters->maxSegmentLength(),
                                     customSegmentIntervals,
                                     completionType,
                                     exportDate );
    }

    if ( !useLegacyPath )
    {
        return buildFlatSegmentsDirect( exportInfo,
                                        mswParameters->maxSegmentLength(),
                                        customSegmentIntervals,
                                        exportCompletionsAfterMainBoreSegments,
                                        exportDate );
    }

    // Run all collection functions into a temporary table to compute segment numbers, lengths, depths, etc.
    RigMswTableData tempTableData( wellPath->completionSettings()->wellNameForExport().toStdString(), unitSystem );

    RicMswTableDataTools::collectWelsegsData( tempTableData,
                                              exportInfo,
                                              mswParameters->maxSegmentLength(),
                                              customSegmentIntervals,
                                              exportCompletionsAfterMainBoreSegments,
                                              exportDate );
    RicMswTableDataTools::collectCompsegData( tempTableData, exportInfo, false, exportDate );
    RicMswTableDataTools::collectCompsegData( tempTableData, exportInfo, true, exportDate );
    RicMswTableDataTools::collectWsegvalvData( tempTableData, exportInfo, exportDate );
    RicMswTableDataTools::collectWsegAicdData( tempTableData, exportInfo, exportDate );
    RicMswTableDataTools::collectWsegSicdData( tempTableData, exportInfo, exportDate );

    buildFlatMswSegmentList( exportInfo, tempTableData );

    // Build result
    RigMswFlatExportData result;
    result.header   = tempTableData.welsegsHeader();
    result.segments = tempTableData.mswSegments();

    return result;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RicWellPathExportMswTableData::CompletionType
    RicWellPathExportMswTableData::convertFromExportSettings( const RicExportCompletionDataSettingsUi& settings )
{
    CompletionType result = CompletionType::NONE;

    if ( settings.includePerforations() )
    {
        result |= CompletionType::PERFORATIONS;
    }

    if ( settings.includeFishbones() )
    {
        result |= CompletionType::FISHBONES;
    }

    if ( settings.includeFractures() )
    {
        result |= CompletionType::FRACTURES;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellPathExportMswTableData::generateFishbonesMswExportInfoForWell( const RimEclipseCase* eclipseCase,
                                                                           const RimWellPath*    wellPath,
                                                                           RicMswExportInfo*     exportInfo,
                                                                           RicMswBranch*         branch )
{
    if ( !eclipseCase || !wellPath || !eclipseCase->eclipseCaseData() || !exportInfo || !branch )
    {
        return;
    }

    auto mswParameters = wellPath->mswCompletionParameters();
    if ( !mswParameters )
    {
        return;
    }

    auto   cellIntersections = generateCellSegments( eclipseCase, wellPath );
    double initialMD         = computeIntitialMeasuredDepth( eclipseCase, wellPath, mswParameters, cellIntersections );

    const bool createSegmentsForPerforations = true;
    if ( !generateWellSegmentsForMswExportInfo( eclipseCase,
                                                wellPath,
                                                createSegmentsForPerforations,
                                                std::nullopt,
                                                initialMD,
                                                cellIntersections,
                                                exportInfo,
                                                exportInfo->mainBoreBranch() ) )
    {
        return;
    }

    appendFishbonesMswExportInfo( eclipseCase, wellPath, initialMD, cellIntersections, exportInfo, exportInfo->mainBoreBranch() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellPathExportMswTableData::updateDataForMultipleItemsInSameGridCell( gsl::not_null<RicMswBranch*> branch )
{
    auto allSegments = branch->allSegmentsRecursively();

    {
        // Update effective diameter
        // https://github.com/OPM/ResInsight/issues/7686

        std::map<size_t, std::set<RicMswSegment*>> segmentsInCell;
        {
            for ( auto s : allSegments )
            {
                auto cellsIntersected = s->globalCellsIntersected();
                if ( !cellsIntersected.empty() )
                {
                    for ( auto index : cellsIntersected )
                    {
                        segmentsInCell[index].insert( s );
                    }
                }
            }
        }

        for ( auto [index, segmentsInSameCell] : segmentsInCell )
        {
            // Compute effective diameter based on square root of the sum of diameter squared
            // Deff = sqrt(d1^2 + d2^2 + ..)
            double effectiveDiameter = 0.0;

            for ( auto seg : segmentsInSameCell )
            {
                effectiveDiameter += ( seg->equivalentDiameter() * seg->equivalentDiameter() );
            }

            effectiveDiameter = sqrt( effectiveDiameter );

            for ( auto seg : segmentsInSameCell )
            {
                seg->setEffectiveDiameter( effectiveDiameter );
            }
        }

        {
            // Reduce the diameter for segments in the same cell as main bore
            // https://github.com/OPM/ResInsight/issues/7731

            for ( auto s : allSegments )
            {
                for ( auto completion : s->completions() )
                {
                    if ( completion->completionType() == RigCompletionData::CompletionType::FISHBONES )
                    {
                        auto segments = completion->segments();
                        if ( segments.size() > 1 )
                        {
                            auto firstSegment  = segments[0];
                            auto secondSegment = segments[1];

                            double diameter = secondSegment->effectiveDiameter();

                            firstSegment->setEffectiveDiameter( diameter );
                        }
                    }
                }
            }
        }
    }

    {
        // Update IDC area

        std::map<size_t, std::set<RicMswFishbonesICD*>> icdsInCell;

        {
            for ( auto s : allSegments )
            {
                for ( auto completion : s->completions() )
                {
                    if ( auto icd = dynamic_cast<RicMswFishbonesICD*>( completion ) )
                    {
                        for ( auto icdSegment : icd->segments() )
                        {
                            for ( auto gridIntersection : icdSegment->intersections() )
                            {
                                icdsInCell[gridIntersection->globalCellIndex()].insert( icd );
                            }
                        }
                    }
                }
            }
        }

        for ( auto [Index, icdsInSameCell] : icdsInCell )
        {
            // Compute area sum for all ICDs in same grid cell
            double areaSum = 0.0;

            for ( auto icd : icdsInSameCell )
            {
                areaSum += icd->area();
            }

            for ( auto icd : icdsInSameCell )
            {
                icd->setArea( areaSum );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellPathExportMswTableData::appendFishbonesMswExportInfo( const RimEclipseCase*                            eclipseCase,
                                                                  const RimWellPath*                               wellPath,
                                                                  double                                           initialMD,
                                                                  const std::vector<WellPathCellIntersectionInfo>& cellIntersections,
                                                                  gsl::not_null<RicMswExportInfo*>                 exportInfo,
                                                                  gsl::not_null<RicMswBranch*>                     branch )
{
    std::vector<WellPathCellIntersectionInfo> filteredIntersections =
        filterIntersections( cellIntersections, initialMD, wellPath->wellPathGeometry(), eclipseCase );

    auto mswParameters = wellPath->mswCompletionParameters();

    bool foundSubGridIntersections = false;

    double maxSegmentLength = mswParameters->maxSegmentLength();

    auto unitSystem = exportInfo->unitSystem();

    auto fishbonesSubs = wellPath->completions()->fishbonesCollection()->activeFishbonesSubs();
    for ( RimFishbones* subs : fishbonesSubs )
    {
        std::map<size_t, std::vector<size_t>> subAndLateralIndices;
        for ( const auto& [subIndex, lateralIndex] : subs->installedLateralIndices() )
        {
            subAndLateralIndices[subIndex].push_back( lateralIndex );
        }

        // Find cell intersections closest to each sub location
        std::map<size_t, std::vector<size_t>> closestSubForCellIntersections;
        std::map<size_t, size_t>              cellIntersectionContainingSubIndex;
        {
            auto fishboneSectionStart = subs->startMD();
            auto fishboneSectionEnd   = subs->endMD();

            for ( size_t intersectionIndex = 0; intersectionIndex < filteredIntersections.size(); intersectionIndex++ )
            {
                const auto& cellIntersection = filteredIntersections[intersectionIndex];
                if ( ( fishboneSectionEnd >= cellIntersection.startMD ) && ( fishboneSectionStart <= cellIntersection.endMD ) )

                {
                    double intersectionMidpoint = 0.5 * ( cellIntersection.startMD + cellIntersection.endMD );
                    size_t closestSubIndex      = 0;
                    double closestDistance      = std::numeric_limits<double>::infinity();
                    for ( const auto& [subIndex, lateralIndices] : subAndLateralIndices )
                    {
                        double subMD = subs->measuredDepth( subIndex );

                        if ( ( cellIntersection.startMD <= subMD ) && ( subMD <= cellIntersection.endMD ) )
                        {
                            cellIntersectionContainingSubIndex[subIndex] = intersectionIndex;
                        }

                        auto distanceCandicate = std::abs( subMD - intersectionMidpoint );
                        if ( distanceCandicate < closestDistance )
                        {
                            closestDistance = distanceCandicate;
                            closestSubIndex = subIndex;
                        }
                    }

                    closestSubForCellIntersections[closestSubIndex].push_back( intersectionIndex );
                }
            }
        }

        for ( const auto& [subIndex, lateralIndices] : subAndLateralIndices )
        {
            const double subEndMd      = subs->measuredDepth( subIndex );
            const double subEndTvd     = RicMswTableDataTools::tvdFromMeasuredDepth( branch->wellPath(), subEndMd );
            const double startValveMd  = subEndMd - internal::VALVE_SEGMENT_LENGTH;
            const double startValveTvd = RicMswTableDataTools::tvdFromMeasuredDepth( branch->wellPath(), startValveMd );

            {
                // Add completion for ICD. Insert the segment at the end of the fishbone section. The laterals flows into the ICD
                // segment, and the simulator requires increasing MD on laterals. Make sure that the lateral MDs are larger than the ICD
                // segment MDs.
                auto icdSegment = std::make_unique<RicMswSegment>( "ICD segment", startValveMd, subEndMd, startValveTvd, subEndTvd, subIndex );

                for ( auto lateralIndex : lateralIndices )
                {
                    QString label = QString( "Lateral %1" ).arg( lateralIndex + 1 );
                    icdSegment->addCompletion( std::make_unique<RicMswFishbones>( label, wellPath, subEndMd, subEndTvd, lateralIndex ) );
                }

                assignFishbonesLateralIntersections( eclipseCase,
                                                     branch->wellPath(),
                                                     subs,
                                                     icdSegment.get(),
                                                     &foundSubGridIntersections,
                                                     maxSegmentLength,
                                                     unitSystem );

                auto icdCompletion = std::make_unique<RicMswFishbonesICD>( QString( "ICD" ), wellPath, subEndMd, subEndTvd, nullptr );
                icdCompletion->setFlowCoefficient( subs->icdFlowCoefficient() );
                double icdOrificeRadius = subs->icdOrificeDiameter( unitSystem ) / 2;
                icdCompletion->setArea( icdOrificeRadius * icdOrificeRadius * cvf::PI_D * subs->icdCount() );

                // assign open hole segments to sub
                {
                    const RigMainGrid* mainGrid = eclipseCase->mainGrid();

                    std::set<size_t> indices;
                    for ( auto intersectionIndex : closestSubForCellIntersections[subIndex] )
                    {
                        indices.insert( intersectionIndex );
                    }

                    indices.insert( cellIntersectionContainingSubIndex[subIndex] );

                    for ( auto intersectionIndex : indices )
                    {
                        auto intersection = filteredIntersections[intersectionIndex];
                        if ( intersection.globCellIndex >= mainGrid->totalCellCount() ) continue;

                        size_t             localGridCellIndex = 0u;
                        const RigGridBase* localGrid =
                            mainGrid->gridAndGridLocalIdxFromGlobalCellIdx( intersection.globCellIndex, &localGridCellIndex );
                        QString gridName;
                        if ( localGrid != mainGrid )
                        {
                            gridName                  = QString::fromStdString( localGrid->gridName() );
                            foundSubGridIntersections = true;
                        }

                        size_t i, j, k;
                        localGrid->ijkFromCellIndex( localGridCellIndex, &i, &j, &k );

                        // Shift K to fracture section for dual porosity models
                        if ( mainGrid->isDualPorosity() )
                        {
                            k += mainGrid->cellCountK();
                        }

                        caf::VecIjk0 localIJK( i, j, k );

                        auto mswIntersect = std::make_shared<RicMswSegmentCellIntersection>( gridName,
                                                                                             intersection.globCellIndex,
                                                                                             localIJK,
                                                                                             intersection.intersectionLengthsInCellCS );
                        icdSegment->addIntersection( mswIntersect );
                    }
                }

                icdCompletion->addSegment( std::move( icdSegment ) );

                RicMswSegment* segmentOnParentBranch = branch->findClosestSegmentWithLowerMD( subEndMd );
                if ( segmentOnParentBranch )
                {
                    segmentOnParentBranch->addCompletion( std::move( icdCompletion ) );
                }
            }
        }
    }
    exportInfo->setHasSubGridIntersections( exportInfo->hasSubGridIntersections() || foundSubGridIntersections );

    branch->branches();
    for ( auto& childBranch : branch->branches() )
    {
        auto childWellPath          = childBranch->wellPath();
        auto childCellIntersections = generateCellSegments( eclipseCase, childWellPath );
        appendFishbonesMswExportInfo( eclipseCase, childWellPath, initialMD, childCellIntersections, exportInfo, childBranch );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicWellPathExportMswTableData::appendFracturesMswExportInfo( RimEclipseCase*                                  eclipseCase,
                                                                  const RimWellPath*                               wellPath,
                                                                  double                                           initialMD,
                                                                  const std::vector<WellPathCellIntersectionInfo>& cellIntersections,
                                                                  gsl::not_null<RicMswExportInfo*>                 exportInfo,
                                                                  gsl::not_null<RicMswBranch*>                     branch )
{
    auto fractures = wellPath->fractureCollection()->activeFractures();
    if ( !fractures.empty() )
    {
        std::vector<WellPathCellIntersectionInfo> filteredIntersections =
            filterIntersections( cellIntersections, initialMD, wellPath->wellPathGeometry(), eclipseCase );

        bool foundSubGridIntersections = false;

        // Check if fractures are to be assigned to current main bore segment
        for ( RimWellPathFracture* fracture : fractures )
        {
            fracture->ensureValidNonDarcyProperties();

            double fractureStartMD = fracture->fractureMD();
            if ( fracture->fractureTemplate()->orientationType() == RimFractureTemplate::ALONG_WELL_PATH )
            {
                double perforationLength = fracture->fractureTemplate()->perforationLength();
                fractureStartMD -= 0.5 * perforationLength;
            }

            auto segment = branch->findClosestSegmentWithLowerMD( fractureStartMD );
            if ( segment )
            {
                std::vector<RigCompletionData> completionData =
                    RicExportFractureCompletionsImpl::generateCompdatValues( eclipseCase,
                                                                             wellPath->completionSettings()->wellNameForExport(),
                                                                             wellPath->wellPathGeometry(),
                                                                             { fracture },
                                                                             nullptr,
                                                                             nullptr );

                assignFractureCompletionsToCellSegment( eclipseCase, wellPath, fracture, completionData, segment, &foundSubGridIntersections );
            }
        }

        exportInfo->setHasSubGridIntersections( exportInfo->hasSubGridIntersections() || foundSubGridIntersections );
    }

    for ( auto& childBranch : branch->branches() )
    {
        auto childWellPath          = childBranch->wellPath();
        auto childCellIntersections = generateCellSegments( eclipseCase, childWellPath );

        appendFracturesMswExportInfo( eclipseCase, childWellPath, initialMD, childCellIntersections, exportInfo, childBranch );
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicWellPathExportMswTableData::generateWellSegmentsForMswExportInfo( const RimEclipseCase*           eclipseCase,
                                                                          const RimWellPath*              wellPath,
                                                                          bool                            createSegmentsForPerforations,
                                                                          const std::optional<QDateTime>& exportDate,
                                                                          double                          initialMD,
                                                                          const std::vector<WellPathCellIntersectionInfo>& cellIntersections,
                                                                          gsl::not_null<RicMswExportInfo*> exportInfo,
                                                                          gsl::not_null<RicMswBranch*>     branch )
{
    auto perforationIntervals = createSegmentsForPerforations ? wellPath->perforationIntervalCollection()->activePerforations()
                                                              : std::vector<const RimPerforationInterval*>();

    // Check if there exist overlap between valves in a perforation interval
    for ( const auto& perfInterval : perforationIntervals )
    {
        for ( const auto& valve : perfInterval->valves() )
        {
            for ( const auto& otherValve : perfInterval->valves() )
            {
                if ( otherValve != valve )
                {
                    bool hasIntersection = ( valve->endMD() >= otherValve->startMD() ) && ( otherValve->endMD() >= valve->startMD() );

                    if ( hasIntersection )
                    {
                        RiaLogging::error( QString( "Valve overlap detected for perforation interval : %1" ).arg( perfInterval->name() ) );

                        RiaLogging::error( "Name of valves" );
                        RiaLogging::error( valve->name() );
                        RiaLogging::error( otherValve->name() );

                        RiaLogging::error( "Failed to export well segments" );

                        return false;
                    }
                }
            }
        }
    }

    std::vector<WellPathCellIntersectionInfo> filteredIntersections =
        filterIntersections( cellIntersections, initialMD, wellPath->wellPathGeometry(), eclipseCase );

    bool foundSubGridIntersections = false;

    createWellPathSegments( branch, filteredIntersections, perforationIntervals, wellPath, exportDate, eclipseCase, &foundSubGridIntersections );

    createValveCompletions( branch, perforationIntervals, wellPath, exportInfo->unitSystem() );

    const RigActiveCellInfo* activeCellInfo = eclipseCase->eclipseCaseData()->activeCellInfo( RiaDefines::PorosityModelType::MATRIX_MODEL );

    assignValveContributionsToSuperXICDs( branch, perforationIntervals, filteredIntersections, activeCellInfo, exportInfo->unitSystem() );
    moveIntersectionsToICVs( branch, perforationIntervals, exportInfo->unitSystem() );
    moveIntersectionsToSuperXICDs( branch );

    exportInfo->setHasSubGridIntersections( exportInfo->hasSubGridIntersections() || foundSubGridIntersections );
    branch->sortSegments();

    auto connectedWellPaths = wellPathsWithTieIn( wellPath );

    for ( auto childWellPath : connectedWellPaths )
    {
        auto childMswBranch         = createChildMswBranch( childWellPath );
        auto childCellIntersections = generateCellSegments( eclipseCase, childWellPath );

        // Start MD of child well path at the tie in location
        const double tieInOnParentWellPath = childWellPath->wellPathTieIn() ? childWellPath->wellPathTieIn()->tieInMeasuredDepth() : initialMD;

        if ( generateWellSegmentsForMswExportInfo( eclipseCase,
                                                   childWellPath,
                                                   createSegmentsForPerforations,
                                                   exportDate,
                                                   tieInOnParentWellPath,
                                                   childCellIntersections,
                                                   exportInfo,
                                                   childMswBranch.get() ) )
        {
            branch->addChildBranch( std::move( childMswBranch ) );
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<WellPathCellIntersectionInfo> RicWellPathExportMswTableData::generateCellSegments( const RimEclipseCase* eclipseCase,
                                                                                               const RimWellPath*    wellPath )
{
    auto wellPathGeometry = wellPath->wellPathGeometry();
    CVF_ASSERT( wellPathGeometry );

    const std::vector<cvf::Vec3d>& coords = wellPathGeometry->uniqueWellPathPoints();
    const std::vector<double>&     mds    = wellPathGeometry->uniqueMeasuredDepths();
    CVF_ASSERT( !coords.empty() && !mds.empty() );

    const RigMainGrid* mainGrid = eclipseCase->mainGrid();

    std::vector<WellPathCellIntersectionInfo> allIntersections =
        RigWellPathIntersectionTools::findCellIntersectionInfosAlongPath( eclipseCase->eclipseCaseData(), wellPath->name(), coords, mds );
    if ( allIntersections.empty() ) return {};

    std::vector<WellPathCellIntersectionInfo> continuousIntersections =
        RigWellPathIntersectionTools::buildContinuousIntersections( allIntersections, mainGrid );

    return continuousIntersections;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RicWellPathExportMswTableData::computeIntitialMeasuredDepth( const RimEclipseCase*                            eclipseCase,
                                                                    const RimWellPath*                               wellPath,
                                                                    const RimMswCompletionParameters*                mswParameters,
                                                                    const std::vector<WellPathCellIntersectionInfo>& allIntersections )
{
    if ( allIntersections.empty() ) return 0.0;

    const RigActiveCellInfo* activeCellInfo = eclipseCase->eclipseCaseData()->activeCellInfo( RiaDefines::PorosityModelType::MATRIX_MODEL );

    double candidateMeasuredDepth = 0.0;
    if ( mswParameters->referenceMDType() == RimMswCompletionParameters::ReferenceMDType::MANUAL_REFERENCE_MD )
    {
        candidateMeasuredDepth = mswParameters->manualReferenceMD();
    }
    else
    {
        for ( const WellPathCellIntersectionInfo& intersection : allIntersections )
        {
            if ( activeCellInfo->isActive( intersection.globCellIndex ) )
            {
                candidateMeasuredDepth = intersection.startMD;
                break;
            }
        }

        double startOfFirstCompletion = std::numeric_limits<double>::infinity();
        {
            std::vector<const RimWellPathComponentInterface*> allCompletions = wellPath->completions()->allCompletions();

            for ( const RimWellPathComponentInterface* completion : allCompletions )
            {
                if ( completion->isEnabled() && completion->startMD() < startOfFirstCompletion )
                {
                    startOfFirstCompletion = completion->startMD();
                }
            }
        }

        // Initial MD is the lowest MD based on grid intersection and start of fracture completions
        // https://github.com/OPM/ResInsight/issues/6071
        candidateMeasuredDepth = std::min( candidateMeasuredDepth, startOfFirstCompletion );
    }

    return candidateMeasuredDepth;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<WellPathCellIntersectionInfo>
    RicWellPathExportMswTableData::filterIntersections( const std::vector<WellPathCellIntersectionInfo>& intersections,
                                                        double                                           initialMD,
                                                        gsl::not_null<const RigWellPath*>                wellPathGeometry,
                                                        gsl::not_null<const RimEclipseCase*>             eclipseCase )
{
    std::vector<WellPathCellIntersectionInfo> filteredIntersections;

    if ( !intersections.empty() && intersections[0].startMD > initialMD )
    {
        WellPathCellIntersectionInfo firstIntersection = intersections[0];

        // Add a segment from user defined MD to start of grid
        cvf::Vec3d intersectionPoint = wellPathGeometry->interpolatedPointAlongWellPath( initialMD );

        WellPathCellIntersectionInfo extraIntersection;

        extraIntersection.globCellIndex         = std::numeric_limits<size_t>::max();
        extraIntersection.startPoint            = intersectionPoint;
        extraIntersection.endPoint              = firstIntersection.startPoint;
        extraIntersection.startMD               = initialMD;
        extraIntersection.endMD                 = firstIntersection.startMD;
        extraIntersection.intersectedCellFaceIn = cvf::StructGridInterface::NO_FACE;

        if ( firstIntersection.intersectedCellFaceIn != cvf::StructGridInterface::NO_FACE )

        {
            extraIntersection.intersectedCellFaceOut = cvf::StructGridInterface::oppositeFace( firstIntersection.intersectedCellFaceIn );
        }
        else if ( firstIntersection.intersectedCellFaceOut != cvf::StructGridInterface::NO_FACE )
        {
            extraIntersection.intersectedCellFaceOut = firstIntersection.intersectedCellFaceOut;
        }

        extraIntersection.intersectionLengthsInCellCS = cvf::Vec3d::ZERO;

        filteredIntersections.push_back( extraIntersection );
    }

    const double epsilon = 1.0e-3;

    for ( const WellPathCellIntersectionInfo& intersection : intersections )
    {
        if ( ( intersection.endMD - initialMD ) < epsilon )
        {
            // Skip all intersections before initial measured depth
            continue;
        }

        if ( ( intersection.startMD - initialMD ) > epsilon )
        {
            filteredIntersections.push_back( intersection );
        }
        else
        {
            // InitialMD is inside intersection, split based on intersection point

            cvf::Vec3d intersectionPoint = wellPathGeometry->interpolatedPointAlongWellPath( initialMD );

            WellPathCellIntersectionInfo extraIntersection;

            extraIntersection.globCellIndex          = intersection.globCellIndex;
            extraIntersection.startPoint             = intersectionPoint;
            extraIntersection.endPoint               = intersection.endPoint;
            extraIntersection.startMD                = initialMD;
            extraIntersection.endMD                  = intersection.endMD;
            extraIntersection.intersectedCellFaceIn  = cvf::StructGridInterface::NO_FACE;
            extraIntersection.intersectedCellFaceOut = intersection.intersectedCellFaceOut;

            const RigMainGrid* grid = eclipseCase->mainGrid();

            if ( intersection.globCellIndex < grid->cellCount() )
            {
                extraIntersection.intersectionLengthsInCellCS =
                    RigWellPathIntersectionTools::calculateLengthInCell( grid, intersection.globCellIndex, intersectionPoint, intersection.endPoint );
            }
            else
            {
                extraIntersection.intersectionLengthsInCellCS = cvf::Vec3d::ZERO;
            }

            filteredIntersections.push_back( extraIntersection );
        }
    }

    return filteredIntersections;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellPathExportMswTableData::createWellPathSegments( gsl::not_null<RicMswBranch*>                      branch,
                                                            const std::vector<WellPathCellIntersectionInfo>&  cellSegmentIntersections,
                                                            const std::vector<const RimPerforationInterval*>& perforationIntervals,
                                                            const RimWellPath*                                wellPath,
                                                            const std::optional<QDateTime>&                   exportDate,
                                                            const RimEclipseCase*                             eclipseCase,
                                                            bool*                                             foundSubGridIntersections )
{
    // Intersections along the well path with grid geometry is handled by well log extraction tools.
    // The threshold in RigWellLogExtractionTools::isEqualDepth is currently set to 0.1m, and this
    // is a pretty large threshold based on the indicated threshold of 0.001m for MSW segments
    const double segmentLengthThreshold = 1.0e-3;

    for ( const auto& cellIntInfo : cellSegmentIntersections )
    {
        const double segmentLength = std::fabs( cellIntInfo.endMD - cellIntInfo.startMD );

        if ( segmentLength > segmentLengthThreshold )
        {
            auto segment = std::make_unique<RicMswSegment>( QString( "%1 segment" ).arg( branch->label() ),
                                                            cellIntInfo.startMD,
                                                            cellIntInfo.endMD,
                                                            cellIntInfo.startTVD(),
                                                            cellIntInfo.endTVD() );

            for ( const RimPerforationInterval* interval : perforationIntervals )
            {
                double overlapStart = std::max( interval->startMD(), segment->startMD() );
                double overlapEnd   = std::min( interval->endMD(), segment->endMD() );
                double overlap      = std::max( 0.0, overlapEnd - overlapStart );
                if ( overlap > 0.0 )
                {
                    double overlapStartTVD = -wellPath->wellPathGeometry()->interpolatedPointAlongWellPath( overlapStart ).z();
                    auto   intervalCompletion =
                        std::make_unique<RicMswPerforation>( interval->name(), wellPath, overlapStart, overlapStartTVD, interval );
                    std::vector<RigCompletionData> completionData =
                        generatePerforationIntersections( wellPath, interval, exportDate, eclipseCase );
                    assignPerforationIntersections( completionData,
                                                    intervalCompletion.get(),
                                                    cellIntInfo,
                                                    overlapStart,
                                                    overlapEnd,
                                                    foundSubGridIntersections );
                    segment->addCompletion( std::move( intervalCompletion ) );
                }
            }
            branch->addSegment( std::move( segment ) );
        }
        else
        {
            QString text = QString( "Skipping segment , threshold = %1, length = %2" ).arg( segmentLengthThreshold ).arg( segmentLength );
            RiaLogging::info( text );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellPathExportMswTableData::createValveCompletions( gsl::not_null<RicMswBranch*>                      branch,
                                                            const std::vector<const RimPerforationInterval*>& perforationIntervals,
                                                            const RimWellPath*                                wellPath,
                                                            RiaDefines::EclipseUnitSystem                     unitSystem )
{
    int  nMainSegment = 0;
    auto segments     = branch->segments();
    for ( auto segment : segments )
    {
        std::unique_ptr<RicMswPerforationICV>  ICV;
        std::unique_ptr<RicMswPerforationICD>  superICD;
        std::unique_ptr<RicMswPerforationAICD> superAICD;
        std::unique_ptr<RicMswPerforationSICD> superSICD;

        double totalICDOverlap  = 0.0;
        double totalAICDOverlap = 0.0;
        double totalSICDOverlap = 0.0;

        for ( const RimPerforationInterval* interval : perforationIntervals )
        {
            if ( !interval->isChecked() ) continue;

            auto perforationValves = interval->descendantsIncludingThisOfType<RimWellPathValve>();
            for ( const RimWellPathValve* valve : perforationValves )
            {
                if ( !valve->isChecked() ) continue;

                for ( size_t nSubValve = 0u; nSubValve < valve->valveLocations().size(); ++nSubValve )
                {
                    double valveMD = valve->valveLocations()[nSubValve];

                    std::pair<double, double> valveSegment = valve->valveSegments()[nSubValve];
                    double                    overlapStart = std::max( valveSegment.first, segment->startMD() );
                    double                    overlapEnd   = std::min( valveSegment.second, segment->endMD() );
                    double                    overlap      = std::max( 0.0, overlapEnd - overlapStart );

                    double exportStartMD = valveMD;
                    double exportEndMD   = valveMD + internal::VALVE_SEGMENT_LENGTH;

                    double exportStartTVD = RicMswTableDataTools::tvdFromMeasuredDepth( wellPath, exportStartMD );
                    double exportEndTVD   = RicMswTableDataTools::tvdFromMeasuredDepth( wellPath, exportEndMD );

                    if ( segment->startMD() <= valveMD && valveMD < segment->endMD() )
                    {
                        if ( valve->componentType() == RiaDefines::WellPathComponentType::AICD )
                        {
                            QString valveLabel = QString( "%1 #%2" ).arg( "Combined Valve for segment" ).arg( nMainSegment + 2 );
                            auto    subSegment =
                                std::make_unique<RicMswSegment>( "Valve segment", exportStartMD, exportEndMD, exportStartTVD, exportEndTVD );

                            superAICD = std::make_unique<RicMswPerforationAICD>( valveLabel, wellPath, exportStartMD, exportStartTVD, valve );
                            superAICD->addSegment( std::move( subSegment ) );
                        }
                        else if ( valve->componentType() == RiaDefines::WellPathComponentType::SICD )
                        {
                            QString valveLabel = QString( "%1 #%2" ).arg( "Combined Valve for segment" ).arg( nMainSegment + 2 );
                            auto    subSegment =
                                std::make_unique<RicMswSegment>( "Valve segment", exportStartMD, exportEndMD, exportStartTVD, exportEndTVD );

                            superSICD = std::make_unique<RicMswPerforationSICD>( valveLabel, wellPath, exportStartMD, exportStartTVD, valve );
                            superSICD->addSegment( std::move( subSegment ) );
                        }
                        else if ( valve->componentType() == RiaDefines::WellPathComponentType::ICD )
                        {
                            QString valveLabel = QString( "%1 #%2" ).arg( "Combined Valve for segment" ).arg( nMainSegment + 2 );
                            auto    subSegment =
                                std::make_unique<RicMswSegment>( "Valve segment", exportStartMD, exportEndMD, exportStartTVD, exportEndTVD );

                            superICD = std::make_unique<RicMswPerforationICD>( valveLabel, wellPath, exportStartMD, exportStartTVD, valve );
                            superICD->addSegment( std::move( subSegment ) );
                        }
                        else if ( valve->componentType() == RiaDefines::WellPathComponentType::ICV )
                        {
                            QString valveLabel = QString( "ICV %1 at segment #%2" ).arg( valve->name() ).arg( nMainSegment + 2 );
                            auto    subSegment =
                                std::make_unique<RicMswSegment>( "Valve segment", exportStartMD, exportEndMD, exportStartTVD, exportEndTVD );

                            ICV = std::make_unique<RicMswPerforationICV>( valveLabel, wellPath, exportStartMD, exportStartTVD, valve );
                            ICV->addSegment( std::move( subSegment ) );
                        }
                    }
                    else if ( overlap > 0.0 && ( valve->componentType() == RiaDefines::WellPathComponentType::ICD && !superICD ) )
                    {
                        QString valveLabel = QString( "%1 #%2" ).arg( "Combined Valve for segment" ).arg( nMainSegment + 2 );

                        auto subSegment =
                            std::make_unique<RicMswSegment>( "Valve segment", exportStartMD, exportEndMD, exportStartTVD, exportEndTVD );
                        superICD = std::make_unique<RicMswPerforationICD>( valveLabel, wellPath, exportStartMD, exportStartTVD, valve );
                        superICD->addSegment( std::move( subSegment ) );
                    }
                    else if ( overlap > 0.0 && ( valve->componentType() == RiaDefines::WellPathComponentType::AICD && !superAICD ) )
                    {
                        QString valveLabel = QString( "%1 #%2" ).arg( "Combined Valve for segment" ).arg( nMainSegment + 2 );

                        auto subSegment =
                            std::make_unique<RicMswSegment>( "Valve segment", exportStartMD, exportEndMD, exportStartTVD, exportEndTVD );
                        superAICD = std::make_unique<RicMswPerforationAICD>( valveLabel, wellPath, exportStartMD, exportStartTVD, valve );
                        superAICD->addSegment( std::move( subSegment ) );
                    }
                    else if ( overlap > 0.0 && ( valve->componentType() == RiaDefines::WellPathComponentType::SICD && !superSICD ) )
                    {
                        QString valveLabel = QString( "%1 #%2" ).arg( "Combined Valve for segment" ).arg( nMainSegment + 2 );

                        auto subSegment =
                            std::make_unique<RicMswSegment>( "Valve segment", exportStartMD, exportEndMD, exportStartTVD, exportEndTVD );
                        superSICD = std::make_unique<RicMswPerforationSICD>( valveLabel, wellPath, exportStartMD, exportStartTVD, valve );
                        superSICD->addSegment( std::move( subSegment ) );
                    }

                    if ( valve->componentType() == RiaDefines::WellPathComponentType::AICD )
                    {
                        totalAICDOverlap += overlap;
                    }
                    else if ( valve->componentType() == RiaDefines::WellPathComponentType::SICD )
                    {
                        totalSICDOverlap += overlap;
                    }
                    else if ( valve->componentType() == RiaDefines::WellPathComponentType::ICD )
                    {
                        totalICDOverlap += overlap;
                    }
                }
            }
        }

        if ( ICV )
        {
            segment->addCompletion( std::move( ICV ) );
        }
        else
        {
            if ( totalICDOverlap > 0.0 || totalAICDOverlap > 0.0 || totalSICDOverlap > 0.0 )
            {
                // pick valve with largest overlap
                if ( totalAICDOverlap >= totalSICDOverlap && totalAICDOverlap >= totalICDOverlap )
                {
                    segment->addCompletion( std::move( superAICD ) );
                }
                else if ( totalSICDOverlap >= totalAICDOverlap && totalSICDOverlap >= totalICDOverlap )
                {
                    segment->addCompletion( std::move( superSICD ) );
                }
                else
                {
                    segment->addCompletion( std::move( superICD ) );
                }
            }
        }
        nMainSegment++;
    }
}

//--------------------------------------------------------------------------------------------------
/// Aggregates individual valve parameters from perforation intervals into segment-level "super"
/// valves (ICDs or AICDs or SICDs) for MSW export. Calculates weighted averages based on overlap with
/// active reservoir cells.
///
/// Key steps:
/// 1. Setup Phase: Creates accumulator objects (ICD/AICD/SICD) for each segment containing a super valve
/// 2. First Pass: Calculates total perforation length overlapping with active cells for each valve
/// 3. Second Pass: For each valve, calculates overlap with segments and accumulates parameters
///    weighted by overlap length. Only considers overlaps with active cells.
/// 4. Apply Phase: Applies accumulated weighted averages to super valve parameters
/// 5. Label Update: Appends contributor valve names to super valve labels for documentation
///
//--------------------------------------------------------------------------------------------------
void RicWellPathExportMswTableData::assignValveContributionsToSuperXICDs( gsl::not_null<RicMswBranch*> branch,
                                                                          const std::vector<const RimPerforationInterval*>& perforationIntervals,
                                                                          const std::vector<WellPathCellIntersectionInfo>& wellPathIntersections,
                                                                          const RigActiveCellInfo*      activeCellInfo,
                                                                          RiaDefines::EclipseUnitSystem unitSystem )
{
    using ValveContributionMap = std::map<RicMswCompletion*, std::vector<const RimWellPathValve*>>;

    ValveContributionMap assignedRegularValves;

    std::map<RicMswSegment*, std::unique_ptr<RicMswValveAccumulator>> accumulators;

    for ( auto segment : branch->segments() )
    {
        RicMswValve* superValve = nullptr;
        for ( auto completion : segment->completions() )
        {
            auto valve = dynamic_cast<RicMswValve*>( completion );
            if ( valve )
            {
                superValve = valve;
                break;
            }
        }
        if ( dynamic_cast<RicMswPerforationICD*>( superValve ) )
        {
            accumulators[segment] = std::make_unique<RicMswICDAccumulator>( superValve, unitSystem );
        }
        else if ( dynamic_cast<RicMswPerforationAICD*>( superValve ) )
        {
            accumulators[segment] = std::make_unique<RicMswAICDAccumulator>( superValve, unitSystem );
        }
        else if ( dynamic_cast<RicMswPerforationSICD*>( superValve ) )
        {
            accumulators[segment] = std::make_unique<RicMswSICDAccumulator>( superValve, unitSystem );
        }
    }

    for ( const RimPerforationInterval* interval : perforationIntervals )
    {
        if ( !interval->isChecked() ) continue;

        std::vector<const RimWellPathValve*> perforationValves      = interval->descendantsIncludingThisOfType<const RimWellPathValve>();
        double                               totalPerforationLength = 0.0;
        for ( const RimWellPathValve* valve : perforationValves )
        {
            if ( !valve->isChecked() ) continue;

            for ( auto segment : branch->segments() )
            {
                double intervalOverlapStart = std::max( interval->startMD(), segment->startMD() );
                double intervalOverlapEnd   = std::min( interval->endMD(), segment->endMD() );
                auto   intervalOverlapWithActiveCells =
                    calculateOverlapWithActiveCells( intervalOverlapStart, intervalOverlapEnd, wellPathIntersections, activeCellInfo );

                totalPerforationLength += intervalOverlapWithActiveCells.second - intervalOverlapWithActiveCells.first;
            }
        }

        for ( const RimWellPathValve* valve : perforationValves )
        {
            if ( !valve->isChecked() ) continue;

            for ( auto segment : branch->segments() )
            {
                double intervalOverlapStart = std::max( interval->startMD(), segment->startMD() );
                double intervalOverlapEnd   = std::min( interval->endMD(), segment->endMD() );

                auto intervalOverlapWithActiveCells =
                    calculateOverlapWithActiveCells( intervalOverlapStart, intervalOverlapEnd, wellPathIntersections, activeCellInfo );

                double overlapLength = intervalOverlapWithActiveCells.second - intervalOverlapWithActiveCells.first;
                if ( overlapLength > 0.0 )
                {
                    auto it = accumulators.find( segment );

                    if ( it != accumulators.end() )
                    {
                        it->second->accumulateValveParameters( valve, overlapLength, totalPerforationLength );
                        assignedRegularValves[it->second->superValve()].push_back( valve );
                    }
                }
            }
        }
    }

    for ( const auto& accumulator : accumulators )
    {
        accumulator.second->applyToSuperValve();
    }

    for ( auto regularValvePair : assignedRegularValves )
    {
        if ( !regularValvePair.second.empty() )
        {
            QStringList valveLabels;
            for ( const RimWellPathValve* regularValve : regularValvePair.second )
            {
                QString valveLabel = QString( "%1" ).arg( regularValve->name() );
                valveLabels.push_back( valveLabel );
            }
            QString valveContribLabel = QString( " with contribution from: %1" ).arg( valveLabels.join( ", " ) );
            regularValvePair.first->setLabel( regularValvePair.first->label() + valveContribLabel );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellPathExportMswTableData::moveIntersectionsToICVs( gsl::not_null<RicMswBranch*>                      branch,
                                                             const std::vector<const RimPerforationInterval*>& perforationIntervals,
                                                             RiaDefines::EclipseUnitSystem                     unitSystem )
{
    std::map<const RimWellPathValve*, RicMswPerforationICV*> icvCompletionMap;

    for ( auto segment : branch->segments() )
    {
        for ( auto completion : segment->completions() )
        {
            auto icv = dynamic_cast<RicMswPerforationICV*>( completion );
            if ( icv )
            {
                icvCompletionMap[icv->wellPathValve()] = icv;
            }
        }
    }

    for ( auto segment : branch->segments() )
    {
        std::vector<RicMswCompletion*> perforations;
        for ( auto completion : segment->completions() )
        {
            if ( completion->completionType() == RigCompletionData::CompletionType::PERFORATION )
            {
                perforations.push_back( completion );
            }
        }

        for ( const RimPerforationInterval* interval : perforationIntervals )
        {
            if ( !interval->isChecked() ) continue;

            std::vector<const RimWellPathValve*> perforationValves = interval->descendantsIncludingThisOfType<const RimWellPathValve>();

            for ( const RimWellPathValve* valve : perforationValves )
            {
                if ( !valve->isChecked() ) continue;
                if ( valve->componentType() != RiaDefines::WellPathComponentType::ICV ) continue;

                auto icvIt = icvCompletionMap.find( valve );
                if ( icvIt == icvCompletionMap.end() ) continue;

                auto icvCompletion = icvIt->second;
                CVF_ASSERT( icvCompletion );

                std::pair<double, double> valveSegment = valve->valveSegments().front();
                double                    overlapStart = std::max( valveSegment.first, segment->startMD() );
                double                    overlapEnd   = std::min( valveSegment.second, segment->endMD() );
                double                    overlap      = std::max( 0.0, overlapEnd - overlapStart );

                if ( overlap > 0.0 )
                {
                    CVF_ASSERT( icvCompletion->segments().size() == 1u );
                    for ( auto perforationPtr : perforations )
                    {
                        for ( auto subSegmentPtr : perforationPtr->segments() )
                        {
                            for ( auto intersectionPtr : subSegmentPtr->intersections() )
                            {
                                icvCompletion->segments()[0]->addIntersection( intersectionPtr );
                            }
                        }
                        segment->removeCompletion( perforationPtr );
                    }
                    perforations.clear();
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellPathExportMswTableData::moveIntersectionsToSuperXICDs( gsl::not_null<RicMswBranch*> branch )
{
    for ( auto segment : branch->segments() )
    {
        RicMswCompletion*              superValve = nullptr;
        std::vector<RicMswCompletion*> perforations;
        for ( auto completion : segment->completions() )
        {
            if ( completion->completionType() == RigCompletionData::CompletionType::PERFORATION_ICD ||
                 completion->completionType() == RigCompletionData::CompletionType::PERFORATION_AICD ||
                 completion->completionType() == RigCompletionData::CompletionType::PERFORATION_SICD )
            {
                superValve = completion;
            }
            else if ( completion->completionType() == RigCompletionData::CompletionType::PERFORATION )
            {
                perforations.push_back( completion );
            }
        }

        if ( superValve == nullptr ) continue;

        CVF_ASSERT( superValve->segments().size() == 1u );

        // Remove and take over ownership of the superValve completion
        auto completionPtr = segment->removeCompletion( superValve );
        for ( auto perforation : perforations )
        {
            for ( auto subSegment : perforation->segments() )
            {
                // The valve completions on the main branch will be deleted. Create a segment with startMD and
                // endMD representing the perforation along main well path to be connected to the valve. When COMPSEGS
                // data is exported, the startMD and endMD of the segment is used to define the Start Length and End
                // Length of the COMPSEGS keyword
                //
                // Example output
                //
                // COMPSEGS
                // --Name
                //     Well - 1 /
                // --I      J      K      Branch no     Start Length     End Length
                //   17     17     9      2             3030.71791       3034.01331 /
                //   17     18     9      3             3034.01331       3125.47617 /

                auto valveInflowSegment = std::make_unique<RicMswSegment>( QString( "%1 real valve segment " ).arg( branch->label() ),
                                                                           subSegment->startMD(),
                                                                           subSegment->endMD(),
                                                                           subSegment->startTVD(),
                                                                           subSegment->endTVD() );

                for ( auto intersectionPtr : subSegment->intersections() )
                {
                    valveInflowSegment->addIntersection( intersectionPtr );
                }

                {
                    double midpoint = ( segment->startMD() + segment->endMD() ) * 0.5;

                    // Set the output MD to the midpoint of the segment, this info is used when exporting WELSEGS in
                    // RicMswTableDataTools::writeValveWelsegsSegment
                    completionPtr->segments()[0]->setOutputMD( midpoint );
                }
                completionPtr->addSegment( std::move( valveInflowSegment ) );
            }
        }

        // Remove all completions and re-add the super valve
        segment->deleteAllCompletions();
        segment->addCompletion( std::move( completionPtr ) );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellPathExportMswTableData::assignFishbonesLateralIntersections( const RimEclipseCase*         eclipseCase,
                                                                         const RimWellPath*            wellPath,
                                                                         const RimFishbones*           fishbonesSubs,
                                                                         gsl::not_null<RicMswSegment*> segment,
                                                                         bool*                         foundSubGridIntersections,
                                                                         double                        maxSegmentLength,
                                                                         RiaDefines::EclipseUnitSystem unitSystem )
{
    CVF_ASSERT( foundSubGridIntersections != nullptr );

    const RigMainGrid* grid = eclipseCase->eclipseCaseData()->mainGrid();

    for ( auto completion : segment->completions() )
    {
        if ( completion->completionType() != RigCompletionData::CompletionType::FISHBONES )
        {
            continue;
        }

        std::vector<std::pair<cvf::Vec3d, double>> lateralCoordMDPairs =
            fishbonesSubs->coordsAndMDForLateral( segment->subIndex(), completion->index() );

        if ( lateralCoordMDPairs.empty() )
        {
            continue;
        }

        std::vector<cvf::Vec3d> lateralCoords;
        std::vector<double>     lateralMDs;

        lateralCoords.reserve( lateralCoordMDPairs.size() );
        lateralMDs.reserve( lateralCoordMDPairs.size() );

        for ( auto& coordMD : lateralCoordMDPairs )
        {
            lateralCoords.push_back( coordMD.first );
            lateralMDs.push_back( coordMD.second );
        }

        std::vector<WellPathCellIntersectionInfo> intersections =
            RigWellPathIntersectionTools::findCellIntersectionInfosAlongPath( eclipseCase->eclipseCaseData(),
                                                                              wellPath->name(),
                                                                              lateralCoords,
                                                                              lateralMDs );

        double previousExitMD  = lateralMDs.front();
        double previousExitTVD = -lateralCoords.front().z();

        for ( const auto& cellIntInfo : intersections )
        {
            size_t             localGridCellIndex = 0u;
            const RigGridBase* localGrid = grid->gridAndGridLocalIdxFromGlobalCellIdx( cellIntInfo.globCellIndex, &localGridCellIndex );
            QString            gridName;
            if ( localGrid != grid )
            {
                gridName                   = QString::fromStdString( localGrid->gridName() );
                *foundSubGridIntersections = true;
            }

            size_t i = 0u, j = 0u, k = 0u;
            localGrid->ijkFromCellIndex( localGridCellIndex, &i, &j, &k );

            // For dual porosity models, shift K to the fracture section so exported completion data references the correct K-layer.
            if ( grid->isDualPorosity() )
            {
                k += grid->cellCountK();
            }

            auto subSegment = std::make_unique<RicMswSegment>( "Sub segment",
                                                               previousExitMD,
                                                               cellIntInfo.endMD,
                                                               previousExitTVD,
                                                               cellIntInfo.endTVD(),
                                                               segment->subIndex() );

            subSegment->setEquivalentDiameter( fishbonesSubs->equivalentDiameter( unitSystem ) );
            subSegment->setHoleDiameter( fishbonesSubs->holeDiameter( unitSystem ) );
            subSegment->setOpenHoleRoughnessFactor( fishbonesSubs->openHoleRoughnessFactor( unitSystem ) );
            subSegment->setSkinFactor( fishbonesSubs->skinFactor() );
            subSegment->setSourcePdmObject( fishbonesSubs );
            subSegment->setIntersectedGlobalCells( { cellIntInfo.globCellIndex } );

            auto intersection = std::make_shared<RicMswSegmentCellIntersection>( gridName,
                                                                                 cellIntInfo.globCellIndex,
                                                                                 caf::VecIjk0( i, j, k ),
                                                                                 cellIntInfo.intersectionLengthsInCellCS );
            subSegment->addIntersection( std::move( intersection ) );
            completion->addSegment( std::move( subSegment ) );

            previousExitMD  = cellIntInfo.endMD;
            previousExitTVD = cellIntInfo.endTVD();
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellPathExportMswTableData::assignFractureCompletionsToCellSegment( const RimEclipseCase*                 eclipseCase,
                                                                            const RimWellPath*                    wellPath,
                                                                            const RimWellPathFracture*            fracture,
                                                                            const std::vector<RigCompletionData>& completionData,
                                                                            gsl::not_null<RicMswSegment*>         segment,
                                                                            bool* foundSubGridIntersections )
{
    CVF_ASSERT( foundSubGridIntersections != nullptr );

    double position = fracture->fractureMD();
    double width    = fracture->fractureTemplate()->computeFractureWidth( fracture );

    auto fractureCompletion = std::make_unique<RicMswFracture>( fracture->name(), wellPath, position, position + width );

    if ( fracture->fractureTemplate()->orientationType() == RimFractureTemplate::ALONG_WELL_PATH )
    {
        double perforationLength = fracture->fractureTemplate()->perforationLength();
        position -= 0.5 * perforationLength;
        width = perforationLength;
    }

    auto subSegment = std::make_unique<RicMswSegment>( "Fracture segment", position, position + width, 0.0, 0.0 );
    for ( const RigCompletionData& compIntersection : completionData )
    {
        const RigCompletionDataGridCell& cell = compIntersection.completionDataGridCell();

        if ( !cell.isMainGridCell() )
        {
            *foundSubGridIntersections = true;
        }

        caf::VecIjk0 localIJK( cell.localCellIndexI(), cell.localCellIndexJ(), cell.localCellIndexK() );

        auto intersection =
            std::make_shared<RicMswSegmentCellIntersection>( cell.lgrName(), cell.globalCellIndex(), localIJK, cvf::Vec3d::ZERO );
        subSegment->addIntersection( intersection );
    }
    fractureCompletion->addSegment( std::move( subSegment ) );
    segment->addCompletion( std::move( fractureCompletion ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RigCompletionData>
    RicWellPathExportMswTableData::generatePerforationIntersections( gsl::not_null<const RimWellPath*>            wellPath,
                                                                     gsl::not_null<const RimPerforationInterval*> perforationInterval,
                                                                     const std::optional<QDateTime>&              exportDate,
                                                                     gsl::not_null<const RimEclipseCase*>         eclipseCase )
{
    std::vector<RigCompletionData> completionData;
    const RigActiveCellInfo* activeCellInfo = eclipseCase->eclipseCaseData()->activeCellInfo( RiaDefines::PorosityModelType::MATRIX_MODEL );

    auto wellPathGeometry = wellPath->wellPathGeometry();
    CVF_ASSERT( wellPathGeometry );
    bool isActive = !exportDate.has_value() || perforationInterval->isActiveOnDate( *exportDate );

    if ( wellPath->perforationIntervalCollection()->isChecked() && perforationInterval->isChecked() && isActive )
    {
        std::pair<std::vector<cvf::Vec3d>, std::vector<double>> perforationPointsAndMD =
            wellPathGeometry->clippedPointSubset( perforationInterval->startMD(), perforationInterval->endMD() );

        std::vector<WellPathCellIntersectionInfo> intersectedCells =
            RigWellPathIntersectionTools::findCellIntersectionInfosAlongPath( eclipseCase->eclipseCaseData(),
                                                                              wellPath->name(),
                                                                              perforationPointsAndMD.first,
                                                                              perforationPointsAndMD.second );

        for ( auto& cell : intersectedCells )
        {
            bool cellIsActive = activeCellInfo->isActive( cell.globCellIndex );
            if ( !cellIsActive ) continue;

            RigCompletionData completion( wellPath->completionSettings()->wellNameForExport(),
                                          RigCompletionDataGridCell( cell.globCellIndex, eclipseCase->mainGrid() ),
                                          cell.startMD );

            completion.setSourcePdmObject( perforationInterval );
            completionData.push_back( completion );
        }
    }

    return completionData;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellPathExportMswTableData::assignPerforationIntersections( const std::vector<RigCompletionData>& completionData,
                                                                    gsl::not_null<RicMswCompletion*>      perforationCompletion,
                                                                    const WellPathCellIntersectionInfo&   cellIntInfo,
                                                                    double                                overlapStart,
                                                                    double                                overlapEnd,
                                                                    bool*                                 foundSubGridIntersections )
{
    size_t currCellId = cellIntInfo.globCellIndex;

    auto subSegment =
        std::make_unique<RicMswSegment>( "Perforation segment", overlapStart, overlapEnd, cellIntInfo.startTVD(), cellIntInfo.endTVD() );
    for ( const RigCompletionData& compIntersection : completionData )
    {
        const RigCompletionDataGridCell& cell = compIntersection.completionDataGridCell();
        if ( !cell.isMainGridCell() )
        {
            *foundSubGridIntersections = true;
        }

        if ( cell.globalCellIndex() != currCellId ) continue;

        caf::VecIjk0 localIJK( cell.localCellIndexI(), cell.localCellIndexJ(), cell.localCellIndexK() );

        auto intersection = std::make_shared<RicMswSegmentCellIntersection>( cell.lgrName(),
                                                                             cell.globalCellIndex(),
                                                                             localIJK,
                                                                             cellIntInfo.intersectionLengthsInCellCS );
        subSegment->addIntersection( intersection );
    }
    perforationCompletion->addSegment( std::move( subSegment ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellPathExportMswTableData::assignBranchNumbersToPerforations( const RimEclipseCase*         eclipseCase,
                                                                       gsl::not_null<RicMswSegment*> segment,
                                                                       int                           branchNumber )
{
    for ( auto completion : segment->completions() )
    {
        if ( completion->completionType() == RigCompletionData::CompletionType::PERFORATION )
        {
            completion->setBranchNumber( branchNumber );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellPathExportMswTableData::assignBranchNumbersToOtherCompletions( const RimEclipseCase*         eclipseCase,
                                                                           gsl::not_null<RicMswSegment*> segment,
                                                                           gsl::not_null<int*>           branchNumber )
{
    for ( auto completion : segment->completions() )
    {
        if ( completion->completionType() != RigCompletionData::CompletionType::PERFORATION )
        {
            completion->setBranchNumber( ++( *branchNumber ) );

            for ( auto seg : completion->segments() )
            {
                assignBranchNumbersToOtherCompletions( eclipseCase, seg, branchNumber );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellPathExportMswTableData::assignBranchNumbersToBranch( const RimEclipseCase*        eclipseCase,
                                                                 RicMswExportInfo*            exportInfo,
                                                                 gsl::not_null<RicMswBranch*> branch,
                                                                 gsl::not_null<int*>          branchNumber )
{
    const auto currentBranchNumber = *branchNumber;
    branch->setBranchNumber( currentBranchNumber );

    for ( auto childBranch : branch->branches() )
    {
        ( *branchNumber )++;
        assignBranchNumbersToBranch( eclipseCase, exportInfo, childBranch, branchNumber );
    }

    // Assign perforations first to ensure the same branch number as the segment
    for ( auto segment : branch->segments() )
    {
        assignBranchNumbersToPerforations( eclipseCase, segment, currentBranchNumber );
    }

    // Assign other completions with an incremented branch number
    for ( auto segment : branch->segments() )
    {
        assignBranchNumbersToOtherCompletions( eclipseCase, segment, branchNumber );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::unique_ptr<RicMswBranch> RicWellPathExportMswTableData::createChildMswBranch( const RimWellPath* childWellPath )
{
    auto initialChildMD  = childWellPath->wellPathTieIn()->tieInMeasuredDepth();
    auto initialChildTVD = -childWellPath->wellPathGeometry()->interpolatedPointAlongWellPath( initialChildMD ).z();

    auto valveMD  = childWellPath->wellPathTieIn()->branchValveMeasuredDepth();
    auto valveTVD = -childWellPath->wellPathGeometry()->interpolatedPointAlongWellPath( valveMD ).z();

    double offset = ( valveMD == initialChildMD ) ? internal::VALVE_SEGMENT_LENGTH : 0.0;

    const RimWellPathValve* outletValve = childWellPath->wellPathTieIn()->outletValve();
    if ( outletValve )
    {
        auto branchStartingWithValve =
            RicMswValve::createTieInValve( QString( "%1 valve for %2" ).arg( outletValve->componentLabel() ).arg( childWellPath->name() ),
                                           childWellPath,
                                           valveMD,
                                           valveTVD,
                                           outletValve );
        if ( branchStartingWithValve )
        {
            const auto segmentEndMd = initialChildMD + offset;
            auto       dummySegment = std::make_unique<RicMswSegment>( QString( "%1 segment" ).arg( outletValve->componentLabel() ),
                                                                 valveMD,
                                                                 segmentEndMd,
                                                                 valveTVD,
                                                                 RicMswTableDataTools::tvdFromMeasuredDepth( childWellPath, segmentEndMd ) );

            branchStartingWithValve->addSegment( std::move( dummySegment ) );

            return branchStartingWithValve;
        }
    }

    auto childBranch = std::make_unique<RicMswBranch>( childWellPath->name(), childWellPath, initialChildMD, initialChildTVD );

    return childBranch;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RimWellPath*> RicWellPathExportMswTableData::wellPathsWithTieIn( const RimWellPath* wellPath )
{
    std::vector<RimWellPath*> connectedWellPaths;
    {
        auto wellPaths = RimProject::current()->allWellPaths();
        for ( auto well : wellPaths )
        {
            if ( well && well->isEnabled() && well->wellPathTieIn() && well->wellPathTieIn()->parentWell() == wellPath )
            {
                connectedWellPaths.push_back( well );
            }
        }
    }

    return connectedWellPaths;
}

//--------------------------------------------------------------------------------------------------
/// Recursively build all WELSEGS/COMPSEGS/valve segments for one lateral (child well path)
/// and any of its own child laterals.
//--------------------------------------------------------------------------------------------------
void RicWellPathExportMswTableData::buildLateralSegments( RimEclipseCase*                 eclipseCase,
                                                          const RimWellPath*              wellPath,
                                                          const RigMainGrid*              mainGrid,
                                                          int                             outletSegNum,
                                                          CompletionType                  completionType,
                                                          const std::optional<QDateTime>& exportDate,
                                                          int&                            segmentNumber,
                                                          int&                            branchNumber,
                                                          RiaDefines::EclipseUnitSystem   unitSystem,
                                                          std::vector<RigMswSegment>&     result )
{
    auto mswParameters = wellPath->mswCompletionParameters();
    if ( !mswParameters ) return;

    const std::string infoType          = mswParameters->lengthAndDepth().text().toStdString();
    const double      tieInMD           = wellPath->wellPathTieIn()->tieInMeasuredDepth();
    const double      tieInTVD          = -wellPath->wellPathGeometry()->interpolatedPointAlongWellPath( tieInMD ).z();
    const std::string wellNameForExport = wellPath->completionSettings()->wellNameForExport().toStdString();

    int childOutletSeg = outletSegNum;

    // Optional ICV valve at the tie-in point
    const RimWellPathValve* outletValve = wellPath->wellPathTieIn()->outletValve();
    if ( outletValve && outletValve->isChecked() )
    {
        const bool isActive = !exportDate.has_value() || outletValve->isActiveOnDate( *exportDate );
        if ( isActive )
        {
            const double valveMD       = wellPath->wellPathTieIn()->branchValveMeasuredDepth();
            const double offset        = ( valveMD == tieInMD ) ? internal::VALVE_SEGMENT_LENGTH : 0.0;
            const double valveEndMD    = tieInMD + offset;
            const double valveStartTVD = RicMswTableDataTools::tvdFromMeasuredDepth( wellPath, valveMD );
            const double valveEndTVD   = RicMswTableDataTools::tvdFromMeasuredDepth( wellPath, valveEndMD );

            double length = 0.0;
            double depth  = 0.0;
            if ( infoType == "INC" )
            {
                length = valveEndMD - valveMD;
                depth  = valveEndTVD - valveStartTVD;
            }
            else
            {
                length = valveEndMD;
                depth  = valveEndTVD;
            }

            const int valveSegNum = segmentNumber++;
            const int valveBranch = ++branchNumber;

            RigMswSegment valveSeg;
            valveSeg.segmentNumber       = valveSegNum;
            valveSeg.branchNumber        = valveBranch;
            valveSeg.outletSegmentNumber = outletSegNum;
            valveSeg.length              = length;
            valveSeg.depth               = depth;
            valveSeg.diameter            = mswParameters->linerDiameter( unitSystem );
            valveSeg.roughness           = mswParameters->roughnessFactor( unitSystem );
            valveSeg.sourceWellName      = wellPath->name().toStdString();
            valveSeg.description = QString( "%1 valve for %2" ).arg( outletValve->componentLabel() ).arg( wellPath->name() ).toStdString();

            WsegvalvRow wv;
            wv.well               = wellNameForExport;
            wv.segmentNumber      = valveSegNum;
            wv.cv                 = outletValve->flowCoefficient();
            wv.area               = outletValve->area( unitSystem );
            wv.status             = outletValve->isOpen() ? "OPEN" : "SHUT";
            wv.description        = valveSeg.description;
            valveSeg.wsegvalvData = wv;

            result.push_back( std::move( valveSeg ) );
            childOutletSeg = valveSegNum;
        }
    }

    // Child main-bore segments
    auto cellIntersections     = generateCellSegments( eclipseCase, wellPath );
    auto filteredIntersections = filterIntersections( cellIntersections, tieInMD, wellPath->wellPathGeometry(), eclipseCase );

    const bool includePerforations = ( completionType & CompletionType::PERFORATIONS ) == CompletionType::PERFORATIONS;
    const std::vector<const RimPerforationInterval*> perforationIntervals =
        includePerforations ? wellPath->perforationIntervalCollection()->activePerforations() : std::vector<const RimPerforationInterval*>();

    std::set<const RimPerforationInterval*> valvedIntervals;
    for ( const auto* perf : perforationIntervals )
    {
        if ( exportDate.has_value() && !perf->isActiveOnDate( exportDate.value() ) ) continue;
        for ( const auto* valve : perf->descendantsIncludingThisOfType<RimWellPathValve>() )
        {
            if ( !valve->isChecked() ) continue;
            if ( exportDate.has_value() && !valve->isActiveOnDate( exportDate.value() ) ) continue;
            const auto t = valve->componentType();
            if ( t == RiaDefines::WellPathComponentType::ICD || t == RiaDefines::WellPathComponentType::ICV ||
                 t == RiaDefines::WellPathComponentType::AICD || t == RiaDefines::WellPathComponentType::SICD )
            {
                valvedIntervals.insert( perf );
                break;
            }
        }
    }

    std::vector<internal::CellSegmentEntry> childCellSegMap;
    const int                               childBoreNum = ++branchNumber;

    auto mainBoreSegs = internal::buildMainBoreSegmentsFromGeometry( wellPath,
                                                                     filteredIntersections,
                                                                     mainGrid,
                                                                     perforationIntervals,
                                                                     valvedIntervals,
                                                                     infoType,
                                                                     tieInMD,
                                                                     tieInTVD,
                                                                     childBoreNum,
                                                                     segmentNumber,
                                                                     childOutletSeg,
                                                                     mswParameters->maxSegmentLength(),
                                                                     {},
                                                                     exportDate,
                                                                     unitSystem,
                                                                     &childCellSegMap );
    result.insert( result.end(), std::make_move_iterator( mainBoreSegs.begin() ), std::make_move_iterator( mainBoreSegs.end() ) );

    auto valveSegs = internal::buildValveSegmentsFromGeometry( wellPath,
                                                               filteredIntersections,
                                                               mainGrid,
                                                               perforationIntervals,
                                                               childCellSegMap,
                                                               infoType,
                                                               wellNameForExport,
                                                               segmentNumber,
                                                               branchNumber,
                                                               mswParameters->maxSegmentLength(),
                                                               {},
                                                               exportDate,
                                                               unitSystem );
    result.insert( result.end(), std::make_move_iterator( valveSegs.begin() ), std::make_move_iterator( valveSegs.end() ) );

    if ( ( completionType & CompletionType::FRACTURES ) == CompletionType::FRACTURES )
    {
        auto fracSegs =
            internal::buildFractureSegmentsFromGeometry( eclipseCase, wellPath, mainGrid, childCellSegMap, infoType, segmentNumber, branchNumber );
        result.insert( result.end(), std::make_move_iterator( fracSegs.begin() ), std::make_move_iterator( fracSegs.end() ) );
    }

    if ( ( completionType & CompletionType::FISHBONES ) == CompletionType::FISHBONES )
    {
        auto fishSegs = internal::buildFishbonesSegmentsFromGeometry( eclipseCase,
                                                                      wellPath,
                                                                      mainGrid,
                                                                      filteredIntersections,
                                                                      childCellSegMap,
                                                                      infoType,
                                                                      wellNameForExport,
                                                                      segmentNumber,
                                                                      branchNumber,
                                                                      unitSystem );
        result.insert( result.end(), std::make_move_iterator( fishSegs.begin() ), std::make_move_iterator( fishSegs.end() ) );
    }

    // Recurse into grandchildren
    for ( auto* grandchild : wellPathsWithTieIn( wellPath ) )
    {
        const int grandchildOutlet = internal::findOutletSegmentForMD( childCellSegMap, grandchild->wellPathTieIn()->tieInMeasuredDepth() );
        buildLateralSegments( eclipseCase,
                              grandchild,
                              mainGrid,
                              grandchildOutlet,
                              completionType,
                              exportDate,
                              segmentNumber,
                              branchNumber,
                              unitSystem,
                              result );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::pair<double, double>
    RicWellPathExportMswTableData::calculateOverlapWithActiveCells( double                                           startMD,
                                                                    double                                           endMD,
                                                                    const std::vector<WellPathCellIntersectionInfo>& wellPathIntersections,
                                                                    const RigActiveCellInfo*                         activeCellInfo )
{
    for ( const WellPathCellIntersectionInfo& intersection : wellPathIntersections )
    {
        if ( intersection.globCellIndex < activeCellInfo->reservoirCellCount() && activeCellInfo->isActive( intersection.globCellIndex ) )
        {
            double overlapStart = std::max( startMD, intersection.startMD );
            double overlapEnd   = std::min( endMD, intersection.endMD );
            if ( overlapEnd > overlapStart )
            {
                return std::make_pair( overlapStart, overlapEnd );
            }
        }
    }
    return std::make_pair( 0.0, 0.0 );
}

//--------------------------------------------------------------------------------------------------
/// Build the flat RigMswSegment list from the already-collected table data and tree intersections.
/// Each WelsegsRow becomes one RigMswSegment. Cell intersections are looked up from the tree
/// (each RicMswSegment has its segment number assigned by collectWelsegsData). Valve data is
/// joined from the separate WSEGVALV/WSEGAICD/WSEGSICD table rows.
//--------------------------------------------------------------------------------------------------
void RicWellPathExportMswTableData::buildFlatMswSegmentList( const RicMswExportInfo& exportInfo, RigMswTableData& tableData )
{
    // Build map: segmentNumber -> cell intersections, from all segments in the tree
    std::map<int, std::vector<RigMswCellIntersection>> intersectionsBySegment;
    {
        auto allSegments = const_cast<RicMswBranch*>( exportInfo.mainBoreBranch() )->allSegmentsRecursively();
        for ( const auto* seg : allSegments )
        {
            int segNum = seg->segmentNumber();
            if ( segNum <= 0 ) continue;

            for ( const auto& inter : seg->intersections() )
            {
                RigMswCellIntersection ci;
                auto                   ijk = inter->gridLocalCellIJK().toOneBased();
                ci.i                       = ijk.i();
                ci.j                       = ijk.j();
                ci.k                       = ijk.k();
                ci.distanceStart           = seg->startMD();
                ci.distanceEnd             = seg->endMD();
                ci.gridName                = inter->gridName().toStdString();
                intersectionsBySegment[segNum].push_back( ci );
            }
        }
    }

    // Build maps: segmentNumber -> valve row (at most one type per segment)
    std::map<int, WsegvalvRow> valvBySegment;
    for ( const auto& row : tableData.wsegvalvData() )
        valvBySegment[row.segmentNumber] = row;

    std::map<int, WsegaicdRow> aicdBySegment;
    for ( const auto& row : tableData.wsegaicdData() )
        aicdBySegment[row.segment1] = row;

    std::map<int, WsegsicdRow> sicdBySegment;
    for ( const auto& row : tableData.wsegsicdData() )
        sicdBySegment[row.segment1] = row;

    // Convert each WelsegsRow to a RigMswSegment
    for ( const auto& row : tableData.welsegsData() )
    {
        RigMswSegment seg;
        seg.segmentNumber       = row.segment1;
        seg.branchNumber        = row.branch;
        seg.outletSegmentNumber = row.joinSegment;
        seg.length              = row.length;
        seg.depth               = row.depth;
        seg.diameter            = row.diameter;
        seg.roughness           = row.roughness;
        seg.description         = row.description;
        seg.sourceWellName      = row.sourceWellName;

        auto iIt = intersectionsBySegment.find( row.segment1 );
        if ( iIt != intersectionsBySegment.end() ) seg.intersections = iIt->second;

        auto vIt = valvBySegment.find( row.segment1 );
        if ( vIt != valvBySegment.end() ) seg.wsegvalvData = vIt->second;

        auto aIt = aicdBySegment.find( row.segment1 );
        if ( aIt != aicdBySegment.end() ) seg.wsegaicdData = aIt->second;

        auto sIt = sicdBySegment.find( row.segment1 );
        if ( sIt != sicdBySegment.end() ) seg.wsegsicdData = sIt->second;

        tableData.addMswSegment( std::move( seg ) );
    }
}

//--------------------------------------------------------------------------------------------------
/// Build the flat MSW export data directly from well-path geometry and Rim completion objects,
/// without building the RicMswBranch / RicMswItem tree.
///
/// Currently implemented: main-bore WELSEGS segments + perforation COMPSEGS entries.
/// TODO: valve completions (ICD/AICD/SICD/ICV), fishbones laterals, fractures, tie-in wells.
//--------------------------------------------------------------------------------------------------
RigMswFlatExportData RicWellPathExportMswTableData::buildMswFromGeometry( RimEclipseCase*    eclipseCase,
                                                                          const RimWellPath* wellPath,
                                                                          double             maxSegmentLength,
                                                                          const std::vector<std::pair<double, double>>& customSegmentIntervals,
                                                                          CompletionType                  completionType,
                                                                          const std::optional<QDateTime>& exportDate )
{
    auto mswParameters = wellPath->mswCompletionParameters();
    CVF_ASSERT( mswParameters );

    const RiaDefines::EclipseUnitSystem unitSystem = eclipseCase->eclipseCaseData()->unitsType();
    const RigMainGrid*                  mainGrid   = eclipseCase->mainGrid();
    const std::string                   infoType   = mswParameters->lengthAndDepth().text().toStdString();

    auto   cellIntersections = generateCellSegments( eclipseCase, wellPath );
    double initialMD         = computeIntitialMeasuredDepth( eclipseCase, wellPath, mswParameters, cellIntersections );
    double initialTVD        = -wellPath->wellPathGeometry()->interpolatedPointAlongWellPath( initialMD ).z();

    auto filteredIntersections = filterIntersections( cellIntersections, initialMD, wellPath->wellPathGeometry(), eclipseCase );

    WelsegsHeader header;
    header.well               = wellPath->completionSettings()->wellNameForExport().toStdString();
    header.topLength          = initialMD;
    header.topDepth           = initialTVD;
    header.infoType           = infoType;
    header.pressureComponents = mswParameters->pressureDrop().text().toStdString();

    const bool includePerforations = ( completionType & CompletionType::PERFORATIONS ) == CompletionType::PERFORATIONS;
    const std::vector<const RimPerforationInterval*> perforationIntervals =
        includePerforations ? wellPath->perforationIntervalCollection()->activePerforations() : std::vector<const RimPerforationInterval*>();

    // Determine which perforation intervals have at least one active, checked valve.
    // Main bore COMPSEGS are skipped for these; the valve builder will emit them instead.
    std::set<const RimPerforationInterval*> valvedIntervals;
    for ( const auto* perf : perforationIntervals )
    {
        if ( exportDate.has_value() && !perf->isActiveOnDate( exportDate.value() ) ) continue;
        for ( const auto* valve : perf->descendantsIncludingThisOfType<RimWellPathValve>() )
        {
            if ( !valve->isChecked() ) continue;
            if ( exportDate.has_value() && !valve->isActiveOnDate( exportDate.value() ) ) continue;
            const auto t = valve->componentType();
            if ( t == RiaDefines::WellPathComponentType::ICD || t == RiaDefines::WellPathComponentType::ICV ||
                 t == RiaDefines::WellPathComponentType::AICD || t == RiaDefines::WellPathComponentType::SICD )
            {
                valvedIntervals.insert( perf );
                break;
            }
        }
    }

    int                                     segmentNumber = 2; // Segment 1 is the implicit well heel.
    int                                     branchNumber  = 1; // Incremented for each valve branch.
    std::vector<internal::CellSegmentEntry> cellSegMap;

    auto mainBoreSegments = internal::buildMainBoreSegmentsFromGeometry( wellPath,
                                                                         filteredIntersections,
                                                                         mainGrid,
                                                                         perforationIntervals,
                                                                         valvedIntervals,
                                                                         infoType,
                                                                         initialMD,
                                                                         initialTVD,
                                                                         branchNumber,
                                                                         segmentNumber,
                                                                         1, // outlet = heel (segment 1)
                                                                         maxSegmentLength,
                                                                         customSegmentIntervals,
                                                                         exportDate,
                                                                         unitSystem,
                                                                         &cellSegMap );

    const std::string wellNameForExport = wellPath->completionSettings()->wellNameForExport().toStdString();

    auto valveSegments = internal::buildValveSegmentsFromGeometry( wellPath,
                                                                   filteredIntersections,
                                                                   mainGrid,
                                                                   perforationIntervals,
                                                                   cellSegMap,
                                                                   infoType,
                                                                   wellNameForExport,
                                                                   segmentNumber,
                                                                   branchNumber,
                                                                   maxSegmentLength,
                                                                   customSegmentIntervals,
                                                                   exportDate,
                                                                   unitSystem );

    const bool                 includeFractures = ( completionType & CompletionType::FRACTURES ) == CompletionType::FRACTURES;
    std::vector<RigMswSegment> fractureSegments;
    if ( includeFractures )
    {
        fractureSegments =
            internal::buildFractureSegmentsFromGeometry( eclipseCase, wellPath, mainGrid, cellSegMap, infoType, segmentNumber, branchNumber );
    }

    const bool                 includeFishbones = ( completionType & CompletionType::FISHBONES ) == CompletionType::FISHBONES;
    std::vector<RigMswSegment> fishbonesSegments;
    if ( includeFishbones )
    {
        fishbonesSegments = internal::buildFishbonesSegmentsFromGeometry( eclipseCase,
                                                                          wellPath,
                                                                          mainGrid,
                                                                          filteredIntersections,
                                                                          cellSegMap,
                                                                          infoType,
                                                                          wellNameForExport,
                                                                          segmentNumber,
                                                                          branchNumber,
                                                                          unitSystem );
    }

    // Tie-in child laterals (recursive)
    std::vector<RigMswSegment> lateralSegments;
    for ( auto* childWellPath : wellPathsWithTieIn( wellPath ) )
    {
        const int childOutlet = internal::findOutletSegmentForMD( cellSegMap, childWellPath->wellPathTieIn()->tieInMeasuredDepth() );
        buildLateralSegments( eclipseCase,
                              childWellPath,
                              mainGrid,
                              childOutlet,
                              completionType,
                              exportDate,
                              segmentNumber,
                              branchNumber,
                              unitSystem,
                              lateralSegments );
    }

    RigMswFlatExportData result;
    result.header   = header;
    result.segments = std::move( mainBoreSegments );
    result.segments.insert( result.segments.end(),
                            std::make_move_iterator( valveSegments.begin() ),
                            std::make_move_iterator( valveSegments.end() ) );
    result.segments.insert( result.segments.end(),
                            std::make_move_iterator( fractureSegments.begin() ),
                            std::make_move_iterator( fractureSegments.end() ) );
    result.segments.insert( result.segments.end(),
                            std::make_move_iterator( fishbonesSegments.begin() ),
                            std::make_move_iterator( fishbonesSegments.end() ) );
    result.segments.insert( result.segments.end(),
                            std::make_move_iterator( lateralSegments.begin() ),
                            std::make_move_iterator( lateralSegments.end() ) );
    return result;
}

//--------------------------------------------------------------------------------------------------
/// Build the flat export data by traversing the tree directly, creating RigMswSegment objects
/// inline — without calling the old collectWelsegsData / collectCompsegData functions.
/// AICD/SICD data is joined from the existing per-cell accumulation helpers (segment numbers
/// are already assigned during the recursive traversal above).
//--------------------------------------------------------------------------------------------------
RigMswFlatExportData RicWellPathExportMswTableData::buildFlatSegmentsDirect( RicMswExportInfo& exportInfo,
                                                                             double            maxSegmentLength,
                                                                             const std::vector<std::pair<double, double>>& customSegmentIntervals,
                                                                             bool                            exportCompletionsAfterMainBore,
                                                                             const std::optional<QDateTime>& exportDate )
{
    // Build WELSEGS header (same logic as collectWelsegsData)
    WelsegsHeader header;
    header.well      = exportInfo.mainBoreBranch()->wellPath()->completionSettings()->wellNameForExport().toStdString();
    header.topLength = exportInfo.mainBoreBranch()->startMD();
    header.topDepth  = exportInfo.mainBoreBranch()->startTVD();
    if ( exportInfo.topWellBoreVolume() != RicMswExportInfo::defaultDoubleValue() ) header.wellboreVolume = exportInfo.topWellBoreVolume();
    header.infoType           = exportInfo.lengthAndDepthText().toStdString();
    header.pressureComponents = exportInfo.pressureDropText().toStdString();

    std::vector<RigMswSegment> segments;
    int                        segmentNumber = 2; // Implicit segment 1 is the well heel.

    internal::buildFlatSegmentsRecursive( segments,
                                          exportInfo,
                                          exportInfo.mainBoreBranch(),
                                          &segmentNumber,
                                          header.topLength,
                                          header.topDepth,
                                          1,
                                          maxSegmentLength,
                                          customSegmentIntervals,
                                          exportCompletionsAfterMainBore,
                                          exportDate );

    // Join AICD data into flat segment list by segment number
    {
        std::map<size_t, std::vector<RicMswTableDataTools::AicdWsegvalveData>> aicdValveData;
        RicMswTableDataTools::generateWsegAicdTableRecursively( exportInfo, exportInfo.mainBoreBranch(), aicdValveData, exportDate );

        // Build segment-number -> WsegaicdRow using same accumulation logic as collectWsegAicdData
        auto setOptional = []( double value ) -> std::optional<double>
        {
            if ( value == RicMswExportInfo::defaultDoubleValue() ) return std::nullopt;
            return value;
        };

        std::map<int, WsegaicdRow> aicdBySegment;
        for ( auto& [globalCellIndex, aicdDataForSameCell] : aicdValveData )
        {
            if ( aicdDataForSameCell.empty() ) continue;

            double      accumulatedFlowScalingFactorDivisor = 0.0;
            QStringList comments;
            for ( const auto& aicdData : aicdDataForSameCell )
            {
                accumulatedFlowScalingFactorDivisor += 1.0 / aicdData.m_flowScalingFactor;
                comments.push_back( aicdData.m_comment );
            }

            WsegaicdRow row;
            row.description = comments.join( "; " ).toStdString();

            auto& first  = aicdDataForSameCell.front();
            row.well     = first.m_wellName.toStdString();
            row.segment1 = first.m_segmentNumber;
            row.segment2 = first.m_segmentNumber;

            std::array<double, AICD_NUM_PARAMS> values = first.m_values;
            row.strength                               = values[AICD_STRENGTH];
            row.length                                 = 1.0 / accumulatedFlowScalingFactorDivisor;
            row.densityCali                            = setOptional( values[AICD_DENSITY_CALIB_FLUID] );
            row.viscosityCali                          = setOptional( values[AICD_VISCOSITY_CALIB_FLUID] );
            row.criticalValue                          = setOptional( values[AICD_CRITICAL_WATER_IN_LIQUID_FRAC] );
            row.widthTrans                             = setOptional( values[AICD_EMULSION_VISC_TRANS_REGION] );
            row.maxViscRatio                           = setOptional( values[AICD_MAX_RATIO_EMULSION_VISC] );
            row.methodScalingFactor                    = 1;
            row.maxAbsRate                             = values[AICD_MAX_FLOW_RATE];
            row.flowRateExponent                       = values[AICD_VOL_FLOW_EXP];
            row.viscExponent                           = values[AICD_VISOSITY_FUNC_EXP];
            row.status                                 = first.m_isOpen ? "OPEN" : "SHUT";
            row.oilFlowFraction                        = setOptional( values[AICD_EXP_OIL_FRAC_DENSITY] );
            row.waterFlowFraction                      = setOptional( values[AICD_EXP_WATER_FRAC_DENSITY] );
            row.gasFlowFraction                        = setOptional( values[AICD_EXP_GAS_FRAC_DENSITY] );
            row.oilViscFraction                        = setOptional( values[AICD_EXP_OIL_FRAC_VISCOSITY] );
            row.waterViscFraction                      = setOptional( values[AICD_EXP_WATER_FRAC_VISCOSITY] );
            row.gasViscFraction                        = setOptional( values[AICD_EXP_GAS_FRAC_VISCOSITY] );

            aicdBySegment[row.segment1] = row;
        }

        for ( auto& seg : segments )
        {
            auto it = aicdBySegment.find( seg.segmentNumber );
            if ( it != aicdBySegment.end() ) seg.wsegaicdData = it->second;
        }
    }

    // Join SICD data into flat segment list by segment number
    {
        std::map<size_t, std::vector<RicMswTableDataTools::SicdWsegvalveData>> sicdValveData;
        RicMswTableDataTools::generateWsegSicdTableRecursively( exportInfo, exportInfo.mainBoreBranch(), sicdValveData, exportDate );

        auto setOptional = []( double value ) -> std::optional<double>
        {
            if ( value == RicMswExportInfo::defaultDoubleValue() ) return std::nullopt;
            return value;
        };

        std::map<int, WsegsicdRow> sicdBySegment;
        for ( auto& [globalCellIndex, sicdDataForSameCell] : sicdValveData )
        {
            if ( sicdDataForSameCell.empty() ) continue;

            double      accumulatedFlowScalingFactorDivisor = 0.0;
            QStringList comments;
            for ( const auto& sicdData : sicdDataForSameCell )
            {
                accumulatedFlowScalingFactorDivisor += 1.0 / sicdData.m_flowScalingFactor;
                comments.push_back( sicdData.m_comment );
            }

            WsegsicdRow row;
            row.description = comments.join( "; " ).toStdString();

            auto& first  = sicdDataForSameCell.front();
            row.well     = first.m_wellName.toStdString();
            row.segment1 = first.m_segmentNumber;
            row.segment2 = first.m_segmentNumber;

            std::array<double, SICD_NUM_PARAMS> values = first.m_values;
            row.strength                               = values[SICD_STRENGTH];
            row.length                                 = 1.0 / accumulatedFlowScalingFactorDivisor;
            row.densityCali                            = setOptional( values[SICD_CALIBRATION_DENSITY] );
            row.viscosityCali                          = setOptional( values[SICD_CALIBRATION_VISCOSITY] );
            row.criticalValue                          = setOptional( values[SICD_EML_CRT] );
            row.widthTrans                             = setOptional( values[SICD_EML_TRANS] );
            row.maxViscRatio                           = setOptional( values[SICD_EML_MAX] );
            row.methodScalingFactor                    = 1;
            row.maxAbsRate                             = values[SICD_MAX_CALIB_RATE];
            row.status                                 = first.m_isOpen ? "OPEN" : "SHUT";

            sicdBySegment[row.segment1] = row;
        }

        for ( auto& seg : segments )
        {
            auto it = sicdBySegment.find( seg.segmentNumber );
            if ( it != sicdBySegment.end() ) seg.wsegsicdData = it->second;
        }
    }

    // Join WSEGVALV data (ICD / ICV valves) into flat segment list by segment number
    {
        std::map<int, WsegvalvRow> wsegvalvBySegment;
        internal::collectStandaloneWsegvalvBySegmentRecursive( wsegvalvBySegment, exportInfo.mainBoreBranch(), header.well );
        internal::collectWsegvalvBySegmentRecursive( wsegvalvBySegment, exportInfo.mainBoreBranch(), header.well, exportDate );

        for ( auto& seg : segments )
        {
            auto it = wsegvalvBySegment.find( seg.segmentNumber );
            if ( it != wsegvalvBySegment.end() ) seg.wsegvalvData = it->second;
        }
    }

    RigMswFlatExportData result;
    result.header   = header;
    result.segments = std::move( segments );
    return result;
}
