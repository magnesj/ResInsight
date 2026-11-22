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

#include "RimCurveStyleRule.h"
#include "RimCurveStyleRules.h"

#include "cafPdmUiTreeSelectionEditor.h"

#include <algorithm>

namespace caf
{
template <>
void caf::AppEnum<RimNamedCurveStyleRule::AppliesTo>::setUp()
{
    using T = RimNamedCurveStyleRule::AppliesTo;
    addItem( T::COLOR, "COLOR", "Color" );
    addItem( T::SYMBOL, "SYMBOL", "Symbol" );
    addItem( T::COLOR_AND_SYMBOL, "COLOR_AND_SYMBOL", "Color and Symbol" );
    setDefault( T::COLOR );
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
    CAF_PDM_InitField( &m_dimension, "Dimension", caf::AppEnum<Dimension>( Dimension::WELL ), "Differentiate By" );
    CAF_PDM_InitFieldNoDefault( &m_settings, "Settings", "Global Setting" );
    CAF_PDM_InitField( &m_appliesTo, "AppliesTo", caf::AppEnum<AppliesTo>( AppliesTo::COLOR ), "Applies To" );

    CAF_PDM_InitFieldNoDefault( &m_items, "Items", "Items" );
    m_items.uiCapability()->setUiEditorTypeName( caf::PdmUiTreeSelectionEditor::uiEditorTypeName() );
    m_items.uiCapability()->setUiLabelPosition( caf::PdmUiItemInfo::LabelPosition::TOP );
    m_items.uiCapability()->setAttribute( caf::PdmUiTreeSelectionEditor::Keys::SHOW_TEXT_FILTER, false );
    m_items.uiCapability()->setAttribute( caf::PdmUiTreeSelectionEditor::Keys::HEIGHT_HINT, 200 );
}

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
RimCurveStyleSettings* RimNamedCurveStyleRule::settings() const
{
    return m_settings();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimNamedCurveStyleRule::appliesToColor() const
{
    return m_appliesTo() == AppliesTo::COLOR || m_appliesTo() == AppliesTo::COLOR_AND_SYMBOL;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimNamedCurveStyleRule::appliesToSymbol() const
{
    return m_appliesTo() == AppliesTo::SYMBOL || m_appliesTo() == AppliesTo::COLOR_AND_SYMBOL;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<QString> RimNamedCurveStyleRule::items() const
{
    return m_items();
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
void RimNamedCurveStyleRule::setSettings( RimCurveStyleSettings* settings )
{
    m_settings = settings;
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
void RimNamedCurveStyleRule::setItems( const std::vector<QString>& items )
{
    m_items = items;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimNamedCurveStyleRule::matches( const RifEclipseSummaryAddress& address, const std::string& caseName ) const
{
    if ( !m_settings() || m_dimension() == Dimension::NONE ) return false;

    const auto value = RimCurveStyleSettings::dimensionValue( m_dimension(), address, caseName );
    if ( value.empty() ) return false;

    const auto& selected = m_items();
    return selected.empty() || std::find( selected.begin(), selected.end(), QString::fromStdString( value ) ) != selected.end();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimNamedCurveStyleRule::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
{
    QList<caf::PdmOptionItemInfo> options;

    if ( fieldNeedingOptions == &m_settings )
    {
        if ( auto collection = firstAncestorOrThisOfType<RimCurveStyleRules>() )
        {
            for ( auto settings : collection->presets() )
            {
                options.push_back( caf::PdmOptionItemInfo( settings->name(), settings ) );
            }
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
    else if ( fieldNeedingOptions == &m_items )
    {
        for ( const auto& name : RimCurveStyleRule::availableNames( m_dimension() ) )
        {
            options.push_back( caf::PdmOptionItemInfo( name, name, false, RimCurveStyleRule::dimensionIcon( m_dimension() ) ) );
        }
    }

    return options;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimNamedCurveStyleRule::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    uiOrdering.add( &m_name );
    uiOrdering.add( &m_settings );
    uiOrdering.add( &m_dimension );
    uiOrdering.add( &m_appliesTo );

    auto itemsGroup = uiOrdering.addNewGroup( "Items" );
    itemsGroup->add( &m_items );
    itemsGroup->addNewLabel( "The rule applies to all items when no items are checked." );

    uiOrdering.skipRemainingFields();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimNamedCurveStyleRule::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    // The available items depend on the dimension
    if ( changedField == &m_dimension ) m_items = std::vector<QString>();

    auto collection = firstAncestorOrThisOfType<RimCurveStyleRules>();
    if ( !collection ) return;

    if ( changedField != &m_name ) collection->rulesChanged();
    collection->updateConnectedEditors();
}
