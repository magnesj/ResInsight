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

#include "RimSavedCurveAppearance.h"

#include "RimEnsembleCurveSet.h"
#include "RimPlotCurve.h"
#include "RimPlotCurveAppearance.h"

CAF_PDM_SOURCE_INIT( RimSavedCurveAppearance, "RimSavedCurveAppearance" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSavedCurveAppearance::RimSavedCurveAppearance()
{
    CAF_PDM_InitObject( "Saved Curve Appearance" );

    CAF_PDM_InitFieldNoDefault( &m_curve, "Curve", "Curve" );
    CAF_PDM_InitFieldNoDefault( &m_appearance, "CurveAppearance", "Curve Appearance" );
    CAF_PDM_InitFieldNoDefault( &m_ensembleCurveSet, "EnsembleCurveSet", "Ensemble Curve Set" );
    CAF_PDM_InitFieldNoDefault( &m_ensembleColor, "EnsembleColor", "Ensemble Color" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSavedCurveAppearance::setCurve( RimPlotCurve* curve )
{
    m_curve      = curve;
    m_appearance = curve->curveAppearance()->copyObject<RimPlotCurveAppearance>();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSavedCurveAppearance::setEnsembleCurveSet( RimEnsembleCurveSet* curveSet )
{
    m_ensembleCurveSet = curveSet;
    m_ensembleColor    = curveSet->color();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
caf::PdmObject* RimSavedCurveAppearance::object() const
{
    if ( m_curve() ) return m_curve();
    return m_ensembleCurveSet();
}

//--------------------------------------------------------------------------------------------------
/// The saved settings of a group are the settings before the global appearance took control of it. They are stored when
/// the group becomes controlled, so edits made while it was not controlled are kept. Restoring also when the group is
/// still controlled removes values left by rules that no longer apply.
//--------------------------------------------------------------------------------------------------
void RimSavedCurveAppearance::update( bool colorControlled, bool symbolControlled )
{
    if ( m_curve() && m_appearance() )
    {
        auto* current = m_curve()->curveAppearance();

        if ( colorControlled && !m_colorControlled )
            m_appearance()->copyColorSettingsFrom( *current );
        else if ( m_colorControlled )
            current->copyColorSettingsFrom( *m_appearance() );

        if ( symbolControlled && !m_symbolControlled )
            m_appearance()->copySymbolSettingsFrom( *current );
        else if ( m_symbolControlled )
            current->copySymbolSettingsFrom( *m_appearance() );
    }

    if ( m_ensembleCurveSet() )
    {
        if ( colorControlled && !m_colorControlled )
            m_ensembleColor = m_ensembleCurveSet()->color();
        else if ( m_colorControlled )
            m_ensembleCurveSet()->setColorAndUpdateCurves( m_ensembleColor() );
    }

    m_colorControlled  = colorControlled;
    m_symbolControlled = symbolControlled;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimSavedCurveAppearance::isControlled() const
{
    return m_colorControlled || m_symbolControlled;
}
