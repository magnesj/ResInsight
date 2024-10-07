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

#include "RimValueMultiplexer.h"

#include "RimProject.h"

CAF_PDM_SOURCE_INIT( RimValueMultiplexer, "RimValueMultiplexer" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimValueMultiplexer::RimValueMultiplexer()
{
    CAF_PDM_InitObject( "RimValueMultiplexer" );

    CAF_PDM_InitFieldNoDefault( &m_source, "Source", "Source" );
    CAF_PDM_InitFieldNoDefault( &m_sourceFieldName, "SourceFieldName", "Source Fieldname" );

    CAF_PDM_InitFieldNoDefault( &m_destination, "Destination", "Destination" );
    CAF_PDM_InitFieldNoDefault( &m_destinationFieldName, "DestinationFieldName", "Destination Fieldname" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::PdmObject* RimValueMultiplexer::source() const
{
    return m_source();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimValueMultiplexer::sourceFieldName() const
{
    return m_sourceFieldName();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::PdmValueField* RimValueMultiplexer::sourceField() const
{
    if ( m_source )
    {
        return dynamic_cast<caf::PdmValueField*>( m_source->findField( m_sourceFieldName() ) );
    }

    return nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::PdmObject* RimValueMultiplexer::destination() const
{
    return m_destination();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimValueMultiplexer::destinationFieldName() const
{
    return m_destinationFieldName();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::PdmValueField* RimValueMultiplexer::destinationField() const
{
    if ( m_destination )
    {
        return dynamic_cast<caf::PdmValueField*>( m_destination->findField( m_destinationFieldName() ) );
    }

    return nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimValueMultiplexer::setSource( caf::PdmObject* source, const QString& fieldName )
{
    m_source          = source;
    m_sourceFieldName = fieldName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimValueMultiplexer::setDestination( caf::PdmObject* destination, const QString& fieldName )
{
    m_destination          = destination;
    m_destinationFieldName = fieldName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimValueMultiplexer::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
{
    QList<caf::PdmOptionItemInfo> options;

    if ( fieldNeedingOptions == &m_source || fieldNeedingOptions == &m_destination )
    {
        auto root = RimProject::current();
        if ( root )
        {
            auto allObjects = root->descendantsOfType<caf::PdmObject>();

            for ( auto obj : allObjects )
            {
                options.push_back( caf::PdmOptionItemInfo( obj->uiName(), obj ) );
            }
        }
    }
    else if ( fieldNeedingOptions == &m_sourceFieldName )
    {
        if ( m_source() )
        {
            auto allFields = m_source()->fields();

            for ( auto field : allFields )
            {
                if ( auto valueField = dynamic_cast<caf::PdmValueField*>( field ) )
                {
                    options.push_back( caf::PdmOptionItemInfo( valueField->keyword(), valueField->keyword() ) );
                }
            }
        }
    }
    else if ( fieldNeedingOptions == &m_destinationFieldName )
    {
        if ( m_destination() )
        {
            auto allFields = m_destination()->fields();

            for ( auto field : allFields )
            {
                if ( auto valueField = dynamic_cast<caf::PdmValueField*>( field ) )
                {
                    options.push_back( caf::PdmOptionItemInfo( valueField->keyword(), valueField->keyword() ) );
                }
            }
        }
    }

    return options;
}
