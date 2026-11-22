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
/// Plots using the deleted setting lose it
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
/// Later named rules take precedence over earlier ones. A rule only assigns a property if it has a value for the curve.
//--------------------------------------------------------------------------------------------------
RimNamedCurveStyleRule::Resolved RimCurveStyleRules::resolve( const std::vector<const RimNamedCurveStyleRule*>& namedRules,
                                                              const RifEclipseSummaryAddress&                   address,
                                                              const std::string&                                caseName )
{
    RimNamedCurveStyleRule::Resolved resolved;
    for ( auto rule : namedRules )
    {
        if ( !rule->matches( address, caseName ) ) continue;

        if ( rule->appliesToColor() ) resolved.color = rule;
        if ( rule->appliesToSymbol() ) resolved.symbol = rule;
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

    // The named rules assigning color or symbol to the item in plots. Later rules take precedence
    std::vector<const RimNamedCurveStyleRule*> colorRules;
    std::vector<const RimNamedCurveStyleRule*> symbolRules;

    const auto addUnique = []( std::vector<const RimNamedCurveStyleRule*>& rules, const RimNamedCurveStyleRule* rule )
    {
        if ( std::find( rules.begin(), rules.end(), rule ) == rules.end() ) rules.push_back( rule );
    };

    for ( auto multiPlot : summaryMultiPlots() )
    {
        if ( !multiPlot->isGlobalAppearanceActive() ) continue;

        for ( auto rule : multiPlot->namedRules() )
        {
            if ( rule->dimension() != dimension ) continue;

            if ( rule->appliesToColor() ) addUnique( colorRules, rule );
            if ( rule->appliesToSymbol() ) addUnique( symbolRules, rule );
        }
    }

    if ( colorRules.empty() && symbolRules.empty() ) return tags;

    const auto appearanceFor = [this, &name]( const RimNamedCurveStyleRule* rule ) -> RimNamedCurveStyleRule::ItemAppearance
    {
        auto cacheIt = m_tagCache.find( rule );

        // Items added after the cache was built are not in the cache, rebuild to include them
        if ( cacheIt == m_tagCache.end() || !cacheIt->second.contains( name ) )
        {
            auto appearances = rule->itemAppearances();

            // Names without an assigned appearance get an empty entry, to avoid rebuilding the cache on every repaint
            if ( !appearances.contains( name ) ) appearances[name] = RimNamedCurveStyleRule::ItemAppearance();

            cacheIt = m_tagCache.insert_or_assign( rule, std::move( appearances ) ).first;
        }

        return cacheIt->second.at( name );
    };

    std::vector<cvf::Color3f> shownColors;
    for ( auto rule : colorRules )
    {
        const auto color = appearanceFor( rule ).color;
        if ( !color || std::find( shownColors.begin(), shownColors.end(), *color ) != shownColors.end() ) continue;
        shownColors.push_back( *color );

        auto tag    = caf::PdmUiTreeViewItemTag::create();
        tag.icon    = RimCurveStyleRule::swatchIcon( *color );
        tag.toolTip = "Global curve color";
        tags.push_back( tag );
    }

    std::vector<RiuPlotCurveSymbol::PointSymbolEnum> shownSymbols;
    for ( auto rule : symbolRules )
    {
        const auto symbol = appearanceFor( rule ).symbol;
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
/// Applies the global appearance to the plots when enabled for them, and gives the curves of the other plots back the
/// settings the global appearance has controlled
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::applyGlobalAppearance( const std::vector<RimSummaryPlot*>& plots )
{
    RimCurveStyleRule::AvailableNamesCache namesCache;

    for ( auto plot : plots )
    {
        if ( plot->useGlobalCurveAppearance() )
        {
            plot->updateCurveAppearanceFromGlobalSettings();
        }
        else
        {
            releaseAppearances( plot );
        }
    }

    updateSummaryCaseTags();
    updateConnectedEditors();
}

//--------------------------------------------------------------------------------------------------
/// The settings are saved when the global appearance takes control of them, so they can be restored when it no
/// longer does. Settings the global appearance does not control are never saved, they stay editable per curve.
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::updateSavedAppearance( RimPlotCurve* curve, bool colorControlled, bool symbolControlled, bool otherControlled )
{
    auto saved = findSavedAppearance( curve );
    if ( !saved )
    {
        if ( !colorControlled && !symbolControlled && !otherControlled ) return;

        saved = new RimSavedCurveAppearance;
        saved->setCurve( curve );
        m_savedAppearances.push_back( saved );
        m_savedLookup[curve] = saved;
    }

    saved->update( colorControlled, symbolControlled, otherControlled );
    if ( !saved->isControlled() ) removeSavedAppearance( curve, saved );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::updateSavedAppearance( RimEnsembleCurveSet* curveSet, bool colorControlled )
{
    auto saved = findSavedAppearance( curveSet );
    if ( !saved )
    {
        if ( !colorControlled ) return;

        saved = new RimSavedCurveAppearance;
        saved->setEnsembleCurveSet( curveSet );
        m_savedAppearances.push_back( saved );
        m_savedLookup[curveSet] = saved;
    }

    saved->update( colorControlled, false, false );
    if ( !saved->isControlled() ) removeSavedAppearance( curveSet, saved );
}

//--------------------------------------------------------------------------------------------------
/// The lookup can hold entries for deleted objects, which are not matched as the saved appearance is also checked
//--------------------------------------------------------------------------------------------------
RimSavedCurveAppearance* RimCurveStyleRules::findSavedAppearance( caf::PdmObject* object )
{
    const auto it = m_savedLookup.find( object );
    if ( it == m_savedLookup.end() ) return nullptr;

    if ( it->second->object() == object ) return it->second;

    removeSavedAppearance( object, it->second );
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::removeSavedAppearance( caf::PdmObject* object, RimSavedCurveAppearance* saved )
{
    m_savedLookup.erase( object );
    m_savedAppearances.removeChild( saved );
    delete saved;
}

//--------------------------------------------------------------------------------------------------
/// Restores the settings the global appearance controlled for the curves of a plot not using it
//--------------------------------------------------------------------------------------------------
void RimCurveStyleRules::releaseAppearances( RimSummaryPlot* plot )
{
    bool changed = false;

    for ( auto curve : plot->summaryCurves() )
    {
        if ( !findSavedAppearance( curve ) ) continue;

        updateSavedAppearance( curve, false, false, false );
        changed = true;
    }

    for ( auto curveSet : plot->ensembleCurveSetCollection()->curveSets() )
    {
        if ( !findSavedAppearance( curveSet ) ) continue;

        updateSavedAppearance( curveSet, false );
        changed = true;
    }

    if ( !changed ) return;

    plot->updateConnectedEditors();
    plot->loadDataAndUpdate();
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
        preset->uiOrdering( uiConfigName, *settingGroup );
    }

    auto ruleGroup = uiOrdering.addNewGroup( "Named Rules" );
    ruleGroup->addNewLabel(
        "A named rule distributes either color or symbol over a dimension. Rules are selected per plot and take precedence over "
        "the global setting." );
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
