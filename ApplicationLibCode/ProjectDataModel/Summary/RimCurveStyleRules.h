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

#include "RimCurveStyleRule.h"
#include "RimCurveStyleSettings.h"
#include "RimNamedCurveStyleRule.h"
#include "RimSavedCurveAppearance.h"

#include "cafPdmChildArrayField.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"
#include "cafPdmPtrField.h"
#include "cafPdmUiTreeAttributes.h"

#include "cvfColor3.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

class RimEnsembleCurveSet;
class RimPlotCurve;
class RimSummaryPlot;
class RifEclipseSummaryAddress;

//==================================================================================================
/// Project-stored curve appearance: the global curve settings, which can be several, and the named
/// rules applying a global setting to a data dimension. Summary multi plots select which global
/// setting and named rules to use.
//==================================================================================================
class RimCurveStyleRules : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    RimCurveStyleRules();

    // There is always at least one global setting
    std::vector<RimCurveStyleSettings*> presets() const;
    RimCurveStyleSettings*              defaultSettings() const;
    RimCurveStyleSettings*              addPreset( const QString& name );
    RimCurveStyleSettings*              duplicatePreset( const RimCurveStyleSettings* settings );
    void                                deletePreset( RimCurveStyleSettings* settings );

    std::vector<RimNamedCurveStyleRule*> namedRules() const;
    RimNamedCurveStyleRule*              addNamedRule( const QString& name );
    void                                 deleteNamedRule( RimNamedCurveStyleRule* rule );

    // Where the color and symbol of a curve are assigned from. The named rules are expected in increasing precedence.
    // A property is only assigned if the source has a value for the curve
    static RimCurveStyleSettings::Resolved resolve( const RimCurveStyleSettings*                      globalSettings,
                                                    const std::vector<const RimNamedCurveStyleRule*>& namedRules,
                                                    const RifEclipseSummaryAddress&                   address,
                                                    const std::string&                                caseName );

    // Re-applies the global appearance to all plots using it
    void rulesChanged();

    // Stores custom settings edited for the selected item, and re-applies the global appearance
    void ruleChanged( RimCurveStyleRule* rule );

    // Index used to pick the palette color and symbol for a case or well name. Indices are kept in the project so that
    // existing items keep their appearance when new cases or wells are added.
    // Names not yet registered get the indices they would get if registered now.
    void                   registerItemNames( RimCurveStyleRule::Dimension dimension, const QStringList& names );
    std::map<QString, int> itemIndices( RimCurveStyleRule::Dimension dimension ) const;

    // Configures a field as a label with a link that shows the global curve appearance
    static void addGlobalAppearanceLink( caf::PdmUiOrdering& uiOrdering, const QString& text );

    // Tags for the data source tree showing the global color and symbol assigned to an item
    std::vector<caf::PdmUiTreeViewItemTag> curveStyleRuleTags( RimCurveStyleRule::Dimension dimension, const QString& name ) const;
    void                                   applyGlobalAppearance( const std::vector<RimSummaryPlot*>& plots );

protected:
    QList<caf::PdmOptionItemInfo> calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions ) override;
    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override;
    void defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering ) override;
    void initAfterRead() override;

private:
    void connectAllPlots( bool connect );
    void updateSummaryCaseTags();
    void ensureDefaultPreset();
    void ensureSelection();

    caf::PdmField<std::vector<QString>>*       itemOrder( RimCurveStyleRule::Dimension dimension );
    const caf::PdmField<std::vector<QString>>* itemOrder( RimCurveStyleRule::Dimension dimension ) const;

    void saveAppearance( RimSummaryPlot* plot );
    void restoreAppearances( const std::vector<RimSummaryPlot*>& plots, bool applyDefaultsToUnsaved );

    // The curve appearance before the global appearance was enabled, restored when disabled
    caf::PdmChildArrayField<RimSavedCurveAppearance*> m_savedAppearances;

    caf::PdmChildArrayField<RimCurveStyleSettings*>  m_presets;
    caf::PdmChildArrayField<RimNamedCurveStyleRule*> m_namedRules;
    caf::PdmField<std::vector<QString>>              m_caseOrder;
    caf::PdmField<std::vector<QString>>              m_wellOrder;

    // The setting and named rule currently edited
    caf::PdmPtrField<RimCurveStyleSettings*>  m_selectedPreset;
    caf::PdmPtrField<RimNamedCurveStyleRule*> m_selectedNamedRule;

    // Appearances shown as tags in the data source tree, built on demand and cleared when the rules change
    mutable std::map<std::pair<const RimCurveStyleSettings*, RimCurveStyleRule::Dimension>, std::map<QString, RimCurveStyleSettings::ItemAppearance>> m_tagCache;
};
