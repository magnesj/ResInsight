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

#include <cstddef>
#include <optional>
#include <string>

// clang-format off


// The structures below represent data from various MSW tables
// The variable names correspond to the table names in Opm::ParserKeywords
// Example file path:
// custom-opm-common/generated-opm-common/include/opm/input/eclipse/Parser/ParserKeywords/W.hpp

//==================================================================================================
/// Header structure for WELSEGS table (well-level information)
//==================================================================================================
struct WelsegsHeader
{
    // type                     variableName;       // Opm::ParserKeywords::WELSEGS::...

    std::string                 well;               // WELNAME
    double                      topDepth;           // TOPDEP
    double                      topLength;          // TOPLEN
    std::optional<double>       wellboreVolume;     // WBORVOL
    std::string                 infoType;           // TUBOPT
    std::string                 pressureComponents; // PRESOPT
    std::optional<std::string>  flowModel;          // FLOWOPT
};

//==================================================================================================
/// Row structure for WELSEGS table segment data
//==================================================================================================
struct WelsegsRow
{
    // type               variableName; // Opm::ParserKeywords::WELSEGS::...

    int                   segment1;     // ISEG1
    int                   segment2;     // ISEG2
    int                   branch;       // IBRANCH
    int                   joinSegment;  // ISEG3
    double                length;       // LENGTH
    double                depth;        // DEPTH
    std::optional<double> diameter;     // ID
    std::optional<double> roughness;    // EPSILON
    std::string           description;
};

//==================================================================================================
/// Row structure for COMPSEGS table data
//==================================================================================================
struct CompsegsRow
{
    // type               variableName; // Opm::ParserKeywords::WELSEGS::...

    size_t                      i;              // I
    size_t                      j;              // J
    size_t                      k;              // K
    int                         branch;         // IBRANCH
    double                      distanceStart;  // LENGTH1
    double                      distanceEnd;    // LENGTH2

    std::string gridName; // Empty for main grid, populated for LGR data

    bool isMainGrid() const { return gridName.empty(); }
    bool isLgrGrid() const { return !gridName.empty(); }
};

//==================================================================================================
/// Row structure for WSEGVALV table data
//==================================================================================================
struct WsegvalvRow
{
    std::string           wellName;
    int                   segmentNumber;
    double                flowCoefficient;
    std::optional<double> area;
};

//==================================================================================================
/// Row structure for WSEGAICD table data
//==================================================================================================
struct WsegaicdRow
{
    std::string           wellName;
    int                   segmentNumber;
    double                flowCoefficient;
    std::optional<double> area;
    std::optional<double> oilViscosityParameter;
    std::optional<double> waterViscosityParameter;
    std::optional<double> gasViscosityParameter;
    std::string           deviceType;
};
