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

#include "RicMswDataFormatter.h"

#include "RicMswExportInfo.h"
#include "RicMswTableData.h"
#include "RicMswUnifiedData.h"
#include "RifTextDataTableFormatter.h"

//--------------------------------------------------------------------------------------------------
/// Format WELSEGS table for a single well
//--------------------------------------------------------------------------------------------------
void RicMswDataFormatter::formatWelsegsTable( RifTextDataTableFormatter& formatter, const RicMswTableData& tableData )
{
    if ( !tableData.hasWelsegsData() ) return;

    const auto& welsegsHeader = tableData.welsegsHeader();
    if ( welsegsHeader.has_value() )
    {
        formatter.keyword( "WELSEGS" );
        std::vector<RifTextDataTableColumn> tableHeader = {
            RifTextDataTableColumn( "Well" ),
            RifTextDataTableColumn( "Dep 1" ),
            RifTextDataTableColumn( "Tlen 1" ),
            RifTextDataTableColumn( "Vol 1" ),
            RifTextDataTableColumn( "Len&Dep" ),
            RifTextDataTableColumn( "PresDrop" ),
        };
        formatter.header( tableHeader );

        // Write header row
        formatter.add( welsegsHeader->wellName );
        formatter.add( welsegsHeader->topMD );
        formatter.add( welsegsHeader->topTVD );
        if ( welsegsHeader->volume.has_value() )
            formatter.add( welsegsHeader->volume.value() );
        else
            formatter.add( RicMswExportInfo::defaultDoubleValue() );
        formatter.add( welsegsHeader->lengthAndDepthText );
        formatter.add( welsegsHeader->pressureDropText );
        formatter.rowCompleted();

        // Column headers for segment data
        std::vector<RifTextDataTableColumn> segmentHeader = { RifTextDataTableColumn( "Seg No" ),
                                                              RifTextDataTableColumn( "Seg No" ),
                                                              RifTextDataTableColumn( "Branch No" ),
                                                              RifTextDataTableColumn( "Out" ),
                                                              RifTextDataTableColumn( "Len" ),
                                                              RifTextDataTableColumn( "Dep" ),
                                                              RifTextDataTableColumn( "Diam" ),
                                                              RifTextDataTableColumn( "Rough" ) };
        formatter.header( segmentHeader );
    }

    // Write segment data
    for ( const auto& row : tableData.welsegsData() )
    {
        formatter.add( row.segmentNumber );
        formatter.add( row.segmentNumber );
        formatter.add( row.branchNumber );
        formatter.add( row.outletSegmentNumber );
        formatter.add( row.startMD );

        formatter.addOptionalValue( row.startTVD, RicMswExportInfo::defaultDoubleValue() );
        formatter.addOptionalValue( row.diameter, RicMswExportInfo::defaultDoubleValue() );
        formatter.addOptionalValue( row.roughness, RicMswExportInfo::defaultDoubleValue() );

        formatter.rowCompleted();
    }

    formatter.tableCompleted();
}

//--------------------------------------------------------------------------------------------------
/// Format COMPSEGS table for a single well
//--------------------------------------------------------------------------------------------------
void RicMswDataFormatter::formatCompsegsTable( RifTextDataTableFormatter& formatter, const RicMswTableData& tableData, bool isLgrData )
{
    std::vector<CompsegsRow> rows;
    if ( isLgrData )
    {
        rows = tableData.lgrCompsegsData();
    }
    else
    {
        rows = tableData.mainGridCompsegsData();
    }

    if ( rows.empty() ) return;

    formatter.keyword( "COMPSEGS" );

    if ( isLgrData )
    {
        std::vector<RifTextDataTableColumn> lgrCompsegsHeader = { RifTextDataTableColumn( "Grid" ),
                                                                  RifTextDataTableColumn( "I" ),
                                                                  RifTextDataTableColumn( "J" ),
                                                                  RifTextDataTableColumn( "K" ),
                                                                  RifTextDataTableColumn( "Branch no" ),
                                                                  RifTextDataTableColumn( "Start Length" ),
                                                                  RifTextDataTableColumn( "End Length" ),
                                                                  RifTextDataTableColumn( "Dir Pen" ),
                                                                  RifTextDataTableColumn( "End Range" ),
                                                                  RifTextDataTableColumn( "Connection Factor" ),
                                                                  RifTextDataTableColumn( "Diameter" ),
                                                                  RifTextDataTableColumn( "Skin Factor" ) };
        formatter.header( lgrCompsegsHeader );
    }
    else
    {
        std::vector<RifTextDataTableColumn> compsegsHeader = { RifTextDataTableColumn( "I" ),
                                                               RifTextDataTableColumn( "J" ),
                                                               RifTextDataTableColumn( "K" ),
                                                               RifTextDataTableColumn( "Branch no" ),
                                                               RifTextDataTableColumn( "Start Length" ),
                                                               RifTextDataTableColumn( "End Length" ),
                                                               RifTextDataTableColumn( "Dir Pen" ),
                                                               RifTextDataTableColumn( "End Range" ),
                                                               RifTextDataTableColumn( "Connection Factor" ),
                                                               RifTextDataTableColumn( "Diameter" ),
                                                               RifTextDataTableColumn( "Skin Factor" ) };
        formatter.header( compsegsHeader );
    }

    for ( const auto& row : rows )
    {
        if ( isLgrData )
        {
            formatter.add( row.gridName );
        }

        formatter.add( row.cellI );
        formatter.add( row.cellJ );
        formatter.add( row.cellK );
        formatter.add( row.branchNumber );
        formatter.add( row.startLength );
        formatter.add( row.endLength );

        formatter.addOptionalValue( row.direction, RicMswExportInfo::defaultDoubleValue() );
        formatter.addOptionalValue( row.endRange, RicMswExportInfo::defaultDoubleValue() );
        formatter.addOptionalValue( row.connectionFactor, RicMswExportInfo::defaultDoubleValue() );
        formatter.addOptionalValue( row.diameter, RicMswExportInfo::defaultDoubleValue() );
        formatter.addOptionalValue( row.skinFactor, RicMswExportInfo::defaultDoubleValue() );

        formatter.rowCompleted();
    }

    formatter.tableCompleted();
}

//--------------------------------------------------------------------------------------------------
/// Format WSEGVALV table for a single well
//--------------------------------------------------------------------------------------------------
void RicMswDataFormatter::formatWsegvalvTable( RifTextDataTableFormatter& formatter, const RicMswTableData& tableData )
{
    if ( !tableData.hasWsegvalvData() ) return;

    formatter.keyword( "WSEGVALV" );
    std::vector<RifTextDataTableColumn> wsegvalvHeader = { RifTextDataTableColumn( "Well" ),
                                                           RifTextDataTableColumn( "Seg No" ),
                                                           RifTextDataTableColumn( "Cv" ),
                                                           RifTextDataTableColumn( "Ac" ),
                                                           RifTextDataTableColumn( "Device Type" ),
                                                           RifTextDataTableColumn( "Param 1" ),
                                                           RifTextDataTableColumn( "Param 2" ) };
    formatter.header( wsegvalvHeader );

    for ( const auto& row : tableData.wsegvalvData() )
    {
        formatter.add( row.wellName );
        formatter.add( row.segmentNumber );
        formatter.add( row.flowCoefficient );
        formatter.addOptionalValue( row.area, RicMswExportInfo::defaultDoubleValue() );
        formatter.add( row.deviceType );
        formatter.addOptionalValue( row.additionalParameter1, RicMswExportInfo::defaultDoubleValue() );
        formatter.addOptionalValue( row.additionalParameter2, RicMswExportInfo::defaultDoubleValue() );

        formatter.rowCompleted();
    }

    formatter.tableCompleted();
}

//--------------------------------------------------------------------------------------------------
/// Format WSEGAICD table for a single well
//--------------------------------------------------------------------------------------------------
void RicMswDataFormatter::formatWsegaicdTable( RifTextDataTableFormatter& formatter, const RicMswTableData& tableData )
{
    if ( !tableData.hasWsegaicdData() ) return;

    formatter.keyword( "WSEGAICD" );
    std::vector<RifTextDataTableColumn> wsegaicdHeader = { RifTextDataTableColumn( "Well" ),
                                                           RifTextDataTableColumn( "Seg No" ),
                                                           RifTextDataTableColumn( "Flow Coeff" ),
                                                           RifTextDataTableColumn( "Area" ),
                                                           RifTextDataTableColumn( "Oil Visc" ),
                                                           RifTextDataTableColumn( "Water Visc" ),
                                                           RifTextDataTableColumn( "Gas Visc" ),
                                                           RifTextDataTableColumn( "Device Type" ) };
    formatter.header( wsegaicdHeader );

    for ( const auto& row : tableData.wsegaicdData() )
    {
        formatter.add( row.wellName );
        formatter.add( row.segmentNumber );
        formatter.add( row.flowCoefficient );
        formatter.addOptionalValue( row.area, RicMswExportInfo::defaultDoubleValue() );
        formatter.addOptionalValue( row.oilViscosityParameter, RicMswExportInfo::defaultDoubleValue() );
        formatter.addOptionalValue( row.waterViscosityParameter, RicMswExportInfo::defaultDoubleValue() );
        formatter.addOptionalValue( row.gasViscosityParameter, RicMswExportInfo::defaultDoubleValue() );
        formatter.add( row.deviceType );

        formatter.rowCompleted();
    }

    formatter.tableCompleted();
}

//--------------------------------------------------------------------------------------------------
/// Format unified WELSEGS table for multiple wells
//--------------------------------------------------------------------------------------------------
void RicMswDataFormatter::formatUnifiedWelsegsTable( RifTextDataTableFormatter& formatter, const RicMswUnifiedData& unifiedData )
{
    if ( unifiedData.isEmpty() ) return;

    auto headers = unifiedData.getAllWelsegsHeaders();
    auto rows    = unifiedData.getAllWelsegsRows();

    if ( headers.empty() && rows.empty() ) return;

    formatter.keyword( "WELSEGS" );
    std::vector<RifTextDataTableColumn> unifiedWelsegsHeader = { RifTextDataTableColumn( "Well" ),
                                                                 RifTextDataTableColumn( "Dep 1" ),
                                                                 RifTextDataTableColumn( "Tlen 1" ),
                                                                 RifTextDataTableColumn( "Vol 1" ),
                                                                 RifTextDataTableColumn( "Len&Dep" ),
                                                                 RifTextDataTableColumn( "PresDrop" ) };
    formatter.header( unifiedWelsegsHeader );

    // Write headers for all wells
    for ( const auto& header : headers )
    {
        formatter.add( header.wellName );
        formatter.add( header.topMD );
        formatter.add( header.topTVD );
        if ( header.volume.has_value() )
            formatter.add( header.volume.value() );
        else
            formatter.add( RicMswExportInfo::defaultDoubleValue() );
        formatter.add( header.lengthAndDepthText );
        formatter.add( header.pressureDropText );
        formatter.rowCompleted();
    }

    // Column headers for segment data
    std::vector<RifTextDataTableColumn> unifiedSegmentHeader = { RifTextDataTableColumn( "Seg No" ),
                                                                 RifTextDataTableColumn( "Seg No" ),
                                                                 RifTextDataTableColumn( "Branch No" ),
                                                                 RifTextDataTableColumn( "Out" ),
                                                                 RifTextDataTableColumn( "Len" ),
                                                                 RifTextDataTableColumn( "Dep" ),
                                                                 RifTextDataTableColumn( "Diam" ),
                                                                 RifTextDataTableColumn( "Rough" ) };
    formatter.header( unifiedSegmentHeader );

    // Write all segment data
    for ( const auto& row : rows )
    {
        formatter.add( row.segmentNumber );
        formatter.add( row.segmentNumber );
        formatter.add( row.branchNumber );
        formatter.add( row.outletSegmentNumber );
        formatter.add( row.startMD );

        formatter.addOptionalValue( row.startTVD, RicMswExportInfo::defaultDoubleValue() );
        formatter.addOptionalValue( row.diameter, RicMswExportInfo::defaultDoubleValue() );
        formatter.addOptionalValue( row.roughness, RicMswExportInfo::defaultDoubleValue() );

        formatter.rowCompleted();
    }

    formatter.tableCompleted();
}

//--------------------------------------------------------------------------------------------------
/// Format unified COMPSEGS table for multiple wells
//--------------------------------------------------------------------------------------------------
void RicMswDataFormatter::formatUnifiedCompsegsTable( RifTextDataTableFormatter& formatter, const RicMswUnifiedData& unifiedData, bool isLgrData )
{
    auto rows = unifiedData.getAllCompsegsRows( isLgrData );
    if ( rows.empty() ) return;

    formatter.keyword( "COMPSEGS" );

    if ( isLgrData )
    {
        std::vector<RifTextDataTableColumn> unifiedLgrCompsegsHeader = { RifTextDataTableColumn( "Grid" ),
                                                                         RifTextDataTableColumn( "I" ),
                                                                         RifTextDataTableColumn( "J" ),
                                                                         RifTextDataTableColumn( "K" ),
                                                                         RifTextDataTableColumn( "Branch no" ),
                                                                         RifTextDataTableColumn( "Start Length" ),
                                                                         RifTextDataTableColumn( "End Length" ),
                                                                         RifTextDataTableColumn( "Dir Pen" ),
                                                                         RifTextDataTableColumn( "End Range" ),
                                                                         RifTextDataTableColumn( "Connection Factor" ),
                                                                         RifTextDataTableColumn( "Diameter" ),
                                                                         RifTextDataTableColumn( "Skin Factor" ) };
        formatter.header( unifiedLgrCompsegsHeader );
    }
    else
    {
        std::vector<RifTextDataTableColumn> unifiedCompsegsHeader = { RifTextDataTableColumn( "I" ),
                                                                      RifTextDataTableColumn( "J" ),
                                                                      RifTextDataTableColumn( "K" ),
                                                                      RifTextDataTableColumn( "Branch no" ),
                                                                      RifTextDataTableColumn( "Start Length" ),
                                                                      RifTextDataTableColumn( "End Length" ),
                                                                      RifTextDataTableColumn( "Dir Pen" ),
                                                                      RifTextDataTableColumn( "End Range" ),
                                                                      RifTextDataTableColumn( "Connection Factor" ),
                                                                      RifTextDataTableColumn( "Diameter" ),
                                                                      RifTextDataTableColumn( "Skin Factor" ) };
        formatter.header( unifiedCompsegsHeader );
    }

    for ( const auto& row : rows )
    {
        if ( isLgrData )
        {
            formatter.add( row.gridName );
        }

        formatter.add( row.cellI );
        formatter.add( row.cellJ );
        formatter.add( row.cellK );
        formatter.add( row.branchNumber );
        formatter.add( row.startLength );
        formatter.add( row.endLength );

        formatter.addOptionalValue( row.direction, RicMswExportInfo::defaultDoubleValue() );
        formatter.addOptionalValue( row.endRange, RicMswExportInfo::defaultDoubleValue() );
        formatter.addOptionalValue( row.connectionFactor, RicMswExportInfo::defaultDoubleValue() );
        formatter.addOptionalValue( row.diameter, RicMswExportInfo::defaultDoubleValue() );
        formatter.addOptionalValue( row.skinFactor, RicMswExportInfo::defaultDoubleValue() );

        formatter.rowCompleted();
    }

    formatter.tableCompleted();
}

//--------------------------------------------------------------------------------------------------
/// Format unified WSEGVALV table for multiple wells
//--------------------------------------------------------------------------------------------------
void RicMswDataFormatter::formatUnifiedWsegvalvTable( RifTextDataTableFormatter& formatter, const RicMswUnifiedData& unifiedData )
{
    auto rows = unifiedData.getAllWsegvalvRows();
    if ( rows.empty() ) return;

    formatter.keyword( "WSEGVALV" );
    std::vector<RifTextDataTableColumn> unifiedWsegvalvHeader = { RifTextDataTableColumn( "Well" ),
                                                                  RifTextDataTableColumn( "Seg No" ),
                                                                  RifTextDataTableColumn( "Cv" ),
                                                                  RifTextDataTableColumn( "Ac" ),
                                                                  RifTextDataTableColumn( "Device Type" ),
                                                                  RifTextDataTableColumn( "Param 1" ),
                                                                  RifTextDataTableColumn( "Param 2" ) };
    formatter.header( unifiedWsegvalvHeader );

    for ( const auto& row : rows )
    {
        formatter.add( row.wellName );
        formatter.add( row.segmentNumber );
        formatter.add( row.flowCoefficient );
        formatter.addOptionalValue( row.area, RicMswExportInfo::defaultDoubleValue() );
        formatter.add( row.deviceType );
        formatter.addOptionalValue( row.additionalParameter1, RicMswExportInfo::defaultDoubleValue() );
        formatter.addOptionalValue( row.additionalParameter2, RicMswExportInfo::defaultDoubleValue() );

        formatter.rowCompleted();
    }

    formatter.tableCompleted();
}

//--------------------------------------------------------------------------------------------------
/// Format unified WSEGAICD table for multiple wells
//--------------------------------------------------------------------------------------------------
void RicMswDataFormatter::formatUnifiedWsegaicdTable( RifTextDataTableFormatter& formatter, const RicMswUnifiedData& unifiedData )
{
    auto rows = unifiedData.getAllWsegaicdRows();
    if ( rows.empty() ) return;

    formatter.keyword( "WSEGAICD" );
    std::vector<RifTextDataTableColumn> unifiedWsegaicdHeader = { RifTextDataTableColumn( "Well" ),
                                                                  RifTextDataTableColumn( "Seg No" ),
                                                                  RifTextDataTableColumn( "Flow Coeff" ),
                                                                  RifTextDataTableColumn( "Area" ),
                                                                  RifTextDataTableColumn( "Oil Visc" ),
                                                                  RifTextDataTableColumn( "Water Visc" ),
                                                                  RifTextDataTableColumn( "Gas Visc" ),
                                                                  RifTextDataTableColumn( "Device Type" ) };
    formatter.header( unifiedWsegaicdHeader );

    for ( const auto& row : rows )
    {
        formatter.add( row.wellName );
        formatter.add( row.segmentNumber );
        formatter.add( row.flowCoefficient );
        formatter.addOptionalValue( row.area, RicMswExportInfo::defaultDoubleValue() );
        formatter.addOptionalValue( row.oilViscosityParameter, RicMswExportInfo::defaultDoubleValue() );
        formatter.addOptionalValue( row.waterViscosityParameter, RicMswExportInfo::defaultDoubleValue() );
        formatter.addOptionalValue( row.gasViscosityParameter, RicMswExportInfo::defaultDoubleValue() );
        formatter.add( row.deviceType );

        formatter.rowCompleted();
    }

    formatter.tableCompleted();
}

//--------------------------------------------------------------------------------------------------
/// Format all MSW tables for a single well
//--------------------------------------------------------------------------------------------------
void RicMswDataFormatter::formatMswTables( RifTextDataTableFormatter& formatter, const RicMswTableData& tableData )
{
    formatWelsegsTable( formatter, tableData );
    formatCompsegsTable( formatter, tableData, false ); // Main grid

    if ( tableData.hasLgrData() )
    {
        formatCompsegsTable( formatter, tableData, true ); // LGR data
    }

    formatWsegvalvTable( formatter, tableData );
    formatWsegaicdTable( formatter, tableData );
}

//--------------------------------------------------------------------------------------------------
/// Format all MSW tables for multiple wells
//--------------------------------------------------------------------------------------------------
void RicMswDataFormatter::formatUnifiedMswTables( RifTextDataTableFormatter& formatter, const RicMswUnifiedData& unifiedData )
{
    formatUnifiedWelsegsTable( formatter, unifiedData );
    formatUnifiedCompsegsTable( formatter, unifiedData, false ); // Main grid

    if ( unifiedData.hasAnyLgrData() )
    {
        formatUnifiedCompsegsTable( formatter, unifiedData, true ); // LGR data
    }

    formatUnifiedWsegvalvTable( formatter, unifiedData );
    formatUnifiedWsegaicdTable( formatter, unifiedData );
}
