#pragma once

#include <QtCore>

#include <QNetworkAccessManager>
#include <QOAuth2AuthorizationCodeFlow>

#include <map>

struct SumoField
{
    QString id;
    QString kind;
    QString name;
};

//==================================================================================================
///
//==================================================================================================
class RimSumoConnector : public QObject
{
    Q_OBJECT
public:
    RimSumoConnector( QObject* parent, const QString& server, const QString& authority, const QString& scopes, const QString& clientId );
    ~RimSumoConnector() override;

    void requestFieldsByName( const QString& token, const QString& fieldName );
    void requestFieldsByName( const QString& fieldName );

    QString server() const;

    std::vector<SumoField> fields() const;

public slots:
    void requestToken();
    void parseFields( QNetworkReply* reply );
    void saveFile( QNetworkReply* reply, const QString& fileId );
    void accessGranted();

signals:
    void fileDownloadFinished( const QString& fileId, const QString& filePath );
    void fieldsFinished();
    void wellsFinished();
    void wellboresFinished( const QString& wellId );
    void wellboreTrajectoryFinished( const QString& wellboreId );
    void tokenReady( const QString& token );

private:
    void addStandardHeader( QNetworkRequest& networkRequest, const QString& token );

    QNetworkReply* makeRequest( const std::map<QString, QString>& parameters, const QString& server, const QString& token );

    QNetworkReply* makeDownloadRequest( const QString& server, const QString& id, const QString& token );

    void requestFieldsByName( const QString& server, const QString& token, const QString& fieldName );

    static QString generateRandomString( int length = 20 );
    static QString constructSearchUrl( const QString& server );
    static QString constructDownloadUrl( const QString& server, const QString& fileId );
    static QString constructAuthUrl( const QString& authority );
    static QString constructTokenUrl( const QString& authority );

    QOAuth2AuthorizationCodeFlow* m_authCodeFlow;
    QNetworkAccessManager*        m_networkAccessManager;

    const QString m_server;
    const QString m_authority;
    const QString m_scopes;
    const QString m_clientId;

    QString                m_token;
    std::vector<SumoField> m_fields;
};
