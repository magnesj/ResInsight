/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2026-     Equinor ASA
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

class RimEnsembleFileSet;
class RimSummaryEnsemble;
class RimWellFormationsFile;

//==================================================================================================
///
//==================================================================================================
namespace RimWellFormationsTools
{
// The well formations file linked to the ensemble's file set, if any. Returns nullptr for ensembles
// without a file set (picked file lists, SUMO ensembles, older projects).
RimWellFormationsFile* formationsForEnsemble( RimSummaryEnsemble* ensemble );

// Auto-discovers "share/results/tables/formations.csv" in the file set's realization folders and
// links it, unless the file set already has a well formations entry. Does nothing if not found.
void discoverWellFormations( RimEnsembleFileSet* fileSet );
}; // namespace RimWellFormationsTools
