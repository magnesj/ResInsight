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

#include "RiaDefines.h"

#include <memory>
#include <utility>
#include <vector>

class RimWellLogTrack;
class RiuPlotAnnotationTool;
class RigGeoMechWellLogExtractor;

//--------------------------------------------------------------------------------------------------
/// Helper class for managing region annotations on well log tracks
//--------------------------------------------------------------------------------------------------
class RimWellLogTrackRegionAnnotations
{
public:
    RimWellLogTrackRegionAnnotations( RimWellLogTrack* track );
    ~RimWellLogTrackRegionAnnotations();

    void updateRegionAnnotationsOnPlot();
    void removeRegionAnnotations();

    std::vector<std::pair<double, double>> waterAndRockRegions( RiaDefines::DepthTypeEnum         depthType,
                                                                const RigGeoMechWellLogExtractor* extractor ) const;

private:
    void updateFormationNamesOnPlot();
    void updateResultPropertyNamesOnPlot();
    void updateCurveDataRegionsOnPlot();

    RimWellLogTrack*                       m_track;
    std::unique_ptr<RiuPlotAnnotationTool> m_annotationTool;
};
