/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2024- Equinor ASA
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

#include "RimFieldQuickAccess.h"

#include "RiaQuickAccessScheduler.h"

#include "RimFieldReference.h"

#include "Riu3DMainWindowTools.h"

#include "cafPdmUiToolButtonEditor.h"

CAF_PDM_SOURCE_INIT( RimFieldQuickAccess, "RimFieldQuickAccess" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimFieldQuickAccess::RimFieldQuickAccess()
{
    CAF_PDM_InitObject( "Quick Access" );

    CAF_PDM_InitFieldNoDefault( &m_fieldReference, "FieldReference", "FieldReference" );
    m_fieldReference = new RimFieldReference();

    CAF_PDM_InitFieldNoDefault( &m_selectObjectButton, "SelectObject", "...", ":/Bullet.png", "Select Object in Property Editor" );
    m_selectObjectButton.uiCapability()->setUiEditorTypeName( caf::PdmUiToolButtonEditor::uiEditorTypeName() );
    m_selectObjectButton.registerSetMethod( this, &RimFieldQuickAccess::onSelectObjectButton );
    m_selectObjectButton.xmlCapability()->disableIO();

    CAF_PDM_InitFieldNoDefault( &m_removeItemButton, "RemoveItem", "...", ":/pin.svg", "Remove Quick Access" );
    m_removeItemButton.uiCapability()->setUiEditorTypeName( caf::PdmUiToolButtonEditor::uiEditorTypeName() );
    m_removeItemButton.registerSetMethod( this, &RimFieldQuickAccess::onRemoveObjectButton );
    m_removeItemButton.xmlCapability()->disableIO();

    m_markedForRemoval = false;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFieldQuickAccess::setField( caf::PdmFieldHandle* field )
{
    if ( !m_fieldReference() ) return;

    m_fieldReference->setField( field );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFieldQuickAccess::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    if ( m_fieldReference() && m_fieldReference()->field() )
    {
        uiOrdering.add( m_fieldReference()->field() );
        uiOrdering.add( &m_selectObjectButton, { .newRow = false } );
        uiOrdering.add( &m_removeItemButton, { .newRow = false } );
    }

    uiOrdering.skipRemainingFields();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFieldQuickAccess::onSelectObjectButton( const bool& state )
{
    if ( m_fieldReference() )
    {
        if ( auto pdmObj = dynamic_cast<caf::PdmObject*>( m_fieldReference->object() ) )
        {
            Riu3DMainWindowTools::selectAsCurrentItem( pdmObj );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFieldQuickAccess::onRemoveObjectButton( const bool& state )
{
    m_markedForRemoval = true;

    RiaQuickAccessScheduler::instance()->scheduleDisplayModelUpdateAndRedraw();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFieldQuickAccess::defineEditorAttribute( const caf::PdmFieldHandle* field, QString uiConfigName, caf::PdmUiEditorAttribute* attribute )
{
    if ( auto attr = dynamic_cast<caf::PdmUiToolButtonEditorAttribute*>( attribute ) )
    {
        attr->m_checkable          = false;
        attr->m_invertCurrentValue = true;
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::PdmFieldHandle* RimFieldQuickAccess::field() const
{
    if ( !m_fieldReference() ) return nullptr;

    return m_fieldReference->field();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimFieldQuickAccess::markedForRemoval() const
{
    return m_markedForRemoval;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFieldQuickAccess::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    if ( changedField == &m_selectObjectButton )
    {
        m_selectObjectButton = false;

        if ( m_fieldReference() )
        {
            if ( auto pdmObj = dynamic_cast<caf::PdmObject*>( m_fieldReference->object() ) )
            {
                Riu3DMainWindowTools::selectAsCurrentItem( pdmObj );
            }
        }
    }

    if ( changedField == &m_removeItemButton )
    {
        m_removeItemButton = false;

        m_markedForRemoval = true;
    }
}
