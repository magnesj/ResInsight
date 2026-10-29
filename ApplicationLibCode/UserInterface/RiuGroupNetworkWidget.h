/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2026-     Equinor ASA
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

#include "cafPdmPointer.h"
#include "cafSignal.h"

#include <QWidget>

class QLabel;
class QSlider;
class RimGroupNetwork;
class RiuGroupNetworkGraphView;

//==================================================================================================
/// Central tab of the plot window showing a production network: a time step slider above the graph
/// and a status line below it
//==================================================================================================
class RiuGroupNetworkWidget : public QWidget, public caf::SignalObserver
{
    Q_OBJECT

public:
    explicit RiuGroupNetworkWidget( QWidget* parent = nullptr );
    ~RiuGroupNetworkWidget() override;

    void             setNetwork( RimGroupNetwork* network );
    RimGroupNetwork* network() const;
    void             clear();

signals:
    void nodeSelected( const QString& nodeName );

private:
    void refresh();
    void onNetworkChanged( const caf::SignalEmitter* emitter );
    void onTimeStepChanged( int index );
    void onToggleCollapse( const QString& nodeName );
    void onCreatePlot( const QString& nodeName );

private:
    caf::PdmPointer<RimGroupNetwork> m_network;
    RiuGroupNetworkGraphView*        m_graphView;
    QSlider*                         m_timeStepSlider;
    QLabel*                          m_dateLabel;
    QLabel*                          m_statusLabel;
    bool                             m_refreshing = false;
};
