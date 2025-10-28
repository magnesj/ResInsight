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

#include "RigCompletionData.h"

#include <QString>
#include <vector>
#include <set>

#include <gsl/gsl>

class RicWellSegmentCompletionData;
class RicMswExportInfo;
class RicMswBranch;
class RifTextDataTableFormatter;

//==================================================================================================
///
//==================================================================================================
class RicWellSegmentCompletionDataTools
{
public:
    static std::vector<RicWellSegmentCompletionData> generateCompletionData( RicMswExportInfo&                                  exportInfo,
                                                                      gsl::not_null<const RicMswBranch*>                 branch,
                                                                      bool                                               exportSubGridIntersections,
                                                                      const std::set<RigCompletionData::CompletionType>& exportCompletionTypes );

    static void writeCompsegsTable( RifTextDataTableFormatter&                formatter,
                                    const std::vector<RicWellSegmentCompletionData>&  completionData,
                                    const QString&                             wellName,
                                    RigCompletionData::CompletionType          completionType,
                                    bool                                       isLgr = false );

    static void writeCompseglTable( RifTextDataTableFormatter&                formatter,
                                    const std::vector<RicWellSegmentCompletionData>&  completionData,
                                    const QString&                             wellName,
                                    RigCompletionData::CompletionType          completionType );

private:
    static void generateCompletionDataRecursively( RicMswExportInfo&                                  exportInfo,
                                                   gsl::not_null<const RicMswBranch*>                 branch,
                                                   bool                                               exportSubGridIntersections,
                                                   const std::set<RigCompletionData::CompletionType>& exportCompletionTypes,
                                                   std::vector<RicWellSegmentCompletionData>&                completionData,
                                                   std::set<size_t>&                                  intersectedCells );

    static QString getCompletionTypeComment( RigCompletionData::CompletionType completionType );
};