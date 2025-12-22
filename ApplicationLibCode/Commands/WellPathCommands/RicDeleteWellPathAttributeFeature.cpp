/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2018-     Equinor ASA
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
#include "RicDeleteWellPathAttributeFeature.h"

#include "ProjectDataModel/WellPath/RimWellPath.h"
#include "ProjectDataModel/WellPath/RimWellPathAttribute.h"
#include "ProjectDataModelCommands/RimcWellPath.h"

#include "cafPdmUiTree.h"
#include "cafPdmUiTreeView.h"
#include "cafSelectionManager.h"

#include <QPointer>

CAF_CMD_HEADER_INIT( RicDeleteWellPathAttributeFeature );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicDeleteWellPathAttributeFeature::isCommandEnabled() const
{
    auto attributes = caf::SelectionManager::instance()->objectsByType<RimWellPathAttribute>();
    if ( attributes.size() == 1 )
    {
        return true;
    }

    auto attributeCollections = caf::SelectionManager::instance()->objectsByType<RimWellPathAttributeCollection>();
    if ( attributeCollections.size() == 1 )
    {
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicDeleteWellPathAttributeFeature::onActionTriggered( bool )
{
    auto attributes = caf::SelectionManager::instance()->objectsByType<RimWellPathAttribute>();

    if ( attributes.empty() )
    {
        return;
    }

    auto firstAttribute = static_cast<RimWellPathAttribute*>( ( *attributes.begin() )->pdmObject() );
    auto wellPath       = firstAttribute->firstAncestorOrThisOfTypeAsserted<RimWellPath>();

    auto cmd = new RimcWellPathDeleteAttribute( wellPath );

    for ( auto attributeUiItem : attributes )
    {
        cmd->addWellPathAttribute( static_cast<RimWellPathAttribute*>( attributeUiItem->pdmObject() ) );
    }
    cmd->execute();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicDeleteWellPathAttributeFeature::setupActionLook( QAction* action )
{
    action->setEnabled( isCommandEnabled() );
}

void RicDeleteWellPathAttributeFeature::execute( const caf::PdmObjectHandle& object )
{
    if ( !object.isSubObject() )
    {
        return;
    }

    auto wellPathAttribute = object.object<RimWellPathAttribute>();
    if ( !wellPathAttribute )
    {
        return;
    }

    auto wellPath = wellPathAttribute->wellPath();
    if ( !wellPath )
    {
        return;
    }

    auto cmd = new RimcWellPathDeleteAttribute( wellPath );
    cmd->addWellPathAttribute( wellPathAttribute );
    cmd->submit();
}
