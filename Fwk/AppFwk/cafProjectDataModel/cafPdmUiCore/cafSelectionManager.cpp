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

#include "cafSelectionManager.h"

#include "cafPdmReferenceHelper.h"
#include "cafPdmUiFieldHandle.h"
#include "cafPdmUiObjectHandle.h"

namespace caf
{
//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
SelectionManager* SelectionManager::instance()
{
    static SelectionManager* singleton = new SelectionManager;
    return singleton;
}

//--------------------------------------------------------------------------------------------------
/// Obsolete.  Do not use this method.
//--------------------------------------------------------------------------------------------------
caf::NotificationCenter* SelectionManager::notificationCenter()
{
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<PdmUiItem*> SelectionManager::selectedItems() const
{
    std::vector<PdmUiItem*> items;
    for ( size_t i = 0; i < m_selection.size(); i++ )
    {
        if ( m_selection[i].first.notNull() )
        {
            items.push_back( m_selection[i].second );
        }
    }

    return items;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool SelectionManager::setSelectedItems( const std::vector<PdmUiItem*>& items )
{
    std::vector<std::pair<PdmPointer<PdmObjectHandle>, PdmUiItem*>> newSelection;
    extractInternalSelectionItems( items, &newSelection );

    if ( hasChanged( newSelection ) )
    {
        m_selection = newSelection;
        notifySelectionChanged();
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void SelectionManager::extractInternalSelectionItems(
    const std::vector<PdmUiItem*>&                                   items,
    std::vector<std::pair<PdmPointer<PdmObjectHandle>, PdmUiItem*>>* newSelection )
{
    for ( size_t i = 0; i < items.size(); i++ )
    {
        PdmUiFieldHandle* fieldHandle = dynamic_cast<PdmUiFieldHandle*>( items[i] );
        if ( fieldHandle )
        {
            PdmObjectHandle* obj = fieldHandle->fieldHandle()->ownerObject();

            newSelection->push_back( std::make_pair( obj, fieldHandle ) );
        }
        else
        {
            PdmUiObjectHandle* obj = dynamic_cast<PdmUiObjectHandle*>( items[i] );
            if ( obj )
            {
                newSelection->push_back( std::make_pair( obj->objectHandle(), obj ) );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void SelectionManager::setSelection( const std::vector<SelectionItem> completeSelection )
{
    std::vector<PdmUiItem*> items;
    for ( const auto& selectionItem : completeSelection )
    {
        items.push_back( selectionItem.item );
    }
    setSelectedItems( items );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool SelectionManager::hasChanged(
    const std::vector<std::pair<PdmPointer<PdmObjectHandle>, PdmUiItem*>>& newSelection ) const
{
    return m_selection != newSelection;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
PdmUiItem* SelectionManager::selectedItem() const
{
    if ( m_selection.size() == 1 )
    {
        if ( m_selection[0].first.notNull() )
        {
            return m_selection[0].second;
        }
    }

    return nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void SelectionManager::setSelectedItem( PdmUiItem* item )
{
    std::vector<PdmUiItem*> singleSelection;
    if ( item )
    {
        singleSelection.push_back( item );
    }

    setSelectedItems( singleSelection );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
SelectionManager::SelectionManager()
{
    m_activeChildArrayFieldHandle = nullptr;
    m_rootObject                  = nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<QString> SelectionManager::selectionAsReferences() const
{
    std::vector<QString> referenceList;
    for ( size_t i = 0; i < m_selection.size(); i++ )
    {
        if ( !m_selection[i].first.isNull() )
        {
            PdmUiObjectHandle* pdmObj = dynamic_cast<PdmUiObjectHandle*>( m_selection[i].second );
            if ( pdmObj )
            {
                QString itemRef = PdmReferenceHelper::referenceFromRootToObject( m_rootObject, pdmObj->objectHandle() );

                referenceList.push_back( itemRef );
            }
        }
    }

    return referenceList;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void SelectionManager::setSelectionFromReferences( const std::vector<QString>& referenceList )
{
    std::vector<PdmUiItem*> uiItems;

    for ( size_t i = 0; i < referenceList.size(); i++ )
    {
        QString reference = referenceList[i];

        PdmObjectHandle* pdmObj = PdmReferenceHelper::objectFromReference( m_rootObject, reference );
        if ( pdmObj )
        {
            caf::PdmUiObjectHandle* uiObject = uiObj( pdmObj );
            if ( uiObject )
            {
                uiItems.push_back( uiObject );
            }
        }
    }

    setSelectedItems( uiItems );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool SelectionManager::isSelected( PdmUiItem* item ) const
{
    auto iter = m_selection.begin();
    while ( iter != m_selection.end() )
    {
        if ( iter->second == item )
        {
            return true;
        }
        else
        {
            ++iter;
        }
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void SelectionManager::clearAll()
{
    if ( !m_selection.empty() )
    {
        m_selection.clear();
        notifySelectionChanged();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void SelectionManager::clear()
{
    clearAll();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void SelectionManager::removeObjectFromAllSelections( PdmObjectHandle* pdmObject )
{
    bool changed = false;
    m_selection.erase( std::remove_if( m_selection.begin(),
                                      m_selection.end(),
                                      [&]( const std::pair<PdmPointer<PdmObjectHandle>, PdmUiItem*>& selection ) {
                                          if ( selection.first.p() == pdmObject )
                                          {
                                              changed = true;
                                              return true;
                                          }
                                          return false;
                                      } ),
                       m_selection.end() );

    if ( changed )
    {
        notifySelectionChanged();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void SelectionManager::notifySelectionChanged()
{
    for ( auto receiver : m_selectionReceivers )
    {
        receiver->onSelectionManagerSelectionChanged();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void SelectionManager::setActiveChildArrayFieldHandle( PdmChildArrayFieldHandle* childArray )
{
    m_activeChildArrayFieldHandle = childArray;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
PdmChildArrayFieldHandle* SelectionManager::activeChildArrayFieldHandle()
{
    return m_activeChildArrayFieldHandle;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void SelectionManager::setPdmRootObject( PdmObjectHandle* root )
{
    m_rootObject = root;
}

} // namespace caf