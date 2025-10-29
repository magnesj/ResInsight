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
    // Consolidated data formatting - works with any compatible data source
    void formatCompsegsTable( RifTextDataTableFormatter& formatter, const RicMswTableData& tableData, bool isLgrData = false );
    void formatCompsegsTable( RifTextDataTableFormatter& formatter, const RicMswUnifiedData& unifiedData, bool isLgrData = false );
    void formatWsegvalvTable( RifTextDataTableFormatter& formatter, const RicMswTableData& tableData );
    void formatWsegvalvTable( RifTextDataTableFormatter& formatter, const RicMswUnifiedData& unifiedData );
    void formatWsegaicdTable( RifTextDataTableFormatter& formatter, const RicMswTableData& tableData );
    void formatWsegaicdTable( RifTextDataTableFormatter& formatter, const RicMswUnifiedData& unifiedData );
    void formatWelsegsTable( RifTextDataTableFormatter& formatter, const RicMswTableData& tableData );
    void formatWelsegsTable( RifTextDataTableFormatter& formatter, const RicMswUnifiedData& unifiedData );

    // Complete MSW export
    void formatMswTables( RifTextDataTableFormatter& formatter, const RicMswTableData& tableData );
    void formatMswTables( RifTextDataTableFormatter& formatter, const RicMswUnifiedData& unifiedData );
}