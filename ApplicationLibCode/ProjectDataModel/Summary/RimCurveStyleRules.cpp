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

#include "RimCurveStyleRules.h"

#include "RimColorLegend.h"
#include "RimColorLegendCollection.h"
#include "RimColorLegendItem.h"
#include "RimCurveStyleRule.h"
#include "RimCurveStyleSettings.h"
#include "RimEnsembleCurveSet.h"
#include "RimEnsembleCurveSetCollection.h"
#include "RimProject.h"
#include "RimSummaryCurve.h"
#include "RimSummaryCurveAppearanceCalculator.h"
#include "RimSummaryPlot.h"

#include "cafPdmUiLabelEditor.h"
#include "cafPdmUiListEditor.h"

#include "RiaApplication.h"
#include "RiaColorTools.h"
#include "RiaGuiApplication.h"
#include "RiuPlotCurveSymbolImageCreator.h"
#include "RiuPlotMainWindow.h"

CAF_PDM_SOURCE_INIT( RimCurveStyleRules, "RimCurveStyleRules" );

namespace caf
{
template <>
void caf::AppEnum<RimCurveStyleRules::AppearanceType>::setUp()
{
    using T = RimCurveStyleRules::AppearanceType;
    addItem( T::COLOR, "COLOR", "Color" );
    addItem( T::SYMBOL, "SYMBOL", "Symbol" );
    setDefault( T::COLOR );
}
} // namespace caf

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimCurveStyleRules::RimCurveStyleRules()
{
    CAF_PDM_InitObject( "Curve Appearance Overrides" );

    CAF_PDM_InitField( &m_useGlobalAppearance,
                       "UseGlobalAppearance",
                       false,
                       "Use Global Curve Appearance",
                       "",
                       "Assign curve appearance in all summary plots from the global curve appearance definition" );

    CAF_PDM_InitFieldNoDefault( &m_settings, "Settings", "Settings" );
    m_settings.uiCapability()->setUiTreeChildrenHidden( true );
    m_settings.uiCapability()->setUiHidden( true );
    m_settings = new RimCurveStyleSettings;

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
                       caf::AppEnum<RimCurveStyleRule::Dimension>( RimCurveStyleRule::Dimension::CASE ),
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
RimCurveStyleSettings* RimCurveStyleRules::settings() const
{
    return m_settings();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RimCurveStyleRule*> RimCurveStyleRules::overrides() const
{
    return m_overrides.childrenByType();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<const RimCurveStyleRule*> RimCurveStyleRules::overridesFor( const RifEclipseSummaryAddress& address, const std::string& caseName ) const
{
    std::vector<const RimCurveStyleRule*> result;
    for ( int pass = 0; pass < 2; pass++ )
    {
        for ( auto item : m_overrides )
        {
            const bool isCase = item->dimension() == RimCurveStyleRule::Dimension::CASE;
            if ( isCase != ( pass == 0 ) ) continue;

            const auto value = RimCurveStyleSettings::dimensionValue( item->dimension(), address, caseName );
            if ( !value.empty() && QString::fromStdString( value ) == item->itemName() ) result.push_back( item );
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::overridesChanged()
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
void RimCurveStyleRules::configureGlobalAppearanceLink( caf::PdmFieldHandle* field, const QString& text )
{
    field->uiCapability()->setUiEditorTypeName( caf::PdmUiLabelEditor::uiEditorTypeName() );
    field->uiCapability()->setAttribute( caf::PdmUiLabelEditor::Keys::USE_WORD_WRAP, true );
    field->uiCapability()->setAttribute( caf::PdmUiLabelEditor::Keys::LINK_TEXT, text );
    field->uiCapability()->setAttribute<std::function<void( const QString& )>>( caf::PdmUiLabelEditor::Keys::LINK_ACTIVATED_CALLBACK,
                                                                                []( const QString& )
                                                                                {
                                                                                    if ( auto window =
                                                                                             RiaGuiApplication::instance()->mainPlotWindow() )
                                                                                        window->showCurveAppearance();
                                                                                } );
    field->xmlCapability()->disableIO();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimCurveStyleRules::useGlobalAppearance() const
{
    return m_useGlobalAppearance();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::setUseGlobalAppearance( bool enable )
{
    if ( m_useGlobalAppearance() == enable ) return;

    m_useGlobalAppearance = enable;
    applyGlobalAppearanceMode();
}

//--------------------------------------------------------------------------------------------------
/// Applies the global appearance to all plots when enabled, and restores the original appearance when disabled
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::applyGlobalAppearanceMode()
{
    auto project = RimProject::current();
    if ( !project ) return;

    for ( auto plot : project->descendantsIncludingThisOfType<RimSummaryPlot>() )
    {
        if ( m_useGlobalAppearance() )
        {
            saveAppearance( plot );
            plot->updateCurveAppearanceFromGlobalSettings();
        }
        else
        {
            // Plots without a saved appearance get the calculated default appearance
            if ( !restoreAppearance( plot ) )
            {
                plot->applyDefaultCurveAppearances();
            }

            plot->updateConnectedEditors();
            plot->loadDataAndUpdate();
        }
    }

    if ( !m_useGlobalAppearance() ) m_savedAppearances.clear();

    updateConnectedEditors();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::saveAppearance( RimSummaryPlot* plot )
{
    SavedAppearance saved;

    for ( auto curve : plot->summaryCurves() )
    {
        saved.curves[curve].reset( curve->curveAppearance()->copyObject<RimPlotCurveAppearance>() );
    }

    for ( auto curveSet : plot->ensembleCurveSetCollection()->curveSets() )
    {
        saved.ensembleColors[curveSet] = RiaColorTools::fromQColorTo3f( curveSet->mainEnsembleColor() );
    }

    m_savedAppearances[plot] = std::move( saved );
}

//--------------------------------------------------------------------------------------------------
/// Returns false if nothing was saved for the plot
//--------------------------------------------------------------------------------------------------
bool RimCurveStyleRules::restoreAppearance( RimSummaryPlot* plot )
{
    auto it = m_savedAppearances.find( plot );
    if ( it == m_savedAppearances.end() ) return false;

    const auto& saved = it->second;

    for ( auto curve : plot->summaryCurves() )
    {
        auto curveIt = saved.curves.find( curve );
        if ( curveIt != saved.curves.end() ) curve->curveAppearance()->copyFrom( *curveIt->second );
    }

    for ( auto curveSet : plot->ensembleCurveSetCollection()->curveSets() )
    {
        auto colorIt = saved.ensembleColors.find( curveSet );
        if ( colorIt != saved.ensembleColors.end() ) curveSet->setColorAndUpdateCurves( colorIt->second );
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::optional<cvf::Color3f> RimCurveStyleRules::autoLegendColor( int index ) const
{
    if ( !m_autoColorLegend() || index < 0 ) return std::nullopt;

    const auto items = m_autoColorLegend()->colorLegendItems();
    if ( items.empty() ) return std::nullopt;

    return items[index % items.size()]->color();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimCurveStyleRules::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
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
        using D = RimCurveStyleRule::Dimension;
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
        for ( const auto& name : RimCurveStyleRule::availableNames( m_currentType() ) )
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
            const auto icon = RimCurveStyleRule::dimensionColorIcon( m_currentType(), color, symbol );
            options.push_back(
                caf::PdmOptionItemInfo( isCustom ? name + "  [Custom]" : name + "  [Auto]", static_cast<int>( options.size() ), false, icon ) );
        }
    }

    return options;
}

//--------------------------------------------------------------------------------------------------
/// The color and symbol the global appearance assigns to the item at the given position
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::assignedAppearance( const QString&                                      name,
                                             int                                                 index,
                                             std::optional<cvf::Color3f>&                        color,
                                             std::optional<RiuPlotCurveSymbol::PointSymbolEnum>& symbol ) const
{
    const bool projectWide = m_currentType() == RimCurveStyleRule::Dimension::CASE || m_currentType() == RimCurveStyleRule::Dimension::WELL;

    if ( projectWide ) color = RimSummaryCurveAppearanceCalculator::cycledPaletteColor( index );
    if ( !color ) color = RimCurveStyleRule::colorFromPlots( m_currentType(), name );
    if ( !color ) color = RimSummaryCurveAppearanceCalculator::cycledPaletteColor( index );

    if ( projectWide ) symbol = RimSummaryCurveAppearanceCalculator::cycledSymbol( index );
    if ( !symbol ) symbol = RimCurveStyleRule::symbolFromPlots( m_currentType(), name );
    if ( !symbol ) symbol = RimSummaryCurveAppearanceCalculator::cycledSymbol( index );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimCurveStyleRule* RimCurveStyleRules::findOverride( RimCurveStyleRule::Dimension dimension, const QString& name ) const
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
void RimCurveStyleRules::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    if ( changedField == &m_useGlobalAppearance )
    {
        applyGlobalAppearanceMode();
        return;
    }

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
            auto item = new RimCurveStyleRule;
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
QString RimCurveStyleRules::currentItemName() const
{
    const auto names = RimCurveStyleRule::availableNames( m_currentType() );
    const int  index = m_currentItemIndex();
    return index >= 0 && index < names.size() ? names[index] : QString();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimCurveStyleRule::Dimension RimCurveStyleRules::currentDimension() const
{
    auto settings = m_settings();
    if ( !settings ) return RimCurveStyleRule::Dimension::NONE;

    return m_appearanceType() == AppearanceType::COLOR ? settings->colorDimension() : settings->symbolDimension();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    // The dimension is stored in the global settings, which can also be changed elsewhere
    m_currentType = currentDimension();

    uiOrdering.add( &m_useGlobalAppearance );
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