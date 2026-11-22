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
#pragma once

#include "cafPdmChildField.h"
#include "cafPdmField.h"
#include "cafPdmFieldCvfColor.h"
#include "cafPdmObject.h"
#include "cafPdmPtrField.h"

class RimEnsembleCurveSet;
class RimPlotCurve;
class RimPlotCurveAppearance;

//==================================================================================================
/// The color and symbol settings of a curve, or the color of an ensemble curve set, from before the global curve appearance
/// took control of them. Curves are referenced by PDM pointers so deleted objects are not matched.
//==================================================================================================
class RimSavedCurveAppearance : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    RimSavedCurveAppearance();

    void setCurve( RimPlotCurve* curve );
    void setEnsembleCurveSet( RimEnsembleCurveSet* curveSet );

    // The saved curve or curve set, nullptr if it no longer exists
    caf::PdmObject* object() const;

    // Updates the saved settings and restores the curve to match which settings the global appearance controls
    void update( bool colorControlled, bool symbolControlled );

    // True if the global appearance controls the color or symbol
    bool isControlled() const;

private:
    caf::PdmPtrField<RimPlotCurve*>             m_curve;
    caf::PdmChildField<RimPlotCurveAppearance*> m_appearance;

    caf::PdmPtrField<RimEnsembleCurveSet*> m_ensembleCurveSet;
    caf::PdmField<cvf::Color3f>            m_ensembleColor;

    bool m_colorControlled  = false;
    bool m_symbolControlled = false;
};
