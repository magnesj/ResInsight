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

#include "RiaApplication.h"
#include "RiaLogging.h"
#include "RiaSummaryTools.h"
#include "RiaTimeTTools.h"

#include "RifArrowTools.h"
#include "RifByteArrayArrowRandomAccessFile.h"
#include "RifEclipseSummaryAddress.h"

#include "Cloud/RimCloudDataSourceCollection.h"
#include "RimSummaryCaseSumo.h"
#include "RimSummarySumoDataSource.h"

#include <QSettings>

CAF_PDM_SOURCE_INIT( RimSummaryEnsembleSumo, "RimSummaryEnsembleSumo" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSummaryEnsembleSumo::RimSummaryEnsembleSumo()
{
    CAF_PDM_InitObject( "Sumo Ensemble", ":/SummaryCase.svg", "", "The Base Class for all Summary Cases" );

    CAF_PDM_InitFieldNoDefault( &m_sumoDataSource, "SumoDataSource", "Sumo Data Source" );

    setAsEnsemble( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryEnsembleSumo::setSumoDataSource( RimSummarySumoDataSource* sumoDataSource )
{
    m_sumoDataSource = sumoDataSource;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::string RimSummaryEnsembleSumo::unitName( const RifEclipseSummaryAddress& resultAddress )
{
    loadSummaryData( resultAddress );

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
std::set<RifEclipseSummaryAddress> RimSummaryEnsembleSumo::allResultAddresses() const
{
    return m_resultAddresses;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryEnsembleSumo::loadSummaryData( const RifEclipseSummaryAddress& resultAddress )
{
    if ( resultAddress.category() == SummaryCategory::SUMMARY_ENSEMBLE_STATISTICS ) return;

    if ( !m_sumoDataSource() ) return;

    auto resultText = QString::fromStdString( resultAddress.toEclipseTextAddress() );

    auto sumoCaseId       = m_sumoDataSource->caseId();
    auto sumoEnsembleName = m_sumoDataSource->ensembleName();

    auto key = ParquetKey{ sumoCaseId, sumoEnsembleName, resultText };

    if ( m_parquetTable.find( key ) == m_parquetTable.end() )
    {
        auto contents = loadParquetData( key );

        arrow::MemoryPool* pool = arrow::default_memory_pool();

        std::shared_ptr<arrow::io::RandomAccessFile> input = std::make_shared<RifByteArrayArrowRandomAccessFile>( contents );

        std::shared_ptr<arrow::Table>               table;
        std::unique_ptr<parquet::arrow::FileReader> arrow_reader;
        if ( parquet::arrow::OpenFile( input, pool, &arrow_reader ).ok() )
        {
            if ( arrow_reader->ReadTable( &table ).ok() )
            {
                RiaLogging::info( "Parquet: Read table" );
            }
            else
            {
                RiaLogging::warning( "Parquet: Error detected during parsing of table" );
            }
        }
        else
        {
            RiaLogging::warning( "Parquet: Not able to open data stream" );
        }

        m_parquetTable[key] = table;

        distributeDataToRealizations( resultAddress, table );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QByteArray RimSummaryEnsembleSumo::loadParquetData( const ParquetKey& parquetKey )
{
    createSumoConnector();

    auto data = m_sumoConnector->requestParquetDataBlocking( SumoCaseId( parquetKey.caseId ), parquetKey.ensembleId, parquetKey.vectorName );

    return data;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryEnsembleSumo::distributeDataToRealizations( const RifEclipseSummaryAddress& resultAddress, std::shared_ptr<arrow::Table> table )
{
    if ( !table )
    {
        RiaLogging::warning( "Failed to load table" );
        return;
    }

    {
        // print header information
        QString txt = "Column Names: ";

        for ( std::string columnName : table->ColumnNames() )
        {
            txt += QString::fromStdString( columnName ) + " ";
        }

        RiaLogging::info( txt );
    }

    std::vector<time_t>  timeSteps;
    std::vector<int16_t> realizations;
    std::vector<float>   values;

    {
        const std::string                    columnName = "DATE";
        std::shared_ptr<arrow::ChunkedArray> column     = table->GetColumnByName( columnName );
        if ( column && column->type()->id() == arrow::Type::TIMESTAMP )
        {
            auto timeColumn = RifArrowTools::chunkedArrayToVector<arrow::Int64Array, int64_t>( column );
            timeSteps       = std::vector<time_t>( timeColumn.size() );

            for ( size_t i = 0; i < timeColumn.size(); ++i )
            {
                timeSteps[i] = RiaTimeTTools::fromDouble( timeColumn[i] );
            }
        }
        else
        {
            RiaLogging::warning( "Failed to find DATE column" );
            return;
        }
    }

    {
        const std::string                    columnName = "REAL";
        std::shared_ptr<arrow::ChunkedArray> column     = table->GetColumnByName( columnName );
        if ( column && column->type()->id() == arrow::Type::INT16 )
        {
            realizations = RifArrowTools::chunkedArrayToVector<arrow::Int16Array, int16_t>( column );
        }
        else
        {
            RiaLogging::warning( "Failed to find realization column" );
            return;
        }
    }

    {
        const std::string                    columnName = resultAddress.toEclipseTextAddress();
        std::shared_ptr<arrow::ChunkedArray> column     = table->GetColumnByName( columnName );
        if ( column && column->type()->id() == arrow::Type::FLOAT )
        {
            values = RifArrowTools::chunkedArrayToVector<arrow::FloatArray, float>( column );
        }
        else
        {
            RiaLogging::warning( "Failed to find values column" );
            return;
        }
    }

    // find unique realizations
    std::set<int16_t> uniqueRealizations;
    for ( auto realizationNumber : realizations )
    {
        uniqueRealizations.insert( realizationNumber );
    }

    if ( m_cases.size() != uniqueRealizations.size() )
    {
        m_cases.deleteChildren();
    }

    // find start and end index for a given realization number
    std::map<int16_t, std::pair<size_t, size_t>> realizationIndex;
    for ( size_t i = 0; i < realizations.size(); ++i )
    {
        auto realizationNumber = realizations[i];
        uniqueRealizations.insert( realizationNumber );

        if ( realizationIndex.find( realizationNumber ) == realizationIndex.end() )
        {
            realizationIndex[realizationNumber] = { i, i };
        }
        else
        {
            realizationIndex[realizationNumber].second = i;
        }
    }

    auto findSummaryCase = [this]( int16_t realizationNumber ) -> RimSummaryCaseSumo*
    {
        for ( auto sumCase : allSummaryCases() )
        {
            auto sumCaseSumo = dynamic_cast<RimSummaryCaseSumo*>( sumCase );
            if ( sumCaseSumo->realizationNumber() == realizationNumber ) return sumCaseSumo;
        }

        return nullptr;
    };

    bool anyCaseCreated = false;

    for ( auto realizationNumber : uniqueRealizations )
    {
        auto summaryCase = findSummaryCase( realizationNumber );
        if ( !summaryCase )
        {
            summaryCase = new RimSummaryCaseSumo();
            summaryCase->setEnsemble( this );
            summaryCase->setRealizationNumber( realizationNumber );
            summaryCase->setRealizationName( QString( "Realization %1" ).arg( realizationNumber ) );
            m_cases.push_back( summaryCase );

            anyCaseCreated = true;
        }

        auto start = realizationIndex[realizationNumber].first;
        auto end   = realizationIndex[realizationNumber].second;

        std::vector<time_t> realizationTimeSteps( timeSteps.begin() + start, timeSteps.begin() + end );
        std::vector<float>  realizationValues( values.begin() + start, values.begin() + end );

        summaryCase->setValues( realizationTimeSteps, resultAddress.toEclipseTextAddress(), realizationValues );
    }

    if ( anyCaseCreated )
    {
        buildMetaData();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryEnsembleSumo::buildMetaData()
{
    if ( !allSummaryCases().empty() )
    {
        auto firstCase = allSummaryCases().front();

        firstCase->summaryReader()->buildMetaData();
    }

    RimSummaryCaseCollection::buildMetaData();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryEnsembleSumo::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    auto group = uiOrdering.addNewGroup( "General" );

    RimSummaryCaseCollection::defineUiOrdering( uiConfigName, *group );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimSummaryEnsembleSumo::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
{
    createSumoConnector();

    QList<caf::PdmOptionItemInfo> options;
    if ( fieldNeedingOptions == &m_sumoDataSource )
    {
        for ( const auto& sumoDataSource : RimCloudDataSourceCollection::instance()->sumoDataSources() )
        {
            options.push_back( { sumoDataSource->name(), sumoDataSource } );
        }
    }

    return options;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryEnsembleSumo::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    if ( changedField == &m_sumoDataSource )
    {
        clearCachedData();
        getAvailableVectorNames();

        buildMetaData();

        updateConnectedEditors();

        RiaSummaryTools::reloadSummaryEnsemble( this );
    }
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
void RimSummaryEnsembleSumo::getAvailableVectorNames()
{
    if ( !m_sumoConnector ) return;
    if ( !m_sumoDataSource() ) return;

    m_sumoConnector->requestVectorNamesForEnsembleBlocking( m_sumoDataSource->caseId(), m_sumoDataSource->ensembleName() );

    auto vectorNames = m_sumoConnector->vectorNames();
    for ( auto vectorName : vectorNames )
    {
        auto adr = RifEclipseSummaryAddress::fromEclipseTextAddress( vectorName.toStdString() );
        m_resultAddresses.insert( adr );
    }

    auto caseName = m_sumoDataSource->caseId().get();
    auto ensName  = m_sumoDataSource->ensembleName();

    RiaLogging::info( QString( "Case: %1, ens: %2,  vector count: %3" ).arg( caseName ).arg( ensName ).arg( m_resultAddresses.size() ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryEnsembleSumo::clearCachedData()
{
    m_resultAddresses.clear();
    m_parquetTable.clear();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryEnsembleSumo::onLoadDataAndUpdate()
{
    // load summary data from sumo
    // create the realizations and add them to the ensemble

    createSumoConnector();

    m_cases.deleteChildren();

    size_t realizationCount = 10;
    for ( size_t i = 0; i < realizationCount; ++i )
    {
        auto realization = new RimSummaryCaseSumo();
        realization->setEnsemble( this );
        realization->setRealizationName( QString( "Realization %1" ).arg( i ) );

        m_cases.push_back( realization );
    }

    getAvailableVectorNames();

    buildMetaData();

    // call the base class method after data has been loaded
    RimSummaryCaseCollection::onLoadDataAndUpdate();
}
