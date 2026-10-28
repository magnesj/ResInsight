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

#include "RicNewGroupNetworkFeature.h"

#include "RiaPreferencesSystem.h"

#include "GroupNetwork/RimGroupNetwork.h"
#include "GroupNetwork/RimGroupNetworkCollection.h"
#include "RimOilField.h"
#include "RimProject.h"
#include "RimSummaryCase.h"

#include "RiuPlotMainWindowTools.h"

#include "cafSelectionManager.h"

#include <QAction>

CAF_CMD_SOURCE_INIT( RicNewGroupNetworkFeature, "RicNewGroupNetworkFeature" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicNewGroupNetworkFeature::isCommandEnabled() const
{
    if ( !RiaPreferencesSystem::current()->isFeatureEnabled( "group-network" ) ) return false;

    auto* project = RimProject::current();
    return project && project->activeOilField() && project->activeOilField()->groupNetworkCollection();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicNewGroupNetworkFeature::onActionTriggered( bool isChecked )
{
    auto* project = RimProject::current();
    if ( !project || !project->activeOilField() || !project->activeOilField()->groupNetworkCollection() ) return;
    RimGroupNetworkCollection* collection = project->activeOilField()->groupNetworkCollection();

    RimSummaryCase* summaryCase = caf::SelectionManager::instance()->selectedItemOfType<RimSummaryCase>();
    if ( !summaryCase )
    {
        const auto summaryCases = RimProject::current()->allSummaryCases();
        if ( !summaryCases.empty() ) summaryCase = summaryCases.front();
    }

    auto* network = new RimGroupNetwork();
    network->setSummaryCase( summaryCase );
    collection->addItem( network );
    collection->updateConnectedEditors();

    RiuPlotMainWindowTools::showPlotMainWindow();
    RiuPlotMainWindowTools::onObjectAppended( network, collection );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicNewGroupNetworkFeature::setupActionLook( QAction* actionToSetup )
{
    actionToSetup->setText( "New Production Network" );
    actionToSetup->setIcon( QIcon( ":/components/images/network.svg" ) );
}
