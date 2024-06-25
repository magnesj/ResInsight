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
#include "../../../Application/RiaApplication.h"
#include "RimSummaryCaseSumo.h"

CAF_PDM_SOURCE_INIT( RimSummaryEnsembleSumo, "RimSummaryEnsembleSumo" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSummaryEnsembleSumo::RimSummaryEnsembleSumo()
{
    CAF_PDM_InitObject( "Sumo Ensemble", ":/SummaryCase.svg", "", "The Base Class for all Summary Cases" );

    CAF_PDM_InitFieldNoDefault( &m_sumoFieldName, "SumoFieldId", "Field Id" );
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
void RimSummaryEnsembleSumo::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    uiOrdering.add( &m_sumoFieldName );
    uiOrdering.add( &m_sumoCaseId );
    uiOrdering.add( &m_sumoEnsembleId );

    RimSummaryCaseCollection::defineUiOrdering( uiConfigName, uiOrdering );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimSummaryEnsembleSumo::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
{
    createSumoConnector();

    QList<caf::PdmOptionItemInfo> options;
    if ( fieldNeedingOptions == &m_sumoFieldName )
    {
        if ( m_sumoConnector->assets().empty() )
        {
            m_sumoConnector->requestAssets();

            // wait in loop until assets is not empty
            while ( m_sumoConnector->assets().empty() )
            {
                qApp->processEvents();
            }
        }

        for ( const auto& asset : m_sumoConnector->assets() )
        {
            options.push_back( { asset.name, asset.name } );
        }
    }
    else if ( fieldNeedingOptions == &m_sumoCaseId && !m_sumoFieldName().isEmpty() )
    {
        if ( m_sumoConnector->cases().empty() )
        {
            m_sumoConnector->requestCasesForField( m_sumoFieldName );

            /*
                        while ( m_sumoConnector->cases().empty() )
                        {
                            qApp->processEvents();
                        }
            */
        }

        for ( const auto& sumoCase : m_sumoConnector->cases() )
        {
            options.push_back( { sumoCase.name, sumoCase.id } );
        }
    }
    else if ( fieldNeedingOptions == &m_sumoEnsembleId && !m_sumoCaseId().isEmpty() )
    {
        if ( m_sumoConnector->ensembleNamesForCase( m_sumoCaseId ).empty() )
        {
            m_sumoConnector->requestEnsembleByCasesId( m_sumoCaseId );

            /*
                        while ( m_sumoConnector->cases().empty() )
                        {
                            qApp->processEvents();
                        }
            */
        }

        for ( const auto& name : m_sumoConnector->ensembleNamesForCase( m_sumoCaseId ) )
        {
            options.push_back( { name, name } );
        }
    }

    return options;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryEnsembleSumo::createSumoConnector()
{
    if ( m_sumoConnector != nullptr ) return;

    m_sumoConnector = RiaApplication::instance()->makeSumoConnector();

    QSettings settings;
    auto      bearerToken = settings.value( m_registryKeyBearerToken_DEBUG_ONLY ).toString();

    if ( bearerToken.isEmpty() )
    {
        m_sumoConnector->requestToken();
    }
    else
    {
        m_sumoConnector->setToken( bearerToken );
    }
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
