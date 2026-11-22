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

#include "RiuPlotCurveSymbol.h"

#include "cafAppEnum.h"
#include "cafPdmChildArrayField.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"
#include "cafPdmProxyValueField.h"
#include "cafPdmPtrField.h"

#include "cvfColor3.h"

#include <QString>
#include <QStringList>

#include <map>
#include <optional>
#include <string>
#include <vector>

class RifEclipseSummaryAddress;
class RimColorLegend;
class RimCurveStyleRule;
class RimSummaryCase;

//==================================================================================================
/// A named rule defining how either the curve color or the curve symbol is distributed over the
/// items of a data dimension, like the cases, the wells or the vectors. The colors are taken from a
/// color palette, and the symbols from the symbol sequence. Single items can be highlighted.
/// Summary multi plots select the named rules to use. Named rules take precedence over the global
/// setting of the plot, and later selected rules take precedence over earlier ones.
//==================================================================================================
class RimNamedCurveStyleRule : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    enum class Dimension
    {
        NONE,
        CASE,
        VECTOR,
        WELL,
        GROUP,
        NETWORK,
        REGION,
        REGION_2_REGION,
        AQUIFER,
        LGR,
        BLOCK,
        CONNECTION,
        SEGMENT
    };
    using DimensionEnum = caf::AppEnum<Dimension>;

    enum class AppliesTo
    {
        COLOR,
        SYMBOL
    };

    // The rules assigning the color and the symbol of a curve
    struct Resolved
    {
        const RimNamedCurveStyleRule* color  = nullptr;
        const RimNamedCurveStyleRule* symbol = nullptr;
    };

    // A rule for a specific item to apply to a curve, with the properties it is allowed to change
    struct RuleApplication
    {
        const RimCurveStyleRule* rule        = nullptr;
        bool                     applyColor  = false;
        bool                     applySymbol = false;
    };

    // The appearance assigned to an item
    struct ItemAppearance
    {
        std::optional<cvf::Color3f>                        color;
        std::optional<RiuPlotCurveSymbol::PointSymbolEnum> symbol;
    };

    RimNamedCurveStyleRule();
    ~RimNamedCurveStyleRule() override;

    QString   name() const;
    Dimension dimension() const;
    bool      appliesToColor() const;
    bool      appliesToSymbol() const;

    void setName( const QString& name );
    void setDimension( Dimension dimension );
    void setAppliesTo( AppliesTo appliesTo );

    // The value of the dimension for the address, empty if the address has no value for the dimension
    static std::string dimensionValue( Dimension dimension, const RifEclipseSummaryAddress& address, const std::string& caseName );

    // The case name used by dimensionValue: ensemble name for ensemble members, otherwise the display name
    static std::string caseName( const RimSummaryCase* summaryCase );

    // Whether the address has a value for the dimension of the rule
    bool matches( const RifEclipseSummaryAddress& address, const std::string& caseName ) const;

    // The rules for specific items, only kept for items that are custom or highlighted
    std::vector<RimCurveStyleRule*> rules() const;
    RimCurveStyleRule*              findOrCreateRule( Dimension dimension, const QString& name );
    RimCurveStyleRule*              findRule( Dimension dimension, const QString& name ) const;

    // The rules for specific items matching the address, for the rules assigning the color and the symbol
    static std::vector<RuleApplication>
        rulesFor( const Resolved& resolved, const RifEclipseSummaryAddress& address, const std::string& caseName );

    // Color from the selected color palette, the default palette if none is selected
    cvf::Color3f paletteColor( int index ) const;
    void         setColorPalette( RimColorLegend* palette );

    // The color or symbol assigned to each item of the dimension, including the custom ones
    std::map<QString, ItemAppearance> itemAppearances() const;

protected:
    QList<caf::PdmOptionItemInfo> calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions ) override;
    void                          defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering ) override;
    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override;

private:
    void assignedAppearance( const QString&                           name,
                             int                                      index,
                             const std::map<QString, ItemAppearance>& plotAppearances,
                             ItemAppearance&                          appearance ) const;

    // The names of the highlighted items, as edited with checkboxes
    std::vector<QString> highlightedItems() const;
    void                 setHighlightedItems( const std::vector<QString>& names );

    caf::PdmField<QString>                 m_name;
    caf::PdmField<caf::AppEnum<AppliesTo>> m_appliesTo;
    caf::PdmField<DimensionEnum>           m_dimension;

    caf::PdmChildArrayField<RimCurveStyleRule*> m_rules;

    caf::PdmPtrField<RimColorLegend*> m_autoColorLegend;

    caf::PdmField<int>                            m_symbolLegend;
    caf::PdmProxyValueField<std::vector<QString>> m_highlightedItems;
};
