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

#include "RicExportSurfaceToIrapFeature.h"

#include "RimSurface.h"

#include "RicExportSurfaceToGriFeature.h"

#include "cafSelectionManagerTools.h"

#include <QAction>

CAF_CMD_SOURCE_INIT( RicExportSurfaceToIrapFeature, "RicExportSurfaceToIrapFeature" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicExportSurfaceToIrapFeature::isCommandEnabled() const
{
    return !caf::selectedObjectsByTypeStrict<RimSurface*>().empty();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicExportSurfaceToIrapFeature::onActionTriggered( bool isChecked )
{
    auto surfaces = caf::selectedObjectsByTypeStrict<RimSurface*>();
    if ( surfaces.empty() ) return;

    RicExportSurfaceToGriFeature::exportToFolder( surfaces, RicExportSurfaceToGriFeature::ExportFormat::IRAP );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicExportSurfaceToIrapFeature::setupActionLook( QAction* actionToSetup )
{
    actionToSetup->setIcon( QIcon( ":/ReservoirSurfaces16x16.png" ) );
    actionToSetup->setText( "Export Surface to IRAP Classic (text) file" );
}
