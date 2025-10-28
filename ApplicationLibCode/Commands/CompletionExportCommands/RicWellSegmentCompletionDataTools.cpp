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

#include "RicWellSegmentCompletionDataTools.h"

#include "RicWellSegmentCompletionData.h"
#include "RicMswExportInfo.h"
#include "RicMswBranch.h"
#include "RicMswSegment.h"
#include "RicMswCompletions.h"

#include "RifTextDataTableFormatter.h"

#include "RimWellPathCompletionSettings.h"
#include "RimWellPath.h"

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RicWellSegmentCompletionData> RicWellSegmentCompletionDataTools::generateCompletionData( RicMswExportInfo&                                  exportInfo,
                                                                                        gsl::not_null<const RicMswBranch*>                 branch,
                                                                                        bool                                               exportSubGridIntersections,
                                                                                        const std::set<RigCompletionData::CompletionType>& exportCompletionTypes )
{
    std::vector<RicWellSegmentCompletionData> completionData;
    std::set<size_t>                   intersectedCells;

    generateCompletionDataRecursively( exportInfo, branch, exportSubGridIntersections, exportCompletionTypes, completionData, intersectedCells );

    return completionData;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellSegmentCompletionDataTools::writeCompsegsTable( RifTextDataTableFormatter&                formatter,
                                                     const std::vector<RicWellSegmentCompletionData>&  completionData,
                                                     const QString&                             wellName,
                                                     RigCompletionData::CompletionType          completionType,
                                                     bool                                       isLgr )
{
    if ( completionData.empty() ) return;

    if ( isLgr )
    {
        formatter.keyword( "COMPSEGL" );
    }
    else
    {
        formatter.keyword( "COMPSEGS" );
    }

    QString comment = getCompletionTypeComment( completionType );
    if ( !comment.isEmpty() )
    {
        formatter.comment( comment );
    }

    // Write well name header
    {
        std::vector<RifTextDataTableColumn> header = { RifTextDataTableColumn( "Name" ) };
        formatter.header( header );
        formatter.add( wellName );
        formatter.rowCompleted();
    }

    // Write data header
    {
        std::vector<RifTextDataTableColumn> headers;
        
        if ( isLgr )
        {
            headers.push_back( RifTextDataTableColumn( "Grid" ) );
        }
        
        headers.insert( headers.end(), {
            RifTextDataTableColumn( "I" ),
            RifTextDataTableColumn( "J" ),
            RifTextDataTableColumn( "K" ),
            RifTextDataTableColumn( "Branch no" ),
            RifTextDataTableColumn( "Start Length" ),
            RifTextDataTableColumn( "End Length" ),
            RifTextDataTableColumn( "Dir Pen" ),
            RifTextDataTableColumn( "End Range" ),
            RifTextDataTableColumn( "Connection Depth" )
        });
        
        formatter.header( headers );
    }

    // Write completion data
    for ( const auto& completion : completionData )
    {
        if ( !completion.comment().isEmpty() )
        {
            formatter.addOptionalComment( completion.comment() );
        }

        if ( isLgr )
        {
            formatter.add( completion.gridName() );
        }

        formatter.add( completion.i() + 1 ); // Convert to 1-based indexing
        formatter.add( completion.j() + 1 ); // Convert to 1-based indexing  
        formatter.add( completion.k() + 1 ); // Convert to 1-based indexing
        formatter.add( completion.branchNumber() );
        formatter.add( completion.startLength() );
        formatter.add( completion.endLength() );
        
        if ( !completion.directionPenetration().isEmpty() )
            formatter.add( completion.directionPenetration() );
        else
            formatter.addValueOrDefaultMarker( 0.0, RicWellSegmentCompletionData::defaultValue() );
            
        if ( !RicWellSegmentCompletionData::isDefaultValue( completion.endRange() ) )
            formatter.add( completion.endRange() );
        else
            formatter.addValueOrDefaultMarker( completion.endRange(), RicWellSegmentCompletionData::defaultValue() );
            
        if ( !RicWellSegmentCompletionData::isDefaultValue( completion.connectionDepth() ) )
            formatter.add( completion.connectionDepth() );
        else
            formatter.addValueOrDefaultMarker( completion.connectionDepth(), RicWellSegmentCompletionData::defaultValue() );

        formatter.rowCompleted();
    }

    formatter.tableCompleted();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellSegmentCompletionDataTools::writeCompseglTable( RifTextDataTableFormatter&                formatter,
                                                     const std::vector<RicWellSegmentCompletionData>&  completionData,
                                                     const QString&                             wellName,
                                                     RigCompletionData::CompletionType          completionType )
{
    // Filter for LGR data only
    std::vector<RicWellSegmentCompletionData> lgrData;
    for ( const auto& completion : completionData )
    {
        if ( completion.isLgr() )
        {
            lgrData.push_back( completion );
        }
    }
    
    writeCompsegsTable( formatter, lgrData, wellName, completionType, true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellSegmentCompletionDataTools::generateCompletionDataRecursively( RicMswExportInfo&                                  exportInfo,
                                                                    gsl::not_null<const RicMswBranch*>                 branch,
                                                                    bool                                               exportSubGridIntersections,
                                                                    const std::set<RigCompletionData::CompletionType>& exportCompletionTypes,
                                                                    std::vector<RicWellSegmentCompletionData>&                completionData,
                                                                    std::set<size_t>&                                  intersectedCells )
{
    QString wellName = exportInfo.mainBoreBranch()->wellPath()->completionSettings()->wellNameForExport();

    for ( auto segment : branch->segments() )
    {
        auto completion = dynamic_cast<const RicMswCompletion*>( branch.get() );

        for ( auto intersection : segment->intersections() )
        {
            bool isSubGridIntersection = !intersection->gridName().isEmpty();
            if ( isSubGridIntersection != exportSubGridIntersections ) continue;

            double startLength = segment->startMD();
            double endLength   = segment->endMD();

            if ( completion )
            {
                bool isPerforationValve = completion->completionType() == RigCompletionData::CompletionType::PERFORATION_ICD ||
                                          completion->completionType() == RigCompletionData::CompletionType::PERFORATION_AICD ||
                                          completion->completionType() == RigCompletionData::CompletionType::PERFORATION_ICV;

                if ( isPerforationValve )
                {
                    startLength = segment->startMD();
                    endLength   = segment->endMD();
                }
            }

            size_t globalCellIndex = intersection->globalCellIndex();

            // Check if the cell is already reported
            if ( !intersectedCells.count( globalCellIndex ) )
            {
                RicWellSegmentCompletionData compData( wellName, intersection->gridName() );
                
                cvf::Vec3st ijk = intersection->gridLocalCellIJK();
                compData.setGridCell( static_cast<int>( ijk.x() ), static_cast<int>( ijk.y() ), static_cast<int>( ijk.z() ) );

                int branchNumber = -1;
                if ( completion ) branchNumber = completion->branchNumber();
                compData.setBranchNumber( branchNumber );

                compData.setSegmentRange( startLength, endLength );

                completionData.push_back( compData );
                intersectedCells.insert( globalCellIndex );
            }
        }

        // Report connected completions after the intersection on current segment has been reported
        for ( auto segmentCompletion : segment->completions() )
        {
            if ( segmentCompletion->segments().empty() || !exportCompletionTypes.count( segmentCompletion->completionType() ) ) continue;

            generateCompletionDataRecursively( exportInfo, segmentCompletion, exportSubGridIntersections, exportCompletionTypes, completionData, intersectedCells );
        }
    }

    for ( auto childBranch : branch->branches() )
    {
        generateCompletionDataRecursively( exportInfo, childBranch, exportSubGridIntersections, exportCompletionTypes, completionData, intersectedCells );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RicWellSegmentCompletionDataTools::getCompletionTypeComment( RigCompletionData::CompletionType completionType )
{
    switch ( completionType )
    {
        case RigCompletionData::CompletionType::FISHBONES_ICD:
            return "Fishbones";
        case RigCompletionData::CompletionType::FRACTURE:
            return "Fractures";
        case RigCompletionData::CompletionType::PERFORATION:
        case RigCompletionData::CompletionType::PERFORATION_ICD:
        case RigCompletionData::CompletionType::PERFORATION_AICD:
        case RigCompletionData::CompletionType::PERFORATION_ICV:
            return "Perforations";
        default:
            return QString();
    }
}