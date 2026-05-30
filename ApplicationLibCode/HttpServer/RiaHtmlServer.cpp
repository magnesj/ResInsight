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

#include "RiaHtmlServer.h"

#include "RiaApplication.h"
#include "RiaLogging.h"

#include "Rim3dView.h"
#include "RimProject.h"

#include "cafPdmFieldHandle.h"
#include "cafPdmObjectHandle.h"
#include "cafPdmUiFieldHandle.h"
#include "cafPdmUiObjectHandle.h"
#include "cafPdmValueField.h"
#include "cafPdmXmlObjectHandle.h"

#include <QBuffer>
#include <QDateTime>
#include <QHostAddress>
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QImage>
#include <QUrlQuery>

namespace
{
//--------------------------------------------------------------------------------------------------
/// Minimal HTML escaping for text inserted into the generated pages.
//--------------------------------------------------------------------------------------------------
QString htmlEscape( const QString& text )
{
    QString escaped = text;
    escaped.replace( '&', "&amp;" );
    escaped.replace( '<', "&lt;" );
    escaped.replace( '>', "&gt;" );
    escaped.replace( '"', "&quot;" );
    return escaped;
}
} // namespace

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaHtmlServer::RiaHtmlServer( QObject* parent )
    : QObject( parent )
    , m_httpServer( nullptr )
    , m_port( 0 )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaHtmlServer::~RiaHtmlServer()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaHtmlServer::start( quint16 preferredPort )
{
    m_httpServer = new QHttpServer( this );

    m_httpServer->route( "/",
                         [this]( const QHttpServerRequest& request ) -> QHttpServerResponse
                         {
                             Q_UNUSED( request );
                             return QHttpServerResponse( QByteArray( "text/html; charset=utf-8" ),
                                                         renderTreePage().toUtf8() );
                         } );

    m_httpServer->route( "/object",
                         [this]( const QHttpServerRequest& request ) -> QHttpServerResponse
                         {
                             const QString path = request.query().queryItemValue( "path" );

                             if ( request.method() == QHttpServerRequest::Method::Post )
                             {
                                 if ( caf::PdmObjectHandle* object = resolvePath( path ) )
                                 {
                                     applyFieldChanges( object, request );
                                 }
                             }

                             return QHttpServerResponse( QByteArray( "text/html; charset=utf-8" ),
                                                         renderObjectPage( path ).toUtf8() );
                         } );

    m_httpServer->route( "/viewsnapshot",
                         []( const QHttpServerRequest& request ) -> QHttpServerResponse
                         {
                             Q_UNUSED( request );

                             Rim3dView* view = RiaApplication::instance()->activeReservoirView();
                             if ( !view )
                             {
                                 return QHttpServerResponse( QHttpServerResponder::StatusCode::NotFound );
                             }

                             QImage image = view->snapshotWindowContent();
                             if ( image.isNull() )
                             {
                                 return QHttpServerResponse( QHttpServerResponder::StatusCode::NotFound );
                             }

                             QByteArray bytes;
                             QBuffer    buffer( &bytes );
                             buffer.open( QIODevice::WriteOnly );
                             image.save( &buffer, "PNG" );

                             return QHttpServerResponse( QByteArray( "image/png" ), bytes );
                         } );

    // Try the preferred port, then fall back to a small range if it is taken.
    for ( quint16 candidate = preferredPort; candidate < preferredPort + 20; ++candidate )
    {
        quint16 boundPort = m_httpServer->listen( QHostAddress::LocalHost, candidate );
        if ( boundPort != 0 )
        {
            m_port = boundPort;
            RiaLogging::info(
                QString( "HTML project browser started. Open %1 in a web browser." ).arg( url() ).toStdString() );
            return true;
        }
    }

    RiaLogging::warning( "Failed to start the HTML project browser. No free port found." );
    return false;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
quint16 RiaHtmlServer::port() const
{
    return m_port;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiaHtmlServer::url() const
{
    return QString( "http://localhost:%1/" ).arg( m_port );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::PdmObjectHandle* RiaHtmlServer::rootObject()
{
    return RimProject::current();
}

//--------------------------------------------------------------------------------------------------
/// Returns the child objects of the given object in field/declaration order. This mirrors the
/// structure of the project data model and gives each child a stable index for addressing.
//--------------------------------------------------------------------------------------------------
std::vector<caf::PdmObjectHandle*> RiaHtmlServer::orderedChildren( caf::PdmObjectHandle* object )
{
    std::vector<caf::PdmObjectHandle*> children;
    if ( !object ) return children;

    for ( caf::PdmFieldHandle* field : object->fields() )
    {
        for ( caf::PdmObjectHandle* child : field->children() )
        {
            if ( child ) children.push_back( child );
        }
    }

    return children;
}

//--------------------------------------------------------------------------------------------------
/// Resolves a dotted path of child indices (e.g. "0.3.1") to an object, starting at the project
/// root. An empty path resolves to the root object.
//--------------------------------------------------------------------------------------------------
caf::PdmObjectHandle* RiaHtmlServer::resolvePath( const QString& path )
{
    caf::PdmObjectHandle* current = rootObject();
    if ( !current || path.isEmpty() ) return current;

    const QStringList indices = path.split( '.', Qt::SkipEmptyParts );
    for ( const QString& indexText : indices )
    {
        bool      ok    = false;
        const int index = indexText.toInt( &ok );

        std::vector<caf::PdmObjectHandle*> children = orderedChildren( current );
        if ( !ok || index < 0 || index >= static_cast<int>( children.size() ) )
        {
            return nullptr;
        }
        current = children[index];
    }

    return current;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiaHtmlServer::renderTreePage() const
{
    caf::PdmObjectHandle* root = rootObject();

    QString tree;
    if ( !root )
    {
        tree = "<p>No project is currently open.</p>";
    }
    else
    {
        tree = "<ul class=\"tree\">";
        renderTreeNode( root, "", tree );
        tree += "</ul>";
    }

    // Two-pane layout: the collapsible project tree on the left, the property editor for the
    // selected node loaded into a separate view (iframe) on the right.
    QString body;
    body += "<div class=\"layout\">";
    body += QString( "<div class=\"treepane\"><h2>Project tree</h2>%1</div>" ).arg( tree );
    body += "<iframe class=\"editorpane\" name=\"editor\" src=\"/object\" "
            "title=\"Property editor\"></iframe>";
    body += "</div>";

    return pageShell( "ResInsight Project Tree", body );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaHtmlServer::renderTreeNode( caf::PdmObjectHandle* object, const QString& path, QString& html ) const
{
    if ( !object ) return;

    caf::PdmUiObjectHandle* uiObject = object->uiCapability();
    QString                 name     = uiObject ? uiObject->uiName() : QString();
    if ( name.isEmpty() && object->xmlCapability() ) name = object->xmlCapability()->classKeyword();
    if ( name.isEmpty() ) name = "Object";

    const QString link =
        QString( "<a href=\"/object?path=%1\" target=\"editor\">%2</a>" ).arg( path, htmlEscape( name ) );

    std::vector<caf::PdmObjectHandle*> children = orderedChildren( object );

    html += "<li>";
    if ( children.empty() )
    {
        // Leaf node: align with parents that show an expander triangle.
        html += QString( "<span class=\"leaf\">%1</span>" ).arg( link );
    }
    else
    {
        // Expandable node: <details>/<summary> provides a native expand/collapse triangle.
        html += "<details open><summary>" + link + "</summary><ul>";
        for ( size_t i = 0; i < children.size(); ++i )
        {
            const QString childPath = path.isEmpty() ? QString::number( i ) : QString( "%1.%2" ).arg( path ).arg( i );
            renderTreeNode( children[i], childPath, html );
        }
        html += "</ul></details>";
    }
    html += "</li>";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiaHtmlServer::renderObjectPage( const QString& path ) const
{
    caf::PdmObjectHandle* object = resolvePath( path );
    if ( !object )
    {
        return pageShell( "Object not found",
                          "<div class=\"editorpane-body\">"
                          "<p>Select an object in the project tree to edit its properties.</p>"
                          "</div>" );
    }

    caf::PdmUiObjectHandle* uiObject  = object->uiCapability();
    QString                 name      = uiObject ? uiObject->uiName() : QString();
    QString                 className = object->xmlCapability() ? object->xmlCapability()->classKeyword() : QString();
    if ( name.isEmpty() ) name = className.isEmpty() ? QString( "Object" ) : className;

    QString body;
    body += "<div class=\"editorpane-body\">";
    body += QString( "<h2>%1</h2>" ).arg( htmlEscape( name ) );
    if ( !className.isEmpty() ) body += QString( "<p class=\"classname\">%1</p>" ).arg( htmlEscape( className ) );

    // Two columns: properties (and children) on the left, the active 3D view snapshot on the right.
    body += "<div class=\"objcols\"><div class=\"objmain\">";

    body += QString( "<form method=\"post\" action=\"/object?path=%1\">" ).arg( path );
    body += "<table class=\"props\">";
    body += "<tr><th>Field</th><th>Keyword</th><th>Value</th></tr>";

    int valueFieldCount = 0;
    for ( caf::PdmFieldHandle* field : object->fields() )
    {
        auto* valueField = dynamic_cast<caf::PdmValueField*>( field );
        if ( !valueField ) continue;

        valueFieldCount++;

        caf::PdmUiFieldHandle* uiField   = field->uiCapability();
        QString                fieldName = uiField ? uiField->uiName() : QString();
        if ( fieldName.isEmpty() ) fieldName = field->keyword();

        const QString keyword  = field->keyword();
        const QString value    = valueField->toQVariant().toString();
        const bool    readOnly = valueField->isReadOnly();

        body += "<tr>";
        body += QString( "<td>%1</td>" ).arg( htmlEscape( fieldName ) );
        body += QString( "<td class=\"keyword\">%1</td>" ).arg( htmlEscape( keyword ) );
        body += QString( "<td><input type=\"text\" name=\"%1\" value=\"%2\"%3></td>" )
                    .arg( htmlEscape( keyword ), htmlEscape( value ), readOnly ? QString( " readonly" ) : QString() );
        body += "</tr>";
    }

    body += "</table>";

    if ( valueFieldCount > 0 )
    {
        body += "<p><button type=\"submit\">Apply changes</button></p>";
    }
    else
    {
        body += "<p>This object has no editable value fields.</p>";
    }
    body += "</form>";

    std::vector<caf::PdmObjectHandle*> children = orderedChildren( object );
    if ( !children.empty() )
    {
        body += "<h3>Children</h3><ul class=\"tree\">";
        for ( size_t i = 0; i < children.size(); ++i )
        {
            const QString childPath = path.isEmpty() ? QString::number( i ) : QString( "%1.%2" ).arg( path ).arg( i );

            caf::PdmUiObjectHandle* childUi   = children[i]->uiCapability();
            QString                 childName = childUi ? childUi->uiName() : QString();
            if ( childName.isEmpty() ) childName = "Object";

            body += QString( "<li><a href=\"/object?path=%1\">%2</a></li>" ).arg( childPath, htmlEscape( childName ) );
        }
        body += "</ul>";
    }

    body += "</div>"; // .objmain

    // Active 3D view screenshot in the right column. The cache-busting timestamp forces the browser
    // to fetch a fresh image every time this page is (re-)rendered, e.g. after applying a change.
    body += "<div class=\"objside\">";
    if ( RiaApplication::instance()->activeReservoirView() )
    {
        body += "<h3>Active 3D view</h3>";
        body += QString( "<img class=\"viewshot\" src=\"/viewsnapshot?ts=%1\" alt=\"Active 3D view\">" )
                    .arg( QDateTime::currentMSecsSinceEpoch() );
    }
    body += "</div>"; // .objside

    body += "</div>"; // .objcols

    body += "</div>"; // .editorpane-body

    return pageShell( name, body );
}

//--------------------------------------------------------------------------------------------------
/// Parses the posted form body and applies submitted values to matching value fields. GUI editors
/// and dependent state are updated for each changed field.
//--------------------------------------------------------------------------------------------------
QString RiaHtmlServer::applyFieldChanges( caf::PdmObjectHandle* object, const QHttpServerRequest& request ) const
{
    // In application/x-www-form-urlencoded bodies, spaces are encoded as '+'. Translate them to the
    // percent form so QUrlQuery decodes them back to spaces (a literal '+' arrives as "%2B").
    QString rawBody = QString::fromUtf8( request.body() );
    rawBody.replace( '+', "%20" );
    const QUrlQuery form( rawBody );

    int changedCount = 0;
    for ( caf::PdmFieldHandle* field : object->fields() )
    {
        auto* valueField = dynamic_cast<caf::PdmValueField*>( field );
        if ( !valueField || valueField->isReadOnly() ) continue;

        const QString keyword = field->keyword();
        if ( !form.hasQueryItem( keyword ) ) continue;

        const QString submitted = form.queryItemValue( keyword, QUrl::FullyDecoded );
        if ( submitted.isEmpty() ) continue;

        const QVariant oldValue = valueField->toQVariant();

        QVariant newValue( submitted );
        if ( oldValue.isValid() && oldValue.typeId() != QMetaType::QString )
        {
            QVariant converted = newValue;
            if ( converted.convert( oldValue.metaType() ) ) newValue = converted;
        }

        if ( newValue == oldValue ) continue;

        valueField->setFromQVariant( newValue );
        if ( object->uiCapability() )
        {
            object->uiCapability()->fieldChangedByUi( field, oldValue, newValue );
        }
        changedCount++;
    }

    if ( changedCount > 0 && object->uiCapability() )
    {
        object->uiCapability()->updateConnectedEditors();
    }

    return QString();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiaHtmlServer::pageShell( const QString& title, const QString& body )
{
    QString page;
    page += "<!DOCTYPE html><html><head><meta charset=\"utf-8\">";
    page += QString( "<title>%1</title>" ).arg( htmlEscape( title ) );
    page += "<style>"
            "html,body{height:100%;}"
            "body{font-family:Segoe UI,Arial,sans-serif;margin:0;color:#222;}"
            "h2{margin-bottom:0.2em;}"
            ".classname{color:#888;margin-top:0;font-size:0.85em;}"
            ".layout{display:flex;height:100vh;}"
            ".treepane{flex:0 0 24em;overflow:auto;padding:1em;border-right:1px solid #ccc;}"
            ".treepane h2{margin-top:0;}"
            ".editorpane{flex:1 1 auto;border:0;height:100%;}"
            "ul.tree,ul.tree ul{list-style:none;padding-left:1.1em;margin:0;}"
            "ul.tree{padding-left:0;}"
            "details>summary{cursor:pointer;list-style:revert;}"
            "li .leaf{display:inline-block;padding-left:1.1em;}"
            "a{color:#1565c0;text-decoration:none;}"
            "a:hover{text-decoration:underline;}"
            ".editorpane-body,body.editor{padding:1.5em;}"
            "table.props{border-collapse:collapse;margin-top:0.5em;}"
            "table.props th,table.props td{border:1px solid #ddd;padding:4px 8px;text-align:left;}"
            "table.props th{background:#f3f3f3;}"
            ".keyword{color:#888;font-family:Consolas,monospace;font-size:0.85em;}"
            "input[type=text]{min-width:18em;}"
            "button{padding:5px 14px;}"
            ".objcols{display:flex;gap:1.5em;align-items:flex-start;flex-wrap:wrap;}"
            ".objmain{flex:1 1 28em;min-width:0;}"
            ".objside{flex:1 1 24em;min-width:0;}"
            ".objside h3{margin-top:0;}"
            ".viewshot{max-width:100%;border:1px solid #ccc;margin-top:0.5em;}"
            "</style></head><body>";
    page += body;
    page += "</body></html>";
    return page;
}
