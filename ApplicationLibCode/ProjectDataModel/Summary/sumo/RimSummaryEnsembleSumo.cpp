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
#include "RimSummaryCaseSumo.h"

#include "RifEclipseSummaryAddress.h"

#include "cafPdmUiTreeSelectionEditor.h"

#include "RifByteArrayArrowRandomAccessFile.h"

#include "../../../Application/Tools/RiaTimeTTools.h"
#include "../../../FileInterface/RifArrowTools.h"
#include "RiaLogging.h"

#pragma optimize( "", off )

CAF_PDM_SOURCE_INIT( RimSummaryEnsembleSumo, "RimSummaryEnsembleSumo" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSummaryEnsembleSumo::RimSummaryEnsembleSumo()
{
    CAF_PDM_InitObject( "Sumo Ensemble", ":/SummaryCase.svg", "", "The Base Class for all Summary Cases" );

    CAF_PDM_InitFieldNoDefault( &m_sumoFieldName, "SumoFieldId", "Field Id" );
    CAF_PDM_InitFieldNoDefault( &m_sumoCaseId, "SumoCaseId", "Case Id" );
    m_sumoCaseId.uiCapability()->setUiEditorTypeName( caf::PdmUiTreeSelectionEditor::uiEditorTypeName() );

    CAF_PDM_InitFieldNoDefault( &m_ensembleId, "SumoEnsembleId", "Ensemble Id" );

    setAsEnsemble( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<time_t> RimSummaryEnsembleSumo::timeSteps( const QString& realizationName, const RifEclipseSummaryAddress& resultAddress )
{
    loadSummaryData( resultAddress );

    auto key =
        ParquetKey{ m_sumoFieldName(), m_sumoCaseId(), m_ensembleId(), QString::fromStdString( resultAddress.toEclipseTextAddress() ) };

    // check if the table is loaded
    if ( m_parquetTable.find( key ) == m_parquetTable.end() ) return {};

    auto table = m_parquetTable[key];

    auto timeColumn = dataForColumn( table, "real-0", "Date" );

    // convert from double to time_t
    std::vector<time_t> timeSteps;
    for ( auto time : timeColumn )
    {
        time_t timeStep = static_cast<time_t>( time );
        timeSteps.push_back( timeStep );
    }

    return timeSteps;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<double> RimSummaryEnsembleSumo::values( const QString& realizationName, const RifEclipseSummaryAddress& resultAddress )
{
    loadSummaryData( resultAddress );

    auto key =
        ParquetKey{ m_sumoFieldName(), m_sumoCaseId(), m_ensembleId(), QString::fromStdString( resultAddress.toEclipseTextAddress() ) };

    // check if the table is loaded
    if ( m_parquetTable.find( key ) == m_parquetTable.end() ) return {};

    auto table = m_parquetTable[key];

    auto dataValues = dataForColumn( table, "real-0", QString::fromStdString( resultAddress.toEclipseTextAddress() ) );

    return dataValues;
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
QByteArray RimSummaryEnsembleSumo::loadSummaryData( const RifEclipseSummaryAddress& resultAddress )
{
    // create job to download data from sumo
    // download data, and notify when done

    auto resultText = QString::fromStdString( resultAddress.toEclipseTextAddress() );

    auto key = ParquetKey{ m_sumoFieldName(), m_sumoCaseId(), m_ensembleId(), resultText };

    if ( m_parquetData.find( key ) == m_parquetData.end() )
    {
        auto contents      = loadParquetData( key );
        m_parquetData[key] = contents;

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

    return m_parquetData[key];
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QByteArray RimSummaryEnsembleSumo::loadParquetData( const ParquetKey& parquetKey )
{
    createSumoConnector();

    auto data = m_sumoConnector->requestParquetDataBlocking( parquetKey.caseId, parquetKey.ensembleId, parquetKey.vectorName );

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
    std::vector<int16_t> realisation;
    std::vector<float>   values;

    {
        const std::string                    columnName = "DATE";
        std::shared_ptr<arrow::ChunkedArray> column     = table->GetColumnByName( columnName );
        if ( column && column->type()->id() == arrow::Type::TIMESTAMP )
        {
            auto timeColumn = RifArrowTools::convertChunkedArrayToStdInt64Vector( column );
            timeSteps       = std::vector<time_t>( timeColumn.size() );

            for ( size_t i = 0; i < timeColumn.size(); ++i )
            {
                timeSteps[i] = RiaTimeTTools::fromDouble( timeColumn[i] );
            }
        }
        else
        {
            RiaLogging::warning( "Failed to find DATE column" );
        }
    }

    {
        const std::string                    columnName = "REAL";
        std::shared_ptr<arrow::ChunkedArray> column     = table->GetColumnByName( columnName );
        if ( column && column->type()->id() == arrow::Type::INT16 )
        {
            realisation = RifArrowTools::convertChunkedArrayToStdInt16Vector( column );
        }
        else
        {
            RiaLogging::warning( "Failed to find realization column" );
        }
    }

    {
        const std::string                    columnName = resultAddress.toEclipseTextAddress();
        std::shared_ptr<arrow::ChunkedArray> column     = table->GetColumnByName( columnName );
        if ( column && column->type()->id() == arrow::Type::FLOAT )
        {
            values = RifArrowTools::convertChunkedArrayToStdFloatVector( column );
        }
        else
        {
            RiaLogging::warning( "Failed to find values column" );
        }
    }

    // find unique realizations
    std::set<int16_t> uniqueRealizations;
    for ( auto realizationNumber : realisation )
    {
        uniqueRealizations.insert( realizationNumber );
    }

    if ( m_cases.size() != uniqueRealizations.size() )
    {
        m_cases.deleteChildren();
    }

    // find start and end index for a given realization number
    std::map<int16_t, std::pair<size_t, size_t>> realizationIndex;
    for ( size_t i = 0; i < realisation.size(); ++i )
    {
        auto realizationNumber = realisation[i];
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
        auto firstCase = allSummaryCases().front();
        firstCase->summaryReader()->buildMetaData();

        buildMetaData();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<double>
    RimSummaryEnsembleSumo::dataForColumn( std::shared_ptr<arrow::Table> table, const QString& realizationName, const QString& columnName )
{
    std::shared_ptr<arrow::ChunkedArray> column = table->GetColumnByName( columnName.toStdString() );

    if ( column->type()->id() == arrow::Type::DOUBLE )
    {
        return RifArrowTools::convertChunkedArrayToStdVector( column );
    }

    if ( column->type()->id() == arrow::Type::FLOAT )
    {
        auto                floatVector = RifArrowTools::convertChunkedArrayToStdFloatVector( column );
        std::vector<double> columnVector( floatVector.begin(), floatVector.end() );
        return columnVector;
    }

    return {};
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryEnsembleSumo::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    uiOrdering.add( &m_sumoFieldName );
    uiOrdering.add( &m_sumoCaseId );
    uiOrdering.add( &m_ensembleId );

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
    else if ( fieldNeedingOptions == &m_ensembleId && !m_sumoCaseId().isEmpty() )
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
void RimSummaryEnsembleSumo::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    if ( changedField == &m_sumoFieldName || changedField == &m_sumoCaseId || changedField == &m_ensembleId )
    {
        clearCachedData();
        getAvailableVectorNames();

        for ( auto sumCase : allSummaryCases() )
        {
            sumCase->summaryReader()->buildMetaData();
        }

        buildMetaData();
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
    m_sumoConnector->requestVectorNamesForEnsembleBlocking( m_sumoCaseId, m_ensembleId );

    auto vectorNames = m_sumoConnector->vectorNames();
    for ( auto vectorName : vectorNames )
    {
        auto adr = RifEclipseSummaryAddress::fromEclipseTextAddress( vectorName.toStdString() );
        m_resultAddresses.insert( adr );
    }

    /*
        auto caseName = m_sumoCaseId.uiCapability().uiValue().toString();
        auto ensName  = m_sumoEnsembleId.uiCapability()->uiValue().toString();
    */
    auto caseName = m_sumoCaseId();
    auto ensName  = m_ensembleId();

    RiaLogging::info( QString( "Case: %1, ens: %2,  vector count: %3" ).arg( caseName ).arg( ensName ).arg( m_resultAddresses.size() ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSummaryEnsembleSumo::clearCachedData()
{
    m_resultAddresses.clear();
    m_parquetData.clear();
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

    for ( auto sumCase : allSummaryCases() )
    {
        sumCase->summaryReader()->buildMetaData();
    }

    buildMetaData();

    // call the base class method after data has been loaded
    RimSummaryCaseCollection::onLoadDataAndUpdate();
}
