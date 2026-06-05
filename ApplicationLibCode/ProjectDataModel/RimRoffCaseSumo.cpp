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

#include "RimRoffCaseSumo.h"

#include "RiaApplication.h"
#include "RiaLogging.h"
#include "RiaPreferencesGrid.h"

#include "Cloud/RiaSumoConnector.h"

#include "RifRoffFileTools.h"

#include "RigCaseCellResultsData.h"
#include "RigEclipseCaseData.h"
#include "RigMainGrid.h"

#include "RimReservoirCellResultsStorage.h"
#include "Sumo/RimSumoDataSource.h"

#include "cafPdmObjectScriptingCapability.h"

#include <sstream>

CAF_PDM_SOURCE_INIT( RimRoffCaseSumo, "RimRoffCaseSumo" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimRoffCaseSumo::RimRoffCaseSumo()
{
    CAF_PDM_InitScriptableObject( "Sumo Grid Case", ":/Case48x48.png" );

    CAF_PDM_InitFieldNoDefault( &m_sumoDataSource, "SumoDataSource", "Sumo Data Source" );
    m_sumoDataSource.uiCapability()->setUiHidden( true );

    CAF_PDM_InitFieldNoDefault( &m_sumoCaseId, "SumoCaseId", "Sumo Case Id" );
    m_sumoCaseId.uiCapability()->setUiReadOnly( true );

    CAF_PDM_InitFieldNoDefault( &m_ensembleName, "EnsembleName", "Ensemble Name" );
    m_ensembleName.uiCapability()->setUiReadOnly( true );

    CAF_PDM_InitFieldNoDefault( &m_gridName, "GridName", "Grid Name" );
    m_gridName.uiCapability()->setUiReadOnly( true );

    CAF_PDM_InitField( &m_realization, "Realization", -1, "Realization" );
    m_realization.uiCapability()->setUiReadOnly( true );

    m_sumoConnector = RiaApplication::instance()->makeSumoConnector();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimRoffCaseSumo::~RimRoffCaseSumo()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimRoffCaseSumo* RimRoffCaseSumo::createFromDataSource( RimSumoDataSource* dataSource, const QString& gridName, int realization )
{
    if ( !dataSource ) return nullptr;

    auto* gridCase = new RimRoffCaseSumo();
    gridCase->setSumoDataSource( dataSource );
    gridCase->setSumoCaseId( dataSource->caseId().get() );
    gridCase->setEnsembleName( dataSource->ensembleName() );
    gridCase->setGridName( gridName );
    gridCase->setRealization( realization );

    // Name the case using grid name, asset, ensemble and realization, e.g. "Geogrid_Drogon_iter-0_Real_0".
    QString caseDisplayName = QString( "%1_%2_%3_Real_%4" )
                                  .arg( gridName, dataSource->assetName(), dataSource->ensembleName() )
                                  .arg( realization );
    gridCase->setCustomCaseName( caseDisplayName );

    return gridCase;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimRoffCaseSumo::setSumoDataSource( RimSumoDataSource* dataSource )
{
    m_sumoDataSource = dataSource;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimRoffCaseSumo::setSumoCaseId( const QString& sumoCaseId )
{
    m_sumoCaseId = sumoCaseId;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimRoffCaseSumo::setEnsembleName( const QString& ensembleName )
{
    m_ensembleName = ensembleName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimRoffCaseSumo::setGridName( const QString& gridName )
{
    m_gridName = gridName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimRoffCaseSumo::setRealization( int realization )
{
    m_realization = realization;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimRoffCaseSumo::gridName() const
{
    return m_gridName();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RimRoffCaseSumo::realization() const
{
    return m_realization();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimRoffCaseSumo::openEclipseGridFile()
{
    if ( eclipseCaseData() )
    {
        // Early exit if reservoir data is created
        return true;
    }

    if ( !m_sumoConnector )
    {
        RiaLogging::error( "No Sumo connector available, unable to load grid from Sumo." );
        return false;
    }

    setReservoirData( new RigEclipseCaseData( this ) );

    if ( eclipseCaseData()->mainGrid()->cellCount() == 0 )
    {
        QByteArray contents = m_sumoConnector->requestGridDataBlocking( SumoCaseId( m_sumoCaseId() ),
                                                                        m_ensembleName(),
                                                                        m_gridName(),
                                                                        m_realization() );
        if ( contents.isEmpty() )
        {
            RiaLogging::error( std::format( "Failed to download grid '{}' (realization {}) from Sumo.",
                                            m_gridName().toStdString(),
                                            m_realization() ) );
            return false;
        }

        // The downloaded blob is a binary roff grid. Parse it directly from memory.
        std::string        buffer = contents.toStdString();
        std::istringstream stream( buffer, std::ios::binary );

        QString errorMessages;
        if ( RifRoffFileTools::openGridFile( stream, eclipseCaseData(), &errorMessages ) )
        {
            eclipseCaseData()->mainGrid()->setFlipAxis( m_flipXAxis, m_flipYAxis );
            computeCachedData();
        }
        else
        {
            RiaLogging::error( errorMessages.toStdString() );
            return false;
        }
    }

    results( RiaDefines::PorosityModelType::MATRIX_MODEL )->createPlaceholderResultEntries();

    if ( RiaPreferencesGrid::current()->autoComputeDepthRelatedProperties() )
    {
        results( RiaDefines::PorosityModelType::MATRIX_MODEL )->computeDepthRelatedResults();
        results( RiaDefines::PorosityModelType::FRACTURE_MODEL )->computeDepthRelatedResults();
    }

    results( RiaDefines::PorosityModelType::MATRIX_MODEL )->computeCellVolumes();

    return true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimRoffCaseSumo::locationOnDisc() const
{
    // The grid is stored on Sumo, not on disk.
    return QString();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimRoffCaseSumo::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    uiOrdering.add( &m_caseUserDescription );
    uiOrdering.add( &m_displayNameOption );
    uiOrdering.add( &m_caseId );

    auto group = uiOrdering.addNewGroup( "Sumo" );
    group->add( &m_sumoCaseId );
    group->add( &m_ensembleName );
    group->add( &m_gridName );
    group->add( &m_realization );
}
