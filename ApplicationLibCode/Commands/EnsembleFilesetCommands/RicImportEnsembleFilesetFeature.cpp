/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2025     Equinor ASA
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

#include "RicImportEnsembleFilesetFeature.h"

#include "EnsembleFileset/RimEnsembleFileSet.h"
#include "EnsembleFileset/RimEnsembleFileSetCollection.h"
#include "RimProject.h"

#include "RiaApplication.h"
#include "RiaEnsembleNameTools.h"

#include "RicImportSummaryCasesFeature.h"

#include <QAction>

CAF_CMD_SOURCE_INIT( RicImportEnsembleFilesetFeature, "RicImportEnsembleFilesetFeature" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicImportEnsembleFilesetFeature::isCommandEnabled() const
{
    return true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicImportEnsembleFilesetFeature::onActionTriggered( bool isChecked )
{
    QString pathCacheName = "ENSEMBLE_FILES";
    auto    result = RicImportSummaryCasesFeature::runRecursiveSummaryCaseFileSearchDialogWithGrouping( "Import Ensemble", pathCacheName );
    if ( !result.ok )
    {
        return;
    }

    auto collection = RimProject::current()->ensembleFileSetCollection();

    auto grouping = RiaEnsembleNameTools::groupFilesByEnsembleName( result.files, result.groupingMode );
    for ( const auto& [groupName, fileNames] : grouping )
    {
        auto ensembleFileset = new RimEnsembleFileSet();
        ensembleFileset->setName( groupName );
        ensembleFileset->findAndSetPathPatternAndRangeString( fileNames );

        collection->addFileset( ensembleFileset );
    }

    collection->updateAllRequiredEditors();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicImportEnsembleFilesetFeature::setupActionLook( QAction* actionToSetup )
{
    actionToSetup->setIcon( QIcon( ":/Cases16x16.png" ) );
    actionToSetup->setText( "Import Ensemble File Set" );
}
