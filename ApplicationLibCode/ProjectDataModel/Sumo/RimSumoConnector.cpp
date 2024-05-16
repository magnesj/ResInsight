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
void RimSumoConnector::requestFieldsByName( const QString& token, const QString& fieldName )
{
    requestFieldsByName( m_server, token, fieldName );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoConnector::requestFieldsByName( const QString& fieldName )
{
    requestFieldsByName( m_token, fieldName );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSumoConnector::requestFieldsByName( const QString& server, const QString& token, const QString& fieldName )
{
    std::map<QString, QString> params;
    params["kind"]  = "kind";
    params["limit"] = "10000";
    params["query"] = "data.FieldName:" + fieldName;

    auto reply = makeRequest( params, server, token );
    connect( reply,
             &QNetworkReply::finished,
             [this, reply]()
             {
                 if ( reply->error() == QNetworkReply::NoError )
                 {
                     parseFields( reply );
                 }
             } );
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
QString RimSumoConnector::constructSearchUrl( const QString& server )
{
    return server + "/api/search/v2/query";
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
void RimSumoConnector::parseFields( QNetworkReply* reply )
{
    QByteArray result = reply->readAll();
    reply->deleteLater();

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJsonDocument doc          = QJsonDocument::fromJson( result );
        QJsonObject   jsonObj      = doc.object();
        QJsonArray    resultsArray = jsonObj["results"].toArray();

        m_fields.clear();

        foreach ( const QJsonValue& value, resultsArray )
        {
            QJsonObject resultObj = value.toObject();

            QString id        = resultObj["id"].toString();
            QString kind      = resultObj["kind"].toString();
            QString fieldName = resultObj["data"].toObject()["FieldName"].toString();
            m_fields.push_back( SumoField{ id, kind, fieldName } );
        }

        emit fieldsFinished();
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
std::vector<SumoField> RimSumoConnector::fields() const
{
    return m_fields;
}
