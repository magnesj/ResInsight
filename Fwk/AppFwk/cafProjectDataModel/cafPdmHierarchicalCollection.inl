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

namespace caf
{

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
template <typename SelfT, typename ItemT>
PdmHierarchicalCollection<SelfT, ItemT>::PdmHierarchicalCollection()
{
    // m_items, m_collectionName and m_subCollections must be initialized by the derived class
    // using CAF_PDM_InitFieldNoDefault, so that the XML keywords are stable for that class.
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
template <typename SelfT, typename ItemT>
PdmHierarchicalCollection<SelfT, ItemT>::~PdmHierarchicalCollection()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
template <typename SelfT, typename ItemT>
QString PdmHierarchicalCollection<SelfT, ItemT>::collectionName() const
{
    return m_collectionName.value();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
template <typename SelfT, typename ItemT>
void PdmHierarchicalCollection<SelfT, ItemT>::setCollectionName( const QString& name )
{
    m_collectionName.setValue( name );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
template <typename SelfT, typename ItemT>
std::vector<SelfT*> PdmHierarchicalCollection<SelfT, ItemT>::subCollections() const
{
    return m_subCollections.childrenByType();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
template <typename SelfT, typename ItemT>
PdmChildArrayField<SelfT*>& PdmHierarchicalCollection<SelfT, ItemT>::subCollectionsField()
{
    return m_subCollections;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
template <typename SelfT, typename ItemT>
const PdmChildArrayField<SelfT*>& PdmHierarchicalCollection<SelfT, ItemT>::subCollectionsField() const
{
    return m_subCollections;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
template <typename SelfT, typename ItemT>
void PdmHierarchicalCollection<SelfT, ItemT>::addSubCollection( SelfT* sub )
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
SelfT* PdmHierarchicalCollection<SelfT, ItemT>::findSubCollectionByName( const QString& name ) const
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
void PdmHierarchicalCollection<SelfT, ItemT>::deleteSubCollectionByName( const QString& name )
{
    auto coll = findSubCollectionByName( name );
    if ( coll )
    {
        auto index = m_subCollections.indexOf( coll );
        m_subCollections.erase( index );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
template <typename SelfT, typename ItemT>
PdmFieldHandle* PdmHierarchicalCollection<SelfT, ItemT>::userDescriptionField()
{
    return &m_collectionName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
template <typename SelfT, typename ItemT>
void PdmHierarchicalCollection<SelfT, ItemT>::defineUiOrdering( QString uiConfigName, PdmUiOrdering& uiOrdering )
{
    uiOrdering.add( &m_collectionName );
    uiOrdering.add( &m_subCollections );
    uiOrdering.add( &this->m_items );
}

} // namespace caf
