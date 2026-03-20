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

#include "RicWellPathExportMswGeometryPath.h"

#include "RicWellPathExportMswTableData.h"

#include "RiaLogging.h"

#include "RicExportFractureCompletionsImpl.h"
#include "RicMswTableDataTools.h"

#include "CompletionsMsw/RigMswSegment.h"
#include "RigEclipseCaseData.h"
#include "RigGridBase.h"
#include "RigMainGrid.h"
#include "Well/RigWellLogExtractor.h"
#include "Well/RigWellPathIntersectionTools.h"

#include "RimEclipseCase.h"
#include "RimFishbones.h"
#include "RimFishbonesCollection.h"
#include "RimFractureTemplate.h"
#include "RimMswCompletionParameters.h"
#include "RimPerforationCollection.h"
#include "RimPerforationInterval.h"
#include "RimWellPath.h"
#include "RimWellPathCompletions.h"
#include "RimWellPathFracture.h"
#include "RimWellPathFractureCollection.h"
#include "RimWellPathTieIn.h"
#include "RimWellPathValve.h"

#include <algorithm>

namespace internal
{
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
                const double valveEndMD    = valveMD + RicMswTableDataTools::valveSegmentLength;
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
            const double startValveMd  = subEndMd - RicMswTableDataTools::valveSegmentLength;
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

} // namespace internal

namespace RicWellPathExportMswGeometryPath
{

using CompletionType = RicWellPathExportMswTableData::CompletionType;

//--------------------------------------------------------------------------------------------------
/// Recursively build all WELSEGS/COMPSEGS/valve segments for one lateral (child well path)
/// and any of its own child laterals.
//--------------------------------------------------------------------------------------------------
void buildLateralSegments( RimEclipseCase*                 eclipseCase,
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
            const double offset        = ( valveMD == tieInMD ) ? RicMswTableDataTools::valveSegmentLength : 0.0;
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
    auto cellIntersections     = RicWellPathExportMswTableData::generateCellSegments( eclipseCase, wellPath );
    auto filteredIntersections = RicWellPathExportMswTableData::filterIntersections( cellIntersections, tieInMD, wellPath->wellPathGeometry(), eclipseCase );

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
    for ( auto* grandchild : RicWellPathExportMswTableData::wellPathsWithTieIn( wellPath ) )
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
/// Build the flat MSW export data directly from well-path geometry and Rim completion objects,
/// without building the RicMswBranch / RicMswItem tree.
///
/// Currently implemented: main-bore WELSEGS segments + perforation COMPSEGS entries.
/// TODO: valve completions (ICD/AICD/SICD/ICV), fishbones laterals, fractures, tie-in wells.
//--------------------------------------------------------------------------------------------------
RigMswFlatExportData buildMswFromGeometry( RimEclipseCase*                               eclipseCase,
                                           const RimWellPath*                            wellPath,
                                           double                                        maxSegmentLength,
                                           const std::vector<std::pair<double, double>>& customSegmentIntervals,
                                           CompletionType                                completionType,
                                           const std::optional<QDateTime>&               exportDate )
{
    auto mswParameters = wellPath->mswCompletionParameters();
    CVF_ASSERT( mswParameters );

    const RiaDefines::EclipseUnitSystem unitSystem = eclipseCase->eclipseCaseData()->unitsType();
    const RigMainGrid*                  mainGrid   = eclipseCase->mainGrid();
    const std::string                   infoType   = mswParameters->lengthAndDepth().text().toStdString();

    auto   cellIntersections = RicWellPathExportMswTableData::generateCellSegments( eclipseCase, wellPath );
    double initialMD         = RicWellPathExportMswTableData::computeIntitialMeasuredDepth( eclipseCase, wellPath, mswParameters, cellIntersections );
    double initialTVD        = -wellPath->wellPathGeometry()->interpolatedPointAlongWellPath( initialMD ).z();

    auto filteredIntersections = RicWellPathExportMswTableData::filterIntersections( cellIntersections, initialMD, wellPath->wellPathGeometry(), eclipseCase );

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
    for ( auto* childWellPath : RicWellPathExportMswTableData::wellPathsWithTieIn( wellPath ) )
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

} // namespace RicWellPathExportMswGeometryPath
