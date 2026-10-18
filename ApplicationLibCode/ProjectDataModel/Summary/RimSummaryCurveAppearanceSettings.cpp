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

#include "RimSummaryCurveAppearanceSettings.h"

#include "RiaApplication.h"

#include "RifEclipseSummaryAddress.h"

#include "RimProject.h"
#include "RimSummaryCase.h"
#include "RimSummaryCurveAppearanceOverrideCollection.h"
#include "RimSummaryEnsemble.h"

namespace caf
{
template <>
void caf::AppEnum<RimSummaryCurveAppearanceSettings::Dimension>::setUp()
{
    using D = RimSummaryCurveAppearanceSettings::Dimension;
    addItem( D::NONE, "NONE", "None" );
    addItem( D::CASE, "CASE", "Summary Case" );
    addItem( D::VECTOR, "VECTOR", "Vector" );
    addItem( D::WELL, "WELL", "Well" );
    addItem( D::GROUP, "GROUP", "Group" );
    addItem( D::NETWORK, "NETWORK", "Network" );
    addItem( D::REGION, "REGION", "Region" );
    addItem( D::REGION_2_REGION, "REGION_2_REGION", "Region-Region" );
    addItem( D::AQUIFER, "AQUIFER", "Aquifer" );
    addItem( D::LGR, "LGR", "LGR" );
    addItem( D::BLOCK, "BLOCK", "Block" );
    addItem( D::CONNECTION, "CONNECTION", "Connection" );
    addItem( D::SEGMENT, "SEGMENT", "Segment" );
    setDefault( D::NONE );
}
} // namespace caf

CAF_PDM_SOURCE_INIT( RimSummaryCurveAppearanceSettings, "RimSummaryCurveAppearanceSettings" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSummaryCurveAppearanceSettings::RimSummaryCurveAppearanceSettings()
{
    CAF_PDM_InitObject( "Global Curve Appearance" );

    CAF_PDM_InitField( &m_colorDimension, "ColorDimension", DimensionEnum( Dimension::CASE ), "Color", "", "Data dimension controlling the curve color" );
    CAF_PDM_InitField( &m_symbolDimension,
                       "SymbolDimension",
                       DimensionEnum( Dimension::NONE ),
                       "Symbol",
                       "",
                       "Data dimension controlling the curve symbol" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSummaryCurveAppearanceSettings::Dimension RimSummaryCurveAppearanceSettings::colorDimension() const
{
    return m_colorDimension();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSummaryCurveAppearanceSettings::Dimension RimSummaryCurveAppearanceSettings::symbolDimension() const
{
    return m_symbolDimension();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryCurveAppearanceSettings::setColorDimension( Dimension dimension )
{
    m_colorDimension = dimension;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryCurveAppearanceSettings::setSymbolDimension( Dimension dimension )
{
    m_symbolDimension = dimension;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::string RimSummaryCurveAppearanceSettings::caseName( const RimSummaryCase* summaryCase )
{
    if ( !summaryCase ) return {};
    if ( summaryCase->ensemble() ) return summaryCase->ensemble()->name().toStdString();
    return summaryCase->displayCaseName().toStdString();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::string RimSummaryCurveAppearanceSettings::dimensionValue( Dimension                       dimension,
                                                               const RifEclipseSummaryAddress& address,
                                                               const std::string&              caseName )
{
    using Category = RifEclipseSummaryAddressDefines::SummaryCategory;

    switch ( dimension )
    {
        case Dimension::CASE:
            return caseName;

        case Dimension::VECTOR:
        {
            std::string name = address.vectorName();
            if ( address.isHistoryVector() && !name.empty() ) name.pop_back();
            return name;
        }

        case Dimension::WELL:
            return address.wellName();

        case Dimension::GROUP:
            return address.groupName();

        case Dimension::NETWORK:
            return address.networkName();

        case Dimension::REGION:
            return address.category() == Category::SUMMARY_REGION ? std::to_string( address.regionNumber() ) : std::string();

        case Dimension::REGION_2_REGION:
            return address.category() == Category::SUMMARY_REGION_2_REGION
                       ? std::to_string( address.regionNumber() ) + "-" + std::to_string( address.regionNumber2() )
                       : std::string();

        case Dimension::AQUIFER:
            return address.category() == Category::SUMMARY_AQUIFER ? std::to_string( address.aquiferNumber() ) : std::string();

        case Dimension::LGR:
            return address.lgrName();

        case Dimension::BLOCK:
            return address.category() == Category::SUMMARY_BLOCK || address.category() == Category::SUMMARY_BLOCK_LGR
                       ? address.blockAsString()
                       : std::string();

        case Dimension::CONNECTION:
            return address.category() == Category::SUMMARY_WELL_COMPLETION || address.category() == Category::SUMMARY_WELL_CONNECTION ||
                           address.category() == Category::SUMMARY_WELL_CONNECTION_LGR
                       ? address.wellName() + ":" + address.connectionAsString()
                       : std::string();

        case Dimension::SEGMENT:
            return address.category() == Category::SUMMARY_WELL_SEGMENT ? address.wellName() + ":" + std::to_string( address.id() )
                                                                        : std::string();

        default:
            return std::string();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryCurveAppearanceSettings::fieldChangedByUi( const caf::PdmFieldHandle* changedField,
                                                          const QVariant&            oldValue,
                                                          const QVariant&            newValue )
{
    // Edits made in the dock window must take effect immediately and be persisted
    if ( auto app = RiaApplication::instance() ) app->applyPreferences();

    // The collection list shows colors that depend on the global appearance
    if ( auto project = RimProject::current() )
    {
        if ( auto collection = project->curveAppearanceOverrides() ) collection->updateConnectedEditors();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryCurveAppearanceSettings::appendFieldsToUiOrdering( caf::PdmUiOrdering& uiOrdering )
{
    uiOrdering.add( &m_colorDimension );
    uiOrdering.add( &m_symbolDimension );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryCurveAppearanceSettings::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    appendFieldsToUiOrdering( uiOrdering );
    uiOrdering.skipRemainingFields();
}
