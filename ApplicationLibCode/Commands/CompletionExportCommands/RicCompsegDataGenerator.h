/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2025-     Equinor ASA
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

#include "RigCompsegData.h"

#include <vector>

class RicMswBranch;
class RicMswExportInfo;

//==================================================================================================
///  Simple generator for COMPSEGS data from MSW structure
///
///  Usage example:
///     // Generate all completion data
///     auto allData = RicCompsegDataGenerator::generateCompsegData(exportInfo);
///     
///     // Filter by completion type
///     auto perforations = RicCompsegDataGenerator::filterByCompletionType(allData, RigCompletionData::CompletionType::PERFORATION);
///     auto fractures = RicCompsegDataGenerator::filterByCompletionType(allData, RigCompletionData::CompletionType::FRACTURE);
///     
///     // Separate by grid type
///     auto mainGrid = RicCompsegDataGenerator::mainGridData(allData);
///     auto lgrData = RicCompsegDataGenerator::lgrData(allData);
///     
///     // Sort and export
///     auto sortedData = RicCompsegDataGenerator::sortedData(mainGrid);
///     QString wellName = exportInfo.mainBoreBranch()->wellPath()->completionSettings()->wellNameForExport();
///     RicMswTableFormatterTools::exportCompsegData(formatter, sortedData, wellName);
///     
///     // Or export with automatic LGR separation
///     RicMswTableFormatterTools::exportCompsegDataSeparated(formatter, allData, wellName);
//==================================================================================================
class RicCompsegDataGenerator
{
public:
    static std::vector<RigCompsegData> generateCompsegData( const RicMswExportInfo& exportInfo );

    // Utility functions for filtering and organizing data
    static std::vector<RigCompsegData> filterByCompletionType( const std::vector<RigCompsegData>& data,
                                                              RigCompletionData::CompletionType completionType );
    
    static std::vector<RigCompsegData> mainGridData( const std::vector<RigCompsegData>& data );
    static std::vector<RigCompsegData> lgrData( const std::vector<RigCompsegData>& data );
    
    static std::vector<RigCompsegData> sortedData( const std::vector<RigCompsegData>& data );
    
    // Validation
    static bool isValidData( const RigCompsegData& data );

private:
    static void processSegmentsRecursively( const RicMswBranch*           branch,
                                           const QString&                wellName,
                                           std::vector<RigCompsegData>&  compsegData );
};