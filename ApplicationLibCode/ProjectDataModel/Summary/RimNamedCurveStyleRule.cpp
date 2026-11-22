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

#include "RimNamedCurveStyleRule.h"

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

#include <set>

namespace caf
{
template <>
void caf::AppEnum<RimNamedCurveStyleRule::AppliesTo>::setUp()
{
    using T = RimNamedCurveStyleRule::AppliesTo;
    addItem( T::COLOR, "COLOR", "Color" );
    addItem( T::SYMBOL, "SYMBOL", "Symbol" );
    setDefault( T::COLOR );
}

template <>
void caf::AppEnum<RimNamedCurveStyleRule::Dimension>::setUp()
{
    using D = RimNamedCurveStyleRule::Dimension;
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

CAF_PDM_SOURCE_INIT( RimNamedCurveStyleRule, "RimNamedCurveStyleRule" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimNamedCurveStyleRule::RimNamedCurveStyleRule()
{
    CAF_PDM_InitObject( "Named Curve Style Rule" );

    CAF_PDM_InitField( &m_name, "Name", QString( "Rule" ), "Name" );
    CAF_PDM_InitField( &m_appliesTo,
                       "AppliesTo",
                       caf::AppEnum<AppliesTo>( AppliesTo::COLOR ),
                       "Distribute",
                       "",
                       "The curve property assigned to the items of the dimension" );
    CAF_PDM_InitField( &m_dimension, "Dimension", DimensionEnum( Dimension::WELL ), "Differentiate By", "", "Data dimension distributing the property" );

    CAF_PDM_InitFieldNoDefault( &m_rules, "Rules", "Rules" );
    m_rules.uiCapability()->setUiTreeChildrenHidden( true );
    m_rules.uiCapability()->setUiHidden( true );

    CAF_PDM_InitFieldNoDefault( &m_autoColorLegend, "AutoColorLegend", "Color Palette" );

    CAF_PDM_InitField( &m_symbolLegend, "SymbolLegend", 0, "Symbol Sequence" );
    m_symbolLegend.uiCapability()->setUiEditorTypeName( caf::PdmUiListEditor::uiEditorTypeName() );
    m_symbolLegend.uiCapability()->setUiLabelPosition( caf::PdmUiItemInfo::LabelPosition::TOP );
    m_symbolLegend.uiCapability()->setUiReadOnly( true );
    m_symbolLegend.uiCapability()->setAttribute( caf::PdmUiListEditor::Keys::HEIGHT_HINT, 120 );
    m_symbolLegend.xmlCapability()->disableIO();

    // Checked items are highlighted
    CAF_PDM_InitFieldNoDefault( &m_highlightedItems, "HighlightedItems", "Items" );
    m_highlightedItems.registerGetMethod( this, &RimNamedCurveStyleRule::highlightedItems );
    m_highlightedItems.registerSetMethod( this, &RimNamedCurveStyleRule::setHighlightedItems );
    m_highlightedItems.uiCapability()->setUiEditorTypeName( caf::PdmUiTreeSelectionEditor::uiEditorTypeName() );
    m_highlightedItems.uiCapability()->setUiLabelPosition( caf::PdmUiItemInfo::LabelPosition::TOP );
    m_highlightedItems.uiCapability()->setAttribute( caf::PdmUiTreeSelectionEditor::Keys::SHOW_TEXT_FILTER, false );
    m_highlightedItems.uiCapability()->setAttribute( caf::PdmUiTreeSelectionEditor::Keys::SHOW_TOGGLE_ALL_CHECKBOX, false );
    m_highlightedItems.uiCapability()->setAttribute( caf::PdmUiTreeSelectionEditor::Keys::HEIGHT_HINT, 200 );
    m_highlightedItems.uiCapability()->setAttribute( caf::PdmUiTreeSelectionEditor::Keys::ICON_SIZE, QSize( 33, 16 ) );
    m_highlightedItems.xmlCapability()->disableIO();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimNamedCurveStyleRule::~RimNamedCurveStyleRule() = default;

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimNamedCurveStyleRule::name() const
{
    return m_name();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimNamedCurveStyleRule::Dimension RimNamedCurveStyleRule::dimension() const
{
    return m_dimension();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimNamedCurveStyleRule::appliesToColor() const
{
    return m_appliesTo() == AppliesTo::COLOR;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimNamedCurveStyleRule::appliesToSymbol() const
{
    return m_appliesTo() == AppliesTo::SYMBOL;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimNamedCurveStyleRule::setName( const QString& name )
{
    m_name = name;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimNamedCurveStyleRule::setDimension( Dimension dimension )
{
    m_dimension = dimension;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimNamedCurveStyleRule::setAppliesTo( AppliesTo appliesTo )
{
    m_appliesTo = appliesTo;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::string RimNamedCurveStyleRule::caseName( const RimSummaryCase* summaryCase )
{
    if ( !summaryCase ) return {};
    if ( summaryCase->ensemble() ) return summaryCase->ensemble()->name().toStdString();
    return summaryCase->displayCaseName().toStdString();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::string RimNamedCurveStyleRule::dimensionValue( Dimension dimension, const RifEclipseSummaryAddress& address, const std::string& caseName )
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
bool RimNamedCurveStyleRule::matches( const RifEclipseSummaryAddress& address, const std::string& caseName ) const
{
    return m_dimension() != Dimension::NONE && !dimensionValue( m_dimension(), address, caseName ).empty();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RimCurveStyleRule*> RimNamedCurveStyleRule::rules() const
{
    return m_rules.childrenByType();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimCurveStyleRule* RimNamedCurveStyleRule::findRule( Dimension dimension, const QString& name ) const
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
RimCurveStyleRule* RimNamedCurveStyleRule::findOrCreateRule( Dimension dimension, const QString& name )
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
std::vector<RimNamedCurveStyleRule::RuleApplication>
    RimNamedCurveStyleRule::rulesFor( const Resolved& resolved, const RifEclipseSummaryAddress& address, const std::string& caseName )
{
    std::vector<RuleApplication> result;

    const auto collect = [&]( const RimNamedCurveStyleRule* source, bool isColor )
    {
        if ( !source || !source->matches( address, caseName ) ) return;

        const auto value = QString::fromStdString( dimensionValue( source->m_dimension(), address, caseName ) );
        for ( auto rule : source->m_rules )
        {
            if ( !rule->isCustom() && !rule->isHighlighted() ) continue;
            if ( rule->dimension() != source->m_dimension() || rule->itemName() != value ) continue;

            result.push_back( { rule, isColor, !isColor } );
        }
    };
    collect( resolved.color, true );
    collect( resolved.symbol, false );

    return result;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
cvf::Color3f RimNamedCurveStyleRule::paletteColor( int index ) const
{
    if ( m_autoColorLegend() && index >= 0 )
    {
        const auto items = m_autoColorLegend()->colorLegendItems();
        if ( !items.empty() ) return items[index % items.size()]->color();
    }

    return RimSummaryCurveAppearanceCalculator::cycledPaletteColor( index );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimNamedCurveStyleRule::setColorPalette( RimColorLegend* palette )
{
    m_autoColorLegend = palette;
}

//--------------------------------------------------------------------------------------------------
/// The names of the highlighted items
//--------------------------------------------------------------------------------------------------
std::vector<QString> RimNamedCurveStyleRule::highlightedItems() const
{
    std::vector<QString> names;
    for ( auto rule : m_rules )
    {
        if ( rule->dimension() == m_dimension() && rule->isHighlighted() ) names.push_back( rule->itemName() );
    }

    return names;
}

//--------------------------------------------------------------------------------------------------
/// Rules are only kept for items that are highlighted or custom
//--------------------------------------------------------------------------------------------------
void RimNamedCurveStyleRule::setHighlightedItems( const std::vector<QString>& names )
{
    const std::set<QString> highlighted( names.begin(), names.end() );

    for ( const auto& name : RimCurveStyleRule::availableNames( m_dimension() ) )
    {
        if ( highlighted.contains( name ) )
        {
            findOrCreateRule( m_dimension(), name )->setHighlighted( true );
        }
        else if ( auto rule = findRule( m_dimension(), name ) )
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
/// The color and symbol the rule assigns to the item at the given position
//--------------------------------------------------------------------------------------------------
void RimNamedCurveStyleRule::assignedAppearance( const QString&                           name,
                                                 int                                      index,
                                                 const std::map<QString, ItemAppearance>& plotAppearances,
                                                 ItemAppearance&                          appearance ) const
{
    const bool projectWide = m_dimension() == Dimension::CASE || m_dimension() == Dimension::WELL;

    const auto fromPlots = plotAppearances.find( name );
    const bool inPlots   = fromPlots != plotAppearances.end();

    auto& color  = appearance.color;
    auto& symbol = appearance.symbol;

    if ( projectWide ) color = paletteColor( index );
    if ( !color && inPlots ) color = fromPlots->second.color;
    if ( !color ) color = paletteColor( index );

    if ( projectWide ) symbol = RimSummaryCurveAppearanceCalculator::cycledSymbol( index );
    if ( !symbol && inPlots ) symbol = fromPlots->second.symbol;
    if ( !symbol ) symbol = RimSummaryCurveAppearanceCalculator::cycledSymbol( index );
}

//--------------------------------------------------------------------------------------------------
/// The assigned appearances are found once per call, as it can require scanning all summary addresses and plots.
//--------------------------------------------------------------------------------------------------
std::map<QString, RimNamedCurveStyleRule::ItemAppearance> RimNamedCurveStyleRule::itemAppearances() const
{
    RimCurveStyleRule::AvailableNamesCache namesCache;

    std::map<QString, ItemAppearance> appearances;

    std::map<QString, int> indices;
    if ( auto collection = firstAncestorOrThisOfType<RimCurveStyleRules>() ) indices = collection->itemIndices( m_dimension() );

    const auto plotAppearances = RimCurveStyleRule::appearancesFromPlots( m_dimension(), this );
    for ( const auto& itemName : RimCurveStyleRule::availableNames( m_dimension() ) )
    {
        const auto indexIt = indices.find( itemName );
        const int  index   = indexIt != indices.end() ? indexIt->second : static_cast<int>( appearances.size() );

        ItemAppearance appearance;
        assignedAppearance( itemName, index, plotAppearances, appearance );

        if ( const auto rule = findRule( m_dimension(), itemName ); rule && rule->isCustom() )
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
QList<caf::PdmOptionItemInfo> RimNamedCurveStyleRule::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
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
    else if ( fieldNeedingOptions == &m_dimension )
    {
        for ( size_t i = 0; i < caf::AppEnum<Dimension>::size(); i++ )
        {
            const auto dimension = caf::AppEnum<Dimension>::fromIndex( i );
            if ( dimension == Dimension::NONE ) continue;

            options.push_back( caf::PdmOptionItemInfo( caf::AppEnum<Dimension>::uiText( dimension ), static_cast<int>( dimension ) ) );
        }
    }
    else if ( fieldNeedingOptions == &m_highlightedItems )
    {
        RimCurveStyleRule::AvailableNamesCache namesCache;

        const Dimension dimension = m_dimension();

        std::map<QString, int> indices;
        if ( auto collection = firstAncestorOrThisOfType<RimCurveStyleRules>() ) indices = collection->itemIndices( dimension );

        const auto plotAppearances = RimCurveStyleRule::appearancesFromPlots( dimension, this );
        for ( const auto& name : RimCurveStyleRule::availableNames( dimension ) )
        {
            const auto customRule = findRule( dimension, name );
            const bool isCustom   = customRule && customRule->isCustom();
            const auto indexIt    = indices.find( name );
            const int  index      = indexIt != indices.end() ? indexIt->second : static_cast<int>( options.size() );

            ItemAppearance autoAppearance;
            assignedAppearance( name, index, plotAppearances, autoAppearance );

            std::optional<cvf::Color3f> color;
            if ( appliesToColor() && autoAppearance.color )
                color = isCustom ? customRule->overriddenColor().value_or( *autoAppearance.color ) : autoAppearance.color;

            std::optional<RiuPlotCurveSymbol::PointSymbolEnum> symbol;
            if ( appliesToSymbol() && autoAppearance.symbol )
                symbol = isCustom ? customRule->overriddenSymbol().value_or( *autoAppearance.symbol ) : autoAppearance.symbol;

            const auto icon = RimCurveStyleRule::dimensionColorIcon( dimension, color, symbol );
            options.push_back( caf::PdmOptionItemInfo( name, name, false, icon ) );
        }
    }

    return options;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimNamedCurveStyleRule::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    RimCurveStyleRule::AvailableNamesCache namesCache;

    uiOrdering.add( &m_name );
    uiOrdering.add( &m_appliesTo );
    uiOrdering.add( &m_dimension );

    auto rulesGroup = uiOrdering.addNewGroup( "Assigned Appearance" );
    if ( appliesToColor() )
        rulesGroup->add( &m_autoColorLegend );
    else
        rulesGroup->add( &m_symbolLegend );
    rulesGroup->add( &m_highlightedItems );
    rulesGroup->addNewLabel( "Checked items are highlighted in the plots, while other curves are dimmed." );

    uiOrdering.skipRemainingFields();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimNamedCurveStyleRule::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    RimCurveStyleRule::AvailableNamesCache namesCache;

    // The items of the rules depend on the dimension
    if ( changedField == &m_dimension ) m_rules.deleteChildren();

    auto collection = firstAncestorOrThisOfType<RimCurveStyleRules>();
    if ( !collection ) return;

    // The name only changes what is shown
    if ( changedField != &m_name ) collection->rulesChanged();

    collection->updateConnectedEditors();
}
