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

#include "EnsembleFileSet/RimEnsembleFileSetTools.h"
#include "RicImportGridAndSummaryEnsembleDialog.h"

#include <QAction>
#include <QIcon>
#include <QSet>

CAF_CMD_SOURCE_INIT( RicImportGridAndSummaryEnsembleFeature, "RicImportGridAndSummaryEnsembleFeature" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicImportGridAndSummaryEnsembleFeature::onActionTriggered( bool isChecked )
{
    auto result = RicImportGridAndSummaryEnsembleDialog::runDialog( RiaGuiApplication::widgetToUseAsParent() );

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

    // Reconstruct with a uniform extension — findAndSetPathPatternAndRangeString strips it anyway
    QStringList representativeFiles;
    for ( const auto& base : basePaths )
        representativeFiles << base + ".EGRID";

    auto fileSets = RimEnsembleFileSetTools::createEnsembleFileSets( representativeFiles, result.groupingMode );
    if ( fileSets.empty() ) return;

    if ( result.createGridEnsemble )
        RimEnsembleFileSetTools::createGridEnsemblesFromFileSets( fileSets );

    if ( result.createSummaryEnsemble )
        RimEnsembleFileSetTools::createSummaryEnsemblesFromFileSets( fileSets );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicImportGridAndSummaryEnsembleFeature::setupActionLook( QAction* actionToSetup )
{
    actionToSetup->setIcon( QIcon( ":/CreateGridCaseGroup16x16.png" ) );
    actionToSetup->setText( "Import Grid and Summary Ensemble" );
}
