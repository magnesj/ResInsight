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

#include "RicDeleteItemExec.h"
#include "RicDeleteItemExecData.h"

#include "cafNotificationCenter.h"
#include "cafPdmChildArrayField.h"
#include "cafPdmDocument.h"
#include "cafPdmReferenceHelper.h"
#include "cafPdmUiFieldHandle.h"
#include "cafSelectionManager.h"

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RicDeleteItemExec::name()
{
    if ( !m_commandData.m_description().isEmpty() )
    {
        return m_commandData.m_description();
    }

    return m_commandData.classKeyword();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicDeleteItemExec::redo()
{
    caf::PdmFieldHandle* field = caf::PdmReferenceHelper::fieldFromReference( m_commandData.m_rootObject, m_commandData.m_pathToField );

    caf::PdmChildArrayFieldHandle* listField = dynamic_cast<caf::PdmChildArrayFieldHandle*>( field );
    if ( listField )
    {
        std::vector<caf::PdmObjectHandle*> children = listField->children();

        caf::PdmObjectHandle* obj = children[m_commandData.m_indexToObject];
        caf::SelectionManager::instance()->removeObjectFromAllSelections( obj );

        std::vector<caf::PdmObjectHandle*> referringObjects = obj->objectsWithReferringPtrFields();

        if ( m_commandData.m_deletedObjectAsXml().isEmpty() )
        {
            m_commandData.m_deletedObjectAsXml = xmlObj( obj )->writeObjectToXmlString();
        }

        caf::PdmObjectHandle* parentObj = listField->ownerObject();

        // Detach the object and refresh the editors before deleting it, so that nothing can reach it
        // while its destructor runs.
        //
        // A destructor can re-enter the user interface. Deleting a 3D view removes its dock widget,
        // the dock area then activates a sibling, and that selection change both repaints the project
        // tree and rebuilds the property editor. The tree asks each of its nodes for a name, and the
        // property editor walks project->allViews() to build the comparison view option list, which
        // also asks every view for its name. The object being deleted has already lost its derived
        // part by then, so producing its name calls a pure virtual function and aborts.
        //
        // erase() only detaches the object from the field, it does not delete it, so the object is
        // still valid while the editors are refreshed. After the refresh no editor holds a node for
        // it, and it is no longer reachable through the project.
        listField->erase( m_commandData.m_indexToObject );
        parentObj->uiCapability()->updateConnectedEditors();

        delete obj;

        parentObj->onChildDeleted( listField, referringObjects );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicDeleteItemExec::undo()
{
    caf::PdmFieldHandle* field = caf::PdmReferenceHelper::fieldFromReference( m_commandData.m_rootObject, m_commandData.m_pathToField );

    caf::PdmChildArrayFieldHandle* listField = dynamic_cast<caf::PdmChildArrayFieldHandle*>( field );
    if ( listField )
    {
        caf::PdmObjectHandle* obj = caf::PdmXmlObjectHandle::readUnknownObjectFromXmlString( m_commandData.m_deletedObjectAsXml(),
                                                                                             caf::PdmDefaultObjectFactory::instance(),
                                                                                             false );

        listField->insertAt( m_commandData.m_indexToObject, obj );

        obj->xmlCapability()->initAfterReadRecursively();
        obj->xmlCapability()->resolveReferencesRecursively();

        listField->uiCapability()->updateConnectedEditors();
        listField->ownerObject()->uiCapability()->updateConnectedEditors();

        caf::PdmObjectHandle* parentObj = listField->ownerObject();
        if ( parentObj )
        {
            std::vector<caf::PdmObjectHandle*> referringObjects;

            // TODO: Here we need a different concept like onChildAdded()
            parentObj->onChildDeleted( listField, referringObjects );
        }

        if ( m_notificationCenter ) m_notificationCenter->notifyObservers();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RicDeleteItemExec::RicDeleteItemExec( caf::NotificationCenter* notificationCenter )
    : CmdExecuteCommand( notificationCenter )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RicDeleteItemExecData& RicDeleteItemExec::commandData()
{
    return m_commandData;
}
