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

#include <map>
#include <memory>
#include <optional>
#include <string>

#include "RimCurveStyleRule.h"
#include "RimPlotCurveAppearance.h"

class RimColorLegend;
class RimEnsembleCurveSet;
class RimPlotCurve;
class RimSummaryPlot;
class RifEclipseSummaryAddress;

//==================================================================================================
/// Project-stored list of per-item color overrides for the global curve appearance.
//==================================================================================================
class RimCurveStyleRules : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    RimCurveStyleRules();

    std::vector<RimCurveStyleRule*> overrides() const;

    // Overrides matching the address, ordered so that later entries win: case first, then other dimensions
    std::vector<const RimCurveStyleRule*> overridesFor( const RifEclipseSummaryAddress& address, const std::string& caseName ) const;

    RimCurveStyleSettings* settings() const;

    // Re-applies the global appearance to all plots using it
    void overridesChanged();

    // Color from the project color legend used for auto colors, if one is selected
    std::optional<cvf::Color3f> autoLegendColor( int index ) const;

    // Configures a field as a label with a link that shows the global curve appearance
    static void configureGlobalAppearanceLink( caf::PdmFieldHandle* field, const QString& text );

    // The global appearance applies to all summary plots in the project
    bool useGlobalAppearance() const;
    void setUseGlobalAppearance( bool enable );

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
    void               assignedAppearance( const QString&                                      name,
                                           int                                                 index,
                                           std::optional<cvf::Color3f>&                        color,
                                           std::optional<RiuPlotCurveSymbol::PointSymbolEnum>& symbol ) const;
    RimCurveStyleRule* findOverride( RimCurveStyleRule::Dimension dimension, const QString& name ) const;
    QString            currentItemName() const;

    // The curve appearance before the global appearance was enabled, kept in memory so it can be restored when disabled
    struct SavedAppearance
    {
        std::map<const RimPlotCurve*, std::unique_ptr<RimPlotCurveAppearance>> curves;
        std::map<const RimEnsembleCurveSet*, cvf::Color3f>                     ensembleColors;
    };

    void applyGlobalAppearanceMode();
    void saveAppearance( RimSummaryPlot* plot );
    bool restoreAppearance( RimSummaryPlot* plot );

    std::map<const RimSummaryPlot*, SavedAppearance> m_savedAppearances;
    caf::PdmField<bool>                              m_useGlobalAppearance;

    // The dimension controlling the selected appearance type, as stored in the global settings
    RimCurveStyleRule::Dimension currentDimension() const;

    caf::PdmChildField<RimCurveStyleSettings*>                m_settings;
    caf::PdmChildArrayField<RimCurveStyleRule*>               m_overrides;
    caf::PdmField<caf::AppEnum<AppearanceType>>               m_appearanceType;
    caf::PdmField<caf::AppEnum<RimCurveStyleRule::Dimension>> m_currentType;
    caf::PdmPtrField<RimColorLegend*>                         m_autoColorLegend;
    caf::PdmField<int>                                        m_symbolLegend;
    caf::PdmField<int>                                        m_currentItemIndex;
};
