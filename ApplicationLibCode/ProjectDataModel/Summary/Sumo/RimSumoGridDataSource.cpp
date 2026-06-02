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

#include "RimSumoGridDataSource.h"

#include "RiaApplication.h"

#include "cafCmdFeatureMenuBuilder.h"
#include "cafPdmUiTreeSelectionEditor.h"

CAF_PDM_SOURCE_INIT( RimSumoGridDataSource, "RimSumoGridDataSource" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSumoGridDataSource::RimSumoGridDataSource()
{
    CAF_PDM_InitObject( "Sumo Grid Data Source", ":/CloudBlobs.svg" );

    CAF_PDM_InitFieldNoDefault( &m_caseId, "CaseId", "Case Id" );
    CAF_PDM_InitFieldNoDefault( &m_assetName, "AssetName", "Asset Name" );
    CAF_PDM_InitFieldNoDefault( &m_caseName, "CaseName", "Case Name" );
    CAF_PDM_InitFieldNoDefault( &m_ensembleName, "EnsembleName", "Ensemble Name" );
    CAF_PDM_InitFieldNoDefault( &m_customName, "CustomName", "Custom Name" );

    CAF_PDM_InitFieldNoDefault( &m_gridName, "GridName", "Grid Name" );
    CAF_PDM_InitFieldNoDefault( &m_realizations, "Realizations", "Realizations" );
    m_realizations.uiCapability()->setUiEditorTypeName( caf::PdmUiTreeSelectionEditor::uiEditorTypeName() );

    setDeletable( true );

    m_sumoConnector = RiaApplication::instance()->makeSumoConnector();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
SumoCaseId RimSumoGridDataSource::caseId() const
{
    return SumoCaseId( m_caseId() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoGridDataSource::setCaseId( const SumoCaseId& caseId )
{
    m_caseId = caseId.get();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimSumoGridDataSource::assetName() const
{
    return m_assetName();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoGridDataSource::setAssetName( const QString& assetName )
{
    m_assetName = assetName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimSumoGridDataSource::caseName() const
{
    return m_caseName();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoGridDataSource::setCaseName( const QString& caseName )
{
    m_caseName = caseName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimSumoGridDataSource::ensembleName() const
{
    return m_ensembleName();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoGridDataSource::setEnsembleName( const QString& ensembleName )
{
    m_ensembleName = ensembleName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimSumoGridDataSource::gridName() const
{
    return m_gridName();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<int> RimSumoGridDataSource::selectedRealizations() const
{
    return m_realizations();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoGridDataSource::updateName()
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
void RimSumoGridDataSource::appendMenuItems( caf::CmdFeatureMenuBuilder& menuBuilder ) const
{
    menuBuilder.addCmdFeature( "RicCreateSumoGridEnsembleFeature" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoGridDataSource::defineEditorAttribute( const caf::PdmFieldHandle* field,
                                                   QString                    uiConfigName,
                                                   caf::PdmUiEditorAttribute* attribute )
{
    if ( field == &m_realizations )
    {
        if ( auto attr = dynamic_cast<caf::PdmUiTreeSelectionEditorAttribute*>( attribute ) )
        {
            attr->showCheckBoxes        = true;
            attr->showToggleAllCheckbox = true;
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoGridDataSource::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    auto group = uiOrdering.addNewGroup( "General" );

    group->add( nameField() );
    nameField()->uiCapability()->setUiReadOnly( true );

    group->add( &m_caseId );
    m_caseId.uiCapability()->setUiReadOnly( true );

    group->add( &m_assetName );
    m_assetName.uiCapability()->setUiReadOnly( true );

    group->add( &m_caseName );
    m_caseName.uiCapability()->setUiReadOnly( true );

    group->add( &m_ensembleName );
    m_ensembleName.uiCapability()->setUiReadOnly( true );

    group->add( &m_customName );

    auto gridGroup = uiOrdering.addNewGroup( "Grid Selection" );
    gridGroup->add( &m_gridName );
    gridGroup->add( &m_realizations );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimSumoGridDataSource::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
{
    QList<caf::PdmOptionItemInfo> options;

    if ( !m_sumoConnector ) return options;

    if ( fieldNeedingOptions == &m_gridName || fieldNeedingOptions == &m_realizations )
    {
        if ( m_sumoConnector->gridInfos().empty() )
        {
            m_sumoConnector->requestGridInfoForEnsembleBlocking( caseId(), ensembleName() );
        }
    }

    if ( fieldNeedingOptions == &m_gridName )
    {
        for ( const auto& gridInfo : m_sumoConnector->gridInfos() )
        {
            options.push_back( caf::PdmOptionItemInfo( gridInfo.name, gridInfo.name ) );
        }
    }
    else if ( fieldNeedingOptions == &m_realizations )
    {
        for ( const auto& gridInfo : m_sumoConnector->gridInfos() )
        {
            if ( gridInfo.name == m_gridName() )
            {
                for ( int realization : gridInfo.realizations )
                {
                    options.push_back( caf::PdmOptionItemInfo( QString::number( realization ), realization ) );
                }
                break;
            }
        }
    }

    return options;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoGridDataSource::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    if ( changedField == &m_customName )
    {
        updateName();
    }
    else if ( changedField == &m_gridName )
    {
        // The set of available realizations depends on the selected grid; clear the current selection.
        m_realizations.v().clear();
        updateConnectedEditors();
    }
}
