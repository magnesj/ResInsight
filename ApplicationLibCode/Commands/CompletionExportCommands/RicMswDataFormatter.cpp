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

namespace
{
//--------------------------------------------------------------------------------------------------
/// Helper function to format WELSEGS segment data rows
//--------------------------------------------------------------------------------------------------
template <typename RowContainer>
void formatWelsegsRows( RifTextDataTableFormatter& formatter, const RowContainer& rows )
{
    for ( const auto& row : rows )
    {
        formatter.add( row.segmentNumber );
        formatter.add( row.segmentNumber );
        formatter.add( row.branchNumber );
        formatter.add( row.outletSegmentNumber );
        formatter.add( row.length );

        formatter.addOptionalValue( row.depth, RicMswExportInfo::defaultDoubleValue() );
        formatter.addOptionalValue( row.diameter, RicMswExportInfo::defaultDoubleValue() );
        formatter.addOptionalValue( row.roughness, RicMswExportInfo::defaultDoubleValue() );

        formatter.addOptionalComment( row.description );
        formatter.rowCompleted();
    }
}

//--------------------------------------------------------------------------------------------------
/// Helper function to format COMPSEGS data rows
//--------------------------------------------------------------------------------------------------
template <typename RowContainer>
void formatCompsegsRows( RifTextDataTableFormatter& formatter, const RowContainer& rows, bool isLgrData )
{
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
        formatter.addOptionalValue( row.connectionDepth, RicMswExportInfo::defaultDoubleValue() );

        formatter.rowCompleted();
    }
}

//--------------------------------------------------------------------------------------------------
/// Helper function to format WSEGVALV data rows
//--------------------------------------------------------------------------------------------------
template <typename RowContainer>
void formatWsegvalvRows( RifTextDataTableFormatter& formatter, const RowContainer& rows )
{
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
}

//--------------------------------------------------------------------------------------------------
/// Helper function to format WSEGAICD data rows
//--------------------------------------------------------------------------------------------------
template <typename RowContainer>
void formatWsegaicdRows( RifTextDataTableFormatter& formatter, const RowContainer& rows )
{
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
}

//--------------------------------------------------------------------------------------------------
/// Helper function to create COMPSEGS headers
//--------------------------------------------------------------------------------------------------
std::vector<RifTextDataTableColumn> createCompsegsHeader( bool isLgrData )
{
    if ( isLgrData )
    {
        return { RifTextDataTableColumn( "Grid" ),
                 RifTextDataTableColumn( "I" ),
                 RifTextDataTableColumn( "J" ),
                 RifTextDataTableColumn( "K" ),
                 RifTextDataTableColumn( "Branch no" ),
                 RifTextDataTableColumn( "Start Length" ),
                 RifTextDataTableColumn( "End Length" ),
                 RifTextDataTableColumn( "Dir Pen" ),
                 RifTextDataTableColumn( "End Range" ),
                 RifTextDataTableColumn( "Connection Depth" ) };
    }
    else
    {
        return { RifTextDataTableColumn( "I" ),
                 RifTextDataTableColumn( "J" ),
                 RifTextDataTableColumn( "K" ),
                 RifTextDataTableColumn( "Branch no" ),
                 RifTextDataTableColumn( "Start Length" ),
                 RifTextDataTableColumn( "End Length" ),
                 RifTextDataTableColumn( "Dir Pen" ),
                 RifTextDataTableColumn( "End Range" ),
                 RifTextDataTableColumn( "Connection Depth" ) };
    }
}

//--------------------------------------------------------------------------------------------------
/// Helper function to create WSEGVALV headers
//--------------------------------------------------------------------------------------------------
std::vector<RifTextDataTableColumn> createWsegvalvHeader()
{
    return { RifTextDataTableColumn( "Well" ),
             RifTextDataTableColumn( "Seg No" ),
             RifTextDataTableColumn( "Cv" ),
             RifTextDataTableColumn( "Ac" ),
             RifTextDataTableColumn( "Device Type" ),
             RifTextDataTableColumn( "Param 1" ),
             RifTextDataTableColumn( "Param 2" ) };
}

//--------------------------------------------------------------------------------------------------
/// Helper function to create WSEGAICD headers
//--------------------------------------------------------------------------------------------------
std::vector<RifTextDataTableColumn> createWsegaicdHeader()
{
    return { RifTextDataTableColumn( "Well" ),
             RifTextDataTableColumn( "Seg No" ),
             RifTextDataTableColumn( "Flow Coeff" ),
             RifTextDataTableColumn( "Area" ),
             RifTextDataTableColumn( "Oil Visc" ),
             RifTextDataTableColumn( "Water Visc" ),
             RifTextDataTableColumn( "Gas Visc" ),
             RifTextDataTableColumn( "Device Type" ) };
}

//--------------------------------------------------------------------------------------------------
/// Helper function to create WELSEGS segment headers
//--------------------------------------------------------------------------------------------------
std::vector<RifTextDataTableColumn> createWelsegsSegmentHeader()
{
    return { RifTextDataTableColumn( "First Seg" ),
             RifTextDataTableColumn( "Last Seg" ),
             RifTextDataTableColumn( "Branch No" ),
             RifTextDataTableColumn( "Outlet Seg" ),
             RifTextDataTableColumn( "Length" ),
             RifTextDataTableColumn( "Depth Change" ),
             RifTextDataTableColumn( "Diam" ),
             RifTextDataTableColumn( "Rough" ) };
}

} // namespace

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
        formatter.addOptionalValue( welsegsHeader->volume, RicMswExportInfo::defaultDoubleValue() );
        formatter.add( welsegsHeader->lengthAndDepthText );
        formatter.add( welsegsHeader->pressureDropText );
        formatter.rowCompleted();

        // Column headers for segment data
        auto segmentHeader = createWelsegsSegmentHeader();
        formatter.header( segmentHeader );
    }

    // Write segment data using helper function
    formatWelsegsRows( formatter, tableData.welsegsData() );
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

    // Add well name
    {
        std::vector<RifTextDataTableColumn> header = { RifTextDataTableColumn( "Name" ) };
        formatter.header( header );
        formatter.add( rows.front().wellName );
        formatter.rowCompleted();
    }

    auto header = createCompsegsHeader( isLgrData );
    formatter.header( header );

    formatCompsegsRows( formatter, rows, isLgrData );
    formatter.tableCompleted();
}

//--------------------------------------------------------------------------------------------------
/// Format WSEGVALV table for a single well
//--------------------------------------------------------------------------------------------------
void RicMswDataFormatter::formatWsegvalvTable( RifTextDataTableFormatter& formatter, const RicMswTableData& tableData )
{
    if ( !tableData.hasWsegvalvData() ) return;

    formatter.keyword( "WSEGVALV" );
    auto header = createWsegvalvHeader();
    formatter.header( header );

    formatWsegvalvRows( formatter, tableData.wsegvalvData() );
    formatter.tableCompleted();
}

//--------------------------------------------------------------------------------------------------
/// Format WSEGAICD table for a single well
//--------------------------------------------------------------------------------------------------
void RicMswDataFormatter::formatWsegaicdTable( RifTextDataTableFormatter& formatter, const RicMswTableData& tableData )
{
    if ( !tableData.hasWsegaicdData() ) return;

    formatter.keyword( "WSEGAICD" );
    auto header = createWsegaicdHeader();
    formatter.header( header );

    formatWsegaicdRows( formatter, tableData.wsegaicdData() );
    formatter.tableCompleted();
}

//--------------------------------------------------------------------------------------------------
/// Format WELSEGS table for unified data (multiple wells)
//--------------------------------------------------------------------------------------------------
void RicMswDataFormatter::formatWelsegsTable( RifTextDataTableFormatter& formatter, const RicMswUnifiedData& unifiedData )
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
        formatter.addOptionalValue( header.volume, RicMswExportInfo::defaultDoubleValue() );
        formatter.add( header.lengthAndDepthText );
        formatter.add( header.pressureDropText );
        formatter.rowCompleted();
    }

    // Column headers for segment data
    auto segmentHeader = createWelsegsSegmentHeader();
    formatter.header( segmentHeader );

    // Write all segment data using helper function
    formatWelsegsRows( formatter, rows );

    formatter.tableCompleted();
}

//--------------------------------------------------------------------------------------------------
/// Format WSEGVALV table for unified data (multiple wells)
//--------------------------------------------------------------------------------------------------
void RicMswDataFormatter::formatWsegvalvTable( RifTextDataTableFormatter& formatter, const RicMswUnifiedData& unifiedData )
{
    auto rows = unifiedData.getAllWsegvalvRows();
    if ( rows.empty() ) return;

    formatter.keyword( "WSEGVALV" );
    auto header = createWsegvalvHeader();
    formatter.header( header );

    formatWsegvalvRows( formatter, rows );

    formatter.tableCompleted();
}

//--------------------------------------------------------------------------------------------------
/// Format WSEGAICD table for unified data (multiple wells)
//--------------------------------------------------------------------------------------------------
void RicMswDataFormatter::formatWsegaicdTable( RifTextDataTableFormatter& formatter, const RicMswUnifiedData& unifiedData )
{
    auto rows = unifiedData.getAllWsegaicdRows();
    if ( rows.empty() ) return;

    formatter.keyword( "WSEGAICD" );
    auto header = createWsegaicdHeader();
    formatter.header( header );

    formatWsegaicdRows( formatter, rows );

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
/// Format all MSW tables for unified data (multiple wells)
//--------------------------------------------------------------------------------------------------
void RicMswDataFormatter::formatMswTables( RifTextDataTableFormatter& formatter, const RicMswUnifiedData& unifiedData )
{
    formatWelsegsTable( formatter, unifiedData );

    for ( const auto& wellData : unifiedData.wellDataList() )
    {
        bool isLgrData = false;
        formatCompsegsTable( formatter, wellData, isLgrData ); // Main grid, LGR is handeled separately
    }

    formatWsegvalvTable( formatter, unifiedData );
    formatWsegaicdTable( formatter, unifiedData );
}
