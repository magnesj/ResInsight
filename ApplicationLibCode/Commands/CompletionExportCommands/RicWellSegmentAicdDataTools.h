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

#include <QString>
#include <map>
#include <vector>

#include <gsl/gsl>

class RicWellSegmentAicdData;
class RicMswExportInfo;
class RicMswBranch;
class RifTextDataTableFormatter;

//==================================================================================================
///
//==================================================================================================
class RicWellSegmentAicdDataTools
{
public:
    static std::vector<RicWellSegmentAicdData>
        generateAicdData( RicMswExportInfo& exportInfo, gsl::not_null<RicMswBranch*> branch, const QString& wellNameForExport );

    static void writeWsegaicdTable( RifTextDataTableFormatter&                 formatter,
                                    const std::vector<RicWellSegmentAicdData>& aicdData,
                                    const QString&                             wellName );

private:
    static void generateAicdDataRecursively( gsl::not_null<RicMswBranch*>                           branch,
                                             const QString&                                         wellNameForExport,
                                             std::map<size_t, std::vector<RicWellSegmentAicdData>>& wsegaicdData );
};