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
#pragma once

class RifTextDataTableFormatter;
class RicMswTableData;
class RicMswUnifiedData;

//==================================================================================================
/// Formatter functions that convert MSW data structures to file output using RifTextDataTableFormatter
//==================================================================================================
namespace RicMswDataFormatter
{
    // Single well data formatting
    void formatWelsegsTable( RifTextDataTableFormatter& formatter, const RicMswTableData& tableData );
    void formatCompsegsTable( RifTextDataTableFormatter& formatter, const RicMswTableData& tableData, bool isLgrData = false );
    void formatWsegvalvTable( RifTextDataTableFormatter& formatter, const RicMswTableData& tableData );
    void formatWsegaicdTable( RifTextDataTableFormatter& formatter, const RicMswTableData& tableData );

    // Unified data formatting (multiple wells)
    void formatUnifiedWelsegsTable( RifTextDataTableFormatter& formatter, const RicMswUnifiedData& unifiedData );
    void formatUnifiedCompsegsTable( RifTextDataTableFormatter& formatter, const RicMswUnifiedData& unifiedData, bool isLgrData = false );
    void formatUnifiedWsegvalvTable( RifTextDataTableFormatter& formatter, const RicMswUnifiedData& unifiedData );
    void formatUnifiedWsegaicdTable( RifTextDataTableFormatter& formatter, const RicMswUnifiedData& unifiedData );

    // Complete MSW export for single well
    void formatMswTables( RifTextDataTableFormatter& formatter, const RicMswTableData& tableData );
    
    // Complete MSW export for multiple wells  
    void formatUnifiedMswTables( RifTextDataTableFormatter& formatter, const RicMswUnifiedData& unifiedData );
}