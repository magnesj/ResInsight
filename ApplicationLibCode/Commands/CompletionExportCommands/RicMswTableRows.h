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
#include <cstddef>
#include <optional>

//==================================================================================================
/// Header structure for WELSEGS table (well-level information)
//==================================================================================================
struct WelsegsHeader
{
    QString               wellName;
    double                topMD;
    double                topTVD;
    std::optional<double> volume;
    QString               lengthAndDepthText;
    QString               pressureDropText;
};

//==================================================================================================
/// Row structure for WELSEGS table segment data
//==================================================================================================
struct WelsegsRow
{
    int                   segmentNumber;
    int                   outletSegmentNumber;
    int                   branchNumber;
    double                length;
    std::optional<double> depth;
    std::optional<double> diameter;
    std::optional<double> roughness;
    std::optional<double> volume;
};

//==================================================================================================
/// Row structure for COMPSEGS table data
//==================================================================================================
struct CompsegsRow
{
    QString               wellName;
    size_t                cellI;
    size_t                cellJ;
    size_t                cellK;
    int                   branchNumber;
    double                startLength;
    double                endLength;
    std::optional<int>    direction;
    std::optional<double> endRange;
    std::optional<double> connectionDepth;

    QString gridName; // Empty for main grid, populated for LGR data

    bool isMainGrid() const { return gridName.isEmpty(); }
    bool isLgrGrid() const { return !gridName.isEmpty(); }
};

//==================================================================================================
/// Row structure for WSEGVALV table data
//==================================================================================================
struct WsegvalvRow
{
    QString               wellName;
    int                   segmentNumber;
    double                flowCoefficient;
    std::optional<double> area;
    QString               deviceType;
    std::optional<double> additionalParameter1;
    std::optional<double> additionalParameter2;
};

//==================================================================================================
/// Row structure for WSEGAICD table data
//==================================================================================================
struct WsegaicdRow
{
    QString               wellName;
    int                   segmentNumber;
    double                flowCoefficient;
    std::optional<double> area;
    std::optional<double> oilViscosityParameter;
    std::optional<double> waterViscosityParameter;
    std::optional<double> gasViscosityParameter;
    QString               deviceType;
};
