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
//==================================================================================================
class RicCompsegDataGenerator
{
public:
    static std::vector<RigCompsegData> generateCompsegData( const RicMswExportInfo& exportInfo );

private:
    static void processSegmentsRecursively( const RicMswBranch*           branch,
                                           const QString&                wellName,
                                           std::vector<RigCompsegData>&  compsegData );
};