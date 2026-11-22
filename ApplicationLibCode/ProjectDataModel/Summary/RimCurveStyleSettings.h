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
#include "cafPdmChildField.h"
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
/// A named global curve setting. It defines which data dimension controls the curve color and the
/// curve symbol, the color palette, the line style, the symbol size and the style of individual items.
/// Summary multi plots select which setting to use, and named rules can apply it to other dimensions.
//==================================================================================================
class RimCurveStyleSettings : public caf::PdmObject
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

    enum class AppearanceType
    {
        COLOR,
        SYMBOL
    };

    // The setting and dimension assigning a property of a curve
    struct Source
    {
        const RimCurveStyleSettings* settings  = nullptr;
        Dimension                    dimension = Dimension::NONE;

        bool isSet() const { return settings != nullptr && dimension != Dimension::NONE; }
    };

    // Where the color and symbol of a curve are assigned from
    struct Resolved
    {
        Source color;
        Source symbol;
    };

    // A rule to apply to a curve, with the properties it is allowed to change
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

    RimCurveStyleSettings();
    ~RimCurveStyleSettings() override;

    QString name() const;
    void    setName( const QString& name );

    Dimension colorDimension() const;
    Dimension symbolDimension() const;
    void      setColorDimension( Dimension dimension );
    void      setSymbolDimension( Dimension dimension );

    // The value of the dimension for the address, empty if the address has no value for the dimension
    static std::string dimensionValue( Dimension dimension, const RifEclipseSummaryAddress& address, const std::string& caseName );

    // The case name used by dimensionValue: ensemble name for ensemble members, otherwise the display name
    static std::string caseName( const RimSummaryCase* summaryCase );

    // The rules for specific items, only kept for items that are custom or highlighted
    std::vector<RimCurveStyleRule*> rules() const;
    RimCurveStyleRule*              findOrCreateRule( Dimension dimension, const QString& name );
    RimCurveStyleRule*              findRule( Dimension dimension, const QString& name ) const;

    // The setting as the source of both color and symbol, using its own dimensions
    Resolved resolved() const;

    // Base styles and item rules for the sources, ordered so that later entries win: base styles first, then item rules by dimension
    static std::vector<RuleApplication>
        rulesFor( const Resolved& resolved, const RifEclipseSummaryAddress& address, const std::string& caseName );

    // Color from the selected color palette, if one is selected
    std::optional<cvf::Color3f> autoLegendColor( int index ) const;

    // The color and symbol assigned to each item of the dimension, including the custom ones
    std::map<QString, ItemAppearance> itemAppearances( Dimension dimension ) const;

    int  symbolSize() const;
    void setSymbolSize( const int& size );

    // Fields editing the setting, shown in the curve appearance dock window
    void appendFieldsToUiOrdering( caf::PdmUiOrdering& uiOrdering );

protected:
    QList<caf::PdmOptionItemInfo> calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions ) override;
    void                          defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering ) override;
    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override;
    void initAfterRead() override;

private:
    void setUpBaseStyles();
    void assignedAppearance( Dimension                                dimension,
                             const QString&                           name,
                             int                                      index,
                             const std::map<QString, ItemAppearance>& plotAppearances,
                             ItemAppearance&                          appearance ) const;

    // The names of the highlighted items for the current dimension, as edited with checkboxes
    std::vector<QString> highlightedItems() const;
    void                 setHighlightedItems( const std::vector<QString>& names );

    // The dimension controlling the selected appearance type
    DimensionEnum currentDimension() const;
    void          setCurrentDimension( const DimensionEnum& dimension );

    caf::PdmField<QString>       m_name;
    caf::PdmField<DimensionEnum> m_colorDimension;
    caf::PdmField<DimensionEnum> m_symbolDimension;

    caf::PdmChildArrayField<RimCurveStyleRule*> m_rules;

    // The style shared by all items controlled by color, and the symbol size shared by all items controlled by symbol
    caf::PdmChildField<RimCurveStyleRule*> m_colorBaseStyle;
    caf::PdmChildField<RimCurveStyleRule*> m_symbolBaseStyle;

    caf::PdmPtrField<RimColorLegend*> m_autoColorLegend;

    caf::PdmField<caf::AppEnum<AppearanceType>>   m_appearanceType;
    caf::PdmProxyValueField<DimensionEnum>        m_currentType;
    caf::PdmField<int>                            m_symbolLegend;
    caf::PdmProxyValueField<std::vector<QString>> m_highlightedItems;
    caf::PdmProxyValueField<int>                  m_symbolSize;
};
