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

#include "RimSumoDataSource.h"

#include "RiaStdStringTools.h"

#include "cafCmdFeatureMenuBuilder.h"
#include "cafPdmUiTreeSelectionEditor.h"

CAF_PDM_SOURCE_INIT( RimSumoDataSource, "RimSumoDataSource", "RimSummarySumoDataSource", "RimSumoGridDataSource" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSumoDataSource::RimSumoDataSource()
{
    CAF_PDM_InitObject( "Sumo Data Source", ":/CloudBlobs.svg" );

    CAF_PDM_InitFieldNoDefault( &m_caseId, "CaseId", "Case Id" );
    CAF_PDM_InitFieldNoDefault( &m_assetName, "AssetName", "Asset Name" );
    CAF_PDM_InitFieldNoDefault( &m_caseName, "CaseName", "Case Name" );
    CAF_PDM_InitFieldNoDefault( &m_ensembleName, "EnsembleName", "Ensemble Name" );
    CAF_PDM_InitFieldNoDefault( &m_customName, "CustomName", "Custom Name" );

    CAF_PDM_InitFieldNoDefault( &m_availableRealizationIds, "RealizationIds", "Available Realization Ids" );
    m_availableRealizationIds.uiCapability()->setUiHidden( true );

    CAF_PDM_InitFieldNoDefault( &m_selectedRealizationIds, "SelectedRealizationIds", "Select" );
    m_selectedRealizationIds.uiCapability()->setUiEditorTypeName( caf::PdmUiTreeSelectionEditor::uiEditorTypeName() );

    CAF_PDM_InitFieldNoDefault( &m_selectedRealizationsText, "SelectedRealizationsText", "Realizations" );
    m_selectedRealizationsText.registerGetMethod( this, &RimSumoDataSource::selectedRealizationsText );
    m_selectedRealizationsText.uiCapability()->setUiReadOnly( true );

    CAF_PDM_InitFieldNoDefault( &m_vectorNames, "VectorNames", "Vector Names" );
    m_vectorNames.uiCapability()->setUiReadOnly( true );

    CAF_PDM_InitFieldNoDefault( &m_gridNames, "GridNames", "Grid Names" );
    m_gridNames.uiCapability()->setUiHidden( true );

    CAF_PDM_InitFieldNoDefault( &m_selectedGridName, "GridName", "Grid Name" );

    setDeletable( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
SumoCaseId RimSumoDataSource::caseId() const
{
    return SumoCaseId( m_caseId() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoDataSource::setCaseId( const SumoCaseId& caseId )
{
    m_caseId = caseId.get();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimSumoDataSource::assetName() const
{
    return m_assetName();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoDataSource::setAssetName( const QString& assetName )
{
    m_assetName = assetName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimSumoDataSource::caseName() const
{
    return m_caseName();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoDataSource::setCaseName( const QString& caseName )
{
    m_caseName = caseName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimSumoDataSource::ensembleName() const
{
    return m_ensembleName();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoDataSource::setEnsembleName( const QString& ensembleName )
{
    m_ensembleName = ensembleName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<QString> RimSumoDataSource::availableRealizationIds() const
{
    return m_availableRealizationIds();
}

//--------------------------------------------------------------------------------------------------
/// The available realizations are the source of truth. Default the selection to all of them.
//--------------------------------------------------------------------------------------------------
void RimSumoDataSource::setAvailableRealizationIds( const std::vector<QString>& realizationIds )
{
    m_availableRealizationIds = realizationIds;
    m_selectedRealizationIds  = realizationIds;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<QString> RimSumoDataSource::selectedRealizationIds() const
{
    return m_selectedRealizationIds();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoDataSource::setSelectedRealizationIds( const std::vector<QString>& realizationIds )
{
    m_selectedRealizationIds = realizationIds;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<QString> RimSumoDataSource::vectorNames() const
{
    return m_vectorNames();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoDataSource::setVectorNames( const std::vector<QString>& vectorNames )
{
    m_vectorNames = vectorNames;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<QString> RimSumoDataSource::gridNames() const
{
    return m_gridNames();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoDataSource::setGridNames( const std::vector<QString>& gridNames )
{
    m_gridNames = gridNames;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimSumoDataSource::selectedGridName() const
{
    return m_selectedGridName();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoDataSource::updateName()
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
void RimSumoDataSource::appendMenuItems( caf::CmdFeatureMenuBuilder& menuBuilder ) const
{
    menuBuilder.addCmdFeature( "RicCreateSumoEnsembleFeature" );
    menuBuilder.addCmdFeature( "RicCreateSumoGridEnsembleFeature" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoDataSource::defineEditorAttribute( const caf::PdmFieldHandle* field, QString uiConfigName, caf::PdmUiEditorAttribute* attribute )
{
    if ( field == &m_vectorNames )
    {
        if ( auto attr = dynamic_cast<caf::PdmUiTreeSelectionEditorAttribute*>( attribute ) )
        {
            attr->showCheckBoxes        = false;
            attr->showContextMenu       = false;
            attr->showToggleAllCheckbox = false;
        }
    }
    else if ( field == &m_selectedRealizationIds )
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
void RimSumoDataSource::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
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

    auto ensembleGroup = uiOrdering.addNewGroup( "Ensemble Selection" );
    ensembleGroup->add( &m_selectedRealizationsText );
    ensembleGroup->add( &m_selectedRealizationIds );

    auto gridGroup = uiOrdering.addNewGroup( "Grid Selection" );
    gridGroup->add( &m_selectedGridName );

    auto summaryInfo = uiOrdering.addNewGroup( "Info" );
    summaryInfo->setCollapsedByDefault();
    summaryInfo->add( &m_vectorNames );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimSumoDataSource::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
{
    QList<caf::PdmOptionItemInfo> options;

    if ( fieldNeedingOptions == &m_selectedGridName )
    {
        for ( const auto& gridName : m_gridNames() )
        {
            options.push_back( caf::PdmOptionItemInfo( gridName, gridName ) );
        }
    }
    else if ( fieldNeedingOptions == &m_selectedRealizationIds )
    {
        for ( const auto& realizationId : m_availableRealizationIds() )
        {
            options.push_back( caf::PdmOptionItemInfo( realizationId, realizationId ) );
        }
    }

    return options;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoDataSource::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    if ( changedField == &m_customName )
    {
        updateName();
    }
    else if ( changedField == &m_selectedRealizationIds )
    {
        // Refresh editors so the Realizations label range text stays in sync with the selection.
        updateConnectedEditors();
    }
}

//--------------------------------------------------------------------------------------------------
/// Compact range text for the selected ensemble realizations.
//--------------------------------------------------------------------------------------------------
QString RimSumoDataSource::selectedRealizationsText() const
{
    std::vector<int> intValues;
    for ( const auto& realizationId : m_selectedRealizationIds() )
    {
        bool ok    = false;
        int  value = realizationId.toInt( &ok );
        if ( ok ) intValues.push_back( value );
    }

    auto rangeString = RiaStdStringTools::formatRangeSelection( intValues );
    return QString::fromStdString( rangeString );
}
