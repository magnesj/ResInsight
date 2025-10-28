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

#include "RicWellSegmentValveDataTools.h"

#include "RicMswBranch.h"
#include "RicMswCompletions.h"
#include "RicMswExportInfo.h"
#include "RicMswSegment.h"
#include "RicWellSegmentValveData.h"

#include "RifTextDataTableFormatter.h"

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RicWellSegmentValveData> RicWellSegmentValveDataTools::generateValveData( RicMswExportInfo&            exportInfo,
                                                                                      gsl::not_null<RicMswBranch*> branch,
                                                                                      const QString&               wellNameForExport )
{
    std::map<size_t, std::vector<RicWellSegmentValveData>> wsegvalveData;

    generateValveDataRecursively( branch, wellNameForExport, wsegvalveData );

    std::vector<RicWellSegmentValveData> valveData;
    for ( const auto& [segmentNumber, valves] : wsegvalveData )
    {
        for ( const auto& valve : valves )
        {
            valveData.push_back( valve );
        }
    }

    return valveData;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellSegmentValveDataTools::writeWsegvalvTable( RifTextDataTableFormatter&                  formatter,
                                                       const std::vector<RicWellSegmentValveData>& valveData,
                                                       const QString&                              wellName )
{
    if ( valveData.empty() ) return;

    formatter.keyword( "WSEGVALV" );

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
                                                        RifTextDataTableColumn( "Cv" ),
                                                        RifTextDataTableColumn( "Ac" ),
                                                        RifTextDataTableColumn( "L" ),
                                                        RifTextDataTableColumn( "Type" ) };
        formatter.header( headers );
    }

    // Write valve data
    for ( const auto& valve : valveData )
    {
        if ( !valve.comment().isEmpty() )
        {
            formatter.addOptionalComment( valve.comment() );
        }

        formatter.add( valve.segmentNumber() );

        if ( !RicWellSegmentValveData::isDefaultValue( valve.flowCoefficient() ) )
            formatter.add( valve.flowCoefficient() );
        else
            formatter.addValueOrDefaultMarker( valve.flowCoefficient(), RicWellSegmentValveData::defaultValue() );

        if ( !RicWellSegmentValveData::isDefaultValue( valve.area() ) )
            formatter.add( valve.area() );
        else
            formatter.addValueOrDefaultMarker( valve.area(), RicWellSegmentValveData::defaultValue() );

        if ( !RicWellSegmentValveData::isDefaultValue( valve.additionalPipeLength() ) )
            formatter.add( valve.additionalPipeLength() );
        else
            formatter.addValueOrDefaultMarker( valve.additionalPipeLength(), RicWellSegmentValveData::defaultValue() );

        if ( !valve.valveType().isEmpty() )
            formatter.add( valve.valveType() );
        else
            formatter.addValueOrDefaultMarker( 0.0, RicWellSegmentValveData::defaultValue() );

        formatter.rowCompleted();
    }

    formatter.tableCompleted();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellSegmentValveDataTools::generateValveDataRecursively( gsl::not_null<RicMswBranch*>                            branch,
                                                                 const QString&                                          wellNameForExport,
                                                                 std::map<size_t, std::vector<RicWellSegmentValveData>>& wsegvalveData )
{
    for ( auto segment : branch->segments() )
    {
        for ( auto completion : segment->completions() )
        {
            auto wsegValve = dynamic_cast<RicMswWsegValve*>( completion );
            if ( !wsegValve ) continue;

            RicWellSegmentValveData valveData( wellNameForExport, segment->segmentNumber() );

            valveData.setFlowCoefficient( wsegValve->flowCoefficient() );
            valveData.setArea( wsegValve->area() );
            valveData.setComment( wsegValve->label() );

            wsegvalveData[segment->segmentNumber()].push_back( valveData );
        }
    }

    // Process child branches
    for ( auto childBranch : branch->branches() )
    {
        generateValveDataRecursively( childBranch, wellNameForExport, wsegvalveData );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RicWellSegmentValveDataTools::getValveTypeString( int valveType )
{
    // Based on Eclipse documentation for WSEGVALV valve types
    switch ( valveType )
    {
        case 1:
            return "ICD";
        case 2:
            return "AICD";
        case 3:
            return "ICV";
        case 4:
            return "DAR";
        default:
            return QString();
    }
}