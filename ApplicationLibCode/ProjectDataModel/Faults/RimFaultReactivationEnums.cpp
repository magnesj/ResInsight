/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2024     Equinor ASA
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

#include "RimFaultReactivationEnums.h"

#include "cafAppEnum.h"

namespace caf
{
template <>
void caf::AppEnum<RimFaultReactivation::StressSource>::setUp()
{
    addItem( RimFaultReactivation::StressSource::StressFromEclipse, "StressFromEclipse", "Eclipse Model" );
    addItem( RimFaultReactivation::StressSource::StressFromGeoMech, "StressFromGeoMech", "Geo-Mech Model" );
    setDefault( RimFaultReactivation::StressSource::StressFromEclipse );
}

} // namespace caf
