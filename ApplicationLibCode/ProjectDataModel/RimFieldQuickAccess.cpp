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
    }

    uiOrdering.add( &m_selectObjectButton, { .newRow = false } );
    uiOrdering.add( &m_removeItemButton, { .newRow = false } );

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

//--------------------------------------------------------------------------------------------------
///
///
///
///
///
///
///
//--------------------------------------------------------------------------------------------------

CAF_PDM_SOURCE_INIT( RimFieldQuickAccessGroup, "RimFieldQuickAccessGroup" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimFieldQuickAccessGroup::RimFieldQuickAccessGroup()
{
    CAF_PDM_InitObject( "Quick Access Group" );

    CAF_PDM_InitFieldNoDefault( &m_fieldQuickAccess, "FieldReferences", "Field References" );
    CAF_PDM_InitFieldNoDefault( &m_ownerView, "OwnerView", "Owner View" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimGridView* RimFieldQuickAccessGroup::ownerView() const
{
    return m_ownerView;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFieldQuickAccessGroup::setOwnerView( RimGridView* viewObject )
{
    m_ownerView = viewObject;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFieldQuickAccessGroup::addFields( const std::vector<caf::PdmFieldHandle*>& fields )
{
    if ( !m_ownerView ) return;

    /*
        bool rebuildObject = false;

        for ( auto field : fields )
        {
            if ( !field ) continue;

            if ( !hasField( field ) ) rebuildObject = true;
        }

        if ( !rebuildObject ) return;

        m_fieldQuickAccess.deleteChildren();
    */

    for ( auto field : fields )
    {
        if ( findField( field ) ) continue;

        addField( field );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFieldQuickAccessGroup::addField( caf::PdmFieldHandle* field )
{
    if ( !field ) return;
    if ( !m_ownerView ) return;

    if ( !isOwnerViewMatching( field ) ) return;

    auto fieldReference = new RimFieldQuickAccess();
    fieldReference->setField( field );

    addFieldQuickAccess( fieldReference );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RimFieldQuickAccess*> RimFieldQuickAccessGroup::fieldQuickAccesses() const
{
    return m_fieldQuickAccess.childrenByType();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::PdmObjectHandle* RimFieldQuickAccessGroup::ownerObject_obsolete() const
{
    for ( auto f : m_fieldQuickAccess )
    {
        if ( f && f->field() )
        {
            return f->field()->ownerObject();
        }
    }

    return nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFieldQuickAccessGroup::addFieldQuickAccess( RimFieldQuickAccess* fieldQuickAccess )
{
    if ( !fieldQuickAccess ) return;

    m_fieldQuickAccess.push_back( fieldQuickAccess );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimFieldQuickAccessGroup::removeFieldQuickAccess( RimFieldQuickAccess* fieldQuickAccess )
{
    m_fieldQuickAccess.removeChild( fieldQuickAccess );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimFieldQuickAccessGroup::findField( const caf::PdmFieldHandle* field ) const
{
    for ( auto fieldRef : m_fieldQuickAccess )
    {
        if ( field == fieldRef->field() )
        {
            return true;
        }
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimFieldQuickAccessGroup::isOwnerViewMatching( caf::PdmFieldHandle* field )
{
    if ( !field || !field->ownerObject() ) return false;
    auto parentView = field->ownerObject()->firstAncestorOrThisOfType<RimGridView>();

    if ( parentView != m_ownerView )
    {
        RiaLogging::debug( "Field does not belong to the owner view" );
        return false;
    }

    return true;
}
