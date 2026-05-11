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

#include "cafPdmcAddFolderMethod.h"

#include "cafPdmFieldScriptingCapability.h"
#include "cafPdmNestedCollectionBase.h"

namespace caf
{

CAF_PDM_OBJECT_METHOD_SOURCE_INIT( PdmNestedCollectionBase, PdmcAddFolderMethod, "AddFolder" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
PdmcAddFolderMethod::PdmcAddFolderMethod( PdmObjectHandle* self )
    : PdmObjectCreationMethod( self )
{
    CAF_PDM_InitObject( "Add Folder", "", "", "Add a new folder" );

    CAF_PDM_InitScriptableField( &m_folderName, "FolderName", QString( "Folder" ), "", "", "", "New folder name" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::expected<PdmObjectHandle*, QString> PdmcAddFolderMethod::execute()
{
    auto* coll = self<PdmNestedCollectionBase>();
    if ( !coll || !coll->canAddSubCollection() )
    {
        return std::unexpected<QString>( QString( "Cannot add subfolder" ) );
    }

    PdmObject* added = coll->addNewSubCollection();
    if ( !added )
    {
        return std::unexpected<QString>( QString( "Failed to add subfolder" ) );
    }

    if ( auto* asNested = dynamic_cast<PdmNestedCollectionBase*>( added ) )
    {
        asNested->setCollectionName( m_folderName() );
    }
    coll->uiCapability()->updateConnectedEditors();
    return added;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString PdmcAddFolderMethod::classKeywordReturnedType() const
{
    // Returned at codegen time. The Python generator emits AddFolder on the
    // PdmNestedCollectionBase Python class (since that is where the method is registered),
    // so the declared return type must be stable across all concrete nested collections.
    // The actual runtime instance is whatever the concrete addNewSubCollection() returns;
    // callers can use descendants() / class_from_keyword to recover the precise type.
    return PdmNestedCollectionBase::classKeywordStatic();
}

} // namespace caf
