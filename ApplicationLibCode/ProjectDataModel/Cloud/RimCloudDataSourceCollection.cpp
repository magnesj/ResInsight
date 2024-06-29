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

#include "RimCloudDataSourceCollection.h"

#include "RimOilField.h"
#include "RimProject.h"
#include "sumo/RimSummarySumoDataSource.h"

#include "cafPdmUiPushButtonEditor.h"
#include "cafPdmUiTreeSelectionEditor.h"

CAF_PDM_SOURCE_INIT( RimCloudDataSourceCollection, "RimCloudDataSourceCollection" );
//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimCloudDataSourceCollection::RimCloudDataSourceCollection()
{
    CAF_PDM_InitObject( "Cloud Data", ":/cloud-and-server.svg" );

    CAF_PDM_InitFieldNoDefault( &m_sumoFieldName, "SumoFieldId", "Field Id" );
    CAF_PDM_InitFieldNoDefault( &m_sumoCaseId, "SumoCaseId", "Case Id" );
    m_sumoCaseId.uiCapability()->setUiEditorTypeName( caf::PdmUiTreeSelectionEditor::uiEditorTypeName() );

    CAF_PDM_InitFieldNoDefault( &m_sumoEnsembleId, "SumoEnsembleId", "Ensemble Id" );
    m_sumoEnsembleId.uiCapability()->setUiEditorTypeName( caf::PdmUiTreeSelectionEditor::uiEditorTypeName() );

    CAF_PDM_InitFieldNoDefault( &m_addEnsemble, "ClearSelectedData", "" );
    caf::PdmUiPushButtonEditor::configureEditorLabelLeft( &m_addEnsemble );

    CAF_PDM_InitFieldNoDefault( &m_sumoDataSources, "SumoDataSources", "Sumo Data Sources" );

    m_sumoConnector = RiaApplication::instance()->makeSumoConnector();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimCloudDataSourceCollection* RimCloudDataSourceCollection::instance()
{
    return RimProject::current()->activeOilField()->cloudDataCollection();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCloudDataSourceCollection::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    if ( changedField == &m_addEnsemble )
    {
        addEnsemble();

        m_addEnsemble = false;
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimCloudDataSourceCollection::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
{
    QList<caf::PdmOptionItemInfo> options;
    if ( fieldNeedingOptions == &m_sumoFieldName )
    {
        if ( m_sumoConnector->assets().empty() )
        {
            m_sumoConnector->requestAssetsBlocking();
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
            m_sumoConnector->requestCasesForFieldBlocking( m_sumoFieldName );
        }

        for ( const auto& sumoCase : m_sumoConnector->cases() )
        {
            options.push_back( { sumoCase.name, sumoCase.caseId.get() } );
        }
    }
    else if ( fieldNeedingOptions == &m_sumoEnsembleId && !m_sumoCaseId().isEmpty() )
    {
        if ( m_sumoConnector->ensembleNamesForCase( SumoCaseId( m_sumoCaseId ) ).empty() )
        {
            m_sumoConnector->requestEnsembleByCasesIdBlocking( SumoCaseId( m_sumoCaseId ) );
        }

        for ( const auto& name : m_sumoConnector->ensembleNamesForCase( SumoCaseId( m_sumoCaseId ) ) )
        {
            options.push_back( { name, name } );
        }
    }

    return options;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCloudDataSourceCollection::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    uiOrdering.add( &m_sumoFieldName );
    uiOrdering.add( &m_sumoCaseId );
    uiOrdering.add( &m_sumoEnsembleId );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCloudDataSourceCollection::defineEditorAttribute( const caf::PdmFieldHandle* field,
                                                          QString                    uiConfigName,
                                                          caf::PdmUiEditorAttribute* attribute )
{
    if ( field == &m_addEnsemble )
    {
        auto attrib = dynamic_cast<caf::PdmUiPushButtonEditorAttribute*>( attribute );
        if ( attrib )
        {
            attrib->m_buttonText = "Add Ensemble";
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimCloudDataSourceCollection::addEnsemble()
{
    auto dataSource = new RimSummarySumoDataSource();

    auto sumoCaseId = SumoCaseId( m_sumoCaseId );
    dataSource->setCaseId( sumoCaseId );
    dataSource->setEnsembleName( m_sumoEnsembleId );

    QString caseName;
    for ( const auto& sumoCase : m_sumoConnector->cases() )
    {
        if ( sumoCase.caseId == sumoCaseId )
        {
            caseName = sumoCase.name;
            break;
        }
    }

    dataSource->setCaseName( caseName );

    m_sumoDataSources.push_back( dataSource );
}
