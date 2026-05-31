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
#include "RimGridView.h"
#include "RimProject.h"

#include "RifJsonEncodeDecode.h"

// The triangle extraction reuses the same machinery that RicHoloLensSession uses to ship
// geometry to the HoloLens sharing server. These headers live in the sibling Commands library.
#include "HoloLensCommands/VdeArrayDataPacket.h"
#include "HoloLensCommands/VdeCachingHashedIdFactory.h"
#include "HoloLensCommands/VdePacketDirectory.h"
#include "HoloLensCommands/VdeVizDataExtractor.h"

#include "cafPdmFieldHandle.h"
#include "cafPdmObjectHandle.h"
#include "cafPdmOptionItemInfo.h"
#include "cafPdmPointer.h"
#include "cafPdmUiCommandSystemProxy.h"
#include "cafPdmUiFieldHandle.h"
#include "cafPdmUiObjectHandle.h"
#include "cafPdmValueField.h"
#include "cafPdmXmlObjectHandle.h"

#include <QBuffer>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QHttpServer>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QImage>
#include <QUrlQuery>
#include <QVariantList>
#include <QVariantMap>

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

//--------------------------------------------------------------------------------------------------
/// A pointer field (caf::PdmPtrField) references another object. Its value is a guarded pointer, so
/// its QVariant always wraps a caf::PdmPointer<caf::PdmObjectHandle> (even when null). Such fields
/// are shown read-only and never modified from the HTML editor.
//--------------------------------------------------------------------------------------------------
bool isPointerField( caf::PdmValueField* valueField )
{
    return valueField && valueField->toQVariant().userType() == qMetaTypeId<caf::PdmPointer<caf::PdmObjectHandle>>();
}

//--------------------------------------------------------------------------------------------------
/// Extract the triangle meshes of the active grid view as JSON for the WebGL viewer.
///
/// This drives the exact same extraction pipeline that RicHoloLensSession uses to feed the
/// HoloLens sharing server: VdeVizDataExtractor produces a meta-data JSON describing each mesh
/// plus a set of binary array packets (vertices and connectivities). Here the browser plays the
/// role of the HoloLens client, so we transcode those packets into a compact JSON payload:
///
///   { "meshes": [ { "name", "opacity", "vertices":[x,y,z,...], "indices":[i,j,k,...],
///                   <coloring> }, ... ] }
///
/// <coloring> is either a solid "color":[r,g,b], or, for cell-result meshes, a color-legend
/// texture: "uv":[u,v,...] plus a base64 RGB image in "texData" with "texWidth"/"texHeight".
///
/// Only triangle meshes are emitted; line geometry (verticesPerPrimitive == 2) is skipped.
//--------------------------------------------------------------------------------------------------
QByteArray buildTrianglesJson()
{
    RimGridView* view = RiaApplication::instance()->activeGridView();
    if ( !view )
    {
        return QByteArray( "{\"meshes\":[]}" );
    }

    VdeCachingHashedIdFactory idFactory;
    VdePacketDirectory        packetDirectory;
    VdeVizDataExtractor       extractor( *view, &idFactory );

    QString          modelMetaJsonStr;
    std::vector<int> allReferencedArrayIds;
    extractor.extractViewContents( &modelMetaJsonStr, &allReferencedArrayIds, &packetDirectory );

    const QMap<QString, QVariant> modelMeta = ResInsightInternalJson::Json::decode( modelMetaJsonStr );
    const QVariantList            meshList  = modelMeta.value( "meshArr" ).toList();

    QByteArray json;
    json.reserve( 1024 * 1024 );
    json += "{\"meshes\":[";

    bool firstMesh = true;
    for ( const QVariant& meshVar : meshList )
    {
        const QVariantMap mesh = meshVar.toMap();

        // Triangles only.
        if ( mesh.value( "verticesPerPrimitive" ).toInt() != 3 ) continue;

        const VdeArrayDataPacket* vertexPacket = packetDirectory.lookupPacket( mesh.value( "vertexArrId", -1 ).toInt() );
        const VdeArrayDataPacket* connPacket   = packetDirectory.lookupPacket( mesh.value( "connArrId", -1 ).toInt() );
        if ( !vertexPacket || !connPacket ) continue;
        if ( vertexPacket->elementType() != VdeArrayDataPacket::Float32 ) continue;
        if ( connPacket->elementType() != VdeArrayDataPacket::Uint32 ) continue;

        const float opacity = mesh.value( "opacity", 1.0 ).toFloat();

        // A mesh is either textured (cell results sample a per-vertex texture coordinate into a
        // color-legend image) or carries a single solid color. Reproduce both so the WebGL view
        // shows the same coloring as the native 3D view.
        const VdeArrayDataPacket* texCoordPacket = packetDirectory.lookupPacket( mesh.value( "texCoordsArrId", -1 ).toInt() );
        const VdeArrayDataPacket* texImagePacket = packetDirectory.lookupPacket( mesh.value( "texImageArrId", -1 ).toInt() );
        if ( texCoordPacket && texCoordPacket->elementType() != VdeArrayDataPacket::Float32 ) texCoordPacket = nullptr;
        if ( texImagePacket && texImagePacket->elementType() != VdeArrayDataPacket::Uint8 ) texImagePacket = nullptr;
        const bool textured = texCoordPacket && texImagePacket;

        if ( !firstMesh ) json += ',';
        firstMesh = false;

        QString name = mesh.value( "meshSourceObjName" ).toString();
        name.replace( '\\', "\\\\" ).replace( '"', "\\\"" );

        json += "{\"name\":\"";
        json += name.toUtf8();
        json += "\",\"opacity\":";
        json += QByteArray::number( opacity );

        if ( textured )
        {
            // RGB legend image, base64-encoded. The browser builds a DataTexture directly from
            // these bytes (no PNG round-trip), preserving the OpenGL lower-left origin.
            const QByteArray rgb( texImagePacket->arrayData(), static_cast<int>( texImagePacket->elementCount() ) );
            json += ",\"texWidth\":" + QByteArray::number( texImagePacket->imageWidth() );
            json += ",\"texHeight\":" + QByteArray::number( texImagePacket->imageHeight() );
            json += ",\"texData\":\"" + rgb.toBase64() + "\"";

            json += ",\"uv\":[";
            const float* uv    = reinterpret_cast<const float*>( texCoordPacket->arrayData() );
            const size_t count = texCoordPacket->elementCount();
            for ( size_t i = 0; i < count; i++ )
            {
                if ( i ) json += ',';
                json += QByteArray::number( uv[i] );
            }
            json += "]";
        }
        else
        {
            float r = 0.6f, g = 0.7f, b = 0.85f;
            if ( mesh.contains( "color" ) )
            {
                const QVariantMap color = mesh.value( "color" ).toMap();
                r                       = color.value( "r", r ).toFloat();
                g                       = color.value( "g", g ).toFloat();
                b                       = color.value( "b", b ).toFloat();
            }
            json += ",\"color\":[";
            json += QByteArray::number( r ) + ',' + QByteArray::number( g ) + ',' + QByteArray::number( b );
            json += "]";
        }

        json += ",\"vertices\":[";
        {
            const float* floats = reinterpret_cast<const float*>( vertexPacket->arrayData() );
            const size_t count  = vertexPacket->elementCount();
            for ( size_t i = 0; i < count; i++ )
            {
                if ( i ) json += ',';
                json += QByteArray::number( floats[i] );
            }
        }

        json += "],\"indices\":[";
        {
            const unsigned int* indices = reinterpret_cast<const unsigned int*>( connPacket->arrayData() );
            const size_t        count   = connPacket->elementCount();
            for ( size_t i = 0; i < count; i++ )
            {
                if ( i ) json += ',';
                json += QByteArray::number( indices[i] );
            }
        }
        json += "]}";
    }

    json += "]}";
    return json;
}
} // namespace

std::atomic<quint64> RiaHtmlServer::sm_viewVersion{ 0 };
std::atomic<quint64> RiaHtmlServer::sm_geometryVersion{ 0 };

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
void RiaHtmlServer::notifyViewChanged()
{
    sm_viewVersion.fetch_add( 1, std::memory_order_relaxed );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaHtmlServer::notifyGeometryChanged()
{
    sm_geometryVersion.fetch_add( 1, std::memory_order_relaxed );
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
                             return QHttpServerResponse( QByteArray( "text/html; charset=utf-8" ), renderTreePage().toUtf8() );
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

                             return QHttpServerResponse( QByteArray( "text/html; charset=utf-8" ), renderObjectPage( path ).toUtf8() );
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

    m_httpServer->route( "/trianglesview",
                         [this]( const QHttpServerRequest& request ) -> QHttpServerResponse
                         {
                             Q_UNUSED( request );
                             return QHttpServerResponse( QByteArray( "text/html; charset=utf-8" ), renderTrianglesPage().toUtf8() );
                         } );

    m_httpServer->route( "/triangles",
                         []( const QHttpServerRequest& request ) -> QHttpServerResponse
                         {
                             Q_UNUSED( request );
                             return QHttpServerResponse( QByteArray( "application/json" ), buildTrianglesJson() );
                         } );

    m_httpServer->route( "/appicon.png",
                         []( const QHttpServerRequest& request ) -> QHttpServerResponse
                         {
                             Q_UNUSED( request );
                             QFile iconFile( ":/AppLogo48x48.png" );
                             if ( !iconFile.open( QIODevice::ReadOnly ) )
                             {
                                 return QHttpServerResponse( QHttpServerResponder::StatusCode::NotFound );
                             }
                             return QHttpServerResponse( QByteArray( "image/png" ), iconFile.readAll() );
                         } );

    m_httpServer->route( "/viewstate",
                         []( const QHttpServerRequest& request ) -> QHttpServerResponse
                         {
                             Q_UNUSED( request );
                             const quint64    view     = sm_viewVersion.load( std::memory_order_relaxed );
                             const quint64    geometry = sm_geometryVersion.load( std::memory_order_relaxed );
                             const QByteArray json     = "{\"view\":" + QByteArray::number( view ) +
                                                     ",\"geometry\":" + QByteArray::number( geometry ) + "}";
                             return QHttpServerResponse( QByteArray( "application/json" ), json );
                         } );

    // Try the preferred port, then fall back to a small range if it is taken.
    for ( quint16 candidate = preferredPort; candidate < preferredPort + 20; ++candidate )
    {
        quint16 boundPort = m_httpServer->listen( QHostAddress::LocalHost, candidate );
        if ( boundPort != 0 )
        {
            m_port = boundPort;
            RiaLogging::info( QString( "HTML project browser started. Open %1 in a web browser." ).arg( url() ).toStdString() );
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
/// Returns true if target is object itself or any descendant of it.
//--------------------------------------------------------------------------------------------------
bool RiaHtmlServer::subtreeContainsObject( caf::PdmObjectHandle* object, caf::PdmObjectHandle* target )
{
    if ( !object ) return false;
    if ( object == target ) return true;

    for ( caf::PdmObjectHandle* child : orderedChildren( object ) )
    {
        if ( subtreeContainsObject( child, target ) ) return true;
    }
    return false;
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

    // Title bar with the application icon and, when a project is open, its file name.
    QString projectName;
    if ( RimProject* proj = RiaApplication::instance()->project() )
    {
        if ( !proj->fileName().isEmpty() ) projectName = QFileInfo( proj->fileName() ).fileName();
    }

    QString header = "<div class=\"appheader\">";
    header += "<img class=\"applogo\" src=\"/appicon.png\" alt=\"\">";
    header += "<span class=\"apptitle\">ResInsight";
    if ( !projectName.isEmpty() ) header += QString( " <span class=\"project\">&mdash; %1</span>" ).arg( htmlEscape( projectName ) );
    header += "</span></div>";

    // Two-pane layout: the collapsible project tree on the left, the property editor for the
    // selected node loaded into a separate view (iframe) on the right.
    QString body;
    body += "<div class=\"appshell\">";
    body += header;
    body += "<div class=\"layout\">";
    body += "<div class=\"treepane\"><h2>Project tree</h2>";
    body += "<p class=\"toolbar\"><a href=\"/trianglesview\" target=\"editor\">Open 3D triangle view &rarr;</a></p>";
    body += tree;
    body += "</div>";
    body += "<iframe class=\"editorpane\" name=\"editor\" src=\"/object\" "
            "title=\"Property editor\"></iframe>";
    body += "</div>"; // .layout
    body += "</div>"; // .appshell

    return pageShell( "ResInsight Project Browser", body );
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

    const QString link = QString( "<a href=\"/object?path=%1\" target=\"editor\">%2</a>" ).arg( path, htmlEscape( name ) );

    std::vector<caf::PdmObjectHandle*> children = orderedChildren( object );

    html += "<li>";
    if ( children.empty() )
    {
        // Leaf node: align with parents that show an expander triangle.
        html += QString( "<span class=\"leaf\">%1</span>" ).arg( link );
    }
    else
    {
        // Expandable node: <details>/<summary> provides a native expand/collapse triangle. The root
        // node and the chain of nodes leading to the active 3D view are open by default so that view
        // is revealed; all other nodes start collapsed.
        caf::PdmObjectHandle* activeView   = RiaApplication::instance()->activeReservoirView();
        const bool            onActivePath = activeView && subtreeContainsObject( object, activeView );
        const QString         openAttr     = ( path.isEmpty() || onActivePath ) ? " open" : QString();
        html += "<details" + openAttr + "><summary>" + link + "</summary><ul>";
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

        // Pick an editor based on the field: pointer fields are shown read-only (never editable), a
        // drop-down for fields with selectable options, a checkbox for booleans, otherwise a plain
        // text input.
        const QList<caf::PdmOptionItemInfo> options = ( uiField && !isPointerField( valueField ) ) ? uiField->valueOptions()
                                                                                                   : QList<caf::PdmOptionItemInfo>();

        QString editor;
        if ( isPointerField( valueField ) )
        {
            // Show the referenced object's name as static text; pointer references are not editable.
            QString                                  refName;
            const std::vector<caf::PdmObjectHandle*> referenced = field->ptrReferencedObjects();
            if ( !referenced.empty() && referenced.front() && referenced.front()->uiCapability() )
            {
                refName = referenced.front()->uiCapability()->uiName();
            }
            if ( refName.isEmpty() ) refName = "(none)";
            editor = QString( "<span class=\"ptrref\">%1</span>" ).arg( htmlEscape( refName ) );
        }
        else if ( !options.isEmpty() )
        {
            const int selectedIndex = uiField->uiValue().toInt();
            editor = QString( "<select name=\"%1\"%2>" ).arg( htmlEscape( keyword ), readOnly ? QString( " disabled" ) : QString() );
            for ( int i = 0; i < options.size(); ++i )
            {
                editor += QString( "<option value=\"%1\"%2>%3</option>" )
                              .arg( QString::number( i ),
                                    i == selectedIndex ? QString( " selected" ) : QString(),
                                    htmlEscape( options[i].optionUiText() ) );
            }
            editor += "</select>";
        }
        else if ( valueField->toQVariant().typeId() == QMetaType::Bool )
        {
            editor = QString( "<input type=\"checkbox\" name=\"%1\" value=\"true\"%2%3>" )
                         .arg( htmlEscape( keyword ),
                               valueField->toQVariant().toBool() ? QString( " checked" ) : QString(),
                               readOnly ? QString( " disabled" ) : QString() );
        }
        else
        {
            editor = QString( "<input type=\"text\" name=\"%1\" value=\"%2\"%3>" )
                         .arg( htmlEscape( keyword ), htmlEscape( value ), readOnly ? QString( " readonly" ) : QString() );
        }

        body += "<tr>";
        body += QString( "<td>%1</td>" ).arg( htmlEscape( fieldName ) );
        body += QString( "<td class=\"keyword\">%1</td>" ).arg( htmlEscape( keyword ) );
        body += QString( "<td>%1</td>" ).arg( editor );
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

    // Right column: a static screenshot of the active 3D view and, below it, an interactive WebGL
    // view of the same triangle geometry. The cache-busting timestamp forces the browser to fetch a
    // fresh image every time this page is (re-)rendered, e.g. after applying a change.
    body += "<div class=\"objside\">";
    if ( RiaApplication::instance()->activeReservoirView() )
    {
        body += "<h3>Active 3D view</h3>";
        body +=
            QString( "<img class=\"viewshot\" src=\"/viewsnapshot?ts=%1\" alt=\"Active 3D view\">" ).arg( QDateTime::currentMSecsSinceEpoch() );
    }
    if ( RiaApplication::instance()->activeGridView() )
    {
        body += "<h3>3D triangle view</h3>";
        body += QString( "<iframe class=\"viewframe\" src=\"/trianglesview?ts=%1\" title=\"3D triangle view\"></iframe>" )
                    .arg( QDateTime::currentMSecsSinceEpoch() );
    }
    body += "</div>"; // .objside

    body += "</div>"; // .objcols

    body += "</div>"; // .editorpane-body

    // Poll the view-state versions and reload the snapshot whenever the native 3D view changes in
    // the desktop app, whether from camera navigation or a visible-cell change. The embedded
    // triangle view refreshes itself (on geometry changes only), so it is left untouched here.
    body += "<script>"
            "(function(){var last=null;function poll(){"
            "fetch('/viewstate',{cache:'no-store'}).then(function(r){return r.json();}).then(function(d){"
            "var key=d.view+'/'+d.geometry;"
            "if(last!==null&&key!==last){var img=document.querySelector('.viewshot');"
            "if(img)img.src='/viewsnapshot?ts='+Date.now();}last=key;}).catch(function(){});}"
            "setInterval(poll,750);poll();})();"
            "</script>";

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

        // Pointer fields reference other objects and are never modified from the HTML editor.
        if ( isPointerField( valueField ) ) continue;

        caf::PdmUiFieldHandle* uiField = field->uiCapability();
        if ( !uiField ) continue;

        const QString keyword = field->keyword();

        // The UI value is what the editor submits: an index into the options for a drop-down, the
        // checked state for a checkbox, or the real value otherwise. setUiValueToField() converts
        // it back to the stored value and notifies the data model exactly like the desktop editors.
        const QList<caf::PdmOptionItemInfo> options    = uiField->valueOptions();
        const QVariant                      oldUiValue = uiField->uiValue();

        QVariant newUiValue;
        if ( !options.isEmpty() )
        {
            if ( !form.hasQueryItem( keyword ) ) continue;
            bool      ok    = false;
            const int index = form.queryItemValue( keyword ).toInt( &ok );
            if ( !ok || index < 0 || index >= options.size() ) continue;
            newUiValue = QVariant( index );
        }
        else if ( valueField->toQVariant().typeId() == QMetaType::Bool )
        {
            // An unchecked checkbox submits nothing, so presence of the keyword means "checked".
            newUiValue = QVariant( form.hasQueryItem( keyword ) );
        }
        else
        {
            if ( !form.hasQueryItem( keyword ) ) continue;
            const QString submitted = form.queryItemValue( keyword, QUrl::FullyDecoded );
            if ( submitted.isEmpty() ) continue;

            QVariant converted( submitted );
            if ( oldUiValue.isValid() && oldUiValue.typeId() != QMetaType::QString )
            {
                QVariant tmp = converted;
                if ( tmp.convert( oldUiValue.metaType() ) ) converted = tmp;
            }
            newUiValue = converted;
        }

        if ( newUiValue == oldUiValue ) continue;

        caf::PdmUiCommandSystemProxy::instance()->setUiValueToField( uiField, newUiValue );
        changedCount++;
    }

    if ( changedCount > 0 && object->uiCapability() )
    {
        object->uiCapability()->updateConnectedEditors();
    }

    return QString();
}

//--------------------------------------------------------------------------------------------------
/// Self-contained WebGL page that fetches the active view's triangle meshes from /triangles and
/// renders them with three.js (loaded from a CDN). Drag to orbit, scroll to zoom.
//--------------------------------------------------------------------------------------------------
QString RiaHtmlServer::renderTrianglesPage() const
{
    return QString::fromUtf8(
        R"HTMLPAGE(<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>ResInsight 3D triangle view</title>
<link rel="icon" type="image/png" href="/appicon.png">
<style>
  html,body{margin:0;height:100%;background:#24292e;color:#e6e7ea;font-family:Segoe UI,Arial,sans-serif;}
  #info{position:absolute;top:8px;left:10px;font-size:0.85em;z-index:10;}
  #info a{color:#6fb1ff;}
  canvas{display:block;}
</style>
<script type="importmap">
{ "imports": {
    "three": "https://unpkg.com/three@0.160.0/build/three.module.js",
    "three/addons/": "https://unpkg.com/three@0.160.0/examples/jsm/"
} }
</script>
</head>
<body>
<div id="info">Loading geometry&hellip;</div>
<script type="module">
import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';

const info = document.getElementById('info');

const scene = new THREE.Scene();
scene.background = new THREE.Color(0x24292e);

const camera = new THREE.PerspectiveCamera(45, window.innerWidth / window.innerHeight, 0.01, 1e7);
const renderer = new THREE.WebGLRenderer({ antialias: true });
renderer.setSize(window.innerWidth, window.innerHeight);
renderer.setPixelRatio(window.devicePixelRatio);
document.body.appendChild(renderer.domElement);

const controls = new OrbitControls(camera, renderer.domElement);

scene.add(new THREE.AmbientLight(0xffffff, 0.6));
const headLight = new THREE.DirectionalLight(0xffffff, 0.8);
headLight.position.set(1, 1, 1);
scene.add(headLight);

const group = new THREE.Group();
scene.add(group);

function frameToBox(box) {
  const size = new THREE.Vector3(); box.getSize(size);
  const center = new THREE.Vector3(); box.getCenter(center);
  group.position.sub(center); // recenter at origin to preserve float precision
  const radius = Math.max(size.x, size.y, size.z) || 1;
  const dist = radius * 1.8;
  camera.position.set(dist, dist, dist);
  camera.near = radius / 100;
  camera.far = radius * 100;
  camera.updateProjectionMatrix();
  controls.target.set(0, 0, 0);
  controls.update();
}

function clearGroup() {
  for (const child of group.children) {
    child.geometry.dispose();
    if (child.material.map) child.material.map.dispose();
    child.material.dispose();
  }
  group.clear();
  // Keep group.position (the recentering offset) so the camera, rotation, zoom and panning are
  // preserved across geometry reloads. Only the very first load frames the view.
}

let framed = false; // frame the camera only on the first load; keep the user's view afterwards

function loadGeometry() {
  return fetch('/triangles').then(r => r.json()).then(data => {
    clearGroup();
    const meshes = data.meshes || [];
    let triCount = 0;
    for (const m of meshes) {
      const geom = new THREE.BufferGeometry();
      geom.setAttribute('position', new THREE.Float32BufferAttribute(m.vertices, 3));
      geom.setIndex(m.indices);
      geom.computeVertexNormals();
      const opacity = (m.opacity === undefined) ? 1 : m.opacity;
      const params = { transparent: opacity < 1, opacity: opacity, side: THREE.DoubleSide };
      if (m.texData) {
        // Cell-result coloring: sample the color-legend image through per-vertex UVs. The legend is
        // RGB; expand to RGBA for a DataTexture, keeping the OpenGL lower-left origin (flipY = false).
        const rgb = Uint8Array.from(atob(m.texData), ch => ch.charCodeAt(0));
        const pixelCount = m.texWidth * m.texHeight;
        const rgba = new Uint8Array(pixelCount * 4);
        for (let i = 0; i < pixelCount; i++) {
          rgba[i*4] = rgb[i*3]; rgba[i*4+1] = rgb[i*3+1]; rgba[i*4+2] = rgb[i*3+2]; rgba[i*4+3] = 255;
        }
        const tex = new THREE.DataTexture(rgba, m.texWidth, m.texHeight, THREE.RGBAFormat);
        tex.flipY = false;
        tex.minFilter = THREE.LinearFilter;
        tex.magFilter = THREE.LinearFilter;
        tex.wrapS = THREE.ClampToEdgeWrapping;
        tex.wrapT = THREE.ClampToEdgeWrapping;
        tex.needsUpdate = true;
        geom.setAttribute('uv', new THREE.Float32BufferAttribute(m.uv, 2));
        params.map = tex;
      } else {
        const c = m.color || [0.7, 0.7, 0.7];
        params.color = new THREE.Color(c[0], c[1], c[2]);
      }
      const mat = new THREE.MeshLambertMaterial(params);
      group.add(new THREE.Mesh(geom, mat));
      triCount += m.indices.length / 3;
    }
    if (meshes.length === 0) {
      info.innerHTML = 'No active 3D view with triangle geometry. <a href="/">Back to project tree</a>';
      return;
    }
    if (!framed) { frameToBox(new THREE.Box3().setFromObject(group)); framed = true; }
    info.innerHTML = meshes.length + ' mesh(es), ' + triCount + ' triangles. Drag to orbit, scroll to zoom. <a href="/">Back</a>';
  }).catch(e => { info.textContent = 'Failed to load geometry: ' + e; });
}

loadGeometry();

// Refetch the geometry whenever the visible cells change in the native 3D view (filters, time
// step, etc.). Pure camera navigation does not bump the geometry version, so the orbit view is
// preserved.
let lastGeometry = null;
setInterval(() => {
  fetch('/viewstate', { cache: 'no-store' }).then(r => r.json()).then(d => {
    if (lastGeometry !== null && d.geometry !== lastGeometry) loadGeometry();
    lastGeometry = d.geometry;
  }).catch(() => {});
}, 750);

window.addEventListener('resize', () => {
  camera.aspect = window.innerWidth / window.innerHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(window.innerWidth, window.innerHeight);
});

function animate() { requestAnimationFrame(animate); controls.update(); renderer.render(scene, camera); }
animate();
</script>
</body>
</html>)HTMLPAGE" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiaHtmlServer::pageShell( const QString& title, const QString& body )
{
    QString page;
    page += "<!DOCTYPE html><html><head><meta charset=\"utf-8\">";
    page += QString( "<title>%1</title>" ).arg( htmlEscape( title ) );
    page += "<link rel=\"icon\" type=\"image/png\" href=\"/appicon.png\">";
    // Dark theme, matching the colors of the ResInsight dark theme (ApplicationExeCode/Resources/themes/dark.qss).
    page += "<style>"
            "html,body{height:100%;}"
            "body{font-family:Segoe UI,Arial,sans-serif;margin:0;background:#24292e;color:#e6e7ea;}"
            "h2{margin-bottom:0.2em;}"
            "h3{color:#e6e7ea;}"
            ".classname{color:#89939d;margin-top:0;font-size:0.85em;}"
            ".appshell{display:flex;flex-direction:column;height:100vh;}"
            ".appheader{display:flex;align-items:center;gap:0.6em;flex:0 0 auto;padding:0.5em 1em;"
            "background:#1b1f24;border-bottom:1px solid #394046;}"
            ".applogo{height:28px;width:28px;}"
            ".apptitle{font-size:1.1em;font-weight:600;}"
            ".apptitle .project{color:#89939d;font-weight:400;}"
            ".layout{display:flex;flex:1 1 auto;min-height:0;}"
            ".treepane{flex:0 0 24em;overflow:auto;padding:1em;border-right:1px solid #394046;}"
            ".treepane h2{margin-top:0;}"
            ".editorpane{flex:1 1 auto;border:0;height:100%;background:#24292e;}"
            "ul.tree,ul.tree ul{list-style:none;padding-left:1.1em;margin:0;}"
            "ul.tree{padding-left:0;}"
            "details>summary{cursor:pointer;list-style:revert;}"
            "li .leaf{display:inline-block;padding-left:1.1em;}"
            "a{color:#6fb1ff;text-decoration:none;}"
            "a:hover{text-decoration:underline;}"
            ".editorpane-body,body.editor{padding:1.5em;}"
            "table.props{border-collapse:collapse;margin-top:0.5em;}"
            "table.props th,table.props td{border:1px solid #394046;padding:4px 8px;text-align:left;}"
            "table.props th{background:#394046;}"
            "table.props td{background:#2f353b;}"
            ".keyword{color:#89939d;font-family:Consolas,monospace;font-size:0.85em;}"
            ".ptrref{color:#adbac6;font-style:italic;}"
            "input[type=text],select{min-width:18em;background:#394046;color:#e6e7ea;"
            "border:1px solid #5a6067;padding:3px 5px;}"
            "button{padding:5px 14px;background:#0a639d;color:#fff;border:0;border-radius:3px;cursor:pointer;}"
            "button:hover{background:#136fa3;}"
            ".objcols{display:flex;gap:1.5em;align-items:flex-start;flex-wrap:wrap;}"
            ".objmain{flex:1 1 28em;min-width:0;}"
            ".objside{flex:1 1 24em;min-width:0;}"
            ".objside h3{margin-top:0;}"
            ".viewshot{max-width:100%;border:1px solid #394046;margin-top:0.5em;}"
            ".viewframe{width:100%;height:24em;border:1px solid #394046;margin-top:0.5em;}"
            "</style></head><body>";
    page += body;
    page += "</body></html>";
    return page;
}
