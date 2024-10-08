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

#include "RicCreateValueMultiplexerFeature.h"

#include "RimProject.h"
#include "Tools/RimValueMultiplexerCollection.h"

#include "cafSelectionManagerTools.h"

#include <QAction>

CAF_CMD_SOURCE_INIT( RicCreateValueMultiplexerFeature, "RicCreateValueMultiplexerFeature" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicCreateValueMultiplexerFeature::onActionTriggered( bool isChecked )
{
    auto            generalObjects = caf::selectedObjectsByType<caf::PdmObject*>();
    caf::PdmObject* firstObject    = nullptr;
    caf::PdmObject* secondObject   = nullptr;

    if ( !generalObjects.empty() )
    {
        firstObject = generalObjects.front();
    }

    if ( generalObjects.size() > 1 )
    {
        secondObject = generalObjects.at( 1 );
    }

    auto multiplexerCollection = RimProject::current()->valueMultiplexerCollection();
    multiplexerCollection->addMultiplexer( firstObject, "", secondObject, "" );
    multiplexerCollection->updateAllRequiredEditors();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicCreateValueMultiplexerFeature::setupActionLook( QAction* actionToSetup )
{
    actionToSetup->setText( "Create Value Multiplexer" );
    // actionToSetup->setIcon( QIcon( ":/SummaryEnsemble.svg" ) );
}
