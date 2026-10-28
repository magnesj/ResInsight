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

#include "RiuGroupNetworkGraphView.h"

#include "RiuGroupNetworkLayout.h"

#include <QContextMenuEvent>
#include <QFontMetrics>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QMenu>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPen>
#include <QWheelEvent>

namespace
{
constexpr qreal nodeWidth    = 170.0;
constexpr qreal nodeHeight   = 44.0;
constexpr qreal columnStep   = 290.0;
constexpr qreal rowStep      = nodeHeight + 22.0;
constexpr qreal stackOffset  = 4.0;
constexpr qreal minEdgeWidth = 1.5;
constexpr qreal maxEdgeWidth = 10.0;

// The palette of the workflow graph, with green for producers and blue for injectors
const QColor groupBorder( 60, 85, 115 );
const QColor groupBackground( 226, 238, 250 );
const QColor otherBackground( 232, 232, 232 );
const QColor producerBorder( 42, 120, 65 );
const QColor producerBackground( 215, 242, 222 );
const QColor injectorBorder( 50, 115, 165 );
const QColor injectorBackground( 214, 230, 248 );
const QColor edgeColor( 75, 95, 115 );
const QColor selectedColor( 30, 120, 220 );
const QColor labelColor( 70, 70, 70 );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QColor flowColor( const RimGroupNetworkGraph::Node& node )
{
    if ( node.isProducer && !node.isInjector ) return producerBorder;
    if ( node.isInjector && !node.isProducer ) return injectorBorder;
    return edgeColor;
}
} // namespace

namespace RiuGroupNetworkGraphViewItems
{
//==================================================================================================
///
//==================================================================================================
class NetworkNode : public QGraphicsRectItem
{
public:
    explicit NetworkNode( const RimGroupNetworkGraph::Node& node );

    const QString& name() const { return m_name; }
    QPointF        inPoint() const;
    QPointF        outPoint() const;
    void           applyStyle();

protected:
    QVariant itemChange( GraphicsItemChange change, const QVariant& value ) override;

private:
    QString                         m_name;
    QColor                          m_border;
    QColor                          m_background;
    std::vector<QGraphicsRectItem*> m_stack;
};

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
NetworkNode::NetworkNode( const RimGroupNetworkGraph::Node& node )
    : m_name( node.name )
{
    setRect( 0, 0, nodeWidth, nodeHeight );
    setFlags( ItemIsSelectable );
    setZValue( 1 );
    setToolTip( node.toolTip );

    if ( !node.isWell )
    {
        m_border     = groupBorder;
        m_background = groupBackground;
    }
    else if ( node.isProducer && !node.isInjector )
    {
        m_border     = producerBorder;
        m_background = producerBackground;
    }
    else if ( node.isInjector && !node.isProducer )
    {
        m_border     = injectorBorder;
        m_background = injectorBackground;
    }
    else
    {
        m_border     = groupBorder;
        m_background = otherBackground;
    }

    // A collapsed node is drawn as a stack of boxes, as a mapped task in the workflow graph
    if ( node.collapsed )
    {
        for ( int depth = 2; depth >= 1; --depth )
        {
            auto* card = new QGraphicsRectItem( rect().translated( depth * stackOffset, depth * stackOffset ), this );
            card->setFlag( ItemStacksBehindParent );
            card->setAcceptedMouseButtons( Qt::NoButton );
            m_stack.push_back( card );
        }
    }

    QFont titleFont;
    titleFont.setBold( true );
    auto* title = new QGraphicsSimpleTextItem( this );
    title->setFont( titleFont );
    title->setText( QFontMetrics( titleFont ).elidedText( node.name, Qt::ElideRight, nodeWidth - ( node.collapsed ? 56 : 16 ) ) );
    title->setPos( 8, 5 );
    title->setAcceptedMouseButtons( Qt::NoButton );

    QFont smallFont;
    smallFont.setPointSize( 8 );
    if ( !node.valueText.isEmpty() )
    {
        auto* value = new QGraphicsSimpleTextItem( this );
        value->setFont( smallFont );
        value->setBrush( labelColor );
        value->setText( QFontMetrics( smallFont ).elidedText( node.valueText, Qt::ElideRight, nodeWidth - 16 ) );
        value->setPos( 8, 25 );
        value->setAcceptedMouseButtons( Qt::NoButton );
    }

    if ( node.collapsed )
    {
        auto* badge = new QGraphicsSimpleTextItem( this );
        smallFont.setBold( true );
        badge->setFont( smallFont );
        badge->setBrush( m_border );
        badge->setText( QString( "+%1" ).arg( node.hiddenNodeCount ) );
        badge->setPos( nodeWidth - badge->boundingRect().width() - 8, 6 );
        badge->setAcceptedMouseButtons( Qt::NoButton );
    }

    applyStyle();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QPointF NetworkNode::inPoint() const
{
    return mapToScene( QPointF( 0, nodeHeight / 2 ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QPointF NetworkNode::outPoint() const
{
    return mapToScene( QPointF( nodeWidth, nodeHeight / 2 ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void NetworkNode::applyStyle()
{
    setPen( QPen( isSelected() ? selectedColor : m_border, isSelected() ? 2.5 : 1.0 ) );
    setBrush( m_background );
    for ( QGraphicsRectItem* card : m_stack )
    {
        card->setPen( QPen( m_border, 1.0 ) );
        card->setBrush( m_background.darker( 104 ) );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QVariant NetworkNode::itemChange( GraphicsItemChange change, const QVariant& value )
{
    if ( change == ItemSelectedHasChanged ) applyStyle();
    return QGraphicsRectItem::itemChange( change, value );
}
} // namespace RiuGroupNetworkGraphViewItems

using RiuGroupNetworkGraphViewItems::NetworkNode;

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiuGroupNetworkGraphView::RiuGroupNetworkGraphView( QWidget* parent )
    : QGraphicsView( parent )
    , m_scene( new QGraphicsScene( this ) )
{
    setScene( m_scene );
    setRenderHint( QPainter::Antialiasing );
    setDragMode( QGraphicsView::ScrollHandDrag );
    setTransformationAnchor( QGraphicsView::AnchorUnderMouse );
    setFocusPolicy( Qt::StrongFocus );

    connect( m_scene, &QGraphicsScene::selectionChanged, this, &RiuGroupNetworkGraphView::onSelectionChanged );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiuGroupNetworkGraphView::~RiuGroupNetworkGraphView() = default;

//--------------------------------------------------------------------------------------------------
/// Rebuilds the graph. The zoom and scroll position are kept if the nodes are the same as before, so
/// stepping in time only changes the values.
//--------------------------------------------------------------------------------------------------
void RiuGroupNetworkGraphView::setNodes( const std::vector<RimGroupNetworkGraph::Node>& nodes )
{
    m_updating = true;
    m_scene->clear();
    m_nodes.clear();

    std::vector<std::pair<QString, QString>> nodeParents;
    QStringList                              structure;
    for ( const auto& node : nodes )
    {
        nodeParents.emplace_back( node.name, node.parent );
        structure.append( node.name + "/" + node.parent + ( node.collapsed ? "+" : "" ) );
    }
    const auto positions = RiuGroupNetworkLayout::computePositions( nodeParents );

    for ( const auto& node : nodes )
    {
        if ( !positions.contains( node.name ) ) continue;

        const auto position = positions.value( node.name );
        auto*      item     = new NetworkNode( node );
        item->setPos( position.column * columnStep, position.row * rowStep );
        m_scene->addItem( item );
        m_nodes.insert( node.name, item );
    }

    QFont labelFont;
    labelFont.setPointSize( 8 );
    for ( const auto& node : nodes )
    {
        NetworkNode* child  = m_nodes.value( node.name );
        NetworkNode* parent = m_nodes.value( node.parent );
        if ( !child || !parent ) continue;

        const QPointF start = parent->outPoint();
        const QPointF end   = child->inPoint();
        const qreal   dx    = ( end.x() - start.x() ) / 2.0;

        QPainterPath path( start );
        path.cubicTo( start + QPointF( dx, 0 ), end - QPointF( dx, 0 ), end );

        const qreal width = node.edgeValue ? minEdgeWidth + ( maxEdgeWidth - minEdgeWidth ) * node.edgeWidthFraction : minEdgeWidth;
        QPen        pen( flowColor( node ), width );
        pen.setCapStyle( Qt::FlatCap );
        if ( !node.edgeValue ) pen.setStyle( Qt::DashLine );

        auto* edge = m_scene->addPath( path, pen );
        edge->setZValue( 0 );
        edge->setToolTip( QString( "%1 to %2<br>%3" ).arg( node.name, node.parent, node.edgeText.isEmpty() ? "No value" : node.edgeText ) );

        if ( !node.edgeText.isEmpty() )
        {
            auto* label = m_scene->addSimpleText( node.edgeText, labelFont );
            label->setBrush( labelColor );
            const QRectF bounds = label->boundingRect();
            label->setPos( end.x() - bounds.width() - 6, end.y() - bounds.height() - 2 );
            label->setZValue( 2 );
        }
    }

    const QRectF bounds = m_scene->itemsBoundingRect();
    m_scene->setSceneRect( bounds.adjusted( -40, -40, 40, 40 ) );

    if ( NetworkNode* selected = m_nodes.value( m_selectedNode ) )
    {
        selected->setSelected( true );
    }
    else if ( !m_selectedNode.isEmpty() )
    {
        m_selectedNode.clear();
        emit nodeSelected( QString() );
    }
    m_updating = false;

    if ( structure != m_structure )
    {
        m_structure   = structure;
        m_fitOnResize = true;
        fitGraph();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuGroupNetworkGraphView::clear()
{
    m_updating = true;
    m_scene->clear();
    m_nodes.clear();
    m_structure.clear();
    m_selectedNode.clear();
    m_updating = false;
}

//--------------------------------------------------------------------------------------------------
/// Fits the graph in the view, but does not magnify small graphs beyond the natural size
//--------------------------------------------------------------------------------------------------
void RiuGroupNetworkGraphView::fitGraph()
{
    if ( m_scene->sceneRect().isEmpty() ) return;

    fitInView( m_scene->sceneRect(), Qt::KeepAspectRatio );
    if ( transform().m11() > 1.0 ) resetTransform();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiuGroupNetworkGraphView::selectedNode() const
{
    return m_selectedNode;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuGroupNetworkGraphView::resizeEvent( QResizeEvent* event )
{
    QGraphicsView::resizeEvent( event );
    if ( m_fitOnResize ) fitGraph();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuGroupNetworkGraphView::mousePressEvent( QMouseEvent* event )
{
    if ( event->button() == Qt::LeftButton ) m_fitOnResize = false;
    QGraphicsView::mousePressEvent( event );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuGroupNetworkGraphView::mouseDoubleClickEvent( QMouseEvent* event )
{
    if ( NetworkNode* node = nodeAt( event->pos() ) )
    {
        emit toggleCollapseRequested( node->name() );
        event->accept();
        return;
    }
    QGraphicsView::mouseDoubleClickEvent( event );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuGroupNetworkGraphView::wheelEvent( QWheelEvent* event )
{
    if ( event->modifiers() & Qt::ControlModifier )
    {
        m_fitOnResize      = false;
        const qreal factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
        scale( factor, factor );
        event->accept();
        return;
    }
    QGraphicsView::wheelEvent( event );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuGroupNetworkGraphView::contextMenuEvent( QContextMenuEvent* event )
{
    QMenu menu( this );
    if ( NetworkNode* node = nodeAt( event->pos() ) )
    {
        const QString name = node->name();
        m_scene->clearSelection();
        node->setSelected( true );

        menu.addAction( "Create Summary Plot", this, [this, name]() { emit createPlotRequested( name ); } );
        menu.addAction( "Collapse/Expand", this, [this, name]() { emit toggleCollapseRequested( name ); } );
        menu.addSeparator();
    }
    menu.addAction( "Expand All", this, [this]() { emit expandAllRequested(); } );
    menu.addAction( "Collapse All", this, [this]() { emit collapseAllRequested(); } );
    menu.addAction( "Fit to View",
                    this,
                    [this]()
                    {
                        m_fitOnResize = true;
                        fitGraph();
                    } );
    menu.exec( event->globalPos() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiuGroupNetworkGraphView::onSelectionChanged()
{
    if ( m_updating ) return;

    QString name;
    for ( QGraphicsItem* item : m_scene->selectedItems() )
    {
        if ( auto* node = dynamic_cast<NetworkNode*>( item ) ) name = node->name();
    }
    if ( name == m_selectedNode ) return;

    m_selectedNode = name;
    emit nodeSelected( name );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
NetworkNode* RiuGroupNetworkGraphView::nodeAt( const QPoint& viewPos ) const
{
    for ( QGraphicsItem* item = itemAt( viewPos ); item; item = item->parentItem() )
    {
        if ( auto* node = dynamic_cast<NetworkNode*>( item ) ) return node;
    }
    return nullptr;
}
