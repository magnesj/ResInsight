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
#include "RicNewWellPathListTargetFeature.h"

#include "ProjectDataModel/WellPath/RimWellPath.h"
#include "ProjectDataModel/WellPath/RimWellPathGeometryDef.h"
#include "ProjectDataModel/WellPath/RimWellPathTarget.h"
#include "ProjectDataModelCommands/RimcWellPath.h"

#include "cafPdmUiItem.h"
#include <cafPdmUiTree.h>
#include <cafPdmUiTreeView.h>
#include <cafSelectionManager.h>

#include <QAction>
#include <QMenu>

CAF_CMD_SOURCE_INIT( RicNewWellPathListTargetFeature, "RicNewWellPathListTargetFeature" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicNewWellPathListTargetFeature::isCommandEnabled() const
{
    auto selectedTargets = caf::SelectionManager::instance()->objectsByType<RimWellPathTarget>();
    if ( selectedTargets.size() == 1 )
    {
        auto firstTarget = static_cast<RimWellPathTarget*>( ( *selectedTargets.begin() )->pdmObject() );
        auto wellPath    = firstTarget->firstAncestorOrThisOfTypeAsserted<RimWellPath>();
        if ( firstTarget->parent() != wellPath->wellPathGeometry() )
        {
            return false;
        }
    }
    else if ( selectedTargets.size() > 1 )
    {
        return false;
    }

    auto geomDefs = caf::SelectionManager::instance()->objectsByType<RimWellPathGeometryDef>();
    if ( !geomDefs.empty() )
    {
        auto wellGeomDef = static_cast<RimWellPathGeometryDef*>( ( *geomDefs.begin() )->pdmObject() );
        auto wellPath    = wellGeomDef->firstAncestorOrThisOfTypeAsserted<RimWellPath>();
        if ( wellGeomDef->wellPath() == wellPath )
        {
            return true;
        }
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicNewWellPathListTargetFeature::onActionTriggered( bool )
{
    auto selectedTargets = caf::SelectionManager::instance()->objectsByType<RimWellPathTarget>();
    if ( !selectedTargets.empty() )
    {
        auto                    firstTarget = static_cast<RimWellPathTarget*>( ( *selectedTargets.begin() )->pdmObject() );
        RimWellPathGeometryDef* wellGeomDef = firstTarget->firstAncestorOrThisOfTypeAsserted<RimWellPathGeometryDef>();
        auto                    cmd         = new RimcWellPathNewTarget( wellGeomDef );
        cmd->execute();
    }
    else
    {
        auto geomDefs = caf::SelectionManager::instance()->objectsByType<RimWellPathGeometryDef>();
        if ( !geomDefs.empty() )
        {
            auto wellGeomDef = static_cast<RimWellPathGeometryDef*>( ( *geomDefs.begin() )->pdmObject() );
            auto wellPath    = wellGeomDef->firstAncestorOrThisOfTypeAsserted<RimWellPath>();
            if ( wellGeomDef->wellPath() == wellPath )
            {
                auto cmd = new RimcWellPathNewTarget( wellGeomDef );
                cmd->execute();
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicNewWellPathListTargetFeature::setupActionLook( QAction* action )
{
    action->setEnabled( true );

    auto selectedTargets = caf::SelectionManager::instance()->objectsByType<RimWellPathTarget>();
    if ( selectedTargets.size() == 1 )
    {
        auto firstTarget = static_cast<RimWellPathTarget*>( ( *selectedTargets.begin() )->pdmObject() );
        auto wellPath    = firstTarget->firstAncestorOrThisOfTypeAsserted<RimWellPath>();
        if ( firstTarget->parent() == wellPath->wellPathGeometry() )
        {
            action->setEnabled( false );
            action->setToolTip( "Cannot add a new target when a target is selected" );
        }
    }

    auto geomDefs = caf::SelectionManager::instance()->objectsByType<RimWellPathGeometryDef>();
    if ( !geomDefs.empty() )
    {
        auto wellGeomDef = static_cast<RimWellPathGeometryDef*>( ( *geomDefs.begin() )->pdmObject() );
        auto wellPath    = wellGeomDef->firstAncestorOrThisOfTypeAsserted<RimWellPath>();
        if ( wellGeomDef->wellPath() == wellPath )
        {
        }
    }
}

void RicNewWellPathListTargetFeature::execute( const caf::PdmObjectHandle& object )
{
    auto wellPath = object.object<RimWellPath>();
    if ( !wellPath )
    {
        return;
    }

    std::vector<caf::PdmObjectHandle> targets;
    if ( object.isSubObject() )
    {
        auto firstTarget = object.object<RimWellPathTarget>();
        if ( firstTarget->parent() != wellPath->wellPathGeometry() )
        {
            return;
        }

        auto treeView = caf::PdmUiTree::instance()->treeView();
        auto objects  = treeView->selection()->selectedObjects();
        for ( const auto& obj : objects )
        {
            if ( obj.object<RimWellPathTarget>() )
            {
                targets.push_back( obj );
            }
        }
    }

    auto cmd = new RimcWellPathNewTarget( wellPath );
    cmd->addWellPathTargets( targets );
    cmd->submit();
}
