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

    CAF_PDM_InitFieldNoDefault( &m_realizationIds, "RealizationIds", "Realizations Ids" );
    m_realizationIds.uiCapability()->setUiReadOnly( true );
    // m_realizationIds.uiCapability()->setUiEditorTypeName( caf::PdmUiListEditor::uiEditorTypeName() );

    CAF_PDM_InitFieldNoDefault( &m_vectorNames, "VectorNames", "Vector Names" );
    // m_vectorNames.uiCapability()->setUiEditorTypeName( caf::PdmUiListEditor::uiEditorTypeName() );

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
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<QString> RimSummarySumoDataSource::realizationIds() const
{
    return m_realizationIds();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummarySumoDataSource::setRealizationIds( const std::vector<QString>& realizationIds )
{
    m_realizationIds = realizationIds;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<QString> RimSummarySumoDataSource::vectorNames() const
{
    return m_vectorNames();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummarySumoDataSource::setVectorNames( const std::vector<QString>& vectorNames )
{
    m_vectorNames = vectorNames;
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

    auto name             = QString( "%1 (%2)" ).arg( ensembleName(), caseName() );
    auto realizationCount = realizationIds().size();
    if ( realizationCount > 0 )
    {
        name += QString( " - %1 realizations" ).arg( realizationCount );
    }

    setName( name );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummarySumoDataSource::appendMenuItems( caf::CmdFeatureMenuBuilder& menuBuilder ) const
{
    menuBuilder.addCmdFeature( "RicCreateSumoEnsembleFeature" );
}
