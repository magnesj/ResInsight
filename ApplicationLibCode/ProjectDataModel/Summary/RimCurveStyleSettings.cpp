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

#include "RimCurveStyleRules.h"
#include "RimPlotCurveAppearance.h"

CAF_PDM_SOURCE_INIT( RimCurveStyleSettings, "RimCurveStyleSettings" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimCurveStyleSettings::RimCurveStyleSettings()
{
    CAF_PDM_InitObject( "Global Curve Setting" );

    CAF_PDM_InitField( &m_name, "Name", QString( "Appearance" ), "Name" );

    CAF_PDM_InitFieldNoDefault( &m_curveAppearance, "CurveAppearance", "Curve Appearance" );
    m_curveAppearance = new RimPlotCurveAppearance;
    m_curveAppearance.uiCapability()->setUiTreeChildrenHidden( true );
    m_curveAppearance->appearanceChanged.connect( this, &RimCurveStyleSettings::onAppearanceChanged );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimCurveStyleSettings::~RimCurveStyleSettings() = default;

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
RimPlotCurveAppearance* RimCurveStyleSettings::curveAppearance() const
{
    return m_curveAppearance();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleSettings::copyAppearanceFrom( const RimCurveStyleSettings& other )
{
    m_curveAppearance->copyFrom( *other.m_curveAppearance() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleSettings::initAfterRead()
{
    if ( m_curveAppearance() ) m_curveAppearance->appearanceChanged.connect( this, &RimCurveStyleSettings::onAppearanceChanged );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleSettings::onAppearanceChanged( const caf::SignalEmitter* emitter )
{
    if ( auto collection = firstAncestorOrThisOfType<RimCurveStyleRules>() ) collection->rulesChanged();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleSettings::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    uiOrdering.add( &m_name );

    auto appearanceGroup = uiOrdering.addNewGroup( "Curve Appearance" );
    m_curveAppearance->uiOrdering( "AppearanceOrdering", *appearanceGroup );

    uiOrdering.skipRemainingFields();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCurveStyleSettings::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    // The name only changes what is shown
    if ( changedField == &m_name )
    {
        if ( auto collection = firstAncestorOrThisOfType<RimCurveStyleRules>() ) collection->updateConnectedEditors();
    }
}
