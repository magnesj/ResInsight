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

#include "RiaGuiApplication.h"
#include "Summary/RiaSummaryPlotTools.h"

#include "EnsembleFileSet/RimEnsembleFileSetTools.h"
#include "RicImportGridAndSummaryEnsembleDialog.h"
#include "RicNewViewFeature.h"
#include "Rim3dView.h"
#include "RimReservoirGridEnsemble.h"
#include "RimSummaryEnsemble.h"
#include "RimViewNameConfig.h"

#include "RiuPlotMainWindowTools.h"

#include <QAction>
#include <QIcon>
#include <QSet>
#include <QTimer>

CAF_CMD_SOURCE_INIT( RicImportGridAndSummaryEnsembleFeature, "RicImportGridAndSummaryEnsembleFeature" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicImportGridAndSummaryEnsembleFeature::onActionTriggered( bool isChecked )
{
    QPointer<QWidget> activeWindowBeforeImport = RiaGuiApplication::widgetToUseAsParent();

    auto result = RicImportGridAndSummaryEnsembleDialog::runDialog( activeWindowBeforeImport );

    if ( !result.ok ) return;
    if ( !result.createGridEnsemble && !result.createSummaryEnsemble ) return;

    // Build the union of base paths (extension-stripped) from both lists so every realization
    // appears exactly once. findPathPattern requires each realization number to be unique across
    // the input; duplicating paths with different extensions breaks the pattern detection.
    auto stripExtension = []( const QString& path ) -> QString
    {
        int dot = path.lastIndexOf( '.' );
        return dot != -1 ? path.left( dot ) : path;
    };

    QSet<QString> basePaths;
    for ( const auto& f : result.gridFiles )
        basePaths.insert( stripExtension( f ) );
    for ( const auto& f : result.summaryFiles )
        basePaths.insert( stripExtension( f ) );

    if ( basePaths.isEmpty() ) return;

    // Convert to list for the file set creation. The dialog should ensure that the base paths are in a consistent order, so the grouping
    // should work as intended.
    QStringList representativeFiles;
    for ( const auto& base : basePaths )
        representativeFiles << base;

    auto fileSets = RimEnsembleFileSetTools::createEnsembleFileSets( representativeFiles, result.groupingMode );
    if ( fileSets.empty() ) return;

    bool gridEnsemblesCreated = false;
    if ( result.createGridEnsemble && !result.gridFiles.isEmpty() )
    {
        auto gridEnsembles = RimEnsembleFileSetTools::createGridEnsemblesFromFileSets( fileSets );
        if ( !gridEnsembles.empty() )
        {
            gridEnsemblesCreated = true;
            auto firstEnsemble   = gridEnsembles.front();
            auto cases           = firstEnsemble->cases();
            if ( !cases.empty() )
            {
                auto view = RicNewViewFeature::addReservoirView( cases.front(), nullptr, firstEnsemble->viewCollection() );
                if ( view ) view->nameConfig()->setAddCaseName( true );
            }
        }
    }

    if ( result.createSummaryEnsemble && !result.summaryFiles.isEmpty() )
    {
        auto summaryEnsembles = RimEnsembleFileSetTools::createSummaryEnsemblesFromFileSets( fileSets );
        for ( auto ensemble : summaryEnsembles )
        {
            RiaSummaryPlotTools::createAndAppendDefaultSummaryMultiPlot( {}, { ensemble } );
        }
        if ( !summaryEnsembles.empty() ) RiuPlotMainWindowTools::showPlotMainWindow();
    }

    if ( gridEnsemblesCreated && activeWindowBeforeImport )
    {
        QTimer::singleShot( 500,
                            activeWindowBeforeImport,
                            [activeWindowBeforeImport]()
                            {
                                activeWindowBeforeImport->raise();
                                activeWindowBeforeImport->activateWindow();
                            } );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicImportGridAndSummaryEnsembleFeature::setupActionLook( QAction* actionToSetup )
{
    actionToSetup->setIcon( QIcon( ":/GridAndSummaryEnsemble.svg" ) );
    actionToSetup->setText( "Import Grid and Summary Ensemble" );
}
