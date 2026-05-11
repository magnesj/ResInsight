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

#include "cafPdmNestedCollectionBase.h"

namespace caf
{

CAF_PDM_ABSTRACT_SOURCE_INIT( PdmNestedCollectionBase, "PdmNestedCollectionBase" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
PdmNestedCollectionBase::PdmNestedCollectionBase()
    : m_isTopLevelFolder( false )
{
    CAF_PDM_InitObject( "Nested Collection" );

    // m_collectionName is initialized by derived classes with a derived-specific XML keyword,
    // matching the existing per-class scriptable-field convention.
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
PdmNestedCollectionBase::~PdmNestedCollectionBase()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString PdmNestedCollectionBase::collectionName() const
{
    return m_collectionName.value();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void PdmNestedCollectionBase::setCollectionName( const QString& name )
{
    m_collectionName.setValue( name );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void PdmNestedCollectionBase::setAsTopmostFolder()
{
    m_collectionName.uiCapability()->setUiHidden( true );
    m_collectionName.xmlCapability()->disableIO();
    setDeletable( false );
    m_isTopLevelFolder = true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
PdmFieldHandle* PdmNestedCollectionBase::userDescriptionField()
{
    if ( m_isTopLevelFolder ) return nullptr;
    return &m_collectionName;
}

} // namespace caf
