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

#include <memory>

class QHttpServer;
class QTcpServer;

//==================================================================================================
///
/// Localhost-only HTTP server exposing the UI automation API described in
/// docs/automation/openapi.yaml. Implemented with Qt HTTP Server (Qt 6.6.3).
///
/// The server runs on the GUI thread event loop, so route handlers may safely access the
/// project data model. It is intended for UI workflow tests (Playwright) and AI tooling on
/// the same machine, and never binds to a non-loopback interface.
///
//==================================================================================================
class RiaAutomationServer : public QObject
{
    Q_OBJECT

public:
    explicit RiaAutomationServer( QObject* parent = nullptr );
    ~RiaAutomationServer() override;

    // Start listening on 127.0.0.1. Returns true on success. If preferredPort is taken or 0,
    // an ephemeral port is chosen; the actual port is available from listenPort().
    bool start( quint16 preferredPort );

    bool    isRunning() const;
    quint16 listenPort() const;

private:
    void registerRoutes();

private:
    std::unique_ptr<QHttpServer> m_httpServer;
    QTcpServer*                  m_tcpServer;
    quint16                      m_listenPort;
};
