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

#include <vector>

class QHttpServer;
class QHttpServerRequest;

namespace caf
{
class PdmObjectHandle;
}

//==================================================================================================
///
/// Lightweight HTTP server exposing the ResInsight project tree and a property editor as HTML.
///
/// Routes:
///   GET  /                 Project tree
///   GET  /object?path=...  Property editor for the object at the given tree path
///   POST /object?path=...  Apply edited field values, then re-render the editor
///
/// Objects are addressed by a dotted path of child indices from the project root, e.g. "0.3.1".
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

private:
    static caf::PdmObjectHandle*               rootObject();
    static std::vector<caf::PdmObjectHandle*>  orderedChildren( caf::PdmObjectHandle* object );
    static caf::PdmObjectHandle*               resolvePath( const QString& path );

    QString renderTreePage() const;
    void    renderTreeNode( caf::PdmObjectHandle* object, const QString& path, QString& html ) const;
    QString renderObjectPage( const QString& path ) const;
    QString applyFieldChanges( caf::PdmObjectHandle* object, const QHttpServerRequest& request ) const;

    static QString pageShell( const QString& title, const QString& body );

private:
    QHttpServer* m_httpServer;
    quint16      m_port;
};
