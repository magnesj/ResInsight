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

#include "RimCurveStyleSettings.h"

#include "RifEclipseSummaryAddress.h"

#include "RimColorLegend.h"
#include "RimColorLegendCollection.h"
#include "RimColorLegendItem.h"
#include "RimCurveStyleRule.h"
#include "RimCurveStyleRules.h"
#include "RimProject.h"
#include "RimSummaryCase.h"
#include "RimSummaryCurveAppearanceCalculator.h"
#include "RimSummaryEnsemble.h"

#include "RiuPlotCurveSymbolImageCreator.h"

#include "cafPdmUiListEditor.h"
#include "cafPdmUiTreeSelectionEditor.h"

#include <QPixmap>

#include <algorithm>
#include <set>

namespace caf
{
template <>
void caf::AppEnum<RimCurveStyleSettings::AppearanceType>::setUp()
{
    using T = RimCurveStyleSettings::AppearanceType;
    addItem( T::COLOR, "COLOR", "Color" );
    addItem( T::SYMBOL, "SYMBOL", "Symbol" );
    setDefault( T::COLOR );
}

template <>
void caf::AppEnum<RimCurveStyleSettings::Dimension>::setUp()
{
    using D = RimCurveStyleSettings::Dimension;
    addItem( D::NONE, "NONE", "None" );
    addItem( D::CASE, "CASE", "Summary Case" );
    addItem( D::VECTOR, "VECTOR", "Vector" );
    addItem( D::WELL, "WELL", "Well" );
    addItem( D::GROUP, "GROUP", "Group" );
    addItem( D::NETWORK, "NETWORK", "Network" );
    addItem( D::REGION, "REGION", "Region" );
    addItem( D::REGION_2_REGION, "REGION_2_REGION", "Region-Region" );
    addItem( D::AQUIFER, "AQUIFER", "Aquifer" );
    addItem( D::LGR, "LGR", "LGR" );
    addItem( D::BLOCK, "BLOCK", "Block" );
    addItem( D::CONNECTION, "CONNECTION", "Connection" );
    addItem( D::SEGMENT, "SEGMENT", "Segment" );
    setDefault( D::NONE );
}
} // namespace caf

CAF_PDM_SOURCE_INIT( RimCurveStyleSettings, "RimCurveStyleSettings" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimCurveStyleSettings::RimCurveStyleSettings()
{
    CAF_PDM_InitObject( "Global Curve Setting" );

    CAF_PDM_InitField( &m_name, "Name", QString( "Default" ), "Name" );
    CAF_PDM_InitField( &m_colorDimension, "ColorDimension", DimensionEnum( Dimension::CASE ), "Color", "", "Data dimension controlling the curve color" );
    CAF_PDM_InitField( &m_symbolDimension,
                       "SymbolDimension",
                       DimensionEnum( Dimension::NONE ),
                       "Symbol",
                       "",
                       "Data dimension controlling the curve symbol" );

    CAF_PDM_InitFieldNoDefault( &m_rules, "Rules", "Rules" );
    m_rules.uiCapability()->setUiTreeChildrenHidden( true );
    m_rules.uiCapability()->setUiHidden( true );

    CAF_PDM_InitFieldNoDefault( &m_colorBaseStyle, "ColorBaseStyle", "Color Base Style" );
    m_colorBaseStyle.uiCapability()->setUiTreeChildrenHidden( true );
    m_colorBaseStyle.uiCapability()->setUiHidden( true );
    m_colorBaseStyle = new RimCurveStyleRule;

    CAF_PDM_InitFieldNoDefault( &m_symbolBaseStyle, "SymbolBaseStyle", "Symbol Base Style" );
    m_symbolBaseStyle.uiCapability()->setUiTreeChildrenHidden( true );
    m_symbolBaseStyle.uiCapability()->setUiHidden( true );
    m_symbolBaseStyle = new RimCurveStyleRule;

    setUpBaseStyles();

    CAF_PDM_InitFieldNoDefault( &m_autoColorLegend, "AutoColorLegend", "Color Palette" );

    CAF_PDM_InitFieldNoDefault( &m_appearanceType, "AppearanceType", "Appearance Property" );
    m_appearanceType.xmlCapability()->disableIO();

    CAF_PDM_InitField( &m_symbolLegend, "SymbolLegend", 0, "Symbol Sequence" );
    m_symbolLegend.uiCapability()->setUiEditorTypeName( caf::PdmUiListEditor::uiEditorTypeName() );
    m_symbolLegend.uiCapability()->setUiLabelPosition( caf::PdmUiItemInfo::LabelPosition::TOP );
    m_symbolLegend.uiCapability()->setUiReadOnly( true );
    m_symbolLegend.uiCapability()->setAttribute( caf::PdmUiListEditor::Keys::HEIGHT_HINT, 120 );
    m_symbolLegend.xmlCapability()->disableIO();

    CAF_PDM_InitFieldNoDefault( &m_currentType, "CurrentType", "Differentiate By" );
    m_currentType.registerGetMethod( this, &RimCurveStyleSettings::currentDimension );
    m_currentType.registerSetMethod( this, &RimCurveStyleSettings::setCurrentDimension );
    m_currentType.xmlCapability()->disableIO();

    // Checked items are highlighted
    CAF_PDM_InitFieldNoDefault( &m_highlightedItems, "HighlightedItems", "Items" );
    m_highlightedItems.registerGetMethod( this, &RimCurveStyleSettings::highlightedItems );
    m_highlightedItems.registerSetMethod( this, &RimCurveStyleSettings::setHighlightedItems );
    m_highlightedItems.uiCapability()->setUiEditorTypeName( caf::PdmUiTreeSelectionEditor::uiEditorTypeName() );
    m_highlightedItems.uiCapability()->setUiLabelPosition( caf::PdmUiItemInfo::LabelPosition::TOP );
    m_highlightedItems.uiCapability()->setAttribute( caf::PdmUiTreeSelectionEditor::Keys::SHOW_TEXT_FILTER, false );
    m_highlightedItems.uiCapability()->setAttribute( caf::PdmUiTreeSelectionEditor::Keys::SHOW_TOGGLE_ALL_CHECKBOX, false );
    m_highlightedItems.uiCapability()->setAttribute( caf::PdmUiTreeSelectionEditor::Keys::HEIGHT_HINT, 200 );
    m_highlightedItems.uiCapability()->setAttribute( caf::PdmUiTreeSelectionEditor::Keys::ICON_SIZE, QSize( 33, 16 ) );
    m_highlightedItems.xmlCapability()->disableIO();

    CAF_PDM_InitFieldNoDefault( &m_symbolSize, "SymbolSize", "Symbol Size" );
    m_symbolSize.registerGetMethod( this, &RimCurveStyleSettings::symbolSize );
    m_symbolSize.registerSetMethod( this, &RimCurveStyleSettings::setSymbolSize );
    m_symbolSize.xmlCapability()->disableIO();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimCurveStyleSettings::~RimCurveStyleSettings() = default;

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleSettings::initAfterRead()
{
    setUpBaseStyles();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleSettings::setUpBaseStyles()
{
    if ( m_colorBaseStyle() ) m_colorBaseStyle()->setBaseKind( RimCurveStyleRule::BaseKind::COLOR );
    if ( m_symbolBaseStyle() ) m_symbolBaseStyle()->setBaseKind( RimCurveStyleRule::BaseKind::SYMBOL );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimCurveStyleSettings::name() const
{
    return m_name();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleSettings::setName( const QString& name )
{
    m_name = name;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimCurveStyleSettings::Dimension RimCurveStyleSettings::colorDimension() const
{
    return m_colorDimension();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimCurveStyleSettings::Dimension RimCurveStyleSettings::symbolDimension() const
{
    return m_symbolDimension();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleSettings::setColorDimension( Dimension dimension )
{
    m_colorDimension = dimension;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleSettings::setSymbolDimension( Dimension dimension )
{
    m_symbolDimension = dimension;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::string RimCurveStyleSettings::caseName( const RimSummaryCase* summaryCase )
{
    if ( !summaryCase ) return {};
    if ( summaryCase->ensemble() ) return summaryCase->ensemble()->name().toStdString();
    return summaryCase->displayCaseName().toStdString();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::string RimCurveStyleSettings::dimensionValue( Dimension dimension, const RifEclipseSummaryAddress& address, const std::string& caseName )
{
    using Category = RifEclipseSummaryAddressDefines::SummaryCategory;

    switch ( dimension )
    {
        case Dimension::CASE:
            return caseName;

        case Dimension::VECTOR:
        {
            std::string name = address.vectorName();
            if ( address.isHistoryVector() && !name.empty() ) name.pop_back();
            return name;
        }

        case Dimension::WELL:
            return address.wellName();

        case Dimension::GROUP:
            return address.groupName();

        case Dimension::NETWORK:
            return address.networkName();

        case Dimension::REGION:
            return address.category() == Category::SUMMARY_REGION ? address.itemUiText() : std::string();

        case Dimension::REGION_2_REGION:
            return address.category() == Category::SUMMARY_REGION_2_REGION ? address.itemUiText() : std::string();

        case Dimension::AQUIFER:
            return address.category() == Category::SUMMARY_AQUIFER ? address.itemUiText() : std::string();

        case Dimension::LGR:
            return address.lgrName();

        case Dimension::BLOCK:
            return address.category() == Category::SUMMARY_BLOCK || address.category() == Category::SUMMARY_BLOCK_LGR ? address.itemUiText()
                                                                                                                      : std::string();

        case Dimension::CONNECTION:
            return address.category() == Category::SUMMARY_WELL_COMPLETION || address.category() == Category::SUMMARY_WELL_CONNECTION ||
                           address.category() == Category::SUMMARY_WELL_CONNECTION_LGR
                       ? address.itemUiText()
                       : std::string();

        case Dimension::SEGMENT:
            return address.category() == Category::SUMMARY_WELL_SEGMENT ? address.itemUiText() : std::string();

        default:
            return std::string();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleSettings::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    RimCurveStyleRule::AvailableNamesCache namesCache;

    auto collection = firstAncestorOrThisOfType<RimCurveStyleRules>();
    if ( !collection ) return;

    // The name and the selected appearance type only change what is shown
    if ( changedField != &m_name && changedField != &m_appearanceType ) collection->rulesChanged();

    collection->updateConnectedEditors();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleSettings::appendFieldsToUiOrdering( caf::PdmUiOrdering& uiOrdering )
{
    RimCurveStyleRule::AvailableNamesCache namesCache;

    uiOrdering.add( &m_name );
    uiOrdering.add( &m_appearanceType );
    uiOrdering.add( &m_currentType );

    if ( m_appearanceType() == AppearanceType::COLOR )
    {
        // Defines the style shared by all items, the color is assigned by the palette
        m_colorBaseStyle()->appendBaseStyleToUiOrdering( uiOrdering );
    }
    else
    {
        uiOrdering.add( &m_symbolSize );
    }

    auto rulesGroup = uiOrdering.addNewGroup( "Assigned Appearance" );
    if ( m_appearanceType() == AppearanceType::COLOR )
        rulesGroup->add( &m_autoColorLegend );
    else
        rulesGroup->add( &m_symbolLegend );
    rulesGroup->add( &m_highlightedItems );
    rulesGroup->addNewLabel( "Checked items are highlighted in the plots, while other curves are dimmed." );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleSettings::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    appendFieldsToUiOrdering( uiOrdering );
    uiOrdering.skipRemainingFields();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RimCurveStyleRule*> RimCurveStyleSettings::rules() const
{
    return m_rules.childrenByType();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimCurveStyleRule* RimCurveStyleSettings::findRule( Dimension dimension, const QString& name ) const
{
    for ( auto item : m_rules )
    {
        if ( item->dimension() == dimension && item->itemName() == name ) return item;
    }

    return nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimCurveStyleRule* RimCurveStyleSettings::findOrCreateRule( Dimension dimension, const QString& name )
{
    if ( auto rule = findRule( dimension, name ) ) return rule;

    auto rule = new RimCurveStyleRule;
    rule->setDimension( dimension );
    rule->setItemName( name );
    m_rules.push_back( rule );

    return rule;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimCurveStyleSettings::Resolved RimCurveStyleSettings::resolved() const
{
    return { { this, m_colorDimension() }, { this, m_symbolDimension() } };
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RimCurveStyleSettings::RuleApplication>
    RimCurveStyleSettings::rulesFor( const Resolved& resolved, const RifEclipseSummaryAddress& address, const std::string& caseName )
{
    const auto hasValue = [&]( const Source& source )
    { return source.isSet() && !dimensionValue( source.dimension, address, caseName ).empty(); };

    // The base styles are applied first, as the rules for specific items build on them
    std::vector<RuleApplication> result;
    if ( hasValue( resolved.color ) && resolved.color.settings->m_colorBaseStyle() )
    {
        result.push_back( { resolved.color.settings->m_colorBaseStyle(), true, false } );
    }
    if ( hasValue( resolved.symbol ) && resolved.symbol.settings->m_symbolBaseStyle() )
    {
        result.push_back( { resolved.symbol.settings->m_symbolBaseStyle(), false, true } );
    }

    std::vector<RuleApplication> items;
    const auto                   collect = [&]( const Source& source, bool isColor )
    {
        if ( !hasValue( source ) ) return;

        const auto value = QString::fromStdString( dimensionValue( source.dimension, address, caseName ) );
        for ( auto rule : source.settings->m_rules )
        {
            if ( !rule->isCustom() && !rule->isHighlighted() ) continue;
            if ( rule->dimension() != source.dimension || rule->itemName() != value ) continue;

            auto existing = std::find_if( items.begin(), items.end(), [rule]( const RuleApplication& a ) { return a.rule == rule; } );
            if ( existing == items.end() ) existing = items.insert( items.end(), RuleApplication{ rule, false, false } );

            if ( isColor )
                existing->applyColor = true;
            else
                existing->applySymbol = true;
        }
    };
    collect( resolved.color, true );
    collect( resolved.symbol, false );

    // Fixed precedence by dimension, so the result does not depend on the order the rules were created in
    std::stable_sort( items.begin(),
                      items.end(),
                      []( const RuleApplication& a, const RuleApplication& b ) { return a.rule->dimension() < b.rule->dimension(); } );

    result.insert( result.end(), items.begin(), items.end() );
    return result;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::optional<cvf::Color3f> RimCurveStyleSettings::autoLegendColor( int index ) const
{
    if ( !m_autoColorLegend() || index < 0 ) return std::nullopt;

    const auto items = m_autoColorLegend()->colorLegendItems();
    if ( items.empty() ) return std::nullopt;

    return items[index % items.size()]->color();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleSettings::setColorPalette( RimColorLegend* palette )
{
    m_autoColorLegend = palette;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RimCurveStyleSettings::symbolSize() const
{
    return m_symbolBaseStyle() ? m_symbolBaseStyle()->symbolSize() : 0;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleSettings::setSymbolSize( const int& size )
{
    if ( m_symbolBaseStyle() ) m_symbolBaseStyle()->setSymbolSize( size );
}

//--------------------------------------------------------------------------------------------------
/// The names of the highlighted items of the current dimension
//--------------------------------------------------------------------------------------------------
std::vector<QString> RimCurveStyleSettings::highlightedItems() const
{
    std::vector<QString> names;
    for ( auto rule : m_rules )
    {
        if ( rule->dimension() == m_currentType() && rule->isHighlighted() ) names.push_back( rule->itemName() );
    }

    return names;
}

//--------------------------------------------------------------------------------------------------
/// Rules are only kept for items that are highlighted or custom
//--------------------------------------------------------------------------------------------------
void RimCurveStyleSettings::setHighlightedItems( const std::vector<QString>& names )
{
    const std::set<QString> highlighted( names.begin(), names.end() );

    for ( const auto& name : RimCurveStyleRule::availableNames( m_currentType() ) )
    {
        if ( highlighted.contains( name ) )
        {
            findOrCreateRule( m_currentType(), name )->setHighlighted( true );
        }
        else if ( auto rule = findRule( m_currentType(), name ) )
        {
            rule->setHighlighted( false );
            if ( !rule->isCustom() )
            {
                m_rules.removeChild( rule );
                delete rule;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimCurveStyleSettings::DimensionEnum RimCurveStyleSettings::currentDimension() const
{
    return m_appearanceType() == AppearanceType::COLOR ? m_colorDimension() : m_symbolDimension();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleSettings::setCurrentDimension( const DimensionEnum& dimension )
{
    if ( m_appearanceType() == AppearanceType::COLOR )
        m_colorDimension = dimension;
    else
        m_symbolDimension = dimension;
}

//--------------------------------------------------------------------------------------------------
/// The color and symbol the setting assigns to the item at the given position
//--------------------------------------------------------------------------------------------------
void RimCurveStyleSettings::assignedAppearance( Dimension                                dimension,
                                                const QString&                           name,
                                                int                                      index,
                                                const std::map<QString, ItemAppearance>& plotAppearances,
                                                ItemAppearance&                          appearance ) const
{
    const bool projectWide = dimension == Dimension::CASE || dimension == Dimension::WELL;

    const auto fromPlots = plotAppearances.find( name );
    const bool inPlots   = fromPlots != plotAppearances.end();

    auto& color  = appearance.color;
    auto& symbol = appearance.symbol;

    if ( projectWide ) color = RimSummaryCurveAppearanceCalculator::cycledPaletteColor( index, this );
    if ( !color && inPlots ) color = fromPlots->second.color;
    if ( !color ) color = RimSummaryCurveAppearanceCalculator::cycledPaletteColor( index, this );

    if ( projectWide ) symbol = RimSummaryCurveAppearanceCalculator::cycledSymbol( index );
    if ( !symbol && inPlots ) symbol = fromPlots->second.symbol;
    if ( !symbol ) symbol = RimSummaryCurveAppearanceCalculator::cycledSymbol( index );
}

//--------------------------------------------------------------------------------------------------
/// The assigned appearances are found once per call, as it can require scanning all summary addresses and plots.
//--------------------------------------------------------------------------------------------------
std::map<QString, RimCurveStyleSettings::ItemAppearance> RimCurveStyleSettings::itemAppearances( Dimension dimension ) const
{
    RimCurveStyleRule::AvailableNamesCache namesCache;

    std::map<QString, ItemAppearance> appearances;

    std::map<QString, int> indices;
    if ( auto collection = firstAncestorOrThisOfType<RimCurveStyleRules>() ) indices = collection->itemIndices( dimension );

    const auto plotAppearances = RimCurveStyleRule::appearancesFromPlots( dimension, this );
    for ( const auto& itemName : RimCurveStyleRule::availableNames( dimension ) )
    {
        const auto indexIt = indices.find( itemName );
        const int  index   = indexIt != indices.end() ? indexIt->second : static_cast<int>( appearances.size() );

        ItemAppearance appearance;
        assignedAppearance( dimension, itemName, index, plotAppearances, appearance );

        if ( const auto rule = findRule( dimension, itemName ); rule && rule->isCustom() )
        {
            if ( rule->overriddenColor() ) appearance.color = rule->overriddenColor();
            if ( rule->overriddenSymbol() ) appearance.symbol = rule->overriddenSymbol();
        }

        appearances[itemName] = appearance;
    }

    return appearances;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimCurveStyleSettings::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
{
    QList<caf::PdmOptionItemInfo> options;

    if ( fieldNeedingOptions == &m_autoColorLegend )
    {
        if ( auto project = RimProject::current() )
        {
            auto collection    = project->colorLegendCollection();
            auto defaultLegend = collection->findByName( RimColorLegendCollection::summaryCurveColorLegendName() );

            // Not set (null) means the default palette, which is shown with the icon of the standard summary curve legend
            options.push_back( caf::PdmOptionItemInfo( "Default Palette",
                                                       nullptr,
                                                       false,
                                                       defaultLegend ? defaultLegend->paletteIconProvider() : caf::IconProvider() ) );

            for ( auto legend : collection->allColorLegends() )
            {
                if ( legend == defaultLegend ) continue;
                options.push_back( caf::PdmOptionItemInfo( legend->colorLegendName(), legend, false, legend->paletteIconProvider() ) );
            }
        }
        else
        {
            options.push_back( caf::PdmOptionItemInfo( "Default Palette", nullptr ) );
        }
    }
    else if ( fieldNeedingOptions == &m_symbolLegend )
    {
        const int count = static_cast<int>( caf::AppEnum<RiuPlotCurveSymbol::PointSymbolEnum>::size() ) - 1;
        for ( int i = 0; i < count; i++ )
        {
            const auto symbol = RimSummaryCurveAppearanceCalculator::cycledSymbol( i );
            const auto image  = RiuPlotCurveSymbolImageCreator::createSymbolImage( symbol, QSize( 14, 14 ), QColor( Qt::gray ) );
            options.push_back( caf::PdmOptionItemInfo( caf::AppEnum<RiuPlotCurveSymbol::PointSymbolEnum>::uiText( symbol ),
                                                       i,
                                                       false,
                                                       caf::IconProvider( QPixmap::fromImage( image ) ) ) );
        }
    }
    else if ( fieldNeedingOptions == &m_currentType )
    {
        for ( size_t i = 0; i < caf::AppEnum<Dimension>::size(); i++ )
        {
            const auto dimension = caf::AppEnum<Dimension>::fromIndex( i );
            options.push_back( caf::PdmOptionItemInfo( caf::AppEnum<Dimension>::uiText( dimension ), static_cast<int>( dimension ) ) );
        }
    }
    else if ( fieldNeedingOptions == &m_highlightedItems )
    {
        RimCurveStyleRule::AvailableNamesCache namesCache;

        const Dimension dimension = m_currentType();

        std::map<QString, int> indices;
        if ( auto collection = firstAncestorOrThisOfType<RimCurveStyleRules>() ) indices = collection->itemIndices( dimension );

        const bool isColorDim      = m_appearanceType() == AppearanceType::COLOR;
        const bool isSymbolDim     = m_appearanceType() == AppearanceType::SYMBOL;
        const auto plotAppearances = RimCurveStyleRule::appearancesFromPlots( dimension, this );
        for ( const auto& name : RimCurveStyleRule::availableNames( dimension ) )
        {
            const auto customRule = findRule( dimension, name );
            const bool isCustom   = customRule && customRule->isCustom();
            const auto indexIt    = indices.find( name );
            const int  index      = indexIt != indices.end() ? indexIt->second : static_cast<int>( options.size() );

            ItemAppearance autoAppearance;
            assignedAppearance( dimension, name, index, plotAppearances, autoAppearance );

            std::optional<cvf::Color3f> color;
            if ( isColorDim && autoAppearance.color )
                color = isCustom ? customRule->overriddenColor().value_or( *autoAppearance.color ) : autoAppearance.color;

            std::optional<RiuPlotCurveSymbol::PointSymbolEnum> symbol;
            if ( isSymbolDim && autoAppearance.symbol )
                symbol = isCustom ? customRule->overriddenSymbol().value_or( *autoAppearance.symbol ) : autoAppearance.symbol;

            const auto icon = RimCurveStyleRule::dimensionColorIcon( dimension, color, symbol );
            options.push_back( caf::PdmOptionItemInfo( name, name, false, icon ) );
        }
    }

    return options;
}
