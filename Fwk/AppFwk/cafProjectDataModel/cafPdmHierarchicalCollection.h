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

#pragma once

#include "cafPdmChildArrayField.h"
#include "cafPdmField.h"
#include "cafPdmObjectCollection.h"

#include <QString>

#include <vector>

namespace caf
{

//==================================================================================================
///
/// Generic templated hierarchical collection base class
///
/// Extends PdmObjectCollection<ItemT> with a user-visible collection name and a vector of
/// subcollections of the same (derived) type. Use this as a base for tree-shaped PDM containers
/// such as a folder of items that can also contain folders of the same kind.
///
/// Template parameters:
///   SelfT - The derived class (CRTP). Must inherit from caf::PdmObject.
///   ItemT - Item type held in the flat items vector. Must inherit from caf::PdmObject.
///
/// Derived classes must call CAF_PDM_InitFieldNoDefault for the inherited fields
/// m_items, m_collectionName and m_subCollections so the XML keywords stay derived-class
/// specific (matching the existing PdmObjectCollection<T> convention).
///
//==================================================================================================
template <typename SelfT, typename ItemT>
class PdmHierarchicalCollection : public PdmObjectCollection<ItemT>
{
    // No static_assert on SelfT here: in the typical CRTP usage SelfT is the derived class
    // and is incomplete at the point this template is instantiated as a base. The constraint
    // that SelfT inherits from PdmObject is enforced implicitly when m_subCollections is
    // exercised (PdmChildArrayField requires the pointee to be a PdmObject).

public:
    // Collection name
    QString collectionName() const;
    void    setCollectionName( const QString& name );

    // Subcollection access
    std::vector<SelfT*>               subCollections() const;
    PdmChildArrayField<SelfT*>&       subCollectionsField();
    const PdmChildArrayField<SelfT*>& subCollectionsField() const;

    // Subcollection CRUD
    void   addSubCollection( SelfT* sub );
    SelfT* findSubCollectionByName( const QString& name ) const;
    void   deleteSubCollectionByName( const QString& name );

protected:
    PdmHierarchicalCollection();
    ~PdmHierarchicalCollection() override;

    PdmFieldHandle* userDescriptionField() override;
    void            defineUiOrdering( QString uiConfigName, PdmUiOrdering& uiOrdering ) override;

    PdmField<QString>          m_collectionName;
    PdmChildArrayField<SelfT*> m_subCollections;
};

} // namespace caf

#include "cafPdmHierarchicalCollection.inl"
