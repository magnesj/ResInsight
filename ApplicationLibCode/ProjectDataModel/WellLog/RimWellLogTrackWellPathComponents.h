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

#include <memory>
#include <vector>

class RimWellLogTrack;
class RimWellPathAttributeCollection;
class RiuWellPathComponentPlotItem;

//--------------------------------------------------------------------------------------------------
/// Helper class for managing well path component visualization on well log tracks
//--------------------------------------------------------------------------------------------------
class RimWellLogTrackWellPathComponents
{
public:
    explicit RimWellLogTrackWellPathComponents( RimWellLogTrack* track );
    ~RimWellLogTrackWellPathComponents();

    void updateWellPathAttributesOnPlot();
    void updateWellPathAttributesCollection();

    void detachAllPlotItems();
    void reattachAllPlotItems();

    bool depthValueRange( double* minimumDepth, double* maximumDepth ) const;

private:
    RimWellLogTrack*                                           m_track;
    std::vector<std::unique_ptr<RiuWellPathComponentPlotItem>> m_wellPathAttributePlotObjects;
};
