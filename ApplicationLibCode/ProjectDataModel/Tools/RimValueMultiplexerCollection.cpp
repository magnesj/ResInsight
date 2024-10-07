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

#include "RimValueMultiplexerCollection.h"

#include "RimValueMultiplexer.h"

#include "cafCmdFeatureMenuBuilder.h"

CAF_PDM_SOURCE_INIT( RimValueMultiplexerCollection, "RimValueMultiplexerCollection" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimValueMultiplexerCollection::RimValueMultiplexerCollection()
{
    CAF_PDM_InitObject( "RimValueMultiplexerCollection" );

    CAF_PDM_InitFieldNoDefault( &m_root, "Root", "Root" );

    CAF_PDM_InitFieldNoDefault( &m_valueMultiplexers, "ValueMultiplexers", "Value Multiplexers" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimValueMultiplexerCollection::setRoot( caf::PdmObject* root )
{
    m_root = root;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::PdmObject* RimValueMultiplexerCollection::root() const
{
    return m_root;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimValueMultiplexerCollection::addMultiplexer( caf::PdmObject* source,
                                                    const QString&  fieldName,
                                                    caf::PdmObject* destination,
                                                    const QString&  destinationFieldName )
{
    if ( source && destination )
    {
        auto sourceValueField      = dynamic_cast<caf::PdmValueField*>( source->findField( fieldName ) );
        auto destinationValueField = dynamic_cast<caf::PdmValueField*>( destination->findField( destinationFieldName ) );

        if ( sourceValueField && destinationValueField )
        {
            for ( auto m : m_valueMultiplexers )
            {
                if ( m->source() == source && m->sourceFieldName() == fieldName && m->destination() == destination &&
                     m->destinationFieldName() == destinationFieldName )
                {
                    return;
                }
            }
        }
    }

    auto multiplexer = new RimValueMultiplexer();
    multiplexer->setSource( source, fieldName );
    multiplexer->setDestination( destination, destinationFieldName );

    m_valueMultiplexers.push_back( multiplexer );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimValueMultiplexerCollection::removeMultiplexer( caf::PdmObject* source,
                                                       const QString&  fieldName,
                                                       caf::PdmObject* destination,
                                                       const QString&  destinationFieldName )
{
    if ( !source || !destination )
    {
        return;
    }

    std::vector<RimValueMultiplexer*> objectsToDelete;

    for ( auto m : m_valueMultiplexers )
    {
        if ( m->source() == source && m->sourceFieldName() == fieldName && m->destination() == destination &&
             m->destinationFieldName() == destinationFieldName )
        {
            objectsToDelete.push_back( m );
        }
    }

    for ( auto obj : objectsToDelete )
    {
        m_valueMultiplexers.removeChild( obj );

        delete obj;
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimValueMultiplexerCollection::notifyFieldChanged( caf::PdmObject* source, const QString& fieldName, QVariant newValue )
{
    for ( auto multiplexer : m_valueMultiplexers )
    {
        if ( multiplexer->source() == source && multiplexer->sourceFieldName() == fieldName )
        {
            if ( auto destinationField = multiplexer->destinationField() )
            {
                destinationField->setFromQVariant( newValue );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimValueMultiplexerCollection::appendMenuItems( caf::CmdFeatureMenuBuilder& menuBuilder ) const
{
    menuBuilder << "RicCreateValueMultiplexerFeature";
}
