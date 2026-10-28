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

#include "RimGroupNetwork.h"

#include "RimGroupNetworkNode.h"

#include "Summary/RiaSummaryPlotTools.h"
#include "Summary/RiaSummaryTools.h"

#include "RifGroupNetworkReader.h"
#include "RifSummaryReaderInterface.h"

#include "RimProject.h"
#include "RimSummaryCase.h"
#include "RimSummaryPlot.h"

#include "cafCmdFeatureMenuBuilder.h"
#include "cafPdmUiFilePathEditor.h"

#include <QDateTime>
#include <QFileInfo>
#include <QTimeZone>

#include <algorithm>
#include <set>

namespace caf
{
template <>
void AppEnum<RigGroupNetwork::TreeType>::setUp()
{
    addItem( RigGroupNetwork::TreeType::GRUPTREE, "GRUPTREE", "Standard Network (GRUPTREE)" );
    addItem( RigGroupNetwork::TreeType::BRANPROP, "BRANPROP", "Extended Network (BRANPROP)" );
    setDefault( RigGroupNetwork::TreeType::GRUPTREE );
}

template <>
void AppEnum<RifGroupNetworkSummaryData::EdgeMetric>::setUp()
{
    using Metric = RifGroupNetworkSummaryData::EdgeMetric;
    addItem( Metric::OIL_RATE, "OIL_RATE", "Oil Rate" );
    addItem( Metric::GAS_RATE, "GAS_RATE", "Gas Rate" );
    addItem( Metric::WATER_RATE, "WATER_RATE", "Water Rate" );
    addItem( Metric::WATER_INJ_RATE, "WATER_INJ_RATE", "Water Injection Rate" );
    addItem( Metric::GAS_INJ_RATE, "GAS_INJ_RATE", "Gas Injection Rate" );
    setDefault( Metric::OIL_RATE );
}

template <>
void AppEnum<RifGroupNetworkSummaryData::NodeMetric>::setUp()
{
    using Metric = RifGroupNetworkSummaryData::NodeMetric;
    addItem( Metric::PRESSURE, "PRESSURE", "Pressure" );
    addItem( Metric::BHP, "BHP", "Bottom Hole Pressure" );
    addItem( Metric::CONTROL_MODE, "CONTROL_MODE", "Well Control Mode" );
    setDefault( Metric::PRESSURE );
}
} // namespace caf

CAF_PDM_SOURCE_INIT( RimGroupNetwork, "RimGroupNetwork" );

namespace
{
QString dateText( std::time_t date )
{
    return QDateTime::fromSecsSinceEpoch( date, QTimeZone::UTC ).toString( "yyyy-MM-dd" );
}

QString valueWithUnit( std::optional<double> value, const std::string& unit )
{
    if ( !value ) return {};
    QString text = RimGroupNetworkGraph::formatValue( *value );
    if ( !unit.empty() ) text += " " + QString::fromStdString( unit );
    return text;
}

const std::vector<RifGroupNetworkSummaryData::EdgeMetric> allEdgeMetrics = { RifGroupNetworkSummaryData::EdgeMetric::OIL_RATE,
                                                                             RifGroupNetworkSummaryData::EdgeMetric::GAS_RATE,
                                                                             RifGroupNetworkSummaryData::EdgeMetric::WATER_RATE,
                                                                             RifGroupNetworkSummaryData::EdgeMetric::WATER_INJ_RATE,
                                                                             RifGroupNetworkSummaryData::EdgeMetric::GAS_INJ_RATE };

const std::vector<RifGroupNetworkSummaryData::NodeMetric> allNodeMetrics = { RifGroupNetworkSummaryData::NodeMetric::PRESSURE,
                                                                             RifGroupNetworkSummaryData::NodeMetric::BHP,
                                                                             RifGroupNetworkSummaryData::NodeMetric::CONTROL_MODE };
} // namespace

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimGroupNetwork::RimGroupNetwork()
    : networkChanged( this )
{
    CAF_PDM_InitObject( "Production Network", ":/components/images/network.svg" );
    setDeletable( true );

    CAF_PDM_InitField( &m_name, "Name", QString( "Production Network" ), "Name" );
    CAF_PDM_InitFieldNoDefault( &m_summaryCase, "SummaryCase", "Summary Case" );
    CAF_PDM_InitFieldNoDefault( &m_deckFile, "DeckFile", "Deck File" );
    m_deckFile.uiCapability()->setUiEditorTypeName( caf::PdmUiFilePathEditor::uiEditorTypeName() );
    CAF_PDM_InitFieldNoDefault( &m_treeType, "TreeType", "Network" );
    CAF_PDM_InitFieldNoDefault( &m_edgeMetric, "EdgeMetric", "Edge Value" );
    CAF_PDM_InitFieldNoDefault( &m_nodeMetric, "NodeMetric", "Node Value" );
    CAF_PDM_InitField( &m_showProducers, "ShowProducers", true, "Producers" );
    CAF_PDM_InitField( &m_showInjectors, "ShowInjectors", true, "Injectors" );
    CAF_PDM_InitField( &m_showOther, "ShowOther", true, "Other" );
    CAF_PDM_InitField( &m_timeStepIndex, "TimeStepIndex", 0, "Time Step" );
    CAF_PDM_InitFieldNoDefault( &m_collapsedNodes, "CollapsedNodes", "Collapsed Nodes" );
    m_collapsedNodes.uiCapability()->setUiHidden( true );

    CAF_PDM_InitFieldNoDefault( &m_selectedNode, "SelectedNode", "Selected Node" );
    m_selectedNode = new RimGroupNetworkNode();
    m_selectedNode.uiCapability()->setUiTreeHidden( true );
    m_selectedNode.uiCapability()->setUiHidden( true );
    m_selectedNode.xmlCapability()->disableIO();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimGroupNetwork::~RimGroupNetwork() = default;

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimGroupNetwork::name() const
{
    return m_name();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGroupNetwork::setSummaryCase( RimSummaryCase* summaryCase )
{
    m_summaryCase = summaryCase;
    if ( summaryCase )
    {
        m_name = "Network - " + summaryCase->displayCaseName();
        if ( m_deckFile().path().isEmpty() )
        {
            m_deckFile = caf::FilePath( RifGroupNetworkReader::findDeckFile( summaryCase->summaryHeaderFilename() ) );
        }
    }
    clearCache();

    const auto steps = timeSteps();
    m_timeStepIndex  = steps.empty() ? 0 : static_cast<int>( steps.size() ) - 1;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RimGroupNetworkGraph::Node> RimGroupNetwork::graphNodes() const
{
    const RigGroupNetwork::DatedTree* tree = currentTree();
    if ( !tree ) return {};

    const size_t timeStep = currentSummaryTimeStep();

    std::vector<RimGroupNetworkGraph::Node> nodes;
    for ( const RigGroupNetwork::Edge& edge : tree->edges )
    {
        RimGroupNetworkGraph::Node node;
        node.name   = QString::fromStdString( edge.child );
        node.parent = QString::fromStdString( edge.parent );
        node.isWell = edge.isWell();

        if ( m_summaryData )
        {
            const auto category = m_summaryData->category( edge.child, edge.isWell() );
            node.isProducer     = category.isProducer;
            node.isInjector     = category.isInjector;

            node.edgeValue = m_summaryData->edgeValue( edge.child, edge.isWell(), m_edgeMetric(), timeStep );
            node.edgeText  = valueWithUnit( node.edgeValue, m_summaryData->edgeUnit( m_edgeMetric() ) );
            node.valueText = valueWithUnit( m_summaryData->nodeValue( edge.child, edge.isWell(), m_nodeMetric(), timeStep ),
                                            m_summaryData->nodeUnit( m_nodeMetric() ) );
        }

        node.toolTip = toolTip( edge, node );
        nodes.push_back( node );
    }

    RimGroupNetworkGraph::Filter filter;
    filter.showProducers = m_showProducers();
    filter.showInjectors = m_showInjectors();
    filter.showOther     = m_showOther();
    filter.collapsedNodes.insert( m_collapsedNodes().begin(), m_collapsedNodes().end() );

    return RimGroupNetworkGraph::visibleNodes( nodes, filter );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimGroupNetwork::statusText() const
{
    ensureLoaded();
    if ( !m_loadError.isEmpty() ) return m_loadError;
    if ( !m_network ) return "No deck file";

    const auto dates = m_network->dates( m_treeType() );
    if ( dates.empty() ) return QString( "No %1 network in %2" ).arg( m_treeType().uiText(), QFileInfo( m_deckFile().path() ).fileName() );

    QString text = QString( "%1: %2 network definition(s)" ).arg( QFileInfo( m_deckFile().path() ).fileName() ).arg( dates.size() );
    if ( const RigGroupNetwork::DatedTree* tree = currentTree() )
    {
        text += QString( ", showing the network defined at %1" ).arg( dateText( tree->date ) );
    }
    if ( !m_summaryData ) text += ". Select a summary case to show values";
    return text;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QStringList RimGroupNetwork::timeStepTexts() const
{
    QStringList texts;
    for ( std::time_t date : timeSteps() )
    {
        texts.append( dateText( date ) );
    }
    return texts;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RimGroupNetwork::timeStepIndex() const
{
    const int count = static_cast<int>( timeSteps().size() );
    return count == 0 ? 0 : std::clamp( m_timeStepIndex(), 0, count - 1 );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGroupNetwork::setTimeStepIndex( int index )
{
    if ( index == m_timeStepIndex() ) return;

    m_timeStepIndex = index;
    m_timeStepIndex.uiCapability()->updateConnectedEditors();
    networkChanged.send();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimGroupNetwork::isCollapsed( const QString& nodeName ) const
{
    const auto& collapsed = m_collapsedNodes();
    return std::find( collapsed.begin(), collapsed.end(), nodeName ) != collapsed.end();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGroupNetwork::setCollapsed( const QString& nodeName, bool collapsed )
{
    std::vector<QString> nodes = m_collapsedNodes();
    nodes.erase( std::remove( nodes.begin(), nodes.end(), nodeName ), nodes.end() );
    if ( collapsed ) nodes.push_back( nodeName );
    m_collapsedNodes = nodes;
    networkChanged.send();
}

//--------------------------------------------------------------------------------------------------
/// Collapse all nodes below the root, so only the root and its children are shown
//--------------------------------------------------------------------------------------------------
void RimGroupNetwork::collapseAll()
{
    std::vector<QString> nodes;
    if ( const RigGroupNetwork::DatedTree* tree = currentTree() )
    {
        std::set<std::string> parents;
        for ( const RigGroupNetwork::Edge& edge : tree->edges )
        {
            parents.insert( edge.parent );
        }
        for ( const RigGroupNetwork::Edge& edge : tree->edges )
        {
            if ( !edge.isRoot() && parents.count( edge.child ) > 0 ) nodes.push_back( QString::fromStdString( edge.child ) );
        }
    }
    m_collapsedNodes = nodes;
    networkChanged.send();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGroupNetwork::expandAll()
{
    m_collapsedNodes = std::vector<QString>();
    networkChanged.send();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimGroupNetworkNode* RimGroupNetwork::selectNode( const QString& nodeName )
{
    const RigGroupNetwork::Edge* edge = findEdge( nodeName );
    if ( !edge ) return nullptr;

    QString nodeType = edge->isWell() ? "Well" : "Group";
    if ( m_summaryData )
    {
        const auto category = m_summaryData->category( edge->child, edge->isWell() );
        if ( category.isProducer && category.isInjector )
            nodeType += " (producer and injector)";
        else if ( category.isProducer )
            nodeType += " (producer)";
        else if ( category.isInjector )
            nodeType += " (injector)";
    }

    m_selectedNode->setNodeName( nodeName );
    m_selectedNode->setDescription( nodeType,
                                    QString::fromStdString( edge->parent ),
                                    QString::fromStdString( RigGroupNetwork::keywordText( edge->keyword ) ) );
    m_selectedNode->setValues( propertiesText( *edge ), valueTexts( *edge ) );
    m_selectedNode->updateConnectedEditors();
    return m_selectedNode();
}

//--------------------------------------------------------------------------------------------------
/// Plot the rates and pressures of a node
//--------------------------------------------------------------------------------------------------
void RimGroupNetwork::createSummaryPlot( const QString& nodeName )
{
    const RigGroupNetwork::Edge* edge = findEdge( nodeName );
    if ( !edge || !m_summaryData || !m_summaryCase() || !m_summaryCase()->summaryReader() ) return;

    const auto& available = m_summaryCase()->summaryReader()->allResultAddresses();

    std::set<RifEclipseSummaryAddress> addresses;
    for ( auto metric : allEdgeMetrics )
    {
        addresses.insert( m_summaryData->edgeAddress( edge->child, edge->isWell(), metric ) );
    }
    for ( auto metric : { RifGroupNetworkSummaryData::NodeMetric::PRESSURE, RifGroupNetworkSummaryData::NodeMetric::BHP } )
    {
        addresses.insert( m_summaryData->nodeAddress( edge->child, edge->isWell(), metric ) );
    }

    std::set<RifEclipseSummaryAddress> plotAddresses;
    for ( const auto& address : addresses )
    {
        if ( address.isValid() && available.count( address ) > 0 ) plotAddresses.insert( address );
    }
    if ( plotAddresses.empty() ) return;

    RimSummaryPlot* plot = RiaSummaryPlotTools::createPlot( plotAddresses, { m_summaryCase() }, {} );
    RiaSummaryPlotTools::createAndAppendSingleSummaryMultiPlot( plot );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGroupNetwork::reload()
{
    clearCache();
    networkChanged.send();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::PdmFieldHandle* RimGroupNetwork::userDescriptionField()
{
    return &m_name;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGroupNetwork::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    if ( changedField == &m_summaryCase )
    {
        if ( m_summaryCase() && m_deckFile().path().isEmpty() )
        {
            m_deckFile = caf::FilePath( RifGroupNetworkReader::findDeckFile( m_summaryCase()->summaryHeaderFilename() ) );
        }
        clearCache();
        const auto steps = timeSteps();
        m_timeStepIndex  = steps.empty() ? 0 : static_cast<int>( steps.size() ) - 1;
    }
    else if ( changedField == &m_deckFile || changedField == &m_treeType )
    {
        clearCache();
    }
    else if ( changedField == &m_name )
    {
        return;
    }

    networkChanged.send();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimGroupNetwork::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
{
    QList<caf::PdmOptionItemInfo> options;
    if ( fieldNeedingOptions == &m_summaryCase )
    {
        options = RiaSummaryTools::optionsForAllSummaryCases();
    }
    else if ( fieldNeedingOptions == &m_timeStepIndex )
    {
        const QStringList texts = timeStepTexts();
        for ( int i = 0; i < texts.size(); ++i )
        {
            options.push_back( caf::PdmOptionItemInfo( texts[i], i ) );
        }
    }
    return options;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGroupNetwork::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    uiOrdering.add( &m_name );
    uiOrdering.add( &m_summaryCase );
    uiOrdering.add( &m_deckFile );
    uiOrdering.add( &m_treeType );

    caf::PdmUiGroup* valueGroup = uiOrdering.addNewGroup( "Values" );
    valueGroup->add( &m_timeStepIndex );
    valueGroup->add( &m_edgeMetric );
    valueGroup->add( &m_nodeMetric );

    caf::PdmUiGroup* filterGroup = uiOrdering.addNewGroup( "Show Nodes" );
    filterGroup->add( &m_showProducers );
    filterGroup->add( &m_showInjectors );
    filterGroup->add( &m_showOther );

    uiOrdering.skipRemainingFields( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGroupNetwork::initAfterRead()
{
    clearCache();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGroupNetwork::appendMenuItems( caf::CmdFeatureMenuBuilder& menuBuilder ) const
{
    menuBuilder << "RicNewGroupNetworkFeature";
    menuBuilder.addSeparator();
    menuBuilder << "RicDeleteItemFeature";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGroupNetwork::ensureLoaded() const
{
    if ( m_isLoaded ) return;
    m_isLoaded = true;

    const QString deckFile = m_deckFile().path();
    if ( !deckFile.isEmpty() )
    {
        auto network = RifGroupNetworkReader::readFromFile( deckFile );
        if ( network )
            m_network = std::make_unique<RigGroupNetwork>( std::move( *network ) );
        else
            m_loadError = network.error();
    }

    if ( m_summaryCase() )
    {
        if ( RifSummaryReaderInterface* reader = m_summaryCase()->summaryReader() )
        {
            reader->createAddressesIfRequired();
            m_summaryData = std::make_unique<RifGroupNetworkSummaryData>( reader, m_treeType() );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimGroupNetwork::clearCache()
{
    m_isLoaded = false;
    m_loadError.clear();
    m_network.reset();
    m_summaryData.reset();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const RigGroupNetwork::DatedTree* RimGroupNetwork::currentTree() const
{
    ensureLoaded();
    if ( !m_network ) return nullptr;
    return m_network->treeAtDate( m_treeType(), currentDate() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const RigGroupNetwork::Edge* RimGroupNetwork::findEdge( const QString& nodeName ) const
{
    const RigGroupNetwork::DatedTree* tree = currentTree();
    if ( !tree ) return nullptr;

    const std::string name = nodeName.toStdString();
    for ( const RigGroupNetwork::Edge& edge : tree->edges )
    {
        if ( edge.child == name ) return &edge;
    }
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::time_t RimGroupNetwork::currentDate() const
{
    const auto steps = timeSteps();
    if ( steps.empty() ) return 0;
    return steps[timeStepIndex()];
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
size_t RimGroupNetwork::currentSummaryTimeStep() const
{
    return static_cast<size_t>( timeStepIndex() );
}

//--------------------------------------------------------------------------------------------------
/// The summary time steps, or the dates of the network definitions if there is no summary data
//--------------------------------------------------------------------------------------------------
std::vector<std::time_t> RimGroupNetwork::timeSteps() const
{
    ensureLoaded();
    if ( m_summaryData && !m_summaryData->timeSteps().empty() ) return m_summaryData->timeSteps();
    if ( m_network ) return m_network->dates( m_treeType() );
    return {};
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimGroupNetwork::valueTexts( const RigGroupNetwork::Edge& edge ) const
{
    if ( !m_summaryData ) return "No summary case";

    const size_t timeStep = currentSummaryTimeStep();
    QStringList  lines;
    lines.append( QString( "Values at %1" ).arg( dateText( currentDate() ) ) );
    for ( auto metric : allEdgeMetrics )
    {
        const auto value = m_summaryData->edgeValue( edge.child, edge.isWell(), metric, timeStep );
        if ( !value ) continue;
        lines.append( QString( "%1 (%2): %3" )
                          .arg( QString::fromStdString( RifGroupNetworkSummaryData::edgeMetricText( metric ) ) )
                          .arg( QString::fromStdString( m_summaryData->edgeAddress( edge.child, edge.isWell(), metric ).vectorName() ) )
                          .arg( valueWithUnit( value, m_summaryData->edgeUnit( metric ) ) ) );
    }
    for ( auto metric : allNodeMetrics )
    {
        const auto value = m_summaryData->nodeValue( edge.child, edge.isWell(), metric, timeStep );
        if ( !value ) continue;
        lines.append( QString( "%1 (%2): %3" )
                          .arg( QString::fromStdString( RifGroupNetworkSummaryData::nodeMetricText( metric ) ) )
                          .arg( QString::fromStdString( m_summaryData->nodeAddress( edge.child, edge.isWell(), metric ).vectorName() ) )
                          .arg( valueWithUnit( value, m_summaryData->nodeUnit( metric ) ) ) );
    }
    if ( lines.size() == 1 ) lines.append( "No summary vectors found for the node" );
    return lines.join( "\n" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimGroupNetwork::toolTip( const RigGroupNetwork::Edge& edge, const RimGroupNetworkGraph::Node& node ) const
{
    QStringList lines;
    lines.append( QString( "<b>%1</b> (%2)" ).arg( node.name.toHtmlEscaped(), edge.isWell() ? "well" : "group" ) );
    if ( !edge.isRoot() ) lines.append( QString( "Parent: %1" ).arg( node.parent.toHtmlEscaped() ) );
    lines.append( QString( "Defined by: %1" ).arg( QString::fromStdString( RigGroupNetwork::keywordText( edge.keyword ) ).toHtmlEscaped() ) );

    const QString properties = propertiesText( edge );
    if ( !properties.isEmpty() ) lines.append( properties );

    if ( m_summaryData )
    {
        const QString edgeName = QString::fromStdString( RifGroupNetworkSummaryData::edgeMetricText( m_edgeMetric() ) );
        const QString nodeName = QString::fromStdString( RifGroupNetworkSummaryData::nodeMetricText( m_nodeMetric() ) );
        lines.append( QString( "%1: %2" ).arg( edgeName, node.edgeText.isEmpty() ? "-" : node.edgeText ) );
        lines.append( QString( "%1: %2" ).arg( nodeName, node.valueText.isEmpty() ? "-" : node.valueText ) );
    }
    return lines.join( "<br>" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimGroupNetwork::propertiesText( const RigGroupNetwork::Edge& edge )
{
    QStringList texts;
    for ( const auto& [key, value] : edge.properties )
    {
        texts.append( QString( "%1: %2" ).arg( QString::fromStdString( key ).toHtmlEscaped() ).arg( value ) );
    }
    return texts.join( ", " );
}
