/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2026-     Equinor ASA
//
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
//
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////

#include "RicImportGridAndSummaryEnsembleFeature.h"

#include "RiaEnsembleNameTools.h"
#include "RiaGuiApplication.h"
#include "Summary/RiaSummaryDefines.h"
#include "Summary/RiaSummaryTools.h"

#include "RimSummaryCaseMainCollection.h"

#include "EclipseCommands/RicCreateGridCaseEnsemblesFromFilesFeature.h"
#include "RicImportEnsembleFeature.h"
#include "RicImportGridAndSummaryEnsembleDialog.h"

#include <QAction>
#include <QIcon>

CAF_CMD_SOURCE_INIT( RicImportGridAndSummaryEnsembleFeature, "RicImportGridAndSummaryEnsembleFeature" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicImportGridAndSummaryEnsembleFeature::onActionTriggered( bool isChecked )
{
    auto result = RicImportGridAndSummaryEnsembleDialog::runDialog( RiaGuiApplication::widgetToUseAsParent() );

    if ( !result.ok ) return;

    if ( result.createGridEnsemble && !result.gridFiles.isEmpty() )
    {
        if ( result.groupingMode == RiaDefines::EnsembleGroupingMode::NONE )
        {
            RicCreateGridCaseEnsemblesFromFilesFeature::importSingleGridCaseEnsemble( result.gridFiles );
        }
        else
        {
            auto groups = RiaEnsembleNameTools::groupFilesByEnsemble( result.gridFiles, result.groupingMode );
            for ( const auto& group : groups )
            {
                RicCreateGridCaseEnsemblesFromFilesFeature::importSingleGridCaseEnsemble( group );
            }
        }
    }

    if ( result.createSummaryEnsemble && !result.summaryFiles.isEmpty() )
    {
        if ( result.groupingMode == RiaDefines::EnsembleGroupingMode::NONE )
        {
            RicImportEnsembleFeature::importSingleEnsembleFileSet( result.summaryFiles, false, result.groupingMode, RiaDefines::FileType::SMSPEC );
        }
        else
        {
            auto groupedFiles = RiaEnsembleNameTools::groupFilesByEnsembleName( result.summaryFiles, result.groupingMode );
            for ( const auto& [groupName, fileNames] : groupedFiles )
            {
                RicImportEnsembleFeature::importSingleEnsembleFileSet( fileNames, false, result.groupingMode, RiaDefines::FileType::SMSPEC, groupName );
            }
        }
    }

    RiaSummaryTools::updateSummaryEnsembleNames();
    RiaSummaryTools::summaryCaseMainCollection()->updateConnectedEditors();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicImportGridAndSummaryEnsembleFeature::setupActionLook( QAction* actionToSetup )
{
    actionToSetup->setIcon( QIcon( ":/CreateGridCaseGroup16x16.png" ) );
    actionToSetup->setText( "Import Grid and Summary Ensemble" );
}
