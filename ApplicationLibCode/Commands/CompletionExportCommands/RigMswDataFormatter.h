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
class RigMswTableData;
class RicMswUnifiedData_to_be_deleted;

//==================================================================================================
/// Formatter functions that convert MSW data structures to file output using RifTextDataTableFormatter
//==================================================================================================
namespace RigMswDataFormatter
{
// Consolidated data formatting - works with any compatible data source
void formatCompsegsTable( RifTextDataTableFormatter& formatter, const RigMswTableData& tableData, bool isLgrData = false );
void formatWsegvalvTable( RifTextDataTableFormatter& formatter, const RigMswTableData& tableData );
void formatWsegvalvTable( RifTextDataTableFormatter& formatter, const RicMswUnifiedData_to_be_deleted& unifiedData );
void formatWsegaicdTable( RifTextDataTableFormatter& formatter, const RigMswTableData& tableData );
void formatWsegaicdTable( RifTextDataTableFormatter& formatter, const RicMswUnifiedData_to_be_deleted& unifiedData );
void formatWelsegsTable( RifTextDataTableFormatter& formatter, const RigMswTableData& tableData );
void formatWelsegsTable( RifTextDataTableFormatter& formatter, const RicMswUnifiedData_to_be_deleted& unifiedData );

// Complete MSW export
void formatMswTables( RifTextDataTableFormatter& formatter, const RigMswTableData& tableData );
void formatMswTables( RifTextDataTableFormatter& formatter, const RicMswUnifiedData_to_be_deleted& unifiedData );
} // namespace RigMswDataFormatter
