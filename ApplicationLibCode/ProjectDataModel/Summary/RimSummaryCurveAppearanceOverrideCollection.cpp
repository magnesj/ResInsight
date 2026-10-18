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

#include "RimSummaryCurveAppearanceOverrideCollection.h"

#include "RimColorLegend.h"
#include "RimColorLegendCollection.h"
#include "RimColorLegendItem.h"
#include "RimProject.h"
#include "RimSummaryCurveAppearanceCalculator.h"
#include "RimSummaryCurveAppearanceOverride.h"
#include "RimSummaryCurveAppearanceSettings.h"
#include "RimSummaryPlot.h"

#include "cafPdmUiListEditor.h"

#include "RiaApplication.h"
#include "RiuPlotCurveSymbolImageCreator.h"

CAF_PDM_SOURCE_INIT( RimSummaryCurveAppearanceOverrideCollection, "RimSummaryCurveAppearanceOverrideCollection" );

namespace caf
{
template <>
void caf::AppEnum<RimSummaryCurveAppearanceOverrideCollection::AppearanceType>::setUp()
{
    using T = RimSummaryCurveAppearanceOverrideCollection::AppearanceType;
    addItem( T::COLOR, "COLOR", "Color" );
    addItem( T::SYMBOL, "SYMBOL", "Symbol" );
    setDefault( T::COLOR );
}
} // namespace caf

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSummaryCurveAppearanceOverrideCollection::RimSummaryCurveAppearanceOverrideCollection()
{
    CAF_PDM_InitObject( "Curve Appearance Overrides" );

    CAF_PDM_InitFieldNoDefault( &m_settings, "Settings", "Settings" );
    m_settings.uiCapability()->setUiTreeChildrenHidden( true );
    m_settings.uiCapability()->setUiHidden( true );
    m_settings = new RimSummaryCurveAppearanceSettings;

    CAF_PDM_InitFieldNoDefault( &m_overrides, "Overrides", "Overrides" );
    m_overrides.uiCapability()->setUiTreeChildrenHidden( true );
    m_overrides.uiCapability()->setUiHidden( true );

    CAF_PDM_InitFieldNoDefault( &m_appearanceType, "AppearanceType", "Appearance Property" );
    m_appearanceType.xmlCapability()->disableIO();

    CAF_PDM_InitFieldNoDefault( &m_autoColorLegend, "AutoColorLegend", "Color Palette" );

    CAF_PDM_InitField( &m_symbolLegend, "SymbolLegend", 0, "Symbol Sequence" );
    m_symbolLegend.uiCapability()->setUiEditorTypeName( caf::PdmUiListEditor::uiEditorTypeName() );
    m_symbolLegend.uiCapability()->setUiLabelPosition( caf::PdmUiItemInfo::LabelPosition::TOP );
    m_symbolLegend.uiCapability()->setUiReadOnly( true );
    m_symbolLegend.uiCapability()->setAttribute( caf::PdmUiListEditor::Keys::HEIGHT_HINT, 120 );
    m_symbolLegend.xmlCapability()->disableIO();

    CAF_PDM_InitField( &m_currentType,
                       "CurrentType",
                       caf::AppEnum<RimSummaryCurveAppearanceOverride::Dimension>( RimSummaryCurveAppearanceOverride::Dimension::CASE ),
                       "Differentiate By" );
    m_currentType.xmlCapability()->disableIO();

    CAF_PDM_InitField( &m_currentItemIndex, "CurrentItemIndex", 0, "Items" );
    m_currentItemIndex.uiCapability()->setUiEditorTypeName( caf::PdmUiListEditor::uiEditorTypeName() );
    m_currentItemIndex.uiCapability()->setUiLabelPosition( caf::PdmUiItemInfo::LabelPosition::TOP );
    m_currentItemIndex.uiCapability()->setAttribute( caf::PdmUiListEditor::Keys::HEIGHT_HINT, 200 );
    m_currentItemIndex.xmlCapability()->disableIO();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSummaryCurveAppearanceSettings* RimSummaryCurveAppearanceOverrideCollection::settings() const
{
    return m_settings();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RimSummaryCurveAppearanceOverride*> RimSummaryCurveAppearanceOverrideCollection::overrides() const
{
    return m_overrides.childrenByType();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<const RimSummaryCurveAppearanceOverride*>
    RimSummaryCurveAppearanceOverrideCollection::overridesFor( const RifEclipseSummaryAddress& address, const std::string& caseName ) const
{
    std::vector<const RimSummaryCurveAppearanceOverride*> result;
    for ( int pass = 0; pass < 2; pass++ )
    {
        for ( auto item : m_overrides )
        {
            const bool isCase = item->dimension() == RimSummaryCurveAppearanceOverride::Dimension::CASE;
            if ( isCase != ( pass == 0 ) ) continue;

            const auto value = RimSummaryCurveAppearanceSettings::dimensionValue( item->dimension(), address, caseName );
            if ( !value.empty() && QString::fromStdString( value ) == item->itemName() ) result.push_back( item );
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryCurveAppearanceOverrideCollection::overridesChanged()
{
    auto project = RimProject::current();
    if ( !project ) return;

    for ( auto plot : project->descendantsIncludingThisOfType<RimSummaryPlot>() )
    {
        plot->updateCurveAppearanceFromGlobalSettings();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::optional<cvf::Color3f> RimSummaryCurveAppearanceOverrideCollection::autoLegendColor( int index ) const
{
    if ( !m_autoColorLegend() || index < 0 ) return std::nullopt;

    const auto items = m_autoColorLegend()->colorLegendItems();
    if ( items.empty() ) return std::nullopt;

    return items[index % items.size()]->color();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimSummaryCurveAppearanceOverrideCollection::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
{
    QList<caf::PdmOptionItemInfo> options;

    if ( fieldNeedingOptions == &m_autoColorLegend )
    {
        options.push_back( caf::PdmOptionItemInfo( "Default Palette", nullptr ) );
        if ( auto project = RimProject::current() )
        {
            for ( auto legend : project->colorLegendCollection()->allColorLegends() )
            {
                options.push_back( caf::PdmOptionItemInfo( legend->colorLegendName(), legend, false, legend->paletteIconProvider() ) );
            }
        }
    }
    else if ( fieldNeedingOptions == &m_symbolLegend )
    {
        const int count = static_cast<int>( caf::AppEnum<RiuPlotCurveSymbol::PointSymbolEnum>::size() ) - 1;
        for ( int i = 0; i < count; i++ )
        {
            const auto symbol = RimSummaryCurveAppearanceCalculator::cycledSymbol( i );
            const auto image  = RiuPlotCurveSymbolImageCreator::createSymbolImage( symbol, QSize( 14, 14 ), QColor( Qt::black ) );
            options.push_back( caf::PdmOptionItemInfo( caf::AppEnum<RiuPlotCurveSymbol::PointSymbolEnum>::uiText( symbol ),
                                                       i,
                                                       false,
                                                       caf::IconProvider( QPixmap::fromImage( image ) ) ) );
        }
    }
    else if ( fieldNeedingOptions == &m_currentType )
    {
        using D = RimSummaryCurveAppearanceOverride::Dimension;
        for ( size_t i = 0; i < caf::AppEnum<D>::size(); i++ )
        {
            const auto dimension = caf::AppEnum<D>::fromIndex( i );
            options.push_back( caf::PdmOptionItemInfo( caf::AppEnum<D>::uiText( dimension ), static_cast<int>( dimension ) ) );
        }
    }
    else if ( fieldNeedingOptions == &m_currentItemIndex )
    {
        const bool isColorDim  = m_appearanceType() == AppearanceType::COLOR;
        const bool isSymbolDim = m_appearanceType() == AppearanceType::SYMBOL;
        for ( const auto& name : RimSummaryCurveAppearanceOverride::availableNames( m_currentType() ) )
        {
            const auto                                         customOverride = findOverride( m_currentType(), name );
            const bool                                         isCustom       = customOverride && customOverride->isCustom();
            const int                                          index          = static_cast<int>( options.size() );
            std::optional<cvf::Color3f>                        autoColor;
            std::optional<RiuPlotCurveSymbol::PointSymbolEnum> autoSymbol;
            assignedAppearance( name, index, autoColor, autoSymbol );

            std::optional<cvf::Color3f> color;
            if ( isColorDim && autoColor ) color = isCustom ? customOverride->overriddenColor().value_or( *autoColor ) : autoColor;

            std::optional<RiuPlotCurveSymbol::PointSymbolEnum> symbol;
            if ( isSymbolDim && autoSymbol ) symbol = isCustom ? customOverride->overriddenSymbol().value_or( *autoSymbol ) : autoSymbol;
            const auto icon = RimSummaryCurveAppearanceOverride::dimensionColorIcon( m_currentType(), color, symbol );
            options.push_back(
                caf::PdmOptionItemInfo( isCustom ? name + "  [Custom]" : name + "  [Auto]", static_cast<int>( options.size() ), false, icon ) );
        }
    }

    return options;
}

//--------------------------------------------------------------------------------------------------
/// The color and symbol the global appearance assigns to the item at the given position
//--------------------------------------------------------------------------------------------------
void RimSummaryCurveAppearanceOverrideCollection::assignedAppearance( const QString&                                      name,
                                                                      int                                                 index,
                                                                      std::optional<cvf::Color3f>&                        color,
                                                                      std::optional<RiuPlotCurveSymbol::PointSymbolEnum>& symbol ) const
{
    const bool projectWide = m_currentType() == RimSummaryCurveAppearanceOverride::Dimension::CASE ||
                             m_currentType() == RimSummaryCurveAppearanceOverride::Dimension::WELL;

    if ( projectWide ) color = RimSummaryCurveAppearanceCalculator::cycledPaletteColor( index );
    if ( !color ) color = RimSummaryCurveAppearanceOverride::colorFromPlots( m_currentType(), name );
    if ( !color ) color = RimSummaryCurveAppearanceCalculator::cycledPaletteColor( index );

    if ( projectWide ) symbol = RimSummaryCurveAppearanceCalculator::cycledSymbol( index );
    if ( !symbol ) symbol = RimSummaryCurveAppearanceOverride::symbolFromPlots( m_currentType(), name );
    if ( !symbol ) symbol = RimSummaryCurveAppearanceCalculator::cycledSymbol( index );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSummaryCurveAppearanceOverride*
    RimSummaryCurveAppearanceOverrideCollection::findOverride( RimSummaryCurveAppearanceOverride::Dimension dimension, const QString& name ) const
{
    for ( auto item : m_overrides )
    {
        if ( item->dimension() == dimension && item->itemName() == name ) return item;
    }

    return nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryCurveAppearanceOverrideCollection::fieldChangedByUi( const caf::PdmFieldHandle* changedField,
                                                                    const QVariant&            oldValue,
                                                                    const QVariant&            newValue )
{
    if ( changedField == &m_autoColorLegend )
    {
        updateConnectedEditors();
        overridesChanged();
        return;
    }

    if ( changedField == &m_appearanceType )
    {
        m_currentType      = currentDimension();
        m_currentItemIndex = 0;
        updateConnectedEditors();
        return;
    }

    if ( changedField == &m_currentType )
    {
        if ( auto settings = m_settings() )
        {
            if ( m_appearanceType() == AppearanceType::COLOR )
                settings->setColorDimension( m_currentType() );
            else
                settings->setSymbolDimension( m_currentType() );
        }
        if ( auto app = RiaApplication::instance() ) app->applyPreferences();
        m_currentItemIndex = 0;
        overridesChanged();
    }

    const QString itemName = currentItemName();
    if ( ( changedField == &m_currentType || changedField == &m_currentItemIndex ) && !itemName.isEmpty() )
    {
        // Overrides are created when an item is selected, and are Auto until changed to Custom
        if ( !findOverride( m_currentType(), itemName ) )
        {
            auto item = new RimSummaryCurveAppearanceOverride;
            item->setDimension( m_currentType() );
            item->setItemName( itemName );
            m_overrides.push_back( item );
        }
    }

    updateConnectedEditors();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimSummaryCurveAppearanceOverrideCollection::currentItemName() const
{
    const auto names = RimSummaryCurveAppearanceOverride::availableNames( m_currentType() );
    const int  index = m_currentItemIndex();
    return index >= 0 && index < names.size() ? names[index] : QString();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSummaryCurveAppearanceOverride::Dimension RimSummaryCurveAppearanceOverrideCollection::currentDimension() const
{
    auto settings = m_settings();
    if ( !settings ) return RimSummaryCurveAppearanceOverride::Dimension::NONE;

    return m_appearanceType() == AppearanceType::COLOR ? settings->colorDimension() : settings->symbolDimension();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryCurveAppearanceOverrideCollection::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    // The dimension is stored in the global settings, which can also be changed elsewhere
    m_currentType = currentDimension();

    uiOrdering.add( &m_appearanceType );
    uiOrdering.add( &m_currentType );

    auto overridesGroup = uiOrdering.addNewGroup( "Assigned Appearance" );
    if ( m_appearanceType() == AppearanceType::COLOR )
        overridesGroup->add( &m_autoColorLegend );
    else
        overridesGroup->add( &m_symbolLegend );
    overridesGroup->add( &m_currentItemIndex );

    if ( auto selected = findOverride( m_currentType(), currentItemName() ) )
    {
        std::optional<cvf::Color3f>                        autoColor;
        std::optional<RiuPlotCurveSymbol::PointSymbolEnum> autoSymbol;
        assignedAppearance( currentItemName(), m_currentItemIndex(), autoColor, autoSymbol );
        selected->setAutoAssigned( m_appearanceType() == AppearanceType::COLOR ? autoColor : std::nullopt,
                                   m_appearanceType() == AppearanceType::SYMBOL ? autoSymbol : std::nullopt );
        if ( !selected->isCustom() ) selected->updateAppearanceFromCurves();

        auto appearanceGroup = uiOrdering.addNewGroup( "Selected Item" );
        selected->appendFieldsToUiOrdering( uiConfigName, *appearanceGroup );
    }

    uiOrdering.skipRemainingFields();
}