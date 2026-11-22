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

#include "RiaExperimentalFeatures.h"
#include "RiaGuiApplication.h"
#include "RiaPreferencesSystem.h"

#include "RimCurveStyleRule.h"
#include "RimCurveStyleSettings.h"
#include "RimEnsembleCurveSet.h"
#include "RimEnsembleCurveSetCollection.h"
#include "RimMainPlotCollection.h"
#include "RimNamedCurveStyleRule.h"
#include "RimProject.h"
#include "RimSummaryCase.h"
#include "RimSummaryCurve.h"
#include "RimSummaryEnsemble.h"
#include "RimSummaryMultiPlot.h"
#include "RimSummaryMultiPlotCollection.h"
#include "RimSummaryPlot.h"

#include "RiuPlotCurveSymbolImageCreator.h"
#include "RiuPlotMainWindow.h"

#include "cafPdmUiLabel.h"

#include <algorithm>
#include <map>
#include <set>

CAF_PDM_SOURCE_INIT( RimCurveStyleRules, "RimCurveStyleRules" );

namespace
{
std::vector<RimSummaryMultiPlot*> summaryMultiPlots()
{
    auto plotCollection = RimMainPlotCollection::current();
    if ( !plotCollection || !plotCollection->summaryMultiPlotCollection() ) return {};

    return plotCollection->summaryMultiPlotCollection()->multiPlots();
}

std::vector<RimSummaryPlot*> allSummaryPlots()
{
    std::vector<RimSummaryPlot*> plots;
    for ( auto multiPlot : summaryMultiPlots() )
    {
        for ( auto plot : multiPlot->summaryPlots() )
            plots.push_back( plot );
    }
    return plots;
}

QString uniqueName( const QString& base, const QStringList& existing )
{
    for ( int i = 1;; i++ )
    {
        const QString name = QString( "%1 %2" ).arg( base ).arg( i );
        if ( !existing.contains( name ) ) return name;
    }
}
} // namespace

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimCurveStyleRules::RimCurveStyleRules()
{
    CAF_PDM_InitObject( "Curve Style Rules" );

    CAF_PDM_InitFieldNoDefault( &m_presets, "Presets", "Global Settings" );
    m_presets.uiCapability()->setUiTreeChildrenHidden( true );
    m_presets.uiCapability()->setUiHidden( true );

    CAF_PDM_InitFieldNoDefault( &m_namedRules, "NamedRules", "Named Rules" );
    m_namedRules.uiCapability()->setUiTreeChildrenHidden( true );
    m_namedRules.uiCapability()->setUiHidden( true );

    CAF_PDM_InitFieldNoDefault( &m_savedAppearances, "SavedAppearances", "Saved Appearances" );
    m_savedAppearances.uiCapability()->setUiTreeChildrenHidden( true );
    m_savedAppearances.uiCapability()->setUiHidden( true );
    // Only valid during the session, so not stored in the project file
    m_savedAppearances.xmlCapability()->disableIO();

    CAF_PDM_InitFieldNoDefault( &m_caseOrder, "CaseOrder", "Case Order" );
    m_caseOrder.uiCapability()->setUiHidden( true );
    CAF_PDM_InitFieldNoDefault( &m_wellOrder, "WellOrder", "Well Order" );
    m_wellOrder.uiCapability()->setUiHidden( true );

    CAF_PDM_InitFieldNoDefault( &m_selectedPreset, "SelectedPreset", "Global Setting" );
    m_selectedPreset.xmlCapability()->disableIO();
    CAF_PDM_InitFieldNoDefault( &m_selectedNamedRule, "SelectedNamedRule", "Named Rule" );
    m_selectedNamedRule.xmlCapability()->disableIO();

    ensureDefaultPreset();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::initAfterRead()
{
    ensureDefaultPreset();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::ensureDefaultPreset()
{
    if ( m_presets.empty() )
    {
        auto preset = new RimCurveStyleSettings;
        preset->setName( "Default" );
        m_presets.push_back( preset );
    }

    ensureSelection();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::ensureSelection()
{
    if ( !m_selectedPreset() && !m_presets.empty() ) m_selectedPreset = m_presets.childrenByType().front();
    if ( !m_selectedNamedRule() && !m_namedRules.empty() ) m_selectedNamedRule = m_namedRules.childrenByType().front();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RimCurveStyleSettings*> RimCurveStyleRules::presets() const
{
    return m_presets.childrenByType();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimCurveStyleSettings* RimCurveStyleRules::defaultSettings() const
{
    const auto all = m_presets.childrenByType();
    return all.empty() ? nullptr : all.front();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimCurveStyleSettings* RimCurveStyleRules::addPreset( const QString& name )
{
    auto preset = new RimCurveStyleSettings;
    preset->setName( name );
    m_presets.push_back( preset );
    m_selectedPreset = preset;

    return preset;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimCurveStyleSettings* RimCurveStyleRules::duplicatePreset( const RimCurveStyleSettings* settings )
{
    if ( !settings ) return nullptr;

    auto copy = settings->copyObject<RimCurveStyleSettings>();
    if ( !copy ) return nullptr;

    QStringList names;
    for ( auto preset : m_presets )
        names.push_back( preset->name() );

    copy->setName( uniqueName( settings->name(), names ) );
    m_presets.push_back( copy );
    m_selectedPreset = copy;

    return copy;
}

//--------------------------------------------------------------------------------------------------
/// Named rules using the deleted setting are moved to the first remaining one, plots using it as
/// global setting lose it
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::deletePreset( RimCurveStyleSettings* settings )
{
    if ( !settings || m_presets.size() <= 1 ) return;

    RimCurveStyleSettings* fallback = nullptr;
    for ( auto preset : m_presets )
    {
        if ( preset != settings )
        {
            fallback = preset;
            break;
        }
    }

    for ( auto rule : m_namedRules )
    {
        if ( rule->settings() == settings ) rule->setSettings( fallback );
    }
    if ( m_selectedPreset() == settings ) m_selectedPreset = fallback;

    m_presets.removeChild( settings );
    delete settings;

    applyGlobalAppearance( allSummaryPlots() );
    for ( auto multiPlot : summaryMultiPlots() )
        multiPlot->updateConnectedEditors();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RimNamedCurveStyleRule*> RimCurveStyleRules::namedRules() const
{
    return m_namedRules.childrenByType();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimNamedCurveStyleRule* RimCurveStyleRules::addNamedRule( const QString& name )
{
    auto rule = new RimNamedCurveStyleRule;
    rule->setName( name );
    rule->setSettings( m_selectedPreset() ? m_selectedPreset() : defaultSettings() );
    m_namedRules.push_back( rule );
    m_selectedNamedRule = rule;

    return rule;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::deleteNamedRule( RimNamedCurveStyleRule* rule )
{
    if ( !rule ) return;

    m_namedRules.removeChild( rule );
    delete rule;

    ensureSelection();

    applyGlobalAppearance( allSummaryPlots() );
    for ( auto multiPlot : summaryMultiPlots() )
        multiPlot->updateConnectedEditors();
}

//--------------------------------------------------------------------------------------------------
/// The named rules have precedence over the global setting, and later named rules over earlier ones.
/// Item rules of the setting a property is assigned from are applied by RimCurveStyleSettings::rulesFor
//--------------------------------------------------------------------------------------------------
RimCurveStyleSettings::Resolved RimCurveStyleRules::resolve( const RimCurveStyleSettings*                      globalSettings,
                                                             const std::vector<const RimNamedCurveStyleRule*>& namedRules,
                                                             const RifEclipseSummaryAddress&                   address,
                                                             const std::string&                                caseName )
{
    using Settings = RimCurveStyleSettings;

    const auto hasValue = [&]( Settings::Dimension dimension )
    { return dimension != Settings::Dimension::NONE && !Settings::dimensionValue( dimension, address, caseName ).empty(); };

    Settings::Resolved resolved;
    if ( globalSettings )
    {
        if ( hasValue( globalSettings->colorDimension() ) )
            resolved.color = Settings::Source{ globalSettings, globalSettings->colorDimension() };
        if ( hasValue( globalSettings->symbolDimension() ) )
            resolved.symbol = Settings::Source{ globalSettings, globalSettings->symbolDimension() };
    }

    for ( auto rule : namedRules )
    {
        if ( !rule->matches( address, caseName ) ) continue;

        if ( rule->appliesToColor() ) resolved.color = Settings::Source{ rule->settings(), rule->dimension() };
        if ( rule->appliesToSymbol() ) resolved.symbol = Settings::Source{ rule->settings(), rule->dimension() };
    }

    return resolved;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::rulesChanged()
{
    auto project = RimProject::current();
    if ( !project ) return;

    RimCurveStyleRule::AvailableNamesCache namesCache;
    for ( auto plot : project->descendantsIncludingThisOfType<RimSummaryPlot>() )
    {
        plot->updateCurveAppearanceFromGlobalSettings();
    }

    // The data source tree shows a tag with the global color of each case
    updateSummaryCaseTags();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::updateSummaryCaseTags()
{
    m_tagCache.clear();

    auto project = RimProject::current();
    if ( !project ) return;

    for ( auto summaryCase : project->allSummaryCases() )
    {
        summaryCase->updateConnectedEditors();
    }

    for ( auto ensemble : project->summaryEnsembles() )
    {
        ensemble->updateConnectedEditors();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::ruleChanged( RimCurveStyleRule* rule )
{
    rulesChanged();
    updateConnectedEditors();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::registerItemNames( RimCurveStyleRule::Dimension dimension, const QStringList& names )
{
    auto order = itemOrder( dimension );
    if ( !order ) return;

    auto                    values = ( *order )();
    const std::set<QString> registered( values.begin(), values.end() );
    for ( const auto& name : names )
    {
        if ( !registered.contains( name ) ) values.push_back( name );
    }

    if ( values.size() != ( *order )().size() ) order->setValue( values );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::map<QString, int> RimCurveStyleRules::itemIndices( RimCurveStyleRule::Dimension dimension ) const
{
    auto order = itemOrder( dimension );
    if ( !order ) return {};

    std::map<QString, int> indices;

    int index = 0;
    for ( const auto& name : ( *order )() )
    {
        if ( indices.emplace( name, index ).second ) index++;
    }

    for ( const auto& name : RimCurveStyleRule::availableNames( dimension ) )
    {
        if ( indices.emplace( name, index ).second ) index++;
    }

    return indices;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::PdmField<std::vector<QString>>* RimCurveStyleRules::itemOrder( RimCurveStyleRule::Dimension dimension )
{
    if ( dimension == RimCurveStyleRule::Dimension::CASE ) return &m_caseOrder;
    if ( dimension == RimCurveStyleRule::Dimension::WELL ) return &m_wellOrder;
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const caf::PdmField<std::vector<QString>>* RimCurveStyleRules::itemOrder( RimCurveStyleRule::Dimension dimension ) const
{
    return const_cast<RimCurveStyleRules*>( this )->itemOrder( dimension );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::addGlobalAppearanceLink( caf::PdmUiOrdering& uiOrdering, const QString& text )
{
    if ( !RiaPreferencesSystem::current()->isFeatureEnabled( RiaExperimentalFeatures::curveAppearanceKeyword ) ) return;

    auto label = uiOrdering.addNewLabel( text );
    label->setLinkActivatedCallback(
        []( const QString& )
        {
            if ( auto window = RiaGuiApplication::instance()->mainPlotWindow() ) window->showCurveAppearance();
        } );
}

//--------------------------------------------------------------------------------------------------
/// Tags showing the color and symbol assigned to an item of the dimension, for the data source tree.
/// The assigned appearances are found once per setting and dimension, as it can require scanning all summary addresses and plots.
//--------------------------------------------------------------------------------------------------
std::vector<caf::PdmUiTreeViewItemTag> RimCurveStyleRules::curveStyleRuleTags( RimCurveStyleRule::Dimension dimension, const QString& name ) const
{
    std::vector<caf::PdmUiTreeViewItemTag> tags;
    if ( name.isEmpty() ) return tags;

    // The settings assigning color or symbol to the item in plots. Named rules come first as they take precedence
    std::vector<const RimCurveStyleSettings*> colorSettings;
    std::vector<const RimCurveStyleSettings*> symbolSettings;

    const auto addUnique = []( std::vector<const RimCurveStyleSettings*>& settingsList, const RimCurveStyleSettings* settings )
    {
        if ( settings && std::find( settingsList.begin(), settingsList.end(), settings ) == settingsList.end() )
            settingsList.push_back( settings );
    };

    for ( auto multiPlot : summaryMultiPlots() )
    {
        if ( !multiPlot->isGlobalAppearanceActive() ) continue;

        for ( auto rule : multiPlot->namedRules() )
        {
            if ( rule->dimension() != dimension ) continue;

            const auto items = rule->items();
            if ( !items.empty() && std::find( items.begin(), items.end(), name ) == items.end() ) continue;

            if ( rule->appliesToColor() ) addUnique( colorSettings, rule->settings() );
            if ( rule->appliesToSymbol() ) addUnique( symbolSettings, rule->settings() );
        }

        if ( auto global = multiPlot->globalSettings() )
        {
            if ( global->colorDimension() == dimension ) addUnique( colorSettings, global );
            if ( global->symbolDimension() == dimension ) addUnique( symbolSettings, global );
        }
    }

    if ( colorSettings.empty() && symbolSettings.empty() ) return tags;

    const auto appearanceFor = [this, dimension, &name]( const RimCurveStyleSettings* settings ) -> RimCurveStyleSettings::ItemAppearance
    {
        const auto key     = std::make_pair( settings, dimension );
        auto       cacheIt = m_tagCache.find( key );

        // Items added after the cache was built are not in the cache, rebuild to include them
        if ( cacheIt == m_tagCache.end() || !cacheIt->second.contains( name ) )
        {
            auto appearances = settings->itemAppearances( dimension );

            // Names without an assigned appearance get an empty entry, to avoid rebuilding the cache on every repaint
            if ( !appearances.contains( name ) ) appearances[name] = RimCurveStyleSettings::ItemAppearance();

            cacheIt = m_tagCache.insert_or_assign( key, std::move( appearances ) ).first;
        }

        return cacheIt->second.at( name );
    };

    std::vector<cvf::Color3f> shownColors;
    for ( auto settings : colorSettings )
    {
        const auto color = appearanceFor( settings ).color;
        if ( !color || std::find( shownColors.begin(), shownColors.end(), *color ) != shownColors.end() ) continue;
        shownColors.push_back( *color );

        auto tag    = caf::PdmUiTreeViewItemTag::create();
        tag.icon    = RimCurveStyleRule::swatchIcon( *color );
        tag.toolTip = "Global curve color";
        tags.push_back( tag );
    }

    std::vector<RiuPlotCurveSymbol::PointSymbolEnum> shownSymbols;
    for ( auto settings : symbolSettings )
    {
        const auto symbol = appearanceFor( settings ).symbol;
        if ( !symbol || std::find( shownSymbols.begin(), shownSymbols.end(), *symbol ) != shownSymbols.end() ) continue;
        shownSymbols.push_back( *symbol );

        const auto image = RiuPlotCurveSymbolImageCreator::createSymbolImage( *symbol, QSize( 14, 14 ), QColor( Qt::gray ) );

        auto tag    = caf::PdmUiTreeViewItemTag::create();
        tag.icon    = caf::IconProvider( QPixmap::fromImage( image ) );
        tag.toolTip = "Global curve symbol";
        tags.push_back( tag );
    }

    return tags;
}

//--------------------------------------------------------------------------------------------------
/// Applies the global appearance to the plots when enabled for them, and restores the original appearance when disabled
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::applyGlobalAppearance( const std::vector<RimSummaryPlot*>& plots )
{
    RimCurveStyleRule::AvailableNamesCache namesCache;

    // Restore saved appearances first, so properties no longer controlled globally (e.g. color when changing from color
    // and symbol to symbol only) go back to their original values before the global appearance is applied again.
    // Plots still using the global appearance keep curves without a saved appearance as they are.
    std::vector<RimSummaryPlot*> enabledPlots;
    std::vector<RimSummaryPlot*> disabledPlots;
    for ( auto plot : plots )
        ( plot->useGlobalCurveAppearance() ? enabledPlots : disabledPlots ).push_back( plot );

    restoreAppearances( enabledPlots, false );
    restoreAppearances( disabledPlots, true );

    for ( auto plot : enabledPlots )
    {
        saveAppearance( plot );
        plot->updateCurveAppearanceFromGlobalSettings();
    }

    updateSummaryCaseTags();
    updateConnectedEditors();
}

//--------------------------------------------------------------------------------------------------
/// Appearances saved earlier are kept, they are the appearance before the global appearance was used
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::saveAppearance( RimSummaryPlot* plot )
{
    std::set<caf::PdmObject*> alreadySaved;
    for ( auto saved : m_savedAppearances )
    {
        if ( auto object = saved->object() ) alreadySaved.insert( object );
    }

    for ( auto curve : plot->summaryCurves() )
    {
        if ( alreadySaved.contains( curve ) ) continue;

        auto saved = new RimSavedCurveAppearance;
        saved->setCurve( curve );
        m_savedAppearances.push_back( saved );
    }

    for ( auto curveSet : plot->ensembleCurveSetCollection()->curveSets() )
    {
        if ( alreadySaved.contains( curveSet ) ) continue;

        auto saved = new RimSavedCurveAppearance;
        saved->setEnsembleCurveSet( curveSet );
        m_savedAppearances.push_back( saved );
    }
}

//--------------------------------------------------------------------------------------------------
/// Curves without a saved appearance, typically created while the global appearance was enabled, get the
/// calculated default appearance
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::restoreAppearances( const std::vector<RimSummaryPlot*>& plots, bool applyDefaultsToUnsaved )
{
    std::set<caf::PdmObject*> plotObjects;
    for ( auto plot : plots )
    {
        for ( auto curve : plot->summaryCurves() )
            plotObjects.insert( curve );
        for ( auto curveSet : plot->ensembleCurveSetCollection()->curveSets() )
            plotObjects.insert( curveSet );
    }

    std::set<caf::PdmObject*>             restored;
    std::vector<RimSavedCurveAppearance*> consumed;
    for ( auto saved : m_savedAppearances )
    {
        const auto object = saved->object();
        if ( !object || !plotObjects.contains( object ) ) continue;

        if ( saved->restore() ) restored.insert( object );
        consumed.push_back( saved );
    }

    for ( auto saved : consumed )
    {
        m_savedAppearances.removeChild( saved );
        delete saved;
    }

    if ( !applyDefaultsToUnsaved ) return;

    for ( auto plot : plots )
    {
        std::vector<RimSummaryCurve*> curves;
        for ( auto curve : plot->summaryCurves() )
        {
            if ( !restored.contains( curve ) ) curves.push_back( curve );
        }
        if ( !curves.empty() ) plot->applyDefaultCurveAppearances( curves );

        std::vector<RimEnsembleCurveSet*> curveSets;
        for ( auto curveSet : plot->ensembleCurveSetCollection()->curveSets() )
        {
            if ( !restored.contains( curveSet ) ) curveSets.push_back( curveSet );
        }
        if ( !curveSets.empty() ) plot->applyDefaultCurveAppearances( curveSets );

        plot->updateConnectedEditors();
        plot->loadDataAndUpdate();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimCurveStyleRules::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
{
    QList<caf::PdmOptionItemInfo> options;

    if ( fieldNeedingOptions == &m_selectedPreset )
    {
        for ( auto preset : m_presets )
            options.push_back( caf::PdmOptionItemInfo( preset->name(), preset ) );
    }
    else if ( fieldNeedingOptions == &m_selectedNamedRule )
    {
        for ( auto rule : m_namedRules )
            options.push_back( caf::PdmOptionItemInfo( rule->name(), rule ) );
    }

    return options;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    updateConnectedEditors();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    RimCurveStyleRule::AvailableNamesCache namesCache;

    ensureSelection();

    auto presetGroup = uiOrdering.addNewGroup( "Global Settings" );
    presetGroup->add( &m_selectedPreset );
    presetGroup->addNewButton( "New",
                               [this]()
                               {
                                   QStringList names;
                                   for ( auto preset : m_presets )
                                       names.push_back( preset->name() );
                                   addPreset( uniqueName( "Global Setting", names ) );
                                   updateConnectedEditors();
                               } );
    presetGroup->addNewButton( "Duplicate",
                               [this]()
                               {
                                   duplicatePreset( m_selectedPreset() );
                                   updateConnectedEditors();
                               } );
    presetGroup->addNewButton( "Delete",
                               [this]()
                               {
                                   deletePreset( m_selectedPreset() );
                                   updateConnectedEditors();
                               } );

    if ( auto preset = m_selectedPreset() )
    {
        auto settingGroup = presetGroup->addNewGroup( "Selected Global Setting" );
        preset->appendFieldsToUiOrdering( *settingGroup );
    }

    auto ruleGroup = uiOrdering.addNewGroup( "Named Rules" );
    ruleGroup->addNewLabel(
        "A named rule applies a global setting to a dimension. Rules are selected per plot and take precedence over the global setting." );
    ruleGroup->add( &m_selectedNamedRule );
    ruleGroup->addNewButton( "New",
                             [this]()
                             {
                                 QStringList names;
                                 for ( auto rule : m_namedRules )
                                     names.push_back( rule->name() );
                                 addNamedRule( uniqueName( "Rule", names ) );
                                 updateConnectedEditors();
                             } );
    ruleGroup->addNewButton( "Delete",
                             [this]()
                             {
                                 deleteNamedRule( m_selectedNamedRule() );
                                 updateConnectedEditors();
                             } );

    if ( auto rule = m_selectedNamedRule() )
    {
        auto selectedRuleGroup = ruleGroup->addNewGroup( "Selected Named Rule" );
        rule->uiOrdering( uiConfigName, *selectedRuleGroup );
    }

    auto advancedGroup = uiOrdering.addNewGroup( "Advanced" );
    advancedGroup->setCollapsedByDefault();
    advancedGroup->addNewButton( "Connect All Plots to Selected Global Setting", [this]() { connectAllPlots( true ); } );
    advancedGroup->addNewButton( "Disconnect All Plots", [this]() { connectAllPlots( false ); } );

    uiOrdering.skipRemainingFields();
}

//--------------------------------------------------------------------------------------------------
/// Sets all summary multi plots to use the selected global setting, or to use local appearance
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::connectAllPlots( bool connect )
{
    for ( auto multiPlot : summaryMultiPlots() )
    {
        multiPlot->setGlobalSettings( connect ? m_selectedPreset() : nullptr );
        if ( !connect ) multiPlot->clearNamedRules();
        multiPlot->updateConnectedEditors();
    }

    applyGlobalAppearance( allSummaryPlots() );
}
