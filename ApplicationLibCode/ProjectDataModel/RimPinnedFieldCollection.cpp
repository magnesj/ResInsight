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

    CAF_PDM_InitFieldNoDefault( &m_fieldQuickAccesses, "FieldReferences", "Field References" );
    m_fieldQuickAccesses = new RimFieldQuickAccessGroup();

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
        auto fields = quickInterface->quickAccessFields();

        for ( const auto& [groupName, fields] : fields )
        {
            if ( groupName.isEmpty() )
            {
                for ( auto f : fields )
                {
                    addField( f );
                }
            }
            else
            {
                auto group = findGroup( groupName );
                if ( group )
                {
                    group->addFields( fields );
                }
                else
                {
                    group = new RimFieldQuickAccessGroup();
                    group->setName( groupName );
                    group->addFields( fields );
                    m_fieldQuickAccesGroups.push_back( group );
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPinnedFieldCollection::addField( caf::PdmFieldHandle* field )
{
    if ( !field ) return;

    for ( auto quickAccess : m_fieldQuickAccesses->fieldQuickAccesses() )
    {
        if ( field == quickAccess->field() )
        {
            return;
        }
    }

    auto qa = new RimFieldQuickAccess();
    qa->setField( field );

    m_fieldQuickAccesses->addFieldQuickAccess( qa );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPinnedFieldCollection::removeField( caf::PdmFieldHandle* field )
{
    if ( !field ) return;

    for ( auto fieldRef : m_fieldQuickAccesses->fieldQuickAccesses() )
    {
        if ( field == fieldRef->field() )
        {
            m_toBeDeleted.insert( fieldRef );
            return;
        }
    }

    for ( auto group : m_fieldQuickAccesGroups )
    {
        for ( auto fieldRef : group->fieldQuickAccesses() )
        {
            if ( field == fieldRef->field() )
            {
                m_toBeDeleted.insert( fieldRef );
                return;
            }
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

    {
        std::set<RimFieldQuickAccess*> objForView;
        for ( auto qa : m_fieldQuickAccesses->fieldQuickAccesses() )
        {
            if ( !qa ) continue;

            if ( auto field = qa->field() )
            {
                if ( auto ownerObject = field->ownerObject() )
                {
                    auto view = ownerObject->firstAncestorOrThisOfType<RimGridView>();
                    if ( view == activeView )
                    {
                        objForView.insert( qa );
                    }
                }
            }
        }

        for ( auto qa : objForView )
        {
            qa->uiOrdering( uiConfigName, uiOrdering );
        }
    }

    {
        std::set<RimFieldQuickAccessGroup*> groupsForView;
        for ( auto group : m_fieldQuickAccesGroups )
        {
            for ( auto qa : group->fieldQuickAccesses() )
            {
                if ( !qa ) continue;

                if ( auto field = qa->field() )
                {
                    if ( auto ownerObject = field->ownerObject() )
                    {
                        auto view = ownerObject->firstAncestorOrThisOfType<RimGridView>();
                        if ( view == activeView )
                        {
                            groupsForView.insert( group );
                        }
                    }
                }
            }
        }

        for ( auto group : groupsForView )
        {
            auto name = group->name();

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
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPinnedFieldCollection::deleteMarkedObjects()
{
    for ( auto group : allGroups() )
    {
        for ( auto qa : group->fieldQuickAccesses() )
        {
            if ( qa->markedForRemoval() )
            {
                m_toBeDeleted.insert( qa );
            }
        }
    }

    for ( auto qa : m_toBeDeleted )
    {
        m_fieldQuickAccesses.removeChild( qa );

        for ( auto group : m_fieldQuickAccesGroups )
        {
            group->removeFieldQuickAccess( qa );
        }

        delete qa;
    }

    m_toBeDeleted.clear();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimFieldQuickAccessGroup* RimPinnedFieldCollection::findGroup( const QString& groupName ) const
{
    for ( auto group : m_fieldQuickAccesGroups )
    {
        if ( group->name() == groupName )
        {
            return group;
        }
    }

    return nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RimFieldQuickAccessGroup*> RimPinnedFieldCollection::allGroups() const
{
    std::vector<RimFieldQuickAccessGroup*> all;
    all.push_back( m_fieldQuickAccesses );

    for ( auto a : m_fieldQuickAccesGroups )
    {
        all.push_back( a );
    }

    return all;
}
