/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2024- Equinor ASA
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

#include "RiaSumoExplore.h"

#include "RiaLogging.h"
#include "RiaQStringFormatter.h"
#include "RiaSumoConnector.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaSumoExplore::RiaSumoExplore( RiaSumoConnector& connector )
    : m_connector( connector )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<SumoAsset> RiaSumoExplore::assets()
{
    return parseAssets( m_connector.getBlocking( "/assets", "Loading assets from Sumo" ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<SumoCase> RiaSumoExplore::cases( const QString& assetName )
{
    const QString path = QString( "/cases?asset_name=%1" ).arg( QString( QUrl::toPercentEncoding( assetName ) ) );

    return parseCases( m_connector.getBlocking( path, QString( "Loading the cases of %1 from Sumo" ).arg( assetName ) ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<QString> RiaSumoExplore::ensembleNames( const SumoCaseId& caseId )
{
    const QString path = QString( "/cases/%1/ensembles" ).arg( caseId.get() );

    return parseEnsembleNames( m_connector.getBlocking( path, "Loading ensembles from Sumo" ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<QString> RiaSumoExplore::realizationIds( const SumoCaseId& caseId, const QString& ensembleName )
{
    const QString encodedEnsembleName = QUrl::toPercentEncoding( ensembleName );

    const QString path = QString( "/cases/%1/ensembles/%2/realizations" ).arg( caseId.get() ).arg( encodedEnsembleName );

    return parseRealizationIds( m_connector.getBlocking( path, "Loading realizations from Sumo" ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiaSumoExplore::userName()
{
    return parseUserName( m_connector.getBlocking( "/whoami" ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoExplore::casesAsync( const QString&                                             assetName,
                                 const std::function<void( const std::vector<SumoCase>& )>& onFinished,
                                 const void*                                                cancelGroup )
{
    const QString path = QString( "/cases?asset_name=%1" ).arg( QString( QUrl::toPercentEncoding( assetName ) ) );

    m_connector.getAsync( path, [onFinished]( const QByteArray& body ) { onFinished( parseCases( body ) ); }, cancelGroup );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoExplore::ensembleNamesAsync( const SumoCaseId&                                         caseId,
                                         const std::function<void( const std::vector<QString>& )>& onFinished,
                                         const void*                                               cancelGroup )
{
    const QString path = QString( "/cases/%1/ensembles" ).arg( caseId.get() );

    m_connector.getAsync( path, [onFinished]( const QByteArray& body ) { onFinished( parseEnsembleNames( body ) ); }, cancelGroup );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoExplore::userNameAsync( const std::function<void( const QString& )>& onFinished, const void* cancelGroup )
{
    m_connector.getAsync( "/whoami", [onFinished]( const QByteArray& body ) { onFinished( parseUserName( body ) ); }, cancelGroup );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoExplore::assetsAsync( const std::function<void( const std::vector<SumoAsset>& )>& onFinished, const void* cancelGroup )
{
    m_connector.getAsync( "/assets", [onFinished]( const QByteArray& body ) { onFinished( parseAssets( body ) ); }, cancelGroup );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiaSumoExplore::parseUserName( const QByteArray& body )
{
    return QJsonDocument::fromJson( body ).object()["name"].toString();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<SumoAsset> RiaSumoExplore::parseAssets( const QByteArray& body )
{
    std::vector<SumoAsset> assets;

    QJsonDocument doc       = QJsonDocument::fromJson( body );
    QJsonArray    jsonArray = doc.array();

    // This json is an array of AssetInfo
    for ( const QJsonValue& assetInfo : jsonArray )
    {
        QString assetName = assetInfo["name"].toString();
        assets.push_back( SumoAsset{ SumoAssetId( "" ), "", assetName } );
    }

    for ( const auto& asset : assets )
    {
        RiaLogging::debug( std::format( "Asset: {}", asset.name ) );
    }

    return assets;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<SumoCase> RiaSumoExplore::parseCases( const QByteArray& body )
{
    std::vector<SumoCase> cases;

    QJsonDocument doc       = QJsonDocument::fromJson( body );
    QJsonArray    jsonArray = doc.array();

    for ( const QJsonValue& value : jsonArray )
    {
        QJsonObject caseObj = value.toObject();

        SumoCase sumoCase;
        sumoCase.caseId        = SumoCaseId( caseObj["id"].toString() );
        sumoCase.name          = caseObj["name"].toString();
        sumoCase.author        = caseObj["user"].toString();
        sumoCase.status        = caseObj["status"].toString();
        sumoCase.description   = caseObj["description"].toString();
        sumoCase.date          = caseObj["timestamp"].toString();
        sumoCase.modelName     = caseObj["model_name"].toString();
        sumoCase.modelRevision = caseObj["model_revision"].toString();

        for ( const QJsonValue& ensembleName : caseObj["ensemble_names"].toArray() )
        {
            sumoCase.ensembleNames.push_back( ensembleName.toString() );
        }

        cases.push_back( sumoCase );
    }

    RiaLogging::debug( std::format( "Case count : {}", cases.size() ) );

    return cases;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<QString> RiaSumoExplore::parseEnsembleNames( const QByteArray& body )
{
    std::vector<QString> ensembleNames;

    QJsonDocument doc       = QJsonDocument::fromJson( body );
    QJsonArray    jsonArray = doc.array();

    for ( const QJsonValue& value : jsonArray )
    {
        QJsonObject ensembleObj = value.toObject();
        ensembleNames.push_back( ensembleObj["name"].toString() );
    }

    RiaLogging::debug( std::format( "Ensemble count : {}", ensembleNames.size() ) );

    return ensembleNames;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<QString> RiaSumoExplore::parseRealizationIds( const QByteArray& body )
{
    std::vector<QString> realizationIds;

    QJsonDocument doc       = QJsonDocument::fromJson( body );
    QJsonArray    jsonArray = doc.array();

    for ( const QJsonValue& value : jsonArray )
    {
        realizationIds.push_back( QString::number( value.toInt() ) );
    }

    return realizationIds;
}
