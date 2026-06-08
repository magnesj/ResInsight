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

#pragma once

#include "RiaSumoConnector.h"

#include "RiaCloudDefines.h"
#include "RiaLogging.h"
#include "RiaOAuthHttpServerReplyHandler.h"
#include "RiaOsduDefines.h"
#include "RiaQStringFormatter.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaMethod>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>


//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaSumoConnector::RiaSumoConnector( QObject*       parent,
                                    const QString& server,
                                    const QString& authority,
                                    const QString& scopes,
                                    const QString& clientId,
                                    unsigned int   port )
    : RiaCloudConnector( parent, server, authority, scopes, clientId, port )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestFailed( const QAbstractOAuth::Error error )
{
    RiaLogging::error( "Request failed: " );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::parquetDownloadComplete( const QString& blobId, const QByteArray& contents, const QString& url )
{
    SumoRedirect obj;
    obj.objectId = blobId;
    obj.contents = contents;
    obj.url      = url;

    m_redirectInfo.push_back( obj );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaSumoConnector::~RiaSumoConnector()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestCasesForField( const QString& fieldName )
{
    m_cases.clear();

    QNetworkRequest m_networkRequest;
    QString url = QString( "http://localhost:8000/cases?asset_name=%1" ).arg( fieldName );
    m_networkRequest.setUrl( QUrl( url ) );
    m_networkRequest.setHeader( QNetworkRequest::ContentTypeHeader, RiaCloudDefines::contentTypeJson() );

    auto reply = m_networkAccessManager->get( m_networkRequest );

    connect( reply,
             &QNetworkReply::finished,
             [this, reply]()
             {
                 if ( reply->error() == QNetworkReply::NoError )
                 {
                     parseCases( reply );
                 }
             } );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestCasesForFieldBlocking( const QString& fieldName )
{
    auto        requestCallable = [this, fieldName] { requestCasesForField( fieldName ); };
    QMetaMethod signalMethod    = QMetaMethod::fromSignal( &RiaSumoConnector::casesFinished );
    wrapAndCallNetworkRequest( requestCallable, signalMethod );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestAssets()
{
    QNetworkRequest m_networkRequest;
    m_networkRequest.setUrl( QUrl( "http://localhost:8000/assets" ) );
    m_networkRequest.setHeader( QNetworkRequest::ContentTypeHeader, RiaCloudDefines::contentTypeJson() );

    auto reply = m_networkAccessManager->get( m_networkRequest );

    connect( reply,
             &QNetworkReply::finished,
             [this, reply]()
             {
                 if ( reply->error() == QNetworkReply::NoError )
                 {
                     parseAssets( reply );
                 }
             } );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestAssetsBlocking()
{
    auto        requestCallable = [this] { requestAssets(); };
    QMetaMethod signalMethod    = QMetaMethod::fromSignal( &RiaSumoConnector::assetsFinished );
    wrapAndCallNetworkRequest( requestCallable, signalMethod );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestEnsembleByCasesId( const SumoCaseId& caseId )
{
    QNetworkRequest m_networkRequest;
    QString url = QString( "http://localhost:8000/cases/%1/ensembles" ).arg( caseId.get() );
    m_networkRequest.setUrl( QUrl( url ) );
    m_networkRequest.setHeader( QNetworkRequest::ContentTypeHeader, RiaCloudDefines::contentTypeJson() );

    auto reply = m_networkAccessManager->get( m_networkRequest );

    connect( reply,
             &QNetworkReply::finished,
             [this, reply, caseId]()
             {
                 if ( reply->error() == QNetworkReply::NoError )
                 {
                     parseEnsembleNames( reply, caseId );
                 }
             } );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::parseEnsembleNames( QNetworkReply* reply, const SumoCaseId& caseId )
{
    QByteArray result = reply->readAll();
    reply->deleteLater();

    if ( reply->error() == QNetworkReply::NoError )
    {
        m_ensembleNames.clear();

        QJsonDocument doc = QJsonDocument::fromJson( result );
        QJsonArray jsonArray = doc.array();

        for ( const QJsonValue& value : jsonArray )
        {
            QJsonObject ensembleObj = value.toObject();
            QString ensembleName = ensembleObj["name"].toString();
            m_ensembleNames.push_back( { caseId, ensembleName } );
        }

        RiaLogging::debug( std::format( "Ensemble count : {}", m_ensembleNames.size() ) );
    }
    else
    {
        RiaLogging::error( std::format( "Request ensemble names failed: : '%s'", reply->errorString() ) );
    }

    emit ensembleNamesFinished();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestEnsembleByCasesIdBlocking( const SumoCaseId& caseId )
{
    auto        requestCallable = [this, caseId] { requestEnsembleByCasesId( caseId ); };
    QMetaMethod signalMethod    = QMetaMethod::fromSignal( &RiaSumoConnector::ensembleNamesFinished );
    wrapAndCallNetworkRequest( requestCallable, signalMethod );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestVectorNamesForEnsemble( const SumoCaseId& caseId, const QString& ensembleName )
{
    QNetworkRequest m_networkRequest;
    QString url = QString( "http://localhost:8000/cases/%1/ensembles/%2/vector_list" ).arg( caseId.get() ).arg( ensembleName );
    m_networkRequest.setUrl( QUrl( url ) );
    m_networkRequest.setHeader( QNetworkRequest::ContentTypeHeader, RiaCloudDefines::contentTypeJson() );

    auto reply = m_networkAccessManager->get( m_networkRequest );

    connect( reply,
             &QNetworkReply::finished,
             [this, reply, ensembleName, caseId]()
             {
                 // parseVectorNames handles the error case and always emits vectorNamesFinished, so the
                 // blocking caller returns immediately instead of waiting for the request to time out.
                 parseVectorNames( reply, caseId, ensembleName );
             } );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestVectorNamesForEnsembleBlocking( const SumoCaseId& caseId, const QString& ensembleName )
{
    auto        requestCallable = [this, caseId, ensembleName] { requestVectorNamesForEnsemble( caseId, ensembleName ); };
    QMetaMethod signalMethod    = QMetaMethod::fromSignal( &RiaSumoConnector::vectorNamesFinished );
    wrapAndCallNetworkRequest( requestCallable, signalMethod );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestRealizationIdsForEnsemble( const SumoCaseId& caseId, const QString& ensembleName )
{
    m_realizationIds.clear();

    QNetworkRequest m_networkRequest;
    QString url = QString( "http://localhost:8000/cases/%1/ensembles/%2/realizations" ).arg( caseId.get() ).arg( ensembleName );
    m_networkRequest.setUrl( QUrl( url ) );
    m_networkRequest.setHeader( QNetworkRequest::ContentTypeHeader, RiaCloudDefines::contentTypeJson() );

    auto reply = m_networkAccessManager->get( m_networkRequest );

    connect( reply,
             &QNetworkReply::finished,
             [this, reply, ensembleName, caseId]()
             {
                 // parseRealizationNumbers handles the error case and always emits realizationIdsFinished, so
                 // the blocking caller returns immediately instead of waiting for the request to time out.
                 parseRealizationNumbers( reply, caseId, ensembleName );
             } );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestRealizationIdsForEnsembleBlocking( const SumoCaseId& caseId, const QString& ensembleName )
{
    auto        requestCallable = [this, caseId, ensembleName] { requestRealizationIdsForEnsemble( caseId, ensembleName ); };
    QMetaMethod signalMethod    = QMetaMethod::fromSignal( &RiaSumoConnector::realizationIdsFinished );
    wrapAndCallNetworkRequest( requestCallable, signalMethod );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestGridInfoForEnsemble( const SumoCaseId& caseId, const QString& ensembleName )
{
    m_gridInfos.clear();

    QNetworkRequest m_networkRequest;

    QString encodedEnsembleName = QUrl::toPercentEncoding( ensembleName );

    QString url =
        QString( "http://localhost:8000/cases/%1/ensembles/%2/grid_info_list" ).arg( caseId.get() ).arg( encodedEnsembleName );
    m_networkRequest.setUrl( QUrl( url ) );
    m_networkRequest.setHeader( QNetworkRequest::ContentTypeHeader, RiaCloudDefines::contentTypeJson() );

    auto reply = m_networkAccessManager->get( m_networkRequest );

    connect( reply,
             &QNetworkReply::finished,
             [this, reply, ensembleName, caseId]()
             {
                 if ( reply->error() == QNetworkReply::NoError )
                 {
                     parseGridInfo( reply, caseId, ensembleName );
                 }
                 else
                 {
                     RiaLogging::error( std::format( "Request grid info failed: '{}'", reply->errorString().toStdString() ) );
                     emit gridInfoFinished();
                 }
             } );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestGridInfoForEnsembleBlocking( const SumoCaseId& caseId, const QString& ensembleName )
{
    auto        requestCallable = [this, caseId, ensembleName] { requestGridInfoForEnsemble( caseId, ensembleName ); };
    QMetaMethod signalMethod    = QMetaMethod::fromSignal( &RiaSumoConnector::gridInfoFinished );
    wrapAndCallNetworkRequest( requestCallable, signalMethod );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestGridBlobIdForEnsemble( const SumoCaseId& caseId, const QString& ensembleName, const QString& gridName, int realization )
{
    QNetworkRequest m_networkRequest;

    // Properly URL-encode the path components
    QString encodedEnsembleName = QUrl::toPercentEncoding( ensembleName );
    QString encodedGridName     = QUrl::toPercentEncoding( gridName );

    QString url = QString( "http://localhost:8000/cases/%1/ensembles/%2/grids/%3/realizations/%4/blob_url" )
                      .arg( caseId.get() )
                      .arg( encodedEnsembleName )
                      .arg( encodedGridName )
                      .arg( realization );
    m_networkRequest.setUrl( QUrl( url ) );
    m_networkRequest.setHeader( QNetworkRequest::ContentTypeHeader, RiaCloudDefines::contentTypeJson() );

    auto reply = m_networkAccessManager->get( m_networkRequest );

    connect( reply,
             &QNetworkReply::finished,
             [this, reply, ensembleName, caseId, gridName]()
             {
                 if ( reply->error() == QNetworkReply::NoError )
                 {
                     parseBlobUrl( reply, caseId, ensembleName, gridName, false );
                 }
                 else
                 {
                     RiaLogging::error( std::format( "Request grid blob URL failed: '{}'", reply->errorString().toStdString() ) );
                     emit blobIdFinished();
                 }
             } );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestGridBlobIdForEnsembleBlocking( const SumoCaseId& caseId,
                                                             const QString&    ensembleName,
                                                             const QString&    gridName,
                                                             int               realization )
{
    auto requestCallable = [this, caseId, ensembleName, gridName, realization]
    { requestGridBlobIdForEnsemble( caseId, ensembleName, gridName, realization ); };
    QMetaMethod signalMethod = QMetaMethod::fromSignal( &RiaSumoConnector::blobIdFinished );
    wrapAndCallNetworkRequest( requestCallable, signalMethod );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QByteArray
    RiaSumoConnector::requestGridDataBlocking( const SumoCaseId& caseId, const QString& ensembleName, const QString& gridName, int realization )
{
    requestGridBlobIdForEnsembleBlocking( caseId, ensembleName, gridName, realization );

    if ( m_blobUrl.empty() ) return {};

    // The REST API returns the complete blob URL, extract the blob id (last path segment).
    auto blobUrl  = m_blobUrl.back();
    auto urlParts = blobUrl.split( '/' );
    auto blobId   = urlParts.last();

    QEventLoop eventLoop;
    QTimer     timer;
    timer.setSingleShot( true );
    QObject::connect( &timer, SIGNAL( timeout() ), &eventLoop, SLOT( quit() ) );
    QObject::connect( this, SIGNAL( parquetDownloadFinished( const QByteArray&, const QString& ) ), &eventLoop, SLOT( quit() ) );

    requestBlobDownload( blobId );

    timer.start( RiaSumoDefines::requestTimeoutMillis() );
    eventLoop.exec( QEventLoop::ProcessEventsFlag::ExcludeUserInputEvents );

    for ( const auto& blobData : m_redirectInfo )
    {
        if ( blobData.objectId == blobId )
        {
            return blobData.contents;
        }
    }

    return {};
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestGridPropertyInfoForEnsemble( const SumoCaseId& caseId,
                                                           const QString&    ensembleName,
                                                           const QString&    gridName,
                                                           int               realization )
{
    m_gridPropertyInfos.clear();

    QNetworkRequest m_networkRequest;

    QString encodedEnsembleName = QUrl::toPercentEncoding( ensembleName );
    QString encodedGridName     = QUrl::toPercentEncoding( gridName );

    QString url = QString( "http://localhost:8000/cases/%1/ensembles/%2/grids/%3/realizations/%4/property_info_list" )
                      .arg( caseId.get() )
                      .arg( encodedEnsembleName )
                      .arg( encodedGridName )
                      .arg( realization );
    m_networkRequest.setUrl( QUrl( url ) );
    m_networkRequest.setHeader( QNetworkRequest::ContentTypeHeader, RiaCloudDefines::contentTypeJson() );

    auto reply = m_networkAccessManager->get( m_networkRequest );

    connect( reply,
             &QNetworkReply::finished,
             [this, reply, caseId, ensembleName, gridName, realization]()
             {
                 if ( reply->error() == QNetworkReply::NoError )
                 {
                     parseGridPropertyInfo( reply, caseId, ensembleName, gridName, realization );
                 }
                 else
                 {
                     RiaLogging::error( std::format( "Request grid property info failed: '{}'", reply->errorString().toStdString() ) );
                     emit gridPropertyInfoFinished();
                 }
             } );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestGridPropertyInfoForEnsembleBlocking( const SumoCaseId& caseId,
                                                                   const QString&    ensembleName,
                                                                   const QString&    gridName,
                                                                   int               realization )
{
    auto requestCallable = [this, caseId, ensembleName, gridName, realization]
    { requestGridPropertyInfoForEnsemble( caseId, ensembleName, gridName, realization ); };
    QMetaMethod signalMethod = QMetaMethod::fromSignal( &RiaSumoConnector::gridPropertyInfoFinished );
    wrapAndCallNetworkRequest( requestCallable, signalMethod );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestGridPropertyBlobIdForEnsemble( const SumoCaseId& caseId,
                                                             const QString&    ensembleName,
                                                             const QString&    gridName,
                                                             int               realization,
                                                             const QString&    propertyName,
                                                             const QString&    isoDateOrInterval )
{
    QNetworkRequest m_networkRequest;

    // Properly URL-encode the path components
    QString encodedEnsembleName = QUrl::toPercentEncoding( ensembleName );
    QString encodedGridName     = QUrl::toPercentEncoding( gridName );
    QString encodedPropertyName = QUrl::toPercentEncoding( propertyName );

    QString url = QString( "http://localhost:8000/cases/%1/ensembles/%2/grids/%3/realizations/%4/properties/%5/blob_url" )
                      .arg( caseId.get() )
                      .arg( encodedEnsembleName )
                      .arg( encodedGridName )
                      .arg( realization )
                      .arg( encodedPropertyName );

    // The timestamp/interval is an optional query parameter; omit it for static properties.
    if ( !isoDateOrInterval.isEmpty() )
    {
        url += QString( "?property_iso_date_or_interval=%1" ).arg( QString( QUrl::toPercentEncoding( isoDateOrInterval ) ) );
    }

    m_networkRequest.setUrl( QUrl( url ) );
    m_networkRequest.setHeader( QNetworkRequest::ContentTypeHeader, RiaCloudDefines::contentTypeJson() );

    auto reply = m_networkAccessManager->get( m_networkRequest );

    connect( reply,
             &QNetworkReply::finished,
             [this, reply, caseId, ensembleName, propertyName]()
             {
                 if ( reply->error() == QNetworkReply::NoError )
                 {
                     parseBlobUrl( reply, caseId, ensembleName, propertyName, false );
                 }
                 else
                 {
                     RiaLogging::error(
                         std::format( "Request grid property blob URL failed: '{}'", reply->errorString().toStdString() ) );
                     emit blobIdFinished();
                 }
             } );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestGridPropertyBlobIdForEnsembleBlocking( const SumoCaseId& caseId,
                                                                     const QString&    ensembleName,
                                                                     const QString&    gridName,
                                                                     int               realization,
                                                                     const QString&    propertyName,
                                                                     const QString&    isoDateOrInterval )
{
    auto requestCallable = [this, caseId, ensembleName, gridName, realization, propertyName, isoDateOrInterval]
    { requestGridPropertyBlobIdForEnsemble( caseId, ensembleName, gridName, realization, propertyName, isoDateOrInterval ); };
    QMetaMethod signalMethod = QMetaMethod::fromSignal( &RiaSumoConnector::blobIdFinished );
    wrapAndCallNetworkRequest( requestCallable, signalMethod );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QByteArray RiaSumoConnector::requestGridPropertyDataBlocking( const SumoCaseId& caseId,
                                                              const QString&    ensembleName,
                                                              const QString&    gridName,
                                                              int               realization,
                                                              const QString&    propertyName,
                                                              const QString&    isoDateOrInterval )
{
    requestGridPropertyBlobIdForEnsembleBlocking( caseId, ensembleName, gridName, realization, propertyName, isoDateOrInterval );

    if ( m_blobUrl.empty() ) return {};

    // The REST API returns the complete blob URL, extract the blob id (last path segment).
    auto blobUrl  = m_blobUrl.back();
    auto urlParts = blobUrl.split( '/' );
    auto blobId   = urlParts.last();

    QEventLoop eventLoop;
    QTimer     timer;
    timer.setSingleShot( true );
    QObject::connect( &timer, SIGNAL( timeout() ), &eventLoop, SLOT( quit() ) );
    QObject::connect( this, SIGNAL( parquetDownloadFinished( const QByteArray&, const QString& ) ), &eventLoop, SLOT( quit() ) );

    requestBlobDownload( blobId );

    timer.start( RiaSumoDefines::requestTimeoutMillis() );
    eventLoop.exec( QEventLoop::ProcessEventsFlag::ExcludeUserInputEvents );

    for ( const auto& blobData : m_redirectInfo )
    {
        if ( blobData.objectId == blobId )
        {
            return blobData.contents;
        }
    }

    return {};
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QByteArray RiaSumoConnector::requestParametersParquetDataBlocking( const SumoCaseId& caseId, const QString& ensembleName )
{
    requestParametersBlobIdForEnsembleBlocking( caseId, ensembleName );

    if ( m_blobUrl.empty() ) return {};

    auto blobUrl = m_blobUrl.back();

    // Extract blob id from url, split string on "/", and get the last string in split
    auto urlParts = blobUrl.split( '/' );
    auto blobId = urlParts.last();

    QEventLoop eventLoop;
    QTimer     timer;
    timer.setSingleShot( true );
    QObject::connect( &timer, SIGNAL( timeout() ), &eventLoop, SLOT( quit() ) );
    QObject::connect( this, SIGNAL( parquetDownloadFinished( const QByteArray&, const QString& ) ), &eventLoop, SLOT( quit() ) );

    requestBlobDownload( blobId );

    timer.start( RiaSumoDefines::requestTimeoutMillis() );
    eventLoop.exec( QEventLoop::ProcessEventsFlag::ExcludeUserInputEvents );

    for ( const auto& blobData : m_redirectInfo )
    {
        if ( blobData.objectId == blobId )
        {
            return blobData.contents;
        }
    }

    return {};
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestParametersBlobIdForEnsembleBlocking( const SumoCaseId& caseId, const QString& ensembleName )
{
    auto        requestCallable = [this, caseId, ensembleName] { requestParametersBlobIdForEnsemble( caseId, ensembleName ); };
    QMetaMethod signalMethod    = QMetaMethod::fromSignal( &RiaSumoConnector::blobIdFinished );
    wrapAndCallNetworkRequest( requestCallable, signalMethod );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestParametersBlobIdForEnsemble( const SumoCaseId& caseId, const QString& ensembleName )
{
    QNetworkRequest m_networkRequest;

    // Properly URL-encode the path components
    QString encodedEnsembleName = QUrl::toPercentEncoding( ensembleName );

    QString url =
        QString( "http://localhost:8000/cases/%1/ensembles/%2/parameters/blob_url" ).arg( caseId.get() ).arg( encodedEnsembleName );
    m_networkRequest.setUrl( QUrl( url ) );
    m_networkRequest.setHeader( QNetworkRequest::ContentTypeHeader, RiaCloudDefines::contentTypeJson() );

    auto reply = m_networkAccessManager->get( m_networkRequest );

    connect( reply,
             &QNetworkReply::finished,
             [this, reply, ensembleName, caseId]()
             {
                 // parseBlobUrl handles the error case and always emits blobIdFinished, so the blocking
                 // caller returns immediately instead of waiting for the request to time out.
                 parseBlobUrl( reply, caseId, ensembleName, "", true );
             } );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestBlobIdForEnsemble( const SumoCaseId& caseId, const QString& ensembleName, const QString& vectorName )
{
    QNetworkRequest m_networkRequest;

    // Properly URL-encode the path components
    QString encodedVectorName = QUrl::toPercentEncoding( vectorName );
    QString encodedEnsembleName = QUrl::toPercentEncoding( ensembleName );

    QString url = QString( "http://localhost:8000/cases/%1/ensembles/%2/vectors/%3/blob_url" )
                      .arg( caseId.get() )
                      .arg( encodedEnsembleName )
                      .arg( encodedVectorName );
    m_networkRequest.setUrl( QUrl( url ) );
    m_networkRequest.setHeader( QNetworkRequest::ContentTypeHeader, RiaCloudDefines::contentTypeJson() );

    auto reply = m_networkAccessManager->get( m_networkRequest );

    connect( reply,
             &QNetworkReply::finished,
             [this, reply, ensembleName, caseId, vectorName]()
             {
                 // parseBlobUrl handles the error case and always emits blobIdFinished, so the blocking
                 // caller returns immediately instead of waiting for the request to time out.
                 parseBlobUrl( reply, caseId, ensembleName, vectorName, false );  // false = vector data
             } );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestBlobIdForEnsembleBlocking( const SumoCaseId& caseId, const QString& ensembleName, const QString& vectorName )
{
    auto requestCallable     = [this, caseId, ensembleName, vectorName] { requestBlobIdForEnsemble( caseId, ensembleName, vectorName ); };
    QMetaMethod signalMethod = QMetaMethod::fromSignal( &RiaSumoConnector::blobIdFinished );
    wrapAndCallNetworkRequest( requestCallable, signalMethod );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestBlobDownload( const QString& blobId )
{
    requestTokenBlocking();

    QString url = constructDownloadUrl( m_server, blobId );

    QNetworkRequest networkRequest;
    networkRequest.setUrl( url );

    // Other redirection policies are NoLessSafeRedirectPolicy, SameOriginRedirectPolicy, UserVerifiedRedirectPolicy. They were tested, but
    // did not work. Use ManualRedirectPolicy instead, and inspect the reply for the redirection target.
    networkRequest.setAttribute( QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy );

    addStandardHeader( networkRequest, token(), RiaCloudDefines::contentTypeJson() );

    auto reply = m_networkAccessManager->get( networkRequest );

    connect( reply,
             &QNetworkReply::finished,
             [this, reply, blobId, url]()
             {
                 if ( reply->error() == QNetworkReply::NoError )
                 {
                     auto contents = reply->readAll();

                     QVariant redirectUrl = reply->attribute( QNetworkRequest::RedirectionTargetAttribute );
                     if ( redirectUrl.isValid() )
                     {
                         requestBlobByRedirectUri( blobId, redirectUrl.toString() );
                     }
                     else
                     {
                         QString errorMessage = "Not able to parse and interpret valid redirect Url";
                         RiaLogging::error( errorMessage.toStdString() );
                     }
                 }
                 else
                 {
                     QString errorMessage = "Download failed: " + url + " failed." + reply->errorString();
                     RiaLogging::error( errorMessage.toStdString() );
                 }
             } );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestBlobByRedirectUri( const QString& blobId, const QString& redirectUri )
{
    RiaLogging::debug( std::format( "Requesting blob. Id: {} Redirect URL: {}", blobId, redirectUri ) );

    // Always request token for authentication
    requestTokenBlocking();

    QNetworkRequest networkRequest;
    networkRequest.setUrl( redirectUri );

    auto reply = m_networkAccessManager->get( networkRequest );

    connect( reply,
             &QNetworkReply::finished,
             [this, reply, blobId, redirectUri]()
             {
                 if ( reply->error() == QNetworkReply::NoError )
                 {
                     // Check response attributes
                     auto statusCode = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute ).toInt();
                     auto contentLength = reply->header( QNetworkRequest::ContentLengthHeader ).toLongLong();
                     auto bytesAvailable = reply->bytesAvailable();

                     RiaLogging::debug( std::format( "Response: status={}, content-length={}, bytes-available={}", 
                                                     statusCode, contentLength, bytesAvailable ) );

                     auto contents = reply->readAll();

                     RiaLogging::debug( std::format( "Read {} bytes from reply", contents.size() ) );

                     QString msg = "Received data from : " + redirectUri;
                     RiaLogging::info( msg.toStdString() );

                     parquetDownloadComplete( blobId, contents, redirectUri );

                     emit parquetDownloadFinished( contents, redirectUri );
                 }
                 else
                 {
                     QString errorMessage = "Download failed: " + redirectUri + " failed." + reply->errorString();
                     RiaLogging::error( errorMessage.toStdString() );

                     emit parquetDownloadFinished( {}, redirectUri );
                 }
             } );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QByteArray RiaSumoConnector::requestParquetDataBlocking( const SumoCaseId& caseId, const QString& ensembleName, const QString& vectorName )
{
    requestBlobIdForEnsembleBlocking( caseId, ensembleName, vectorName );

    if ( m_blobUrl.empty() ) return {};

    // The REST API now returns the complete blob URL, not just an ID
    auto blobUrl = m_blobUrl.back();

    // Extract blob id from url, split string on "/", and get the last string in split
    auto urlParts = blobUrl.split( '/' );
    auto blobId   = urlParts.last();

    QEventLoop eventLoop;
    QTimer     timer;
    timer.setSingleShot( true );
    QObject::connect( &timer, SIGNAL( timeout() ), &eventLoop, SLOT( quit() ) );
    QObject::connect( this, SIGNAL( parquetDownloadFinished( const QByteArray&, const QString& ) ), &eventLoop, SLOT( quit() ) );

    requestBlobDownload( blobId );

    timer.start( RiaSumoDefines::requestTimeoutMillis() );
    eventLoop.exec( QEventLoop::ProcessEventsFlag::ExcludeUserInputEvents );

    for ( const auto& blobData : m_redirectInfo )
    {
        if ( blobData.objectId == blobId )
        {
            return blobData.contents;
        }
    }

    return {};
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
QString RiaSumoConnector::constructSearchUrl( const QString& server )
{
    return server + "/api/v1/search";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiaSumoConnector::constructDownloadUrl( const QString& server, const QString& blobId )
{
    return server + "/api/v1/objects('" + blobId + "')/blob";
    // https: // main-sumo-prod.radix.equinor.com/api/v1/objects('76d6d11f-2278-3fe2-f12f-77142ad163c6')/blob
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::wrapAndCallNetworkRequest( std::function<void()> requestCallable, const QMetaMethod& signalMethod )
{
    QEventLoop eventLoop;

    QTimer timer;
    timer.setSingleShot( true );

    QObject::connect( &timer, &QTimer::timeout, [] { RiaLogging::error( "Sumo request timed out." ); } );
    QObject::connect( &timer, &QTimer::timeout, &eventLoop, &QEventLoop::quit );

    // Not able to use the modern connect syntax here, as the signal is communicated as a QMetaMethod
    int         methodIndex = eventLoop.metaObject()->indexOfMethod( "quit()" );
    QMetaMethod quitMethod  = eventLoop.metaObject()->method( methodIndex );
    QObject::connect( this, signalMethod, &eventLoop, quitMethod );

    // Call the function that will execute the request
    requestCallable();

    timer.start( RiaSumoDefines::requestTimeoutMillis() );
    eventLoop.exec( QEventLoop::ProcessEventsFlag::ExcludeUserInputEvents );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QNetworkReply* RiaSumoConnector::makeRequest( const std::map<QString, QString>& parameters, const QString& server, const QString& token )
{
    QNetworkRequest m_networkRequest;
    m_networkRequest.setUrl( QUrl( constructSearchUrl( server ) ) );

    addStandardHeader( m_networkRequest, token, RiaCloudDefines::contentTypeJson() );

    QJsonObject obj;
    for ( auto [key, value] : parameters )
    {
        obj.insert( key, value );
    }

    QJsonDocument doc( obj );
    QString       strJson( doc.toJson( QJsonDocument::Compact ) );

    auto reply = m_networkAccessManager->post( m_networkRequest, strJson.toUtf8() );
    return reply;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::parseAssets( QNetworkReply* reply )
{
    QByteArray result = reply->readAll();
    reply->deleteLater();

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJsonDocument doc = QJsonDocument::fromJson( result );
        QJsonArray jsonArray = doc.array();

        m_assets.clear();

        // This json is an array of AssetInfo
        for ( const QJsonValue& assetInfo : jsonArray )
        {
            QString assetName = assetInfo["name"].toString();
            m_assets.push_back( SumoAsset{ SumoAssetId( "" ), "", assetName } );
        }

        for ( auto a : m_assets )
        {
            RiaLogging::info( std::format( "Asset: {}", a.name ) );
        }
    }
    else
    {
        RiaLogging::error( std::format( "Request assets failed: : '%s'", reply->errorString() ) );
    }

    emit assetsFinished();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::parseCases( QNetworkReply* reply )
{
    QByteArray result = reply->readAll();
    reply->deleteLater();

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJsonDocument doc = QJsonDocument::fromJson( result );
        QJsonArray jsonArray = doc.array();

        m_cases.clear();

        for ( const QJsonValue& value : jsonArray )
        {
            QJsonObject caseObj = value.toObject();

            QString id   = caseObj["id"].toString();
            QString kind = "";
            QString name = caseObj["name"].toString();
            m_cases.push_back( SumoCase{ SumoCaseId( id ), kind, name } );
        }

        RiaLogging::debug( std::format( "Case count : {}", m_cases.size() ) );
    }
    else
    {
        RiaLogging::error( std::format( "Request cases failed: : '%s'", reply->errorString() ) );
    }

    emit casesFinished();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::parseVectorNames( QNetworkReply* reply, const SumoCaseId& caseId, const QString& ensembleName )
{
    QByteArray result = reply->readAll();
    reply->deleteLater();

    m_vectorNames.clear();

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJsonDocument doc = QJsonDocument::fromJson( result );
        QJsonArray jsonArray = doc.array();

        for ( const QJsonValue& value : jsonArray )
        {
            QJsonObject vectorObj = value.toObject();
            QString vectorName = vectorObj["name"].toString();
            m_vectorNames.push_back( vectorName );
        }
    }
    else
    {
        RiaLogging::error( std::format( "Request vector names failed: : '%s'", reply->errorString() ) );
    }

    emit vectorNamesFinished();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::parseRealizationNumbers( QNetworkReply* reply, const SumoCaseId& caseId, const QString& ensembleName )
{
    QByteArray result = reply->readAll();
    reply->deleteLater();

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJsonDocument doc = QJsonDocument::fromJson( result );
        QJsonArray jsonArray = doc.array();

        for ( const QJsonValue& value : jsonArray )
        {
            int intValue = value.toInt();
            auto realizationId = QString::number( intValue );
            m_realizationIds.push_back( realizationId );
        }
    }
    else
    {
        RiaLogging::error( std::format( "Request realization IDs failed: '%s'", reply->errorString() ) );
    }

    emit realizationIdsFinished();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::parseGridInfo( QNetworkReply* reply, const SumoCaseId& caseId, const QString& ensembleName )
{
    QByteArray result = reply->readAll();
    reply->deleteLater();

    m_gridInfos.clear();

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJsonDocument doc       = QJsonDocument::fromJson( result );
        QJsonArray    jsonArray = doc.array();

        for ( const QJsonValue& value : jsonArray )
        {
            QJsonObject gridObj = value.toObject();

            SumoGridInfo gridInfo;
            gridInfo.name = gridObj["name"].toString();

            for ( const QJsonValue& realizationValue : gridObj["realizations"].toArray() )
            {
                gridInfo.realizations.push_back( realizationValue.toInt() );
            }

            m_gridInfos.push_back( gridInfo );
        }

        RiaLogging::debug( std::format( "Grid info count : {}", m_gridInfos.size() ) );
    }
    else
    {
        RiaLogging::error( std::format( "Request grid info failed: '{}'", reply->errorString().toStdString() ) );
    }

    emit gridInfoFinished();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::parseGridPropertyInfo( QNetworkReply*    reply,
                                              const SumoCaseId& caseId,
                                              const QString&    ensembleName,
                                              const QString&    gridName,
                                              int               realization )
{
    QByteArray result = reply->readAll();
    reply->deleteLater();

    m_gridPropertyInfos.clear();

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJsonDocument doc       = QJsonDocument::fromJson( result );
        QJsonArray    jsonArray = doc.array();

        for ( const QJsonValue& value : jsonArray )
        {
            QJsonObject propertyObj = value.toObject();

            SumoGridPropertyInfo propertyInfo;
            propertyInfo.name = propertyObj["propertyName"].toString();

            // isoDateOrInterval is null for static properties.
            const auto isoValue = propertyObj["isoDateOrInterval"];
            if ( !isoValue.isNull() ) propertyInfo.isoDateOrInterval = isoValue.toString();

            m_gridPropertyInfos.push_back( propertyInfo );
        }

        RiaLogging::debug( std::format( "Grid property info count : {}", m_gridPropertyInfos.size() ) );
    }
    else
    {
        RiaLogging::error( std::format( "Request grid property info failed: '{}'", reply->errorString().toStdString() ) );
    }

    emit gridPropertyInfoFinished();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::parseBlobUrl( QNetworkReply*    reply,
                                     const SumoCaseId& caseId,
                                     const QString&    ensembleName,
                                     const QString&    vectorName,
                                     bool              isParameters )
{
    QByteArray result = reply->readAll();
    reply->deleteLater();

    m_blobUrl.clear();

    if ( reply->error() == QNetworkReply::NoError )
    {
        // The REST API returns a plain string (the blob URL)
        QString blobUrl = QString::fromUtf8( result ).trimmed();

        // Remove quotes if present (FastAPI returns strings with quotes)
        if ( blobUrl.startsWith( '"' ) && blobUrl.endsWith( '"' ) )
        {
            blobUrl = blobUrl.mid( 1, blobUrl.length() - 2 );
        }

        m_blobUrl.push_back( blobUrl );

        // Context-aware logging
        if ( isParameters )
        {
            RiaLogging::debug( std::format( "Received blob URL for parameters: {}", blobUrl.toStdString() ) );
        }
        else
        {
            RiaLogging::debug( std::format( "Received blob URL for vector '{}': {}", vectorName.toStdString(), blobUrl.toStdString() ) );
        }
    }
    else
    {
        // Context-aware error logging
        QString errorContext = isParameters ? "parameters" : QString( "vector '%1'" ).arg( vectorName );
        RiaLogging::error( std::format( "Request blob URL failed for {}: {}", 
                                       errorContext.toStdString(), 
                                       reply->errorString().toStdString() ) );
    }

    emit blobIdFinished();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::parseBlobIds( QNetworkReply*    reply,
                                     const SumoCaseId& caseId,
                                     const QString&    ensembleName,
                                     const QString&    vectorName,
                                     bool              isParameters )
{
    // TODO: REMOVE


    /*QByteArray result = reply->readAll();
    reply->deleteLater();

    m_blobUrl.clear();

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJsonDocument doc     = QJsonDocument::fromJson( result );
        QJsonObject   jsonObj = doc.object();

        QJsonObject rootHits    = jsonObj["hits"].toObject();
        QJsonArray  hitsObjects = rootHits["hits"].toArray();

        for ( const QJsonValue& value : hitsObjects )
        {
            QJsonObject resultObj = value.toObject();
            QJsonObject sourceObj = resultObj["_source"].toObject();
            QJsonObject fmuObj    = sourceObj["_sumo"].toObject();

            auto blobName = fmuObj["blob_name"].toString();
            m_blobUrl.push_back( blobName );
        }
    }
    else
    {
        QString errorContext = isParameters ? "parameters" : QString( "vector '%1'" ).arg( vectorName );
        RiaLogging::error( std::format( "Request blob IDs failed for {}: {}", 
                                       errorContext.toStdString(), 
                                       reply->errorString().toStdString() ) );
    }

    emit blobIdFinished();*/
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::addStandardHeader( QNetworkRequest& networkRequest, const QString& token, const QString& contentType )
{
    networkRequest.setHeader( QNetworkRequest::ContentTypeHeader, contentType );
    networkRequest.setRawHeader( "Authorization", "Bearer " + token.toUtf8() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QNetworkReply* RiaSumoConnector::makeDownloadRequest( const QString& url, const QString& token, const QString& contentType )
{
    QNetworkRequest m_networkRequest;
    m_networkRequest.setUrl( QUrl( url ) );

    addStandardHeader( m_networkRequest, token, contentType );

    auto reply = m_networkAccessManager->get( m_networkRequest );
    return reply;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoConnector::requestParquetData( const QString& url, const QString& token )
{
    RiaLogging::info( "Requesting download of parquet from: " + url.toStdString() );

    auto reply = makeDownloadRequest( url, token, RiaCloudDefines::contentTypeJson() );
    connect( reply,
             &QNetworkReply::finished,
             [this, reply, url]()
             {
                 if ( reply->error() == QNetworkReply::NoError )
                 {
                     QByteArray contents = reply->readAll();
                     RiaLogging::info( std::format( "Download succeeded: {} bytes.", contents.length() ) );
                     RiaLogging::info( std::format( "Download succeeded for url: {}", url.toStdString() ) );
                     emit parquetDownloadFinished( contents, "" );
                 }
                 else
                 {
                     QString errorMessage = "Download failed: " + url + " failed." + reply->errorString();
                     RiaLogging::error( errorMessage.toStdString() );
                     emit parquetDownloadFinished( QByteArray(), errorMessage );
                 }
             } );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<SumoAsset> RiaSumoConnector::assets() const
{
    return m_assets;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<SumoCase> RiaSumoConnector::cases() const
{
    return m_cases;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<QString> RiaSumoConnector::ensembleNamesForCase( const SumoCaseId& caseId ) const
{
    std::vector<QString> ensembleNames;
    for ( const auto& ensemble : m_ensembleNames )
    {
        if ( ensemble.caseId == caseId )
        {
            ensembleNames.push_back( ensemble.name );
        }
    }
    return ensembleNames;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<QString> RiaSumoConnector::vectorNames() const
{
    return m_vectorNames;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<QString> RiaSumoConnector::realizationIds() const
{
    return m_realizationIds;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<SumoGridInfo> RiaSumoConnector::gridInfos() const
{
    return m_gridInfos;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<SumoGridPropertyInfo> RiaSumoConnector::gridPropertyInfos() const
{
    return m_gridPropertyInfos;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<QString> RiaSumoConnector::blobUrls() const
{
    return m_blobUrl;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<SumoRedirect> RiaSumoConnector::blobContents() const
{
    return m_redirectInfo;
}
