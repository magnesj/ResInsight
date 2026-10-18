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

#include "RimSummaryCurveAppearanceOverride.h"

#include "Summary/RiaSummaryTools.h"

#include "RimEnsembleCurveSet.h"
#include "RimEnsembleCurveSetCollection.h"
#include "RimPlotCurve.h"
#include "RimProject.h"
#include "RimSummaryCase.h"
#include "RimSummaryCurve.h"
#include "RimSummaryCurveAppearanceOverrideCollection.h"
#include "RimSummaryEnsemble.h"
#include "RimSummaryPlot.h"

#include "RiuPlotCurveSymbolImageCreator.h"

#include <QColor>
#include <QLinearGradient>
#include <QPainter>
#include <QPixmap>

#include <set>

namespace caf
{
template <>
void caf::AppEnum<RimSummaryCurveAppearanceOverride::Mode>::setUp()
{
    addItem( RimSummaryCurveAppearanceOverride::Mode::AUTO, "AUTO", "Auto" );
    addItem( RimSummaryCurveAppearanceOverride::Mode::CUSTOM, "CUSTOM", "Custom" );
    setDefault( RimSummaryCurveAppearanceOverride::Mode::AUTO );
}
} // namespace caf

CAF_PDM_SOURCE_INIT( RimSummaryCurveAppearanceOverride, "RimSummaryCurveAppearanceOverride" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSummaryCurveAppearanceOverride::RimSummaryCurveAppearanceOverride()
{
    CAF_PDM_InitObject( "Curve Appearance Override" );

    CAF_PDM_InitFieldNoDefault( &m_dimension, "Dimension", "Type" );
    CAF_PDM_InitFieldNoDefault( &m_itemName, "ItemName", "Name" );

    CAF_PDM_InitFieldNoDefault( &m_mode, "Mode", "Assignment" );
    // m_mode.uiCapability()->setUiIconFromResourceString( ":/edit.svg" );

    CAF_PDM_InitFieldNoDefault( &m_appearance, "CurveAppearance", "Curve Appearance" );
    m_appearance = new RimPlotCurveAppearance;
    m_appearance.uiCapability()->setUiTreeChildrenHidden( true );
    connectAppearanceSignal();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryCurveAppearanceOverride::connectAppearanceSignal()
{
    if ( m_appearance() ) m_appearance->appearanceChanged.connect( this, &RimSummaryCurveAppearanceOverride::onAppearanceChanged );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryCurveAppearanceOverride::initAfterRead()
{
    connectAppearanceSignal();
}

//--------------------------------------------------------------------------------------------------
/// Editing any appearance value turns the item into a custom item
//--------------------------------------------------------------------------------------------------
void RimSummaryCurveAppearanceOverride::onAppearanceChanged( const caf::SignalEmitter* emitter )
{
    m_mode = Mode::CUSTOM;

    if ( auto collection = firstAncestorOrThisOfType<RimSummaryCurveAppearanceOverrideCollection>() )
    {
        collection->overridesChanged();
        collection->updateConnectedEditors();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryCurveAppearanceOverride::applyToCurve( RimPlotCurve* curve ) const
{
    if ( m_mode() != Mode::CUSTOM ) return;

    curve->curveAppearance()->copyFrom( *m_appearance );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::optional<cvf::Color3f> RimSummaryCurveAppearanceOverride::overriddenColor() const
{
    if ( m_mode() != Mode::CUSTOM ) return std::nullopt;
    return m_appearance->color();
}

namespace
{
//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool matchesItem( RimSummaryCurveAppearanceOverride::Dimension dimension,
                  const QString&                               itemName,
                  const RifEclipseSummaryAddress&              address,
                  const std::string&                           caseName )
{
    return QString::fromStdString( RimSummaryCurveAppearanceSettings::dimensionValue( dimension, address, caseName ) ) == itemName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::string ensembleCaseName( RimEnsembleCurveSet* curveSet )
{
    return curveSet->summaryEnsemble() ? curveSet->summaryEnsemble()->name().toStdString() : std::string();
}
} // namespace

//--------------------------------------------------------------------------------------------------
/// Shows what the global appearance gives for the item. The assigned color and symbol are used even if no
/// plot has a matching curve.
//--------------------------------------------------------------------------------------------------
void RimSummaryCurveAppearanceOverride::updateAppearanceFromCurves()
{
    copyAppearanceFromCurves();
    if ( m_autoColor ) m_appearance->setColor( *m_autoColor );
    if ( m_autoSymbol ) m_appearance->setSymbol( *m_autoSymbol );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryCurveAppearanceOverride::setAutoAssigned( const std::optional<cvf::Color3f>&                        color,
                                                         const std::optional<RiuPlotCurveSymbol::PointSymbolEnum>& symbol )
{
    m_autoColor  = color;
    m_autoSymbol = symbol;
}

//--------------------------------------------------------------------------------------------------
/// Copies the appearance of the first matching curve
//--------------------------------------------------------------------------------------------------
void RimSummaryCurveAppearanceOverride::copyAppearanceFromCurves()
{
    auto project = RimProject::current();
    if ( !project ) return;

    for ( auto plot : project->descendantsIncludingThisOfType<RimSummaryPlot>() )
    {
        if ( !plot->useGlobalCurveAppearance() ) continue;

        for ( auto curve : plot->summaryCurves() )
        {
            if ( matchesItem( m_dimension(),
                              m_itemName(),
                              curve->summaryAddressY(),
                              RimSummaryCurveAppearanceSettings::caseName( curve->summaryCaseY() ) ) )
            {
                m_appearance->copyFrom( *curve->curveAppearance() );
                return;
            }
        }

        for ( auto curveSet : plot->ensembleCurveSetCollection()->curveSets() )
        {
            if ( matchesItem( m_dimension(), m_itemName(), curveSet->summaryAddressY(), ensembleCaseName( curveSet ) ) )
            {
                m_appearance->setColor( curveSet->color() );
                return;
            }
        }
    }
}
//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSummaryCurveAppearanceOverride::Dimension RimSummaryCurveAppearanceOverride::dimension() const
{
    return m_dimension();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimSummaryCurveAppearanceOverride::itemName() const
{
    return m_itemName();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryCurveAppearanceOverride::setDimension( Dimension dimension )
{
    m_dimension = dimension;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryCurveAppearanceOverride::setItemName( const QString& itemName )
{
    m_itemName = itemName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QStringList RimSummaryCurveAppearanceOverride::availableNames( Dimension dimension )
{
    std::set<QString> names;
    if ( dimension == Dimension::WELL )
    {
        for ( const auto& wellName : RiaSummaryTools::allSummaryWellNames() )
        {
            names.insert( QString::fromStdString( wellName ) );
        }
    }
    else if ( dimension == Dimension::CASE )
    {
        if ( auto project = RimProject::current() )
        {
            for ( auto ensemble : project->summaryEnsembles() )
            {
                names.insert( ensemble->name() );
            }
            for ( auto summaryCase : project->allSummaryCases() )
            {
                if ( !summaryCase->ensemble() ) names.insert( summaryCase->displayCaseName() );
            }
        }
    }
    else if ( auto project = RimProject::current() )
    {
        for ( auto summaryCase : project->allSummaryCases() )
        {
            auto reader = summaryCase->summaryReader();
            if ( !reader ) continue;

            for ( const auto& address : reader->allResultAddresses() )
            {
                const auto value = RimSummaryCurveAppearanceSettings::dimensionValue( dimension, address, {} );
                if ( !value.empty() ) names.insert( QString::fromStdString( value ) );
            }
        }
    }

    QStringList result;
    for ( const auto& name : names )
    {
        result.push_back( name );
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimSummaryCurveAppearanceOverride::isCustom() const
{
    return m_mode() == Mode::CUSTOM;
}
//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryCurveAppearanceOverride::fieldChangedByUi( const caf::PdmFieldHandle* changedField,
                                                          const QVariant&            oldValue,
                                                          const QVariant&            newValue )
{
    // The available names depend on the type
    if ( changedField == &m_dimension ) m_itemName = QString();
    if ( changedField == &m_mode && m_mode() == Mode::AUTO ) updateAppearanceFromCurves();

    if ( auto collection = firstAncestorOrThisOfType<RimSummaryCurveAppearanceOverrideCollection>() )
    {
        collection->overridesChanged();
        collection->updateConnectedEditors();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryCurveAppearanceOverride::appendFieldsToUiOrdering( const QString& uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    uiOrdering.add( &m_mode );

    // Auto shows the values given by the global appearance, editing any value makes the item custom
    if ( m_mode() == Mode::AUTO ) updateAppearanceFromCurves();

    auto appearanceGroup = uiOrdering.addNewGroup( "Curve Style" );
    m_appearance->uiOrdering( "AppearanceOrdering", *appearanceGroup );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryCurveAppearanceOverride::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    uiOrdering.add( &m_dimension );
    uiOrdering.add( &m_itemName );
    uiOrdering.skipRemainingFields();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::optional<cvf::Color3f> RimSummaryCurveAppearanceOverride::colorFromPlots( Dimension dimension, const QString& name )
{
    auto project = RimProject::current();
    if ( !project ) return std::nullopt;

    for ( auto plot : project->descendantsIncludingThisOfType<RimSummaryPlot>() )
    {
        if ( !plot->useGlobalCurveAppearance() ) continue;

        for ( auto curve : plot->summaryCurves() )
        {
            if ( matchesItem( dimension, name, curve->summaryAddressY(), RimSummaryCurveAppearanceSettings::caseName( curve->summaryCaseY() ) ) )
            {
                return curve->color();
            }
        }

        for ( auto curveSet : plot->ensembleCurveSetCollection()->curveSets() )
        {
            if ( matchesItem( dimension, name, curveSet->summaryAddressY(), ensembleCaseName( curveSet ) ) ) return curveSet->color();
        }
    }

    return std::nullopt;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::IconProvider RimSummaryCurveAppearanceOverride::swatchIcon( const cvf::Color3f& color )
{
    QPixmap pixmap( 16, 16 );
    pixmap.fill( Qt::transparent );

    QPainter painter( &pixmap );
    painter.setPen( Qt::darkGray );
    painter.setBrush( QColor::fromRgbF( color.r(), color.g(), color.b() ) );
    painter.drawRect( 1, 1, 13, 13 );

    return caf::IconProvider( pixmap );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::IconProvider RimSummaryCurveAppearanceOverride::paletteIcon()
{
    QPixmap pixmap( 16, 16 );
    pixmap.fill( Qt::transparent );

    QLinearGradient gradient( 0, 0, 16, 0 );
    gradient.setColorAt( 0.0, Qt::red );
    gradient.setColorAt( 0.5, Qt::green );
    gradient.setColorAt( 1.0, Qt::blue );

    QPainter painter( &pixmap );
    painter.setPen( Qt::darkGray );
    painter.setBrush( gradient );
    painter.drawRect( 1, 1, 13, 13 );

    return caf::IconProvider( pixmap );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::IconProvider RimSummaryCurveAppearanceOverride::symbolIcon()
{
    QPixmap pixmap( 16, 16 );
    pixmap.fill( Qt::transparent );

    QPainter painter( &pixmap );
    painter.setRenderHint( QPainter::Antialiasing );
    painter.setPen( Qt::black );
    painter.setBrush( Qt::NoBrush );
    painter.drawEllipse( 2, 2, 5, 5 );
    painter.drawRect( 9, 2, 5, 5 );
    painter.drawLine( 2, 14, 8, 9 );
    painter.drawLine( 8, 9, 14, 14 );
    painter.drawLine( 2, 14, 14, 14 );

    return caf::IconProvider( pixmap );
}

namespace
{
//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString dimensionIconPath( RimSummaryCurveAppearanceOverride::Dimension dimension )
{
    using D = RimSummaryCurveAppearanceOverride::Dimension;
    switch ( dimension )
    {
        case D::CASE:
            return ":/SummaryCase.svg";
        case D::VECTOR:
            return ":/summary/components/images/misc.svg";
        case D::WELL:
        case D::LGR:
            return ":/summary/components/images/well.svg";
        case D::GROUP:
            return ":/summary/components/images/group.svg";
        case D::NETWORK:
            return ":/summary/components/images/network.svg";
        case D::REGION:
            return ":/summary/components/images/region.svg";
        case D::REGION_2_REGION:
            return ":/summary/components/images/region-region.svg";
        case D::AQUIFER:
            return ":/summary/components/images/aquifer.svg";
        case D::BLOCK:
        case D::CONNECTION:
            return ":/summary/components/images/block.svg";
        case D::SEGMENT:
            return ":/summary/components/images/segment.svg";
        default:
            return ":/DataVector.svg";
    }
}
} // namespace

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::IconProvider RimSummaryCurveAppearanceOverride::dimensionColorIcon( Dimension                          dimension,
                                                                         const std::optional<cvf::Color3f>& color,
                                                                         const std::optional<RiuPlotCurveSymbol::PointSymbolEnum>& symbol )
{
    QPixmap pixmap( 52, 16 );
    pixmap.fill( Qt::transparent );

    QPainter painter( &pixmap );
    painter.drawPixmap( 0, 0, QIcon( dimensionIconPath( dimension ) ).pixmap( 16, 16 ) );

    const QColor qColor = color ? QColor::fromRgbF( color->r(), color->g(), color->b() ) : QColor( Qt::black );

    int x = 19;
    if ( color )
    {
        painter.setPen( Qt::darkGray );
        painter.setBrush( qColor );
        painter.drawRect( x, 1, 13, 13 );
        x += 17;
    }

    if ( symbol && *symbol != RiuPlotCurveSymbol::SYMBOL_NONE )
    {
        painter.drawImage( x, 1, RiuPlotCurveSymbolImageCreator::createSymbolImage( *symbol, QSize( 14, 14 ), qColor ) );
    }

    return caf::IconProvider( pixmap );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::optional<RiuPlotCurveSymbol::PointSymbolEnum> RimSummaryCurveAppearanceOverride::overriddenSymbol() const
{
    if ( m_mode() != Mode::CUSTOM ) return std::nullopt;
    return m_appearance->symbol();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::optional<RiuPlotCurveSymbol::PointSymbolEnum> RimSummaryCurveAppearanceOverride::symbolFromPlots( Dimension dimension, const QString& name )
{
    auto project = RimProject::current();
    if ( !project ) return std::nullopt;

    for ( auto plot : project->descendantsIncludingThisOfType<RimSummaryPlot>() )
    {
        if ( !plot->useGlobalCurveAppearance() ) continue;

        for ( auto curve : plot->summaryCurves() )
        {
            if ( matchesItem( dimension, name, curve->summaryAddressY(), RimSummaryCurveAppearanceSettings::caseName( curve->summaryCaseY() ) ) )
            {
                return curve->symbol();
            }
        }
    }

    return std::nullopt;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::IconProvider RimSummaryCurveAppearanceOverride::dimensionIcon( Dimension dimension )
{
    return caf::IconProvider( dimensionIconPath( dimension ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimSummaryCurveAppearanceOverride::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
{
    QList<caf::PdmOptionItemInfo> options;

    if ( fieldNeedingOptions == &m_mode )
    {
        options.push_back( caf::PdmOptionItemInfo( "Auto", Mode::AUTO, false, caf::IconProvider( ":/Refresh.svg" ) ) );
        options.push_back( caf::PdmOptionItemInfo( "Custom", Mode::CUSTOM, false, caf::IconProvider( ":/edit.svg" ) ) );
    }

    return options;
}