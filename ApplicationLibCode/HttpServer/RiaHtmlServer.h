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

#pragma once

#include <QObject>
#include <QString>

#include <atomic>
#include <vector>

class QHttpServer;
class QHttpServerRequest;

namespace caf
{
class PdmObjectHandle;
class PdmUiTreeOrdering;
} // namespace caf

//==================================================================================================
///
/// Lightweight HTTP server exposing the ResInsight project tree and a property editor as HTML.
///
/// Routes:
///   GET  /                 Project tree
///   GET  /object?path=...  Property editor for the object at the given tree path
///   POST /object?path=...  Apply edited field values, then re-render the editor
///   GET  /icon?id=...      PNG of a tree node icon
///   GET  /viewsnapshot     PNG snapshot of the active 3D view
///   GET  /trianglesview    WebGL page rendering the active view's triangle meshes
///   GET  /triangles        Triangle meshes of the active grid view as JSON
///   GET  /viewstate        Version counters {view, geometry} for camera and visible-cell changes
///
/// The tree mirrors the desktop project tree: it is built from the caf UI tree ordering
/// (defineUiTreeOrdering) of the project root. Nodes are addressed by a dotted path of child
/// indices into that ordering, e.g. "0.3.1".
//==================================================================================================
class RiaHtmlServer : public QObject
{
    Q_OBJECT

public:
    explicit RiaHtmlServer( QObject* parent = nullptr );
    ~RiaHtmlServer() override;

    bool    start( quint16 preferredPort = 8080 );
    quint16 port() const;
    QString url() const;

    // Bump the version counters so polling web pages know to refresh. Safe to call even when no
    // server is running. notifyViewChanged() is for camera navigation (refresh the snapshot);
    // notifyGeometryChanged() is for visible-cell changes (also refetch the triangle geometry).
    static void notifyViewChanged();
    static void notifyGeometryChanged();

private:
    static caf::PdmObjectHandle* rootObject();
    static bool                  subtreeContainsObject( caf::PdmUiTreeOrdering* node, caf::PdmObjectHandle* target );
    static caf::PdmObjectHandle* resolvePath( const QString& path );

    QString renderTreePage() const;
    void    renderTreeNode( caf::PdmUiTreeOrdering* node, const QString& path, QString& html ) const;
    QString renderObjectPage( const QString& path ) const;
    QString applyFieldChanges( caf::PdmObjectHandle* object, const QHttpServerRequest& request ) const;
    QString renderTrianglesPage() const;

    static QString pageShell( const QString& title, const QString& body );

private:
    QHttpServer* m_httpServer;
    quint16      m_port;

    static std::atomic<quint64> sm_viewVersion;
    static std::atomic<quint64> sm_geometryVersion;
};
