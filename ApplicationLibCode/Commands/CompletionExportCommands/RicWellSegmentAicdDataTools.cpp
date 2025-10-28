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

#include "RicWellSegmentAicdDataTools.h"

#include "RicMswBranch.h"
#include "RicMswCompletions.h"
#include "RicMswExportInfo.h"
#include "RicMswSegment.h"
#include "RicWellSegmentAicdData.h"

#include "RifTextDataTableFormatter.h"

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RicWellSegmentAicdData> RicWellSegmentAicdDataTools::generateAicdData( RicMswExportInfo&            exportInfo,
                                                                                   gsl::not_null<RicMswBranch*> branch,
                                                                                   const QString&               wellNameForExport )
{
    std::map<size_t, std::vector<RicWellSegmentAicdData>> wsegaicdData;

    generateAicdDataRecursively( branch, wellNameForExport, wsegaicdData );

    std::vector<RicWellSegmentAicdData> aicdData;
    for ( const auto& [segmentNumber, aicds] : wsegaicdData )
    {
        for ( const auto& aicd : aicds )
        {
            aicdData.push_back( aicd );
        }
    }

    return aicdData;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellSegmentAicdDataTools::writeWsegaicdTable( RifTextDataTableFormatter&                 formatter,
                                                      const std::vector<RicWellSegmentAicdData>& aicdData,
                                                      const QString&                             wellName )
{
    if ( aicdData.empty() ) return;

    formatter.keyword( "WSEGAICD" );

    // Write well name header
    {
        std::vector<RifTextDataTableColumn> header = { RifTextDataTableColumn( "Name" ) };
        formatter.header( header );
        formatter.add( wellName );
        formatter.rowCompleted();
    }

    // Write data header
    {
        std::vector<RifTextDataTableColumn> headers = { RifTextDataTableColumn( "Seg No" ),
                                                        RifTextDataTableColumn( "Alpha" ),
                                                        RifTextDataTableColumn( "x" ),
                                                        RifTextDataTableColumn( "y" ),
                                                        RifTextDataTableColumn( "a" ),
                                                        RifTextDataTableColumn( "b" ),
                                                        RifTextDataTableColumn( "c" ),
                                                        RifTextDataTableColumn( "d" ),
                                                        RifTextDataTableColumn( "e" ),
                                                        RifTextDataTableColumn( "f" ),
                                                        RifTextDataTableColumn( "Rhocal" ),
                                                        RifTextDataTableColumn( "Viscal" ),
                                                        RifTextDataTableColumn( "DeviceOp" ),
                                                        RifTextDataTableColumn( "Length" ),
                                                        RifTextDataTableColumn( "ScalingFac" ) };
        formatter.header( headers );
    }

    // Write AICD data
    for ( const auto& aicd : aicdData )
    {
        if ( !aicd.comment().isEmpty() )
        {
            formatter.addOptionalComment( aicd.comment() );
        }

        formatter.add( aicd.segmentNumber() );

        // Add parameters (Alpha, x, y, a, b, c, d, e, f, Rhocal, Viscal)
        const auto& params = aicd.parameters();
        for ( size_t i = 0; i < params.size(); ++i )
        {
            if ( !RicWellSegmentAicdData::isDefaultValue( params[i] ) )
                formatter.add( params[i] );
            else
                formatter.addValueOrDefaultMarker( params[i], RicWellSegmentAicdData::defaultValue() );
        }

        // Device operation status
        formatter.add( aicd.deviceOpen() ? "OPEN" : "SHUT" );

        // Length
        if ( !RicWellSegmentAicdData::isDefaultValue( aicd.length() ) )
            formatter.add( aicd.length() );
        else
            formatter.addValueOrDefaultMarker( aicd.length(), RicWellSegmentAicdData::defaultValue() );

        // Flow scaling factor
        if ( !RicWellSegmentAicdData::isDefaultValue( aicd.flowScalingFactor() ) )
            formatter.add( aicd.flowScalingFactor() );
        else
            formatter.addValueOrDefaultMarker( aicd.flowScalingFactor(), RicWellSegmentAicdData::defaultValue() );

        formatter.rowCompleted();
    }

    formatter.tableCompleted();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellSegmentAicdDataTools::generateAicdDataRecursively( gsl::not_null<RicMswBranch*>                           branch,
                                                               const QString&                                         wellNameForExport,
                                                               std::map<size_t, std::vector<RicWellSegmentAicdData>>& wsegaicdData )
{
    for ( auto segment : branch->segments() )
    {
        for ( auto completion : segment->completions() )
        {
            auto aicdCompletion = dynamic_cast<RicMswPerforationAICD*>( completion );
            if ( !aicdCompletion ) continue;

            RicWellSegmentAicdData aicdData( wellNameForExport, segment->segmentNumber() );

            aicdData.setDeviceOpen( aicdCompletion->isOpen() );
            aicdData.setLength( aicdCompletion->length() );
            aicdData.setFlowScalingFactor( aicdCompletion->flowScalingFactor() );
            aicdData.setParameters( aicdCompletion->values() );
            aicdData.setComment( aicdCompletion->label() );

            wsegaicdData[segment->segmentNumber()].push_back( aicdData );
        }
    }

    // Process child branches
    for ( auto childBranch : branch->branches() )
    {
        generateAicdDataRecursively( childBranch, wellNameForExport, wsegaicdData );
    }
}