/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2024-     Equinor ASA
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

#include "RicCreateSumoGridEnsembleFeature.h"

#include "RiaDefines.h"
#include "RiaLogging.h"

#include "RicNewViewFeature.h"

#include "Rim3dView.h"
#include "RimEclipseCaseCollection.h"
#include "RimEclipseCaseEnsemble.h"
#include "RimEclipseCaseSumo.h"
#include "RimEclipseViewCollection.h"
#include "RimOilField.h"
#include "RimProject.h"
#include "RimViewNameConfig.h"
#include "Sumo/RimSumoGridDataSource.h"

#include "cafSelectionManagerTools.h"

#include <QAction>

CAF_CMD_SOURCE_INIT( RicCreateSumoGridEnsembleFeature, "RicCreateSumoGridEnsembleFeature" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicCreateSumoGridEnsembleFeature::onActionTriggered( bool isChecked )
{
    auto dataSources = caf::selectedObjectsByType<RimSumoGridDataSource*>();

    for ( auto dataSource : dataSources )
    {
        createGridEnsemble( dataSource );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicCreateSumoGridEnsembleFeature::createGridEnsemble( RimSumoGridDataSource* dataSource )
{
    if ( !dataSource ) return;

    const QString          gridName     = dataSource->gridName();
    const std::vector<int> realizations = dataSource->selectedRealizations();

    if ( gridName.isEmpty() )
    {
        RiaLogging::warning( "No grid selected. Unable to create grid ensemble from Sumo." );
        return;
    }

    if ( realizations.empty() )
    {
        RiaLogging::warning( "No realizations selected. Unable to create grid ensemble from Sumo." );
        return;
    }

    RimProject* project = RimProject::current();
    if ( !project ) return;

    RimOilField* oilfield = project->activeOilField();
    if ( !oilfield ) return;

    auto eclipseCaseEnsemble = new RimEclipseCaseEnsemble;
    eclipseCaseEnsemble->setName( QString( "%1 - %2" ).arg( dataSource->ensembleName(), gridName ) );

    for ( int realization : realizations )
    {
        auto* gridCase = new RimEclipseCaseSumo();
        gridCase->setSumoCaseId( dataSource->caseId().get() );
        gridCase->setEnsembleName( dataSource->ensembleName() );
        gridCase->setGridName( gridName );
        gridCase->setRealization( realization );

        // Name the case using grid name, asset, ensemble and realization, e.g. "Geogrid_Drogon_iter-0_Real_0".
        QString caseDisplayName = QString( "%1_%2_%3_Real_%4" )
                                      .arg( gridName, dataSource->assetName(), dataSource->ensembleName() )
                                      .arg( realization );
        gridCase->setCustomCaseName( caseDisplayName );

        eclipseCaseEnsemble->addCase( gridCase );
    }

    oilfield->analysisModels()->caseEnsembles.push_back( eclipseCaseEnsemble );
    oilfield->analysisModels()->updateConnectedEditors();

    if ( eclipseCaseEnsemble->cases().empty() ) return;

    auto firstCase = eclipseCaseEnsemble->cases().front();
    if ( !firstCase ) return;

    auto view = RicNewViewFeature::addReservoirView( firstCase, nullptr, eclipseCaseEnsemble->viewCollection() );
    if ( view )
    {
        view->nameConfig()->setAddCaseName( true );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicCreateSumoGridEnsembleFeature::setupActionLook( QAction* actionToSetup )
{
    actionToSetup->setText( "Create Grid Ensemble" + RiaDefines::betaFeaturePostfix() );
    actionToSetup->setIcon( QIcon( ":/CreateGridCaseGroup16x16.png" ) );
}
