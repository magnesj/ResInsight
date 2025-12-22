//##################################################################################################
//
//   Custom Visualization Core library
//   Copyright (C) 2011-2013 Ceetron AS
//
//   This library may be used under the terms of either the GNU General Public License or
//   the GNU Lesser General Public License as follows:
//
//   GNU General Public License Usage
//   This library is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//   See the GNU General Public License at <<http://www.gnu.org/licenses/gpl.html>>
//   for more details.
//
//   GNU Lesser General Public License Usage
//   This library is free software; you can redistribute it and/or modify
//   it under the terms of the GNU Lesser General Public License as published by
//   the Free Software Foundation; either version 2.1 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//   See the GNU Lesser General Public License at <<http://www.gnu.org/licenses/lgpl-2.1.html>>
//   for more details.
//
//##################################################################################################
#pragma once

#include "cafSelectionChangedReceiver.h"

#include "cafPdmObjectHandle.h"
#include "cafPdmPointer.h"
#include "cafPdmUiItem.h"

#include <QString>
#include <set>
#include <vector>

namespace caf
{
class NotificationCenter;
class PdmChildArrayFieldHandle;

//==================================================================================================
///
//==================================================================================================
class SelectionManager
{
public:
    static SelectionManager* instance();

    PdmUiItem*              selectedItem() const;
    std::vector<PdmUiItem*> selectedItems() const;

    void setSelectedItem( PdmUiItem* item, int selectionLevel );
    void setSelectedItem( PdmUiItem* item );

    bool setSelectedItems( const std::vector<PdmUiItem*>& items );

    struct SelectionItem
    {
        PdmUiItem* item;
    };
    void setSelection( const std::vector<SelectionItem> completeSelection );

    std::vector<QString> selectionAsReferences() const;
    void                 setSelectionFromReferences( const std::vector<QString>& referenceList );

    bool isSelected( PdmUiItem* item ) const;

    void clearAll();
    void clear();
    void removeObjectFromAllSelections( PdmObjectHandle* pdmObject );

    template <typename T>
    std::vector<T*> objectsByType() const
    {
        std::vector<PdmUiItem*> items = selectedItems();

        std::vector<T*> typedObjects;
        for ( size_t i = 0; i < items.size(); i++ )
        {
            T* obj = dynamic_cast<T*>( items[i] );
            if ( obj ) typedObjects.push_back( obj );
        }

        return typedObjects;
    }

    /// Returns the selected objects of the requested type if _all_ the selected objects are of the requested type

    template <typename T>
    std::vector<T*> objectsByTypeStrict() const
    {
        std::vector<PdmUiItem*> items = selectedItems();

        std::vector<T*> typedObjects;
        for ( size_t i = 0; i < items.size(); i++ )
        {
            T* obj = dynamic_cast<T*>( items[i] );
            if ( !obj )
            {
                return {};
            }
            typedObjects.push_back( obj );
        }
        return typedObjects;
    }

    template <typename T>
    T* selectedItemOfType() const
    {
        std::vector<T*> typedObjects = objectsByType<T>();
        if ( !typedObjects.empty() )
        {
            return typedObjects.front();
        }
        return nullptr;
    }

    template <typename T>
    T* selectedItemAncestorOfType() const
    {
        PdmUiItem*       item           = this->selectedItem();
        PdmObjectHandle* selectedObject = dynamic_cast<PdmObjectHandle*>( item );
        if ( selectedObject )
        {
            return selectedObject->firstAncestorOrThisOfType<T>();
        }
        return nullptr;
    }

    // OBSOLETE ! Remove when time to refactor the command system
    NotificationCenter* notificationCenter();

    void                      setActiveChildArrayFieldHandle( PdmChildArrayFieldHandle* childArray );
    PdmChildArrayFieldHandle* activeChildArrayFieldHandle();

    void             setPdmRootObject( PdmObjectHandle* root );
    PdmObjectHandle* pdmRootObject() { return m_rootObject; }
    // End OBSOLETE

private:
    SelectionManager();

    static void extractInternalSelectionItems(
        const std::vector<PdmUiItem*>&                                   items,
        std::vector<std::pair<PdmPointer<PdmObjectHandle>, PdmUiItem*>>* internalSelectionItems );

    void notifySelectionChanged();
    bool hasChanged( const std::vector<std::pair<PdmPointer<PdmObjectHandle>, PdmUiItem*>>& newCompleteSelection ) const;

    friend class SelectionChangedReceiver;
    void registerSelectionChangedReceiver( SelectionChangedReceiver* receiver )
    {
        m_selectionReceivers.insert( receiver );
    }
    void unregisterSelectionChangedReceiver( SelectionChangedReceiver* receiver )
    {
        m_selectionReceivers.erase( receiver );
    }

private:
    std::vector<std::pair<PdmPointer<PdmObjectHandle>, PdmUiItem*>> m_selection;

    PdmChildArrayFieldHandle*   m_activeChildArrayFieldHandle;
    PdmPointer<PdmObjectHandle> m_rootObject;

    std::set<SelectionChangedReceiver*> m_selectionReceivers;
};

} // end namespace caf
