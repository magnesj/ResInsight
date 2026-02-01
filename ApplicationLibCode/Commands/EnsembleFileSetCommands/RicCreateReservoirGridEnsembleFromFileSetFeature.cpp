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

#include "RicCreateReservoirGridEnsembleFromFileSetFeature.h"

#include "RiaImportEclipseCaseTools.h"
#include "RiaLogging.h"

#include "RifReaderOpmCommon.h"

#include "EnsembleFileSet/RimEnsembleFileSet.h"
#include "RimEclipseCaseCollection.h"
#include "RimEclipseView.h"
#include "RimIdenticalGridCaseGroup.h"
#include "RimOilField.h"
#include "RimProject.h"
#include "RimReservoirGridEnsemble.h"

#include "Riu3DMainWindowTools.h"

#include "cafProgressInfo.h"
#include "cafSelectionManagerTools.h"

#include <QAction>
#include <QFileInfo>

CAF_CMD_SOURCE_INIT( RicCreateReservoirGridEnsembleFromFileSetFeature, "RicCreateReservoirGridEnsembleFromFileSetFeature" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicCreateReservoirGridEnsembleFromFileSetFeature::isCommandEnabled() const
{
    return !caf::selectedObjectsByType<RimEnsembleFileSet*>().empty();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicCreateReservoirGridEnsembleFromFileSetFeature::onActionTriggered( bool isChecked )
{
    std::vector<RimEnsembleFileSet*> selectedFileSets = caf::selectedObjectsByType<RimEnsembleFileSet*>();

    if ( selectedFileSets.empty() ) return;

    RimProject*               project         = RimProject::current();
    RimEclipseCaseCollection* eclipseCaseColl = project->activeOilField()->analysisModels();

    for ( RimEnsembleFileSet* fileSet : selectedFileSets )
    {
        // Get grid file paths from the file set
        QStringList gridFiles = fileSet->createPaths( ".EGRID" );
        if ( gridFiles.empty() )
        {
            gridFiles = fileSet->createPaths( ".GRID" );
        }

        // Filter out non-existing files
        QStringList existingGridFiles;
        for ( const QString& filePath : gridFiles )
        {
            if ( QFileInfo::exists( filePath ) )
            {
                existingGridFiles.append( filePath );
            }
            else
            {
                RiaLogging::warning( QString( "Grid file does not exist: %1" ).arg( filePath ) );
            }
        }
        gridFiles = existingGridFiles;

        if ( gridFiles.empty() )
        {
            RiaLogging::warning( QString( "No existing grid files found for ensemble '%1'" ).arg( fileSet->name() ) );
            continue;
        }

        // Read grid dimensions from all files to determine if they are identical
        bool                                            allGridsIdentical = true;
        RifReaderOpmCommon::GridDimensions              firstGridDimensions;
        std::vector<RifReaderOpmCommon::GridDimensions> allGridDimensions;

        {
            caf::ProgressInfo progress( gridFiles.size(), QString( "Reading grid dimensions for %1 files" ).arg( gridFiles.size() ) );

            for ( int i = 0; i < gridFiles.size(); i++ )
            {
                progress.setProgressDescription( QString( "Reading dimensions: %1" ).arg( gridFiles[i] ) );

                auto gridDimensions = RifReaderOpmCommon::readGridDimensions( gridFiles[i] );

                if ( i == 0 )
                {
                    firstGridDimensions = gridDimensions;
                }
                else
                {
                    if ( gridDimensions.i != firstGridDimensions.i || gridDimensions.j != firstGridDimensions.j ||
                         gridDimensions.k != firstGridDimensions.k )
                    {
                        allGridsIdentical = false;
                    }
                }

                allGridDimensions.push_back( gridDimensions );

                progress.incrementProgress();
            }
        }

        if ( allGridsIdentical )
        {
            // All grids have identical dimensions - use RimIdenticalGridCaseGroup
            RiaLogging::info( QString( "All %1 grids in '%2' have identical dimensions. Creating Identical Grid Case Group." )
                                  .arg( gridFiles.size() )
                                  .arg( fileSet->name() ) );

            RimIdenticalGridCaseGroup* gridCaseGroup = nullptr;
            RiaImportEclipseCaseTools::addEclipseCases( gridFiles, &gridCaseGroup );

            if ( gridCaseGroup )
            {
                gridCaseGroup->name = fileSet->name();
                Riu3DMainWindowTools::selectAsCurrentItem( gridCaseGroup );
            }
        }
        else
        {
            // Grids have different dimensions - use RimReservoirGridEnsemble
            RiaLogging::info( QString( "Grids in '%1' have varying dimensions. Creating Reservoir Grid Ensemble." ).arg( fileSet->name() ) );

            RimReservoirGridEnsemble* gridEnsemble = new RimReservoirGridEnsemble();
            gridEnsemble->setEnsembleFileSet( fileSet );

            project->assignIdToCaseGroup( gridEnsemble );
            eclipseCaseColl->reservoirGridEnsembles.push_back( gridEnsemble );

            gridEnsemble->loadDataAndUpdate();

            // Create view for first case if available
            auto allCases = gridEnsemble->cases();
            if ( !allCases.empty() )
            {
                RimEclipseView* view = gridEnsemble->addViewForCase( allCases[0] );
                if ( view )
                {
                    view->loadDataAndUpdate();
                }
            }

            Riu3DMainWindowTools::selectAsCurrentItem( gridEnsemble );
        }
    }

    eclipseCaseColl->updateAllRequiredEditors();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicCreateReservoirGridEnsembleFromFileSetFeature::setupActionLook( QAction* actionToSetup )
{
    actionToSetup->setIcon( QIcon( ":/GridCaseGroup16x16.png" ) );
    actionToSetup->setText( "Create Reservoir Grid Ensemble" );
}
