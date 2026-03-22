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

#include "RicWellPathExportMswBuildSegments.h"
#include "RicWellPathExportMswTableData.h"

#include "CompletionsMsw/RigMswTableData.h"

#include "RimEclipseCase.h"
#include "RimMswCompletionParameters.h"
#include "RimPerforationCollection.h"
#include "RimPerforationInterval.h"
#include "RimWellPath.h"
#include "RimWellPathTieIn.h"
#include "RimWellPathValve.h"

#include <algorithm>

namespace RicWellPathExportMswGeometryPath
{

using CompletionType = RicWellPathExportMswTableData::CompletionType;

//--------------------------------------------------------------------------------------------------
/// Recursively build all WELSEGS/COMPSEGS/valve segments for one lateral (child well path)
/// and any of its own child laterals.
//--------------------------------------------------------------------------------------------------
std::vector<RigMswSegment> buildLateralSegments( RimEclipseCase*                 eclipseCase,
                                                 const RimWellPath*              wellPath,
                                                 const RigMainGrid*              mainGrid,
                                                 int                             outletSegNum,
                                                 CompletionType                  completionType,
                                                 const std::optional<QDateTime>& exportDate,
                                                 int&                            segmentNumber,
                                                 int&                            branchNumber,
                                                 RiaDefines::EclipseUnitSystem   unitSystem )
{
    std::vector<RigMswSegment> result;
    auto                       mswParameters = wellPath->mswCompletionParameters();
    if ( !mswParameters ) return result;

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
    auto cellIntersections = RicWellPathExportMswTableData::generateCellSegments( eclipseCase, wellPath );
    auto filteredIntersections =
        RicWellPathExportMswTableData::filterIntersections( cellIntersections, tieInMD, wellPath->wellPathGeometry(), eclipseCase );

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

    std::vector<RicWellPathExportMswBuildSegments::CellSegmentEntry> childCellSegMap;
    const int                                                        childBoreNum = ++branchNumber;

    auto mainBoreSegs = RicWellPathExportMswBuildSegments::buildMainBoreSegmentsFromGeometry( wellPath,
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

    auto valveSegs = RicWellPathExportMswBuildSegments::buildValveSegmentsFromGeometry( wellPath,
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
        auto fracSegs = RicWellPathExportMswBuildSegments::buildFractureSegmentsFromGeometry( eclipseCase,
                                                                                              wellPath,
                                                                                              mainGrid,
                                                                                              childCellSegMap,
                                                                                              infoType,
                                                                                              segmentNumber,
                                                                                              branchNumber );
        result.insert( result.end(), std::make_move_iterator( fracSegs.begin() ), std::make_move_iterator( fracSegs.end() ) );
    }

    if ( ( completionType & CompletionType::FISHBONES ) == CompletionType::FISHBONES )
    {
        auto fishSegs = RicWellPathExportMswBuildSegments::buildFishbonesSegmentsFromGeometry( eclipseCase,
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
        const int grandchildOutlet =
            RicWellPathExportMswBuildSegments::findOutletSegmentForMD( childCellSegMap, grandchild->wellPathTieIn()->tieInMeasuredDepth() );
        auto grandchildSegs =
            buildLateralSegments( eclipseCase, grandchild, mainGrid, grandchildOutlet, completionType, exportDate, segmentNumber, branchNumber, unitSystem );
        result.insert( result.end(), std::make_move_iterator( grandchildSegs.begin() ), std::make_move_iterator( grandchildSegs.end() ) );
    }

    return result;
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

    auto cellIntersections = RicWellPathExportMswTableData::generateCellSegments( eclipseCase, wellPath );
    double initialMD = RicWellPathExportMswTableData::computeIntitialMeasuredDepth( eclipseCase, wellPath, mswParameters, cellIntersections );
    double initialTVD = -wellPath->wellPathGeometry()->interpolatedPointAlongWellPath( initialMD ).z();

    auto filteredIntersections =
        RicWellPathExportMswTableData::filterIntersections( cellIntersections, initialMD, wellPath->wellPathGeometry(), eclipseCase );

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

    int                                                              segmentNumber = 2; // Segment 1 is the implicit well heel.
    int                                                              branchNumber  = 1; // Incremented for each valve branch.
    std::vector<RicWellPathExportMswBuildSegments::CellSegmentEntry> cellSegMap;

    auto mainBoreSegments = RicWellPathExportMswBuildSegments::buildMainBoreSegmentsFromGeometry( wellPath,
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

    auto valveSegments = RicWellPathExportMswBuildSegments::buildValveSegmentsFromGeometry( wellPath,
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
        fractureSegments = RicWellPathExportMswBuildSegments::buildFractureSegmentsFromGeometry( eclipseCase,
                                                                                                 wellPath,
                                                                                                 mainGrid,
                                                                                                 cellSegMap,
                                                                                                 infoType,
                                                                                                 segmentNumber,
                                                                                                 branchNumber );
    }

    const bool                 includeFishbones = ( completionType & CompletionType::FISHBONES ) == CompletionType::FISHBONES;
    std::vector<RigMswSegment> fishbonesSegments;
    if ( includeFishbones )
    {
        fishbonesSegments = RicWellPathExportMswBuildSegments::buildFishbonesSegmentsFromGeometry( eclipseCase,
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
        const int childOutlet =
            RicWellPathExportMswBuildSegments::findOutletSegmentForMD( cellSegMap, childWellPath->wellPathTieIn()->tieInMeasuredDepth() );
        auto childSegs =
            buildLateralSegments( eclipseCase, childWellPath, mainGrid, childOutlet, completionType, exportDate, segmentNumber, branchNumber, unitSystem );
        lateralSegments.insert( lateralSegments.end(), std::make_move_iterator( childSegs.begin() ), std::make_move_iterator( childSegs.end() ) );
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
/// Populate RigMswTableData from a pre-built flat segment list.
/// This replaces the recursive tree-based collection functions (collectWelsegsData etc.) with
/// simple iteration over RigMswSegment objects.
//--------------------------------------------------------------------------------------------------
RigMswTableData collectDataFromFlatList( const RigMswFlatExportData& exportData, RiaDefines::EclipseUnitSystem unitSystem )
{
    RigMswTableData tableData( exportData.header.well, unitSystem );
    tableData.setWelsegsHeader( exportData.header );

    for ( const auto& seg : exportData.segments )
    {
        // WELSEGS row
        WelsegsRow welsegsRow;
        welsegsRow.segment1       = seg.segmentNumber;
        welsegsRow.segment2       = seg.segmentNumber;
        welsegsRow.branch         = seg.branchNumber;
        welsegsRow.joinSegment    = seg.outletSegmentNumber;
        welsegsRow.length         = seg.length;
        welsegsRow.depth          = seg.depth;
        welsegsRow.diameter       = seg.diameter;
        welsegsRow.roughness      = seg.roughness;
        welsegsRow.description    = seg.description;
        welsegsRow.sourceWellName = seg.sourceWellName;
        tableData.addWelsegsRow( welsegsRow );

        // COMPSEGS rows
        for ( const auto& inter : seg.intersections )
        {
            CompsegsRow compRow;
            compRow.i             = inter.i;
            compRow.j             = inter.j;
            compRow.k             = inter.k;
            compRow.branch        = seg.branchNumber;
            compRow.distanceStart = inter.distanceStart;
            compRow.distanceEnd   = inter.distanceEnd;
            compRow.gridName      = inter.gridName;
            tableData.addCompsegsRow( compRow );
        }

        // WSEGVALV row
        if ( seg.wsegvalvData ) tableData.addWsegvalvRow( *seg.wsegvalvData );

        // WSEGAICD row
        if ( seg.wsegaicdData ) tableData.addWsegaicdRow( *seg.wsegaicdData );

        // WSEGSICD row
        if ( seg.wsegsicdData ) tableData.addWsegsicdRow( *seg.wsegsicdData );

        // Also store the segment in the flat list on tableData
        tableData.addMswSegment( seg );
    }
    return tableData;
}

} // namespace RicWellPathExportMswGeometryPath
