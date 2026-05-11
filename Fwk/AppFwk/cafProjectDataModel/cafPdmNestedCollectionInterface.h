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

#pragma once

#include <QString>

namespace caf
{
class PdmObject;

//==================================================================================================
///
/// Non-templated interface implemented by caf::PdmNestedCollection<SelfT, ItemT>.
///
/// Lets generic command features operate on any nested collection without knowing the concrete
/// SelfT / ItemT template parameters.
///
//==================================================================================================
class PdmNestedCollectionInterface
{
public:
    virtual ~PdmNestedCollectionInterface() = default;

    virtual QString collectionName() const                   = 0;
    virtual void    setCollectionName( const QString& name ) = 0;

    // Whether this container accepts sub-collections. Leaf containers (e.g. file-backed
    // folders) return false, in which case addNewSubCollection() must not be called and
    // generic UI features should hide the corresponding action.
    virtual bool canAddSubCollection() const { return true; }

    // Creates a subcollection, adds it to this collection, and returns it. Ownership of
    // the returned object is held by this collection. Must not be called when
    // canAddSubCollection() returns false.
    virtual PdmObject* addNewSubCollection() = 0;
};

} // namespace caf
