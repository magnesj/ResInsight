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
#include "cafPdmObject.h"

#include <QString>

class RimPlotCurveAppearance;

//==================================================================================================
/// A named global curve setting defining a full curve appearance: color, line, symbol and fill.
/// Summary multi plots select which setting to use, and apply it to all their curves.
//==================================================================================================
class RimCurveStyleSettings : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    RimCurveStyleSettings();
    ~RimCurveStyleSettings() override;

    QString name() const;
    void    setName( const QString& name );

    RimPlotCurveAppearance* curveAppearance() const;

    void copyAppearanceFrom( const RimCurveStyleSettings& other );

protected:
    void defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering ) override;
    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override;
    void initAfterRead() override;

private:
    void onAppearanceChanged( const caf::SignalEmitter* emitter );

    caf::PdmField<QString>                      m_name;
    caf::PdmChildField<RimPlotCurveAppearance*> m_curveAppearance;
};
