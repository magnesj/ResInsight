/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2024     Equinor ASA
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

#include "RimSummaryCaseSumo.h"

#include "cafPdmObjectScriptingCapability.h"

CAF_PDM_SOURCE_INIT( RimSummaryCaseSumo, "RimSummaryCaseSumo" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSummaryCaseSumo::RimSummaryCaseSumo()
{
    CAF_PDM_InitScriptableObject( "Sumo Realization", ":/SummaryCase.svg" );

    CAF_PDM_InitFieldNoDefault( &m_realizationName, "RealizationName", "Realization Name" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryCaseSumo::createSummaryReaderInterface()
{
    // Nothing to do here
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RifSummaryReaderInterface* RimSummaryCaseSumo::summaryReader()
{
    return this;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<time_t> RimSummaryCaseSumo::timeSteps( const RifEclipseSummaryAddress& resultAddress ) const
{
    return {};
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::pair<bool, std::vector<double>> RimSummaryCaseSumo::values( const RifEclipseSummaryAddress& resultAddress ) const
{
    return {};
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::string RimSummaryCaseSumo::unitName( const RifEclipseSummaryAddress& resultAddress ) const
{
    return {};
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaDefines::EclipseUnitSystem RimSummaryCaseSumo::unitSystem() const
{
    return RiaDefines::EclipseUnitSystem::UNITS_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryCaseSumo::setEnsemble( RimSummaryEnsembleSumo* ensemble )
{
    m_ensemble = ensemble;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimSummaryCaseSumo::caseName() const
{
    return {};
}
