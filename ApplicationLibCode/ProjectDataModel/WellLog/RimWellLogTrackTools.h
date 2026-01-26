/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2015-     Statoil ASA
//  Copyright (C) 2015-     Ceetron Solutions AS
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

#include "RimWellLogPlot.h"

#include <QList>
#include <QString>

#include <utility>
#include <vector>

namespace caf
{
class PdmOptionItemInfo;
}

class RigEclipseWellLogExtractor;
class RigGeoMechWellLogExtractor;
class RigResultAccessor;
class RigFemResultAddress;
class RimCase;
class RimDepthTrackPlot;
class RimWellLogCurve;
class RimWellLogPlotCollection;

struct CurveSamplingPointData;

//--------------------------------------------------------------------------------------------------
/// Static utility functions for RimWellLogTrack
//--------------------------------------------------------------------------------------------------
class RimWellLogTrackTools
{
public:
    static CurveSamplingPointData curveSamplingPointData( RigEclipseWellLogExtractor* extractor, RigResultAccessor* resultAccessor );
    static CurveSamplingPointData curveSamplingPointData( RigGeoMechWellLogExtractor* extractor, const RigFemResultAddress& resultAddress );

    static void findRegionNamesToPlot( const CurveSamplingPointData&           curveData,
                                       const std::vector<QString>&             regionNamesVector,
                                       RimWellLogPlot::DepthTypeEnum           depthType,
                                       std::vector<QString>*                   regionNamesToPlot,
                                       std::vector<std::pair<double, double>>* yValues );

    static std::vector<QString> formationNamesVector( RimCase* rimCase );

    static void addOverburden( std::vector<QString>& namesVector, CurveSamplingPointData& curveData, double height );
    static void addUnderburden( std::vector<QString>& namesVector, CurveSamplingPointData& curveData, double height );

    static void simWellOptionItems( QList<caf::PdmOptionItemInfo>* options, RimCase* rimCase );

    static RigEclipseWellLogExtractor* createSimWellExtractor( RimWellLogPlotCollection* wellLogCollection,
                                                               RimCase*                  rimCase,
                                                               const QString&            simWellName,
                                                               int                       branchIndex,
                                                               bool                      useBranchDetection );

    // Axis range utility functions
    static std::pair<double, double> adjustXRange( double minValue, double maxValue, double tickInterval );
    static std::pair<double, double> extendMinMaxRange( double minValue, double maxValue, double factor );

    // ASCII export
    static QString asciiDataForPlotExport( const QString&                       trackDescription,
                                           RimDepthTrackPlot*                   depthTrackPlot,
                                           const std::vector<RimWellLogCurve*>& curves );
};
