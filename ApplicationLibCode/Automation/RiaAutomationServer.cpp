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

#include "RiaAutomationServer.h"

#include "RiaAutomationJson.h"
#include "RiaLogging.h"
#include "RiaVersionInfo.h"

#include "RicfCommandObject.h"
#include "RifcCommandFileReader.h"

#include "Rim3dView.h"
#include "RimGridView.h"
#include "RimProject.h"

#include "Riu3DMainWindowTools.h"

#include "cafPdmAbstractFieldScriptingCapability.h"
#include "cafPdmDefaultObjectFactory.h"
#include "cafPdmFieldHandle.h"
#include "cafPdmObject.h"
#include "cafPdmObjectHandle.h"
#include "cafPdmScriptIOMessages.h"
#include "cafPdmScriptResponse.h"
#include "cafPdmUiItem.h"
#include "cafPdmUiObjectHandle.h"
#include "cafPdmValueField.h"
#include "cafPdmXmlObjectHandle.h"
#include "cafSelectionManager.h"

#include "cvfArray.h"

#include <QHostAddress>
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponder>
#include <QHttpServerResponse>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QUrlQuery>

#include <algorithm>

using StatusCode = QHttpServerResponder::StatusCode;

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
static QHttpServerResponse makeJsonResponse( const QJsonObject& object, StatusCode statusCode = StatusCode::Ok )
{
    return QHttpServerResponse( object, statusCode );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
static QHttpServerResponse makeJsonResponse( const QJsonArray& array, StatusCode statusCode = StatusCode::Ok )
{
    return QHttpServerResponse( array, statusCode );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
static QHttpServerResponse makeErrorResponse( StatusCode statusCode, const QString& message, const QString& detail = QString() )
{
    QJsonObject error;
    error["error"] = message;
    if ( !detail.isEmpty() ) error["detail"] = detail;
    return makeJsonResponse( error, statusCode );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaAutomationServer::RiaAutomationServer( QObject* parent )
    : QObject( parent )
    , m_listenPort( 0 )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaAutomationServer::~RiaAutomationServer() = default;

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaAutomationServer::start( quint16 preferredPort )
{
    m_httpServer = std::make_unique<QHttpServer>();
    registerRoutes();

    // Bind to loopback only. Fall back to an ephemeral port if the preferred one is taken.
    quint16 boundPort = m_httpServer->listen( QHostAddress::LocalHost, preferredPort );
    if ( boundPort == 0 )
    {
        boundPort = m_httpServer->listen( QHostAddress::LocalHost, 0 );
    }

    if ( boundPort == 0 )
    {
        RiaLogging::error( "UI automation server: failed to listen on 127.0.0.1" );
        return false;
    }

    m_listenPort = boundPort;
    RiaLogging::info( QString( "UI automation server listening on http://127.0.0.1:%1/api/v1" ).arg( m_listenPort ).toStdString() );

    return true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaAutomationServer::isRunning() const
{
    return m_httpServer && m_listenPort != 0;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
quint16 RiaAutomationServer::listenPort() const
{
    return m_listenPort;
}

//--------------------------------------------------------------------------------------------------
/// Set a scriptable field from text, mirroring how the Python/gRPC interface applies edits so
/// enums, units and change notifications behave like the desktop editors.
//--------------------------------------------------------------------------------------------------
static QHttpServerResponse setScriptableField( caf::PdmObjectHandle* object, const QString& fieldName, const QString& valueText )
{
    for ( caf::PdmFieldHandle* field : object->fields() )
    {
        auto scriptability = field->capability<caf::PdmAbstractFieldScriptingCapability>();
        if ( !scriptability || scriptability->scriptFieldName() != fieldName ) continue;

        if ( !scriptability->isIOWriteable() )
        {
            return makeErrorResponse( StatusCode::BadRequest, "Field is not writable", fieldName );
        }

        auto*    valueField = dynamic_cast<caf::PdmValueField*>( field );
        QVariant oldValue   = valueField ? valueField->toQVariant() : QVariant();

        caf::PdmScriptIOMessages messages;
        QTextStream              stream( valueText.toLatin1() );
        scriptability->writeToField( stream, nullptr, &messages, false, RimProject::current(), false );

        QStringList errors;
        for ( const auto& message : messages.m_messages )
        {
            if ( message.first == caf::PdmScriptIOMessages::MESSAGE_ERROR ) errors.append( message.second );
        }
        if ( !errors.isEmpty() )
        {
            if ( valueField && oldValue.isValid() ) valueField->setFromQVariant( oldValue );
            return makeErrorResponse( StatusCode::BadRequest, "Could not parse value for field", errors.join( "; " ) );
        }

        QVariant newValue = valueField ? valueField->toQVariant() : QVariant();
        if ( caf::PdmUiObjectHandle* uiOwner = field->ownerObject() ? field->ownerObject()->uiCapability() : nullptr )
        {
            uiOwner->fieldChangedByUi( field, oldValue, newValue );
            uiOwner->updateConnectedEditors();
        }

        return makeJsonResponse( RiaAutomationJson::pdmObjectToJson( object, 0 ) );
    }

    return makeErrorResponse( StatusCode::NotFound, "Field not found", fieldName );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
static QHttpServerResponse executeCommandText( const QString& commandText )
{
    QString                  mutableText = commandText;
    QTextStream              stream( &mutableText, QIODevice::ReadOnly );
    caf::PdmScriptIOMessages parseMessages;

    std::vector<RicfCommandObject*> commands =
        RicfCommandFileReader::readCommands( stream, caf::PdmDefaultObjectFactory::instance(), &parseMessages );

    QStringList parseErrors;
    for ( const auto& message : parseMessages.m_messages )
    {
        if ( message.first == caf::PdmScriptIOMessages::MESSAGE_ERROR ) parseErrors.append( message.second );
    }
    if ( commands.empty() && !parseErrors.isEmpty() )
    {
        return makeErrorResponse( StatusCode::BadRequest, "Could not parse command", parseErrors.join( "; " ) );
    }

    QJsonArray                     responseMessages;
    caf::PdmScriptResponse::Status worstStatus = caf::PdmScriptResponse::COMMAND_OK;
    for ( RicfCommandObject* command : commands )
    {
        caf::PdmScriptResponse response = command->execute();
        for ( const QString& message : response.messages() )
        {
            responseMessages.append( message );
        }
        worstStatus = std::max( worstStatus, response.status() );
        delete command;
    }

    QString statusText = "ok";
    if ( worstStatus == caf::PdmScriptResponse::COMMAND_WARNING ) statusText = "warning";
    if ( worstStatus == caf::PdmScriptResponse::COMMAND_ERROR ) statusText = "error";

    QJsonObject json;
    json["status"]   = statusText;
    json["messages"] = responseMessages;

    const StatusCode code = worstStatus == caf::PdmScriptResponse::COMMAND_ERROR ? StatusCode::InternalServerError : StatusCode::Ok;
    return makeJsonResponse( json, code );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaAutomationServer::registerRoutes()
{
    QHttpServer* server = m_httpServer.get();

    server->route( "/api/v1/health",
                   QHttpServerRequest::Method::Get,
                   []()
                   {
                       QJsonObject health;
                       health["status"]      = "ok";
                       health["application"] = "ResInsight";
                       health["version"]     = STRPRODUCTVER;
                       health["apiVersion"]  = "1.0.0";
                       return makeJsonResponse( health );
                   } );

    server->route( "/api/v1/project",
                   QHttpServerRequest::Method::Get,
                   []( const QHttpServerRequest& request )
                   {
                       RimProject* project = RimProject::current();
                       if ( !project ) return makeErrorResponse( StatusCode::NotFound, "No project is open" );

                       const QString depthText = request.query().queryItemValue( "maxDepth" );
                       const int     maxDepth  = depthText.isEmpty() ? -1 : depthText.toInt();
                       return makeJsonResponse( RiaAutomationJson::pdmObjectToJson( project, maxDepth ) );
                   } );

    server->route( "/api/v1/objects/<arg>",
                   QHttpServerRequest::Method::Get,
                   []( const QString& address, const QHttpServerRequest& request )
                   {
                       caf::PdmObjectHandle* object = RiaAutomationJson::findObjectByAddress( address );
                       if ( !object ) return makeErrorResponse( StatusCode::NotFound, "Object not found", address );

                       const QString depthText = request.query().queryItemValue( "maxDepth" );
                       const int     maxDepth  = depthText.isEmpty() ? -1 : depthText.toInt();
                       return makeJsonResponse( RiaAutomationJson::pdmObjectToJson( object, maxDepth ) );
                   } );

    server->route( "/api/v1/objects/<arg>/fields/<arg>",
                   QHttpServerRequest::Method::Put,
                   []( const QString& address, const QString& fieldName, const QHttpServerRequest& request )
                   {
                       caf::PdmObjectHandle* object = RiaAutomationJson::findObjectByAddress( address );
                       if ( !object ) return makeErrorResponse( StatusCode::NotFound, "Object not found", address );

                       QJsonParseError parseError;
                       QJsonDocument   document = QJsonDocument::fromJson( request.body(), &parseError );
                       if ( parseError.error != QJsonParseError::NoError || !document.isObject() )
                       {
                           return makeErrorResponse( StatusCode::BadRequest, "Request body must be a JSON object with a 'value'" );
                       }

                       return setScriptableField( object, fieldName, document.object().value( "value" ).toString() );
                   } );

    server->route( "/api/v1/views",
                   QHttpServerRequest::Method::Get,
                   []()
                   {
                       RimProject* project = RimProject::current();
                       QJsonArray  views;
                       if ( project )
                       {
                           for ( Rim3dView* view : project->allViews() )
                           {
                               if ( !view ) continue;
                               QJsonObject viewObject;
                               viewObject["id"]      = view->id();
                               viewObject["name"]    = view->name();
                               viewObject["address"] = RiaAutomationJson::addressOf( view );
                               if ( caf::PdmXmlObjectHandle* xmlObject = view->xmlCapability() )
                               {
                                   viewObject["type"] = xmlObject->classKeyword();
                               }
                               views.append( viewObject );
                           }
                       }
                       return makeJsonResponse( views );
                   } );

    server->route( "/api/v1/views/<arg>/visibleCellCount",
                   QHttpServerRequest::Method::Get,
                   []( int viewId )
                   {
                       RimProject* project = RimProject::current();
                       if ( project )
                       {
                           for ( Rim3dView* view : project->allViews() )
                           {
                               auto* gridView = dynamic_cast<RimGridView*>( view );
                               if ( !gridView || gridView->id() != viewId ) continue;

                               cvf::ref<cvf::UByteArray> visibility = gridView->currentTotalCellVisibility();

                               qint64 visibleCount = 0;
                               qint64 totalCount   = visibility.notNull() ? static_cast<qint64>( visibility->size() ) : 0;
                               for ( qint64 i = 0; i < totalCount; ++i )
                               {
                                   if ( visibility->val( i ) != 0 ) ++visibleCount;
                               }

                               QJsonObject json;
                               json["viewId"]           = viewId;
                               json["visibleCellCount"] = visibleCount;
                               json["totalCellCount"]   = totalCount;
                               return makeJsonResponse( json );
                           }
                       }
                       return makeErrorResponse( StatusCode::NotFound, "No grid view with the given id" );
                   } );

    server->route( "/api/v1/selection",
                   QHttpServerRequest::Method::Get,
                   []()
                   {
                       QJsonArray selected;
                       for ( caf::PdmUiItem* item : caf::SelectionManager::instance()->selectedItems() )
                       {
                           if ( auto* object = dynamic_cast<caf::PdmObjectHandle*>( item ) )
                           {
                               selected.append( RiaAutomationJson::pdmObjectToJson( object, 0 ) );
                           }
                       }
                       return makeJsonResponse( selected );
                   } );

    // Select an object in the project tree the same way a user click does, so the property editor
    // and the active view follow along.
    server->route( "/api/v1/selection",
                   QHttpServerRequest::Method::Put,
                   []( const QHttpServerRequest& request )
                   {
                       QJsonParseError parseError;
                       QJsonDocument   document = QJsonDocument::fromJson( request.body(), &parseError );
                       if ( parseError.error != QJsonParseError::NoError || !document.isObject() )
                       {
                           return makeErrorResponse( StatusCode::BadRequest, "Request body must be a JSON object with an 'address'" );
                       }

                       const QString         address = document.object().value( "address" ).toString();
                       caf::PdmObjectHandle* object  = RiaAutomationJson::findObjectByAddress( address );
                       if ( !object ) return makeErrorResponse( StatusCode::NotFound, "Object not found", address );

                       auto* pdmObject = dynamic_cast<caf::PdmObject*>( object );
                       if ( !pdmObject )
                       {
                           return makeErrorResponse( StatusCode::BadRequest, "Object cannot be selected", address );
                       }

                       Riu3DMainWindowTools::selectAsCurrentItem( pdmObject );
                       return makeJsonResponse( RiaAutomationJson::pdmObjectToJson( object, 0 ) );
                   } );

    server->route( "/api/v1/commands",
                   QHttpServerRequest::Method::Post,
                   []( const QHttpServerRequest& request )
                   {
                       QJsonParseError parseError;
                       QJsonDocument   document = QJsonDocument::fromJson( request.body(), &parseError );
                       if ( parseError.error != QJsonParseError::NoError || !document.isObject() )
                       {
                           return makeErrorResponse( StatusCode::BadRequest, "Request body must be a JSON object with a 'command'" );
                       }

                       const QString commandText = document.object().value( "command" ).toString();
                       if ( commandText.isEmpty() )
                       {
                           return makeErrorResponse( StatusCode::BadRequest, "Missing 'command' text" );
                       }
                       return executeCommandText( commandText );
                   } );
}
