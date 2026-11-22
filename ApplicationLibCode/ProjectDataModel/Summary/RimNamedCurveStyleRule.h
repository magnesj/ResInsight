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

#include "RimCurveStyleSettings.h"

#include "cafAppEnum.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"
#include "cafPdmPtrField.h"

#include <string>
#include <vector>

class RifEclipseSummaryAddress;

//==================================================================================================
/// A named rule combining a global curve setting with a data dimension. Curves with a value for
/// the dimension (optionally limited to selected items) get their color and/or symbol assigned
/// from the dimension using the palette, line style and symbol size of the setting.
/// Summary multi plots select the named rules to use. Named rules take precedence over the global
/// setting of the plot, and later selected rules take precedence over earlier ones.
//==================================================================================================
class RimNamedCurveStyleRule : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    using Dimension = RimCurveStyleSettings::Dimension;

    enum class AppliesTo
    {
        COLOR,
        SYMBOL,
        COLOR_AND_SYMBOL
    };

    RimNamedCurveStyleRule();

    QString                name() const;
    Dimension              dimension() const;
    RimCurveStyleSettings* settings() const;
    bool                   appliesToColor() const;
    bool                   appliesToSymbol() const;
    std::vector<QString>   items() const;

    void setName( const QString& name );
    void setDimension( Dimension dimension );
    void setSettings( RimCurveStyleSettings* settings );
    void setAppliesTo( AppliesTo appliesTo );
    void setItems( const std::vector<QString>& items );

    // Whether the address has a value for the dimension, and the value is one of the selected items if any are selected
    bool matches( const RifEclipseSummaryAddress& address, const std::string& caseName ) const;

protected:
    QList<caf::PdmOptionItemInfo> calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions ) override;
    void                          defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering ) override;
    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override;

private:
    caf::PdmField<QString>                   m_name;
    caf::PdmField<caf::AppEnum<Dimension>>   m_dimension;
    caf::PdmPtrField<RimCurveStyleSettings*> m_settings;
    caf::PdmField<caf::AppEnum<AppliesTo>>   m_appliesTo;
    caf::PdmField<std::vector<QString>>      m_items;
};
