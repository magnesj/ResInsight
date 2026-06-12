/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2025-     Equinor ASA
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

#include "RiaAutomationJson.h"

#include "RimProject.h"

#include "cafPdmAbstractFieldScriptingCapability.h"
#include "cafPdmFieldHandle.h"
#include "cafPdmObjectHandle.h"
#include "cafPdmUiObjectHandle.h"
#include "cafPdmXmlObjectHandle.h"

#include <QJsonArray>
#include <QTextStream>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiaAutomationJson::addressOf( const caf::PdmObjectHandle* object )
{
    return QString::number( reinterpret_cast<quintptr>( object ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
static QJsonArray scriptableFieldsToJson( caf::PdmObjectHandle* object )
{
    QJsonArray fieldsArray;

    for ( caf::PdmFieldHandle* field : object->fields() )
    {
        auto scriptability = field->capability<caf::PdmAbstractFieldScriptingCapability>();
        if ( !scriptability ) continue;

        QString     valueText;
        QTextStream stream( &valueText );
        scriptability->readFromField( stream );

        QJsonObject fieldObject;
        fieldObject["name"]  = scriptability->scriptFieldName();
        fieldObject["value"] = valueText;

        const QString dataType = scriptability->dataType();
        if ( !dataType.isEmpty() ) fieldObject["dataType"] = dataType;

        fieldsArray.append( fieldObject );
    }

    return fieldsArray;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QJsonObject RiaAutomationJson::pdmObjectToJson( caf::PdmObjectHandle* object, int maxDepth )
{
    QJsonObject json;
    if ( !object ) return json;

    json["address"] = addressOf( object );

    if ( caf::PdmXmlObjectHandle* xmlObject = object->xmlCapability() )
    {
        json["keyword"] = xmlObject->classKeyword();
    }

    if ( caf::PdmUiObjectHandle* uiObject = object->uiCapability() )
    {
        json["uiName"] = uiObject->uiName();
    }

    json["fields"] = scriptableFieldsToJson( object );

    if ( maxDepth != 0 )
    {
        const int  childDepth = maxDepth < 0 ? -1 : maxDepth - 1;
        QJsonArray childrenArray;
        for ( caf::PdmFieldHandle* field : object->fields() )
        {
            for ( caf::PdmObjectHandle* child : field->children() )
            {
                if ( child ) childrenArray.append( pdmObjectToJson( child, childDepth ) );
            }
        }
        if ( !childrenArray.isEmpty() ) json["children"] = childrenArray;
    }

    return json;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
static caf::PdmObjectHandle* findObjectRecursive( caf::PdmObjectHandle* object, const QString& address )
{
    if ( !object ) return nullptr;
    if ( RiaAutomationJson::addressOf( object ) == address ) return object;

    for ( caf::PdmFieldHandle* field : object->fields() )
    {
        for ( caf::PdmObjectHandle* child : field->children() )
        {
            if ( caf::PdmObjectHandle* found = findObjectRecursive( child, address ) ) return found;
        }
    }
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::PdmObjectHandle* RiaAutomationJson::findObjectByAddress( const QString& address )
{
    return findObjectRecursive( RimProject::current(), address );
}
