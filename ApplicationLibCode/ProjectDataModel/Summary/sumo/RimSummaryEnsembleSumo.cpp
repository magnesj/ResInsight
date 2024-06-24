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

#include "RimSummaryEnsembleSumo.h"
#include "RimSummaryCaseSumo.h"

CAF_PDM_SOURCE_INIT( RimSummaryEnsembleSumo, "RimSummaryEnsembleSumo" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSummaryEnsembleSumo::RimSummaryEnsembleSumo()
{
    CAF_PDM_InitObject( "Sumo Ensemble", ":/SummaryCase.svg", "", "The Base Class for all Summary Cases" );

    CAF_PDM_InitFieldNoDefault( &m_sumoFieldId, "SumoFieldId", "Field Id" );
    CAF_PDM_InitFieldNoDefault( &m_sumoCaseId, "SumoCaseId", "Case Id" );
    CAF_PDM_InitFieldNoDefault( &m_sumoEnsembleId, "SumoEnsembleId", "Ensemble Id" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<time_t> RimSummaryEnsembleSumo::timeSteps( const RifEclipseSummaryAddress& resultAddress ) const
{
    return {};
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::pair<bool, std::vector<double>> RimSummaryEnsembleSumo::values( const RifEclipseSummaryAddress& resultAddress ) const
{
    return {};
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::string RimSummaryEnsembleSumo::unitName( const RifEclipseSummaryAddress& resultAddress ) const
{
    return {};
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaDefines::EclipseUnitSystem RimSummaryEnsembleSumo::unitSystem() const
{
    return RiaDefines::EclipseUnitSystem::UNITS_UNKNOWN;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimSummaryEnsembleSumo::loadSummaryData( const RifEclipseSummaryAddress& resultAddress )
{
    // create job to download data from sumo
    // download data, and notify when done

    return true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryEnsembleSumo::onLoadDataAndUpdate()
{
    // load summary data from sumo
    // create the realizations and add them to the ensemble

    m_cases.deleteChildren();

    size_t realizationCount = 10;
    for ( size_t i = 0; i < realizationCount; ++i )
    {
        auto realization = new RimSummaryCaseSumo();
        realization->setEnsemble( this );
        realization->setRealizationName( QString( "Realization %1" ).arg( i ) );
        
        m_cases.push_back( realization );
    }

    // call the base class method after data has been loaded
    RimSummaryCaseCollection::onLoadDataAndUpdate();
}
