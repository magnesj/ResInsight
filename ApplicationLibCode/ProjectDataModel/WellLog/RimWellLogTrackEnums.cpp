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

#include "RimWellLogTrackEnums.h"

namespace caf
{
template <>
void AppEnum<RimWellLogTrackTrajectoryType>::setUp()
{
    addItem( RimWellLogTrackTrajectoryType::WELL_PATH, "WELL_PATH", "Well Path" );
    addItem( RimWellLogTrackTrajectoryType::SIMULATION_WELL, "SIMULATION_WELL", "Simulation Well" );
    setDefault( RimWellLogTrackTrajectoryType::WELL_PATH );
}

template <>
void AppEnum<RimWellLogTrackFormationSource>::setUp()
{
    addItem( RimWellLogTrackFormationSource::CASE, "CASE", "Case" );
    addItem( RimWellLogTrackFormationSource::WELL_PICK_FILTER, "WELL_PICK_FILTER", "Well Picks for Well Path" );
    setDefault( RimWellLogTrackFormationSource::CASE );
}

} // namespace caf
