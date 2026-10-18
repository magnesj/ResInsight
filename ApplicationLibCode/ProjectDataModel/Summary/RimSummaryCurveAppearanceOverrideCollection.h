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

#include "cafPdmChildArrayField.h"
#include "cafPdmChildField.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"
#include "cafPdmPtrField.h"

#include "cvfColor3.h"

#include <optional>
#include <string>

#include "RimSummaryCurveAppearanceOverride.h"

class RimColorLegend;
class RifEclipseSummaryAddress;

//==================================================================================================
/// Project-stored list of per-item color overrides for the global curve appearance.
//==================================================================================================
class RimSummaryCurveAppearanceOverrideCollection : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    RimSummaryCurveAppearanceOverrideCollection();

    std::vector<RimSummaryCurveAppearanceOverride*> overrides() const;

    // Overrides matching the address, ordered so that later entries win: case first, then other dimensions
    std::vector<const RimSummaryCurveAppearanceOverride*> overridesFor( const RifEclipseSummaryAddress& address,
                                                                        const std::string&              caseName ) const;

    RimSummaryCurveAppearanceSettings* settings() const;

    // Re-applies the global appearance to all plots using it
    void overridesChanged();

    // Color from the project color legend used for auto colors, if one is selected
    std::optional<cvf::Color3f> autoLegendColor( int index ) const;

protected:
    QList<caf::PdmOptionItemInfo> calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions ) override;
    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override;
    void defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering ) override;

public:
    enum class AppearanceType
    {
        COLOR,
        SYMBOL
    };

private:
    void                               assignedAppearance( const QString&                                      name,
                                                           int                                                 index,
                                                           std::optional<cvf::Color3f>&                        color,
                                                           std::optional<RiuPlotCurveSymbol::PointSymbolEnum>& symbol ) const;
    RimSummaryCurveAppearanceOverride* findOverride( RimSummaryCurveAppearanceOverride::Dimension dimension, const QString& name ) const;
    QString                            currentItemName() const;

    // The dimension controlling the selected appearance type, as stored in the global settings
    RimSummaryCurveAppearanceOverride::Dimension currentDimension() const;

    caf::PdmChildField<RimSummaryCurveAppearanceSettings*>                    m_settings;
    caf::PdmChildArrayField<RimSummaryCurveAppearanceOverride*>               m_overrides;
    caf::PdmField<caf::AppEnum<AppearanceType>>                               m_appearanceType;
    caf::PdmField<caf::AppEnum<RimSummaryCurveAppearanceOverride::Dimension>> m_currentType;
    caf::PdmPtrField<RimColorLegend*>                                         m_autoColorLegend;
    caf::PdmField<int>                                                        m_symbolLegend;
    caf::PdmField<int>                                                        m_currentItemIndex;
};
