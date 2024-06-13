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

#include "RimSumoConnector.h"
#include "RiaFileDownloader.h"
#include "RiaLogging.h"

#include "OsduImportCommands/RiaOsduOAuthHttpServerReplyHandler.h"
#include <QAbstractOAuth>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QOAuth2AuthorizationCodeFlow>
#include <QOAuthHttpServerReplyHandler>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSumoConnector::RimSumoConnector( QObject* parent, const QString& server, const QString& authority, const QString& scopes, const QString& clientId )
    : QObject( parent )
    , m_server( server )
    , m_authority( authority )
    , m_scopes( scopes )
    , m_clientId( clientId )
{
    m_networkAccessManager = new QNetworkAccessManager( this );

    m_authCodeFlow = new QOAuth2AuthorizationCodeFlow( this );

    RiaLogging::debug( "SSL BUILD VERSION: " + QSslSocket::sslLibraryBuildVersionString() );
    RiaLogging::debug( "SSL VERSION STRING: " + QSslSocket::sslLibraryVersionString() );

    int port = 35327;

    connect( m_authCodeFlow,
             &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,
             []( QUrl url )
             {
                 RiaLogging::info( "Authorize with url: " + url.toString() );
                 QUrlQuery query( url );
                 url.setQuery( query );
                 QDesktopServices::openUrl( url );
             } );

    QString authUrl = constructAuthUrl( m_authority );
    m_authCodeFlow->setAuthorizationUrl( QUrl( authUrl ) );

    QString tokenUrl = constructTokenUrl( m_authority );
    m_authCodeFlow->setAccessTokenUrl( QUrl( tokenUrl ) );

    // App key
    m_authCodeFlow->setClientIdentifier( m_clientId );
    m_authCodeFlow->setScope( m_scopes );

    auto replyHandler = new RiaOsduOAuthHttpServerReplyHandler( port, this );
    m_authCodeFlow->setReplyHandler( replyHandler );

    connect( m_authCodeFlow, SIGNAL( granted() ), this, SLOT( accessGranted() ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoConnector::accessGranted()
{
    m_token = m_authCodeFlow->token();
    emit tokenReady( m_token );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoConnector::requestToken()
{
    RiaLogging::debug( "Requesting token." );
    m_authCodeFlow->grant();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSumoConnector::~RimSumoConnector()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoConnector::setToken( const QString& token )
{
    m_token = token;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimSumoConnector::token() const
{
    return m_token;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoConnector::requestCasesForField( const QString& fieldName )
{
    QNetworkRequest m_networkRequest;
    m_networkRequest.setUrl( QUrl( constructSearchUrl( m_server ) ) );

    addStandardHeader( m_networkRequest, m_token );

    QString payload_caseCountForFieldname = R"(
{"size":0,"query":{"bool":{"must_not":{"exists":{"field":"_sumo.parent_object"}},"filter":[{"term":{"access.asset.name.keyword":"Drogon"}}]}},"aggs":{"masterdata.smda.field.identifier.keyword":{"terms":{"field":"masterdata.smda.field.identifier.keyword","size":500,"order":{"_key":"asc"}}}}}
)";

    QString payload_fieldNames = R"(
{"query":{"bool":{"filter":[{"term":{"class.keyword":"case"}},{"term":{"access.asset.name.keyword":"Drogon"}}]}},"sort":[{"tracklog.datetime":{"order":"desc"}}],"track_total_hits":true,"size":20,"from":0}
)";

    auto reply = m_networkAccessManager->post( m_networkRequest, payload_fieldNames.toUtf8() );

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
void RimSumoConnector::requestAssets()
{
    QNetworkRequest m_networkRequest;
    m_networkRequest.setUrl( QUrl( m_server + "/api/v1/userpermissions" ) );

    addStandardHeader( m_networkRequest, m_token );

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
//
//--------------------------------------------------------------------------------------------------
QString RimSumoConnector::constructSearchUrl( const QString& server )
{
    return server + "/api/v1/search";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimSumoConnector::constructDownloadUrl( const QString& server, const QString& fileId )
{
    return server + "/api/file/v2/files/" + fileId + "/downloadURL";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimSumoConnector::constructAuthUrl( const QString& authority )
{
    return authority + "/oauth2/v2.0/authorize";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimSumoConnector::constructTokenUrl( const QString& authority )
{
    return authority + "/oauth2/v2.0/token";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QNetworkReply* RimSumoConnector::makeRequest( const std::map<QString, QString>& parameters, const QString& server, const QString& token )
{
    QNetworkRequest m_networkRequest;
    m_networkRequest.setUrl( QUrl( constructSearchUrl( server ) ) );

    addStandardHeader( m_networkRequest, token );

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
void RimSumoConnector::parseAssets( QNetworkReply* reply )
{
    QByteArray result = reply->readAll();
    reply->deleteLater();

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJsonDocument doc     = QJsonDocument::fromJson( result );
        QJsonObject   jsonObj = doc.object();

        m_assets.clear();

        for ( auto key : jsonObj.keys() )
        {
            QString id;
            QString kind;
            QString fieldName = key;
            m_assets.push_back( SumoAsset{ id, kind, fieldName } );
        }

        for ( auto a : m_assets )
        {
            RiaLogging::info( QString( "Asset: %1" ).arg( a.name ) );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoConnector::parseCases( QNetworkReply* reply )
{
    QByteArray result = reply->readAll();
    reply->deleteLater();

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJsonDocument doc      = QJsonDocument::fromJson( result );
        QJsonObject   jsonObj  = doc.object();
        QJsonObject   rootHits = jsonObj["hits"].toObject();

        QJsonArray hitsObjects = rootHits["hits"].toArray();

        m_cases.clear();

        foreach ( const QJsonValue& value, hitsObjects )
        {
            QJsonObject resultObj = value.toObject();
            auto        keys_1    = resultObj.keys();

            QJsonObject sourceObj  = resultObj["_source"].toObject();
            auto        sourceKeys = sourceObj.keys();

            QJsonObject fmuObj     = sourceObj["fmu"].toObject();
            auto        fmuObjKeys = fmuObj.keys();

            QJsonObject fmuCase     = fmuObj["case"].toObject();
            auto        fmuCaseKeys = fmuCase.keys();

            QString id        = resultObj["_id"].toString();
            QString kind      = "";
            QString fieldName = fmuCase["name"].toString();
            m_cases.push_back( SumoCase{ id, kind, fieldName } );
        }

        emit casesFinished();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoConnector::saveFile( QNetworkReply* reply, const QString& fileId )
{
    QByteArray result = reply->readAll();
    reply->deleteLater();

    if ( reply->error() == QNetworkReply::NoError )
    {
        QEventLoop loop;

        QJsonDocument doc     = QJsonDocument::fromJson( result );
        QJsonObject   jsonObj = doc.object();

        QString signedUrl = jsonObj["SignedUrl"].toString();

        RiaFileDownloader* downloader = new RiaFileDownloader;
        QUrl               url( signedUrl );
        QString            filePath = "/tmp/" + generateRandomString( 30 ) + ".txt";

        QString formattedJsonString = doc.toJson( QJsonDocument::Indented );

        RiaLogging::info( QString( "File download: %1 => %2" ).arg( signedUrl ).arg( filePath ) );
        connect( this, SIGNAL( fileDownloadFinished( const QString&, const QString& ) ), &loop, SLOT( quit() ) );
        connect( downloader,
                 &RiaFileDownloader::done,
                 [this, fileId, filePath]()
                 {
                     RiaLogging::info( QString( "Download complete %1 => %2" ).arg( fileId ).arg( filePath ) );
                     emit( fileDownloadFinished( fileId, filePath ) );
                 } );
        RiaLogging::info( "Starting download" );
        downloader->downloadFile( url, filePath );

        downloader->deleteLater();
        loop.exec();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoConnector::addStandardHeader( QNetworkRequest& networkRequest, const QString& token )
{
    networkRequest.setHeader( QNetworkRequest::ContentTypeHeader, "application/json" );
    networkRequest.setRawHeader( "Authorization", "Bearer " + token.toUtf8() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QNetworkReply* RimSumoConnector::makeDownloadRequest( const QString& server, const QString& id, const QString& token )
{
    QNetworkRequest m_networkRequest;

    QString url = constructDownloadUrl( server, id );

    m_networkRequest.setUrl( QUrl( url ) );

    addStandardHeader( m_networkRequest, token );

    auto reply = m_networkAccessManager->get( m_networkRequest );
    return reply;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimSumoConnector::generateRandomString( int randomStringLength )
{
    const QString possibleCharacters( "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" );
    QString       randomString;
    for ( int i = 0; i < randomStringLength; ++i )
    {
        quint32 value    = QRandomGenerator::global()->generate();
        int     index    = value % possibleCharacters.length();
        QChar   nextChar = possibleCharacters.at( index );
        randomString.append( nextChar );
    }
    return randomString;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimSumoConnector::server() const
{
    return m_server;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<SumoAsset> RimSumoConnector::assets() const
{
    return m_assets;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<SumoCase> RimSumoConnector::cases() const
{
    return m_cases;
}
