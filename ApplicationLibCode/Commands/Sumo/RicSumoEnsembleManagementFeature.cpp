#include "RicSumoEnsembleManagementFeature.h"

#include "RiaDefines.h"

#include "Cloud/RimCloudDataSourceCollection.h"

#include <QAction>

CAF_CMD_SOURCE_INIT( RicSumoEnsembleManagementFeature, "RicSumoEnsembleManagementFeature" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicSumoEnsembleManagementFeature::isCommandEnabled() const
{
    auto* collection = RimCloudDataSourceCollection::instance();
    return collection && collection->sumoConnector() && collection->sumoConnector()->isGranted();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicSumoEnsembleManagementFeature::onActionTriggered( bool isChecked )
{
    if ( auto* collection = RimCloudDataSourceCollection::instance() ) collection->manageEnsemblesInDialog();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicSumoEnsembleManagementFeature::setupActionLook( QAction* actionToSetup )
{
    actionToSetup->setText( "Manage SUMO Ensembles..." + RiaDefines::betaFeaturePostfix() );
    actionToSetup->setIcon( QIcon( ":/SummaryEnsemble.svg" ) );
}
