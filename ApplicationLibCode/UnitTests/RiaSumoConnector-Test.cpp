/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2026- Equinor ASA
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

#include "gtest/gtest.h"

#include "Cloud/RiaCloudApiService.h"
#include "Cloud/RiaSumoConnector.h"
#include "Cloud/RiaSumoDefines.h"
#include "RiaApplication.h"

#include "RifByteArrayArrowRandomAccessFile.h"

#undef signals
#include <arrow/table.h>
#include <arrow/type.h>
#include <parquet/arrow/reader.h>
#define signals Q_SIGNALS

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QString>
#include <QStringList>
#include <QTimer>

#include <algorithm>
#include <iomanip>
#include <iostream>

namespace
{
//--------------------------------------------------------------------------------------------------
/// Spin the event loop until the predicate holds, or the timeout expires.
//--------------------------------------------------------------------------------------------------
bool waitForCondition( const std::function<bool()>& predicate, int timeoutMillis )
{
    QElapsedTimer timer;
    timer.start();

    while ( !predicate() )
    {
        if ( timer.elapsed() > timeoutMillis ) return false;

        QCoreApplication::processEvents( QEventLoop::ExcludeUserInputEvents, 100 );
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
/// Read a parquet blob as downloaded from Sumo into an Arrow table.
//--------------------------------------------------------------------------------------------------
std::shared_ptr<arrow::Table> readParquetTable( const QByteArray& contents )
{
    std::shared_ptr<arrow::io::RandomAccessFile> input = std::make_shared<RifByteArrayArrowRandomAccessFile>( contents );

    std::shared_ptr<arrow::Table> table;

#if ARROW_VERSION_MAJOR >= 20
    auto openResult = parquet::arrow::OpenFile( input, arrow::default_memory_pool() );
    if ( !openResult.ok() ) return {};

    std::unique_ptr<parquet::arrow::FileReader> reader = std::move( openResult ).ValueOrDie();
#else
    std::unique_ptr<parquet::arrow::FileReader> reader;
    if ( !parquet::arrow::OpenFile( input, arrow::default_memory_pool(), &reader ).ok() ) return {};
#endif

    if ( !reader->ReadTable( &table ).ok() ) return {};

    return table;
}

//--------------------------------------------------------------------------------------------------
/// Download a blob and return its contents. RiaSumoConnector::requestParquetDataBlocking resolves the
/// blob id and downloads in a single call, which hides where the time is spent. Driving the download
/// separately makes the two steps individually measurable.
//--------------------------------------------------------------------------------------------------
QByteArray downloadBlobBlocking( RiaSumoConnector* connector, const QString& blobId, int timeoutMillis )
{
    QByteArray contents;

    QEventLoop loop;

    QTimer timer;
    timer.setSingleShot( true );
    QObject::connect( &timer, &QTimer::timeout, &loop, &QEventLoop::quit );

    // The event loop is the connection context, so the connection is dropped when this function returns.
    QObject::connect( connector,
                      &RiaSumoConnector::parquetDownloadFinished,
                      &loop,
                      [&contents, &loop]( const QByteArray& data, const QString& )
                      {
                          contents = data;
                          loop.quit();
                      } );

    connector->requestBlobDownload( blobId );

    timer.start( timeoutMillis );
    loop.exec( QEventLoop::ExcludeUserInputEvents );

    return contents;
}

//--------------------------------------------------------------------------------------------------
/// Report a timing measurement on stdout. Tagged so the numbers can be picked out of the service log,
/// which is interleaved with the test output.
//--------------------------------------------------------------------------------------------------
void reportTiming( const QString& label, qint64 elapsedMillis, const QString& comment = {} )
{
    std::cout << "[  TIMING  ] " << std::left << std::setw( 40 ) << label.toStdString() << std::right << std::setw( 7 ) << elapsedMillis
              << " ms";
    if ( !comment.isEmpty() ) std::cout << "   " << comment.toStdString();
    std::cout << std::endl;
}

// Starting the local service involves launching a Python process and waiting for uvicorn to boot.
constexpr int serviceStartupTimeoutMillis = 60 * 1000;

} // namespace

//--------------------------------------------------------------------------------------------------
/// The address of the local service is not known when the connector is constructed, and changes if
/// the service is restarted on a different port. Verify that the connector reads it from the provider
/// on every call instead of caching it.
//--------------------------------------------------------------------------------------------------
TEST( RiaSumoConnectorTest, serverAddressIsReadFromProviderForEveryCall )
{
    QString currentServerUrl = "http://127.0.0.1:8000";

    RiaSumoConnector connector( nullptr, [&currentServerUrl]() { return currentServerUrl; }, "authority", "scopes", "clientId", 53528 );

    EXPECT_EQ( QString( "http://127.0.0.1:8000" ), connector.server() );

    currentServerUrl = "http://127.0.0.1:8042";
    EXPECT_EQ( QString( "http://127.0.0.1:8042" ), connector.server() );
}

//--------------------------------------------------------------------------------------------------
/// A connector without a server address provider must report an empty address rather than crash.
//--------------------------------------------------------------------------------------------------
TEST( RiaSumoConnectorTest, serverAddressIsEmptyWithoutProvider )
{
    RiaSumoConnector connector( nullptr, {}, "authority", "scopes", "clientId", 53529 );

    EXPECT_TRUE( connector.server().isEmpty() );
}

//--------------------------------------------------------------------------------------------------
/// Download summary vectors for a single well from a known Sumo case.
///
/// Disabled by default: the test talks to the real Sumo backend through the local 'ri_cloud_api'
/// service, and requires a valid Sumo authentication token. If no cached token is available, a
/// browser window is opened for interactive login. Run manually with
///
///   ResInsight-tests --gtest_filter=RiaSumoConnectorTest.DISABLED_DownloadWellVectors --gtest_also_run_disabled_tests
///
//--------------------------------------------------------------------------------------------------
TEST( RiaSumoConnectorTest, DISABLED_DownloadWellVectors )
{
    const SumoCaseId  caseId( "e7f117b6-29fe-488f-989c-dbbc9bd03f09" );
    const QString     wellName    = "A1";
    const QStringList vectorNames = { "WOPR:" + wellName, "WGOR:" + wellName, "WWCT:" + wellName, "WWPR:" + wellName };

    QElapsedTimer totalTimer;
    totalTimer.start();

    QElapsedTimer stepTimer;

    auto* app = RiaApplication::instance();
    ASSERT_TRUE( app != nullptr );

    auto* cloudApiService = app->cloudApiService();
    ASSERT_TRUE( cloudApiService != nullptr );

    stepTimer.start();
    cloudApiService->start();
    ASSERT_TRUE( waitForCondition( [cloudApiService]() { return cloudApiService->isResponding(); }, serviceStartupTimeoutMillis ) )
        << "The local 'ri_cloud_api' service did not start. Check that Python and the service folder are configured in preferences.";
    reportTiming( "Cloud API service startup", stepTimer.elapsed() );

    auto* sumoConnector = app->makeSumoConnector();
    ASSERT_TRUE( sumoConnector != nullptr );

    stepTimer.restart();
    ASSERT_FALSE( sumoConnector->requestTokenBlocking().isEmpty() ) << "No Sumo access token available.";
    reportTiming( "Authentication", stepTimer.elapsed() );

    stepTimer.restart();
    sumoConnector->requestEnsembleByCasesIdBlocking( caseId );
    const auto ensembleNames = sumoConnector->ensembleNamesForCase( caseId );
    reportTiming( "Ensemble names", stepTimer.elapsed(), QString( "%1 ensembles" ).arg( ensembleNames.size() ) );
    ASSERT_FALSE( ensembleNames.empty() ) << "No ensembles found for case " << caseId.get().toStdString();

    const QString ensembleName = ensembleNames.front();

    stepTimer.restart();
    sumoConnector->requestVectorNamesForEnsembleBlocking( caseId, ensembleName );
    const auto availableVectorNames = sumoConnector->vectorNames();
    reportTiming( "Vector names", stepTimer.elapsed(), QString( "%1 vectors" ).arg( availableVectorNames.size() ) );
    ASSERT_FALSE( availableVectorNames.empty() ) << "No vector names found for ensemble " << ensembleName.toStdString();

    for ( const QString& vectorName : vectorNames )
    {
        SCOPED_TRACE( vectorName.toStdString() );

        EXPECT_NE( std::find( availableVectorNames.begin(), availableVectorNames.end(), vectorName ), availableVectorNames.end() )
            << "Vector not present in the ensemble vector list.";

        // Resolving the blob id triggers an aggregation on the Sumo backend, and is expected to dominate
        // the time spent on a vector.
        stepTimer.restart();
        sumoConnector->requestBlobIdForEnsembleBlocking( caseId, ensembleName, vectorName );
        const auto blobIds = sumoConnector->blobIds();
        reportTiming( vectorName + ": blob id", stepTimer.elapsed() );
        ASSERT_FALSE( blobIds.empty() ) << "No blob id returned for the vector.";

        stepTimer.restart();
        const QByteArray contents       = downloadBlobBlocking( sumoConnector, blobIds.back(), RiaSumoDefines::requestTimeoutMillis() );
        const qint64     downloadMillis = stepTimer.elapsed();
        reportTiming( vectorName + ": download", downloadMillis, QString( "%1 bytes" ).arg( contents.size() ) );
        ASSERT_FALSE( contents.isEmpty() ) << "Downloaded parquet data is empty.";

        stepTimer.restart();
        auto         table       = readParquetTable( contents );
        const qint64 parseMillis = stepTimer.elapsed();
        ASSERT_TRUE( table != nullptr ) << "Downloaded data could not be read as a parquet table.";
        reportTiming( vectorName + ": parquet parse",
                      parseMillis,
                      QString( "%1 rows, %2 columns" ).arg( table->num_rows() ).arg( table->num_columns() ) );
        EXPECT_GT( table->num_rows(), 0 );

        // The table holds one row per realization and time step, with the vector values in a column
        // named after the vector.
        auto dateColumn = table->GetColumnByName( "DATE" );
        ASSERT_TRUE( dateColumn != nullptr );
        EXPECT_EQ( arrow::Type::TIMESTAMP, dateColumn->type()->id() );

        auto realizationColumn = table->GetColumnByName( "REAL" );
        ASSERT_TRUE( realizationColumn != nullptr );

        auto valueColumn = table->GetColumnByName( vectorName.toStdString() );
        ASSERT_TRUE( valueColumn != nullptr ) << "No column named after the requested vector.";
        EXPECT_EQ( arrow::Type::FLOAT, valueColumn->type()->id() );

        EXPECT_EQ( table->num_rows(), valueColumn->length() );
    }

    reportTiming( "Total", totalTimer.elapsed(), QString( "%1 vectors" ).arg( vectorNames.size() ) );

    cloudApiService->stop();
}
