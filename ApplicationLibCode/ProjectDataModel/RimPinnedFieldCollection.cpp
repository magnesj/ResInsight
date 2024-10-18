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

#include "RimPinnedFieldCollection.h"

#include "RiaApplication.h"

#include "RimFieldQuickAccess.h"
#include "RimFieldQuickAccessInterface.h"
#include "RimFieldReference.h"
#include "RimGridView.h"
#include "RimProject.h"

#include "cafAssert.h"

CAF_PDM_SOURCE_INIT( RimPinnedFieldCollection, "RimFieldReferenceCollection" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimPinnedFieldCollection::RimPinnedFieldCollection()
{
    CAF_PDM_InitObject( "Field Reference Collection" );

    CAF_PDM_InitFieldNoDefault( &m_fieldQuickAccesGroups, "FieldReferencesGroup", "Field References Group" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimPinnedFieldCollection* RimPinnedFieldCollection::instance()
{
    auto proj = RimProject::current();
    CAF_ASSERT( proj && "RimProject is nullptr when trying to access RimFieldReferenceCollection::instance()" );

    return proj->pinnedFieldCollection();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPinnedFieldCollection::addQuickAccessFieldsRecursively( caf::PdmObjectHandle* object )
{
    if ( object == nullptr ) return;
    addQuickAccessFields( object );

    for ( auto field : object->fields() )
    {
        if ( !field ) continue;

        for ( auto childObject : field->children() )
        {
            addQuickAccessFieldsRecursively( childObject );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPinnedFieldCollection::addQuickAccessFields( caf::PdmObjectHandle* object )
{
    if ( !object ) return;

    if ( auto quickInterface = dynamic_cast<RimFieldQuickAccessInterface*>( object ) )
    {
        for ( const auto& [groupName, fields] : quickInterface->quickAccessFields() )
        {
            if ( auto group = findOrCreateGroup( object, groupName ) )
            {
                group->addFields( fields );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPinnedFieldCollection::addQuickAccessField( const RimFieldReference& fieldReference )
{
    auto object = fieldReference.object();
    auto field  = fieldReference.field();
    if ( object && field )
    {
        if ( auto group = findOrCreateGroup( object, "" ) )
        {
            group->addField( field );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPinnedFieldCollection::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    auto activeView = RiaApplication::instance()->activeGridView();
    if ( !activeView ) return;

    deleteMarkedObjects();

    std::set<RimFieldQuickAccessGroup*> groupsForView;

    for ( auto group : m_fieldQuickAccesGroups )
    {
        if ( group->ownerView() == activeView )
        {
            updateGroupName( group );
            groupsForView.insert( group );
        }
    }

    for ( auto group : groupsForView )
    {
        auto name = group->name();
        if ( name.isEmpty() ) name = defaultGroupName();

        caf::PdmUiGroup* uiGroup = uiOrdering.findGroup( name );
        if ( !uiGroup )
        {
            uiGroup = uiOrdering.addNewGroup( name );
        }

        for ( auto qa : group->fieldQuickAccesses() )
        {
            qa->uiOrdering( uiConfigName, *uiGroup );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPinnedFieldCollection::deleteMarkedObjects()
{
    std::set<RimFieldQuickAccess*> toBeDeleted;

    for ( auto group : m_fieldQuickAccesGroups.childrenByType() )
    {
        for ( auto qa : group->fieldQuickAccesses() )
        {
            if ( qa->markedForRemoval() )
            {
                toBeDeleted.insert( qa );
            }
        }
    }

    for ( auto qa : toBeDeleted )
    {
        for ( auto group : m_fieldQuickAccesGroups )
        {
            group->removeFieldQuickAccess( qa );
        }

        delete qa;
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimFieldQuickAccessGroup* RimPinnedFieldCollection::findOrCreateGroup( caf::PdmObjectHandle* object, const QString& groupName )
{
    if ( !object ) return nullptr;

    auto parentView = object->firstAncestorOrThisOfType<RimGridView>();
    if ( !parentView ) return nullptr;

    for ( auto group : m_fieldQuickAccesGroups )
    {
        if ( group && ( group->name() == groupName ) && ( group->ownerView() == parentView ) ) return group;
    }

    auto group = new RimFieldQuickAccessGroup();
    group->setName( groupName );
    group->setOwnerView( parentView );
    m_fieldQuickAccesGroups.push_back( group );

    return group;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPinnedFieldCollection::updateGroupName( RimFieldQuickAccessGroup* quickAccessGroup )
{
    if ( !quickAccessGroup ) return;

    caf::PdmObjectHandle* commonOwnerObject       = nullptr;
    caf::PdmFieldHandle*  firstFieldInQuickAccess = nullptr;

    for ( auto qa : quickAccessGroup->fieldQuickAccesses() )
    {
        if ( !qa || !qa->field() || !qa->field()->ownerObject() ) continue;

        if ( !firstFieldInQuickAccess ) firstFieldInQuickAccess = qa->field();

        auto ownerToField = qa->field()->ownerObject();
        if ( !commonOwnerObject )
        {
            commonOwnerObject = ownerToField;
        }
        else
        {
            if ( commonOwnerObject != ownerToField ) return;
        }
    }

    if ( auto fieldInterface = dynamic_cast<RimFieldQuickAccessInterface*>( commonOwnerObject ) )
    {
        auto ownerFields = fieldInterface->quickAccessFields();
        for ( const auto& [groupName, fields] : ownerFields )
        {
            for ( auto f : fields )
            {
                if ( f == firstFieldInQuickAccess )
                {
                    quickAccessGroup->setName( groupName );
                    return;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimPinnedFieldCollection::defaultGroupName()
{
    return "RimPinnedFieldCollection_GroupName";
}
