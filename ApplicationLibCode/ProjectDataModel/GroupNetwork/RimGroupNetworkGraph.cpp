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

#include "RimGroupNetworkGraph.h"

#include <QHash>
#include <QLocale>

#include <algorithm>
#include <cmath>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimGroupNetworkGraph::Node::isOther() const
{
    return !isProducer && !isInjector;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RimGroupNetworkGraph::Node> RimGroupNetworkGraph::visibleNodes( const std::vector<Node>& nodes, const Filter& filter )
{
    QHash<QString, size_t> indexOfNode;
    for ( size_t i = 0; i < nodes.size(); ++i )
    {
        indexOfNode.insert( nodes[i].name, i );
    }

    auto parentIndex = [&]( size_t i ) -> std::optional<size_t>
    {
        auto it = indexOfNode.find( nodes[i].parent );
        if ( nodes[i].parent.isEmpty() || it == indexOfNode.end() || it.value() == i ) return std::nullopt;
        return it.value();
    };

    // Children are listed after their parents, so a reverse pass sees all children of a node first
    std::vector<bool> kept( nodes.size(), false );
    std::vector<int>  childCount( nodes.size(), 0 );
    for ( size_t i = nodes.size(); i-- > 0; )
    {
        const Node& node    = nodes[i];
        const bool  isShown = ( node.isProducer && filter.showProducers ) || ( node.isInjector && filter.showInjectors ) ||
                             ( node.isOther() && filter.showOther );
        const auto parent = parentIndex( i );
        if ( isShown || childCount[i] > 0 || !parent ) kept[i] = true;
        if ( kept[i] && parent ) ++childCount[*parent];
    }

    // Kept descendants per node, again counted from the leaves
    std::vector<int> descendantCount( nodes.size(), 0 );
    for ( size_t i = nodes.size(); i-- > 0; )
    {
        const auto parent = parentIndex( i );
        if ( kept[i] && parent ) descendantCount[*parent] += descendantCount[i] + 1;
    }

    std::vector<bool> visible( nodes.size(), false );
    std::vector<Node> result;
    for ( size_t i = 0; i < nodes.size(); ++i )
    {
        if ( !kept[i] ) continue;

        const auto parent = parentIndex( i );
        if ( parent && ( !visible[*parent] || filter.collapsedNodes.count( nodes[*parent].name ) > 0 ) ) continue;

        visible[i]           = true;
        Node node            = nodes[i];
        node.childCount      = childCount[i];
        node.collapsed       = childCount[i] > 0 && filter.collapsedNodes.count( node.name ) > 0;
        node.hiddenNodeCount = node.collapsed ? descendantCount[i] : 0;
        if ( !parent ) node.parent.clear();
        result.push_back( node );
    }

    double maxEdgeValue = 0.0;
    for ( const Node& node : result )
    {
        if ( node.edgeValue ) maxEdgeValue = std::max( maxEdgeValue, std::abs( *node.edgeValue ) );
    }
    for ( Node& node : result )
    {
        node.edgeWidthFraction = ( node.edgeValue && maxEdgeValue > 0.0 ) ? std::abs( *node.edgeValue ) / maxEdgeValue : 0.0;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
/// Compact number for display in the graph
//--------------------------------------------------------------------------------------------------
QString RimGroupNetworkGraph::formatValue( double value )
{
    const double magnitude = std::abs( value );
    if ( magnitude >= 100.0 ) return QLocale::c().toString( value, 'f', 0 );
    if ( magnitude >= 1.0 ) return QLocale::c().toString( value, 'f', 1 );
    if ( magnitude == 0.0 ) return "0";
    return QLocale::c().toString( value, 'g', 3 );
}
