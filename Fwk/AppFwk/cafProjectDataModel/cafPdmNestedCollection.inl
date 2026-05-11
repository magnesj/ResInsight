/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2026     Equinor ASA
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

namespace caf
{

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
template <typename SelfT, typename ItemT>
PdmNestedCollection<SelfT, ItemT>::PdmNestedCollection()
    : m_isTopLevelFolder( false )
{
    static_assert( DerivedFromPdmObject<SelfT>, "SelfT must inherit from caf::PdmObject" );
    // m_items, m_collectionName and m_subCollections must be initialized by the derived class
    // using CAF_PDM_InitFieldNoDefault, so that the XML keywords are stable for that class.
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
template <typename SelfT, typename ItemT>
PdmNestedCollection<SelfT, ItemT>::~PdmNestedCollection()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
template <typename SelfT, typename ItemT>
QString PdmNestedCollection<SelfT, ItemT>::collectionName() const
{
    return m_collectionName.value();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
template <typename SelfT, typename ItemT>
void PdmNestedCollection<SelfT, ItemT>::setCollectionName( const QString& name )
{
    m_collectionName.setValue( name );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
template <typename SelfT, typename ItemT>
std::vector<SelfT*> PdmNestedCollection<SelfT, ItemT>::subCollections() const
{
    return m_subCollections.childrenByType();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
template <typename SelfT, typename ItemT>
void PdmNestedCollection<SelfT, ItemT>::addSubCollection( SelfT* sub )
{
    if ( sub )
    {
        m_subCollections.push_back( sub );
        this->updateConnectedEditors();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
template <typename SelfT, typename ItemT>
PdmObject* PdmNestedCollection<SelfT, ItemT>::addNewSubCollection()
{
    auto* sub = new SelfT();
    addSubCollection( sub );
    return sub;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
template <typename SelfT, typename ItemT>
std::vector<ItemT*> PdmNestedCollection<SelfT, ItemT>::allItems() const
{
    std::vector<ItemT*> result = this->items();
    for ( auto* sub : subCollections() )
    {
        if ( !sub ) continue;
        auto subItems = sub->allItems();
        result.insert( result.end(), subItems.begin(), subItems.end() );
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
template <typename SelfT, typename ItemT>
SelfT* PdmNestedCollection<SelfT, ItemT>::findSubCollectionByName( const QString& name ) const
{
    for ( auto coll : m_subCollections )
    {
        if ( coll && coll->collectionName() == name ) return coll;
    }

    return nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
template <typename SelfT, typename ItemT>
PdmFieldHandle* PdmNestedCollection<SelfT, ItemT>::userDescriptionField()
{
    if ( m_isTopLevelFolder ) return nullptr;
    return &m_collectionName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
template <typename SelfT, typename ItemT>
void PdmNestedCollection<SelfT, ItemT>::setAsTopmostFolder()
{
    m_collectionName.uiCapability()->setUiHidden( true );
    m_collectionName.xmlCapability()->disableIO();
    this->setDeletable( false );
    m_isTopLevelFolder = true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
template <typename SelfT, typename ItemT>
void PdmNestedCollection<SelfT, ItemT>::defineUiOrdering( QString uiConfigName, PdmUiOrdering& uiOrdering )
{
    uiOrdering.add( &m_collectionName );
    uiOrdering.add( &m_subCollections );
    uiOrdering.add( &this->m_items );
}

} // namespace caf
