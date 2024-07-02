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

#include "RimSummarySumoDataSource.h"

#include "Cloud/RiaSumoConnector.h"
#include "cafCmdFeatureMenuBuilder.h"

CAF_PDM_SOURCE_INIT( RimSummarySumoDataSource, "RimSummarySumoDataSource" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSummarySumoDataSource::RimSummarySumoDataSource()
{
    CAF_PDM_InitObject( "Sumo Data Source", ":/SummaryCase.svg" );

    CAF_PDM_InitFieldNoDefault( &m_caseId, "CaseId", "Case Id" );
    CAF_PDM_InitFieldNoDefault( &m_caseName, "CaseName", "Case Name" );
    CAF_PDM_InitFieldNoDefault( &m_ensembleName, "EnsembleName", "Ensemble Name" );
    CAF_PDM_InitFieldNoDefault( &m_customName, "CustomName", "Custom Name" );

    setDeletable( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
SumoCaseId RimSummarySumoDataSource::caseId() const
{
    return SumoCaseId( m_caseId() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummarySumoDataSource::setCaseId( const SumoCaseId& caseId )
{
    m_caseId = caseId.get();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimSummarySumoDataSource::caseName() const
{
    return m_caseName();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummarySumoDataSource::setCaseName( const QString& caseName )
{
    m_caseName = caseName;

    updateName();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimSummarySumoDataSource::ensembleName() const
{
    return m_ensembleName();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummarySumoDataSource::setEnsembleName( const QString& ensembleName )
{
    m_ensembleName = ensembleName;

    updateName();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummarySumoDataSource::updateName()
{
    if ( !m_customName().isEmpty() )
    {
        setName( m_customName() );
        return;
    }

    auto name = QString( "%1 (%2)" ).arg( ensembleName(), caseName() );

    setName( name );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummarySumoDataSource::appendMenuItems( caf::CmdFeatureMenuBuilder& menuBuilder ) const
{
    menuBuilder.addCmdFeature( "RicCreateSumoEnsembleFeature" );
}
