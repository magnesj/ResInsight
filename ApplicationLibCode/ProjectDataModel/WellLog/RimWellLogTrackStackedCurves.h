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

#include <map>
#include <vector>

class RimWellLogTrack;
class RimWellLogCurve;

//--------------------------------------------------------------------------------------------------
/// Helper class for managing stacked curves on well log tracks
//--------------------------------------------------------------------------------------------------
class RimWellLogTrackStackedCurves
{
public:
    explicit RimWellLogTrackStackedCurves( RimWellLogTrack* track );

    void                                         updateStackedCurveData();
    std::map<int, std::vector<RimWellLogCurve*>> visibleStackedCurves() const;

private:
    RimWellLogTrack* m_track;
};
