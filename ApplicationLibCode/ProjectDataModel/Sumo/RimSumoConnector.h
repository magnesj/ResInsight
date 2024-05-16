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

struct SumoCase
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

    void requestCasesForField( const QString& fieldName );

    QString server() const;

    std::vector<SumoField> fields() const;
    std::vector<SumoCase>  cases() const;

public slots:
    void requestToken();
    void parseCases( QNetworkReply* reply );
    void saveFile( QNetworkReply* reply, const QString& fileId );
    void accessGranted();

signals:
    void fileDownloadFinished( const QString& fileId, const QString& filePath );
    void casesFinished();
    void wellsFinished();
    void wellboresFinished( const QString& wellId );
    void wellboreTrajectoryFinished( const QString& wellboreId );
    void tokenReady( const QString& token );

private:
    void addStandardHeader( QNetworkRequest& networkRequest, const QString& token );

    QNetworkReply* makeRequest( const std::map<QString, QString>& parameters, const QString& server, const QString& token );
    QNetworkReply* makeDownloadRequest( const QString& server, const QString& id, const QString& token );

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

    QString m_token;

    std::vector<SumoField> m_fields;
    std::vector<SumoCase>  m_cases;
};
