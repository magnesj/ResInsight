/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2015-     Statoil ASA
//  Copyright (C) 2015-     Ceetron Solutions AS
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

#include "RicWellPathsImportOsduFeature.h"

#include "RiaApplication.h"
#include "RiaGuiApplication.h"
#include "RiaOsduConnector.h"
#include "RiaPreferences.h"

#include "RimFileWellPath.h"
#include "RimOilField.h"
#include "RimOsduWellPath.h"
#include "RimProject.h"
#include "RimTools.h"
#include "RimWellPathCollection.h"
#include "RimWellPathImport.h"

#include "RiuMainWindow.h"
#include "RiuWellImportWizard.h"

#include <QAction>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QThread>

CAF_CMD_SOURCE_INIT( RicWellPathsImportOsduFeature, "RicWellPathsImportOsduFeature" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellPathsImportOsduFeature::onActionTriggered( bool isChecked )
{
    RiaApplication* app = RiaApplication::instance();
    if ( !app->project() ) return;

    if ( !app->isProjectSavedToDisc() )
    {
        RiaGuiApplication* guiApp = RiaGuiApplication::instance();
        if ( guiApp )
        {
            QMessageBox msgBox( guiApp->mainWindow() );
            msgBox.setIcon( QMessageBox::Question );

            QString questionText = QString( "Import of well paths will be stored as a part of a ResInsight project file. Please "
                                            "save the project to file before importing well paths." );

            msgBox.setText( questionText );
            msgBox.setInformativeText( "Do you want to save the project?" );
            msgBox.setStandardButtons( QMessageBox::Yes | QMessageBox::No );

            int ret = msgBox.exec();
            if ( ret == QMessageBox::Yes )
            {
                guiApp->saveProject();
            }
        }

        if ( !app->isProjectSavedToDisc() )
        {
            return;
        }
    }

    // Update the UTM bounding box from the reservoir
    app->project()->computeUtmAreaOfInterest();

    QString wellPathsFolderPath = RimFileWellPath::getCacheDirectoryPath();
    QDir::root().mkpath( wellPathsFolderPath );

    if ( !app->project()->wellPathImport() ) return;

    // Keep a copy of the import settings, and restore if cancel is pressed in the import wizard
    QString copyOfOriginalObject = app->project()->wellPathImport()->writeObjectToXmlString();

    if ( !app->preferences() ) return;

    RimProject* project = RimProject::current();
    if ( !project ) return;

    if ( project->oilFields.empty() ) return;

    RimOilField* oilField = project->activeOilField();
    if ( !oilField ) return;

    const QString server         = "https://npequinor.energy.azure.com";
    const QString dataParitionId = "npequinor-dev";
    const QString authority      = "https://login.microsoftonline.com/3aa4a235-b6e2-48d5-9195-7fcf05b459b0";
    const QString scopes         = "openid offline_access "
                                   "7daee810-3f78-40c4-84c2-7a199428de18/.default "
                                   "profile";
    const QString clientId       = "7a414874-4b27-4378-b34f-bc9e5a5faa4f";

    auto m_osduConnector = new RiaOsduConnector( RiuMainWindow::instance(), server, dataParitionId, authority, scopes, clientId );

    RiuWellImportWizard wellImportwizard( wellPathsFolderPath, m_osduConnector, app->project()->wellPathImport(), RiuMainWindow::instance() );

    // if ( oilField->wellPathCollection == nullptr )
    // {
    //     // printf("Create well path collection.\n");
    //     oilField->wellPathCollection = std::make_unique<RimWellPathCollection>();

    //     project->updateConnectedEditors();
    // }

    if ( QDialog::Accepted == wellImportwizard.exec() )
    {
        std::vector<RiuWellImportWizard::WellInfo> importedWells = wellImportwizard.importedWells();
        for ( auto w : importedWells )
        {
            qDebug() << "IMPORTING WELL: " << w.name;

            auto wellPath = new RimOsduWellPath;
            wellPath->setName( w.name );
            wellPath->setWellId( w.wellId );
            wellPath->setWellboreId( w.wellboreId );
            wellPath->setWellboreTrajectoryId( w.wellboreTrajectoryId );
            wellPath->setFileId( w.fileId );

            oilField->wellPathCollection->addWellPath( wellPath );
            oilField->wellPathCollection->updateConnectedEditors();
        }

        project->updateConnectedEditors();
        app->project()->scheduleCreateDisplayModelAndRedrawAllViews();
    }
    else
    {
        app->project()->wellPathImport()->readObjectFromXmlString( copyOfOriginalObject, caf::PdmDefaultObjectFactory::instance() );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellPathsImportOsduFeature::setupActionLook( QAction* actionToSetup )
{
    actionToSetup->setText( "Import Well Paths from &OSDU" );
    actionToSetup->setIcon( QIcon( ":/WellCollection.png" ) );
}
