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

#include "RifGroupNetworkSummaryData.h"
#include "RigGroupNetwork.h"
#include "RimGroupNetworkGraph.h"

#include "cafAppEnum.h"
#include "cafFilePath.h"
#include "cafPdmChildField.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"
#include "cafPdmPtrField.h"
#include "cafSignal.h"

#include <memory>

class RimSummaryCase;
class RimGroupNetworkNode;

//==================================================================================================
/// A production network tree read from a deck (GRUPTREE or BRANPROP), with node and edge values from
/// a summary case. Shown as a graph in a central tab of the plot window, in the same way as the
/// workflow graph.
//==================================================================================================
class RimGroupNetwork : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    using TreeTypeEnum   = caf::AppEnum<RigGroupNetwork::TreeType>;
    using EdgeMetricEnum = caf::AppEnum<RifGroupNetworkSummaryData::EdgeMetric>;
    using NodeMetricEnum = caf::AppEnum<RifGroupNetworkSummaryData::NodeMetric>;

    caf::Signal<> networkChanged;

public:
    RimGroupNetwork();
    ~RimGroupNetwork() override;

    QString name() const;
    void    setSummaryCase( RimSummaryCase* summaryCase );

    std::vector<RimGroupNetworkGraph::Node> graphNodes() const;
    QString                                 statusText() const;

    QStringList timeStepTexts() const;
    int         timeStepIndex() const;
    void        setTimeStepIndex( int index );

    bool isCollapsed( const QString& nodeName ) const;
    void setCollapsed( const QString& nodeName, bool collapsed );
    void collapseAll();
    void expandAll();

    // Updates and returns the object shown in the property editor for a node of the graph
    RimGroupNetworkNode* selectNode( const QString& nodeName );
    void                 createSummaryPlot( const QString& nodeName );

    void reload();

private:
    caf::PdmFieldHandle* userDescriptionField() override;
    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override;
    QList<caf::PdmOptionItemInfo> calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions ) override;
    void                          defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering ) override;
    void                          initAfterRead() override;
    void                          appendMenuItems( caf::CmdFeatureMenuBuilder& menuBuilder ) const override;

    void ensureLoaded() const;
    void clearCache();

    const RigGroupNetwork::DatedTree* currentTree() const;
    const RigGroupNetwork::Edge*      findEdge( const QString& nodeName ) const;
    std::time_t                       currentDate() const;
    size_t                            currentSummaryTimeStep() const;
    std::vector<std::time_t>          timeSteps() const;

    QString valueTexts( const RigGroupNetwork::Edge& edge ) const;
    QString toolTip( const RigGroupNetwork::Edge& edge, const RimGroupNetworkGraph::Node& node ) const;

    static QString propertiesText( const RigGroupNetwork::Edge& edge );

private:
    caf::PdmField<QString>                   m_name;
    caf::PdmPtrField<RimSummaryCase*>        m_summaryCase;
    caf::PdmField<caf::FilePath>             m_deckFile;
    caf::PdmField<TreeTypeEnum>              m_treeType;
    caf::PdmField<EdgeMetricEnum>            m_edgeMetric;
    caf::PdmField<NodeMetricEnum>            m_nodeMetric;
    caf::PdmField<bool>                      m_showProducers;
    caf::PdmField<bool>                      m_showInjectors;
    caf::PdmField<bool>                      m_showOther;
    caf::PdmField<int>                       m_timeStepIndex;
    caf::PdmField<std::vector<QString>>      m_collapsedNodes;
    caf::PdmChildField<RimGroupNetworkNode*> m_selectedNode;

    mutable bool                                        m_isLoaded = false;
    mutable QString                                     m_loadError;
    mutable std::unique_ptr<RigGroupNetwork>            m_network;
    mutable std::unique_ptr<RifGroupNetworkSummaryData> m_summaryData;
};
