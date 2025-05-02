/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017     Statoil ASA
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

#include "RicCreateEnsembleFilesetFeature.h"

#include "EnsembleFileset/RimEnsembleFileset.h"
#include "EnsembleFileset/RimEnsembleFilesetCollection.h"
#include "RimProject.h"

#include "RiaApplication.h"

#include "RicImportSummaryCasesFeature.h"

#include <QAction>

CAF_CMD_SOURCE_INIT( RicCreateEnsembleFilesetFeature, "RicCreateEnsembleFilesetFeature" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicCreateEnsembleFilesetFeature::isCommandEnabled() const
{
    return true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicCreateEnsembleFilesetFeature::onActionTriggered( bool isChecked )
{
    QString pathCacheName = "ENSEMBLE_FILES";
    auto    result = RicImportSummaryCasesFeature::runRecursiveSummaryCaseFileSearchDialogWithGrouping( "Import Ensemble", pathCacheName );

    auto collection = RimProject::current()->ensembleFilesetCollection();

    collection->addFileset( new RimEnsembleFileset() );
    collection->updateAllRequiredEditors();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicCreateEnsembleFilesetFeature::setupActionLook( QAction* actionToSetup )
{
    // actionToSetup->setIcon(QIcon(":/new_icon16x16.png"));
    actionToSetup->setText( "Create Ensemble Fileset" );
}
