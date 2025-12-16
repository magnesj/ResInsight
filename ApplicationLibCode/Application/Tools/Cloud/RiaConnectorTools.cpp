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

#include "RiaConnectorTools.h"

#include "RiaLogging.h"
#include "RiaPreferences.h"
#include "RiaPreferencesOpenTelemetry.h"
#include "RiaPreferencesOsdu.h"
#include "RiaPreferencesSumo.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QtNetworkAuth/QOAuth2AuthorizationCodeFlow>

#include <QCoreApplication>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiaConnectorTools::tokenDataAsJson( QOAuth2AuthorizationCodeFlow* authCodeFlow )
{
    QJsonObject obj;
    obj.insert( "token", authCodeFlow->token() );
    obj.insert( "refreshToken", authCodeFlow->refreshToken() );
    if ( authCodeFlow->expirationAt().isValid() )
    {
        obj.insert( "expiration", authCodeFlow->expirationAt().toSecsSinceEpoch() );
    }

    QJsonDocument doc( obj );
    return doc.toJson( QJsonDocument::Indented );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaConnectorTools::initializeTokenDataFromJson( QOAuth2AuthorizationCodeFlow* authCodeFlow, const QString& tokenDataJson )
{
    QJsonDocument doc = QJsonDocument::fromJson( tokenDataJson.toUtf8() );
    QJsonObject   obj = doc.object();

    if ( obj.contains( "expiration" ) && obj.contains( "token" ) )
    {
        quint64   secondsSinceEpoch = obj["expiration"].toVariant().toULongLong();
        QDateTime expiration        = QDateTime::fromSecsSinceEpoch( secondsSinceEpoch );
        if ( expiration.isValid() && expiration > QDateTime::currentDateTime() )
        {
            authCodeFlow->setToken( obj["token"].toString() );
        }
    }

    authCodeFlow->setRefreshToken( obj["refreshToken"].toString() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaConnectorTools::writeTokenData( const QString& filePath, const QString& tokenDataJson )
{
    QFile file( filePath );

    // Ensure the directory exists (create it if it doesn't)
    QString dirPath = QFileInfo( file ).absolutePath();
    QDir    dir( dirPath );
    if ( !dir.exists() )
    {
        dir.mkpath( dirPath );
    }

    if ( file.open( QIODevice::WriteOnly ) )
    {
        QTextStream stream( &file );
        stream << tokenDataJson;
        file.close();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiaConnectorTools::readStringFromFile( const QString& filePath )
{
    QFile file( filePath );
    if ( file.open( QIODevice::ReadOnly ) )
    {
        QTextStream stream( &file );
        QString     result = stream.readAll();
        file.close();
        return result;
    }
    return {};
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::map<QString, QString> RiaConnectorTools::readKeyValuePairs( const QString& filePath )
{
    auto content = readStringFromFile( filePath );

    QJsonDocument doc = QJsonDocument::fromJson( content.toUtf8() );
    QJsonObject   obj = doc.object();

    std::map<QString, QString> keyValuePairs;
    for ( auto it = obj.begin(); it != obj.end(); ++it )
    {
        keyValuePairs[it.key()] = it.value().toString();
    }

    return keyValuePairs;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaConnectorTools::readCloudConfigFiles( RiaPreferences* preferences )
{
    if ( preferences == nullptr ) return;

    // Check multiple locations for configuration files. The first valid configuration file is used. Currently, using Qt5 the ResInsight
    // binary file is stored at the root of the installation folder. When moving to Qt6, we will probably use sub folders /bin /lib and
    // others. Support both one and two search levels to support Qt6.
    //
    // home_folder/.resinsight/*_config.json
    // location_of_resinsight_executable/../share/cloud_services/*_config.json
    // location_of_resinsight_executable/../../share/cloud_services/*_config.json
    //

    auto buildConfigFilePathCandidates = []( const QString& fileName ) -> QStringList
    {
        QStringList candidates;
        candidates << QDir::homePath() + "/.resinsight/" + fileName;
        candidates << QCoreApplication::applicationDirPath() + "/../share/cloud_services/" + fileName;
        candidates << QCoreApplication::applicationDirPath() + "/../../share/cloud_services/" + fileName;
        return candidates;
    };

    // Load OSDU configuration
    for ( const auto& filePath : buildConfigFilePathCandidates( "osdu_config.json" ) )
    {
        auto keyValuePairs = RiaConnectorTools::readKeyValuePairs( filePath );
        if ( !keyValuePairs.empty() )
        {
            RiaLogging::info( QString( "Imported OSDU configuration from : '%1'" ).arg( filePath ) );
            preferences->osduPreferences()->setData( keyValuePairs );
            preferences->osduPreferences()->setFieldsReadOnly();
            break;
        }
    }

    // Load SUMO configuration
    for ( const auto& filePath : buildConfigFilePathCandidates( "sumo_config.json" ) )
    {
        auto keyValuePairs = RiaConnectorTools::readKeyValuePairs( filePath );
        if ( !keyValuePairs.empty() )
        {
            RiaLogging::info( QString( "Imported SUMO configuration from : '%1'" ).arg( filePath ) );
            preferences->sumoPreferences()->setData( keyValuePairs );
            preferences->sumoPreferences()->setFieldsReadOnly();
            break;
        }
    }

    // Load OpenTelemetry configuration
    for ( const auto& filePath : buildConfigFilePathCandidates( "opentelemetry_config.json" ) )
    {
        if ( RiaConnectorTools::loadOpenTelemetryConfiguration( filePath ) )
        {
            RiaLogging::info( QString( "Imported OpenTelemetry configuration from : '%1'" ).arg( filePath ) );
            RiaPreferencesOpenTelemetry::current()->setFieldsReadOnly();
            break;
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaConnectorTools::loadOpenTelemetryConfiguration( const QString& filePath )
{
    auto content = readStringFromFile( filePath );
    if ( content.isEmpty() )
    {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson( content.toUtf8() );
    if ( !doc.isObject() )
    {
        return false;
    }

    QJsonObject obj   = doc.object();
    auto*       prefs = RiaPreferencesOpenTelemetry::current();

    // Build a flat key-value map from the nested JSON structure
    std::map<QString, QString> keyValuePairs;

    // Handle active_environment
    if ( obj.contains( "active_environment" ) )
    {
        keyValuePairs["active_environment"] = obj["active_environment"].toString();
    }

    // Handle environments and extract connection settings for active environment
    if ( obj.contains( "environments" ) )
    {
        QJsonObject environments = obj["environments"].toObject();
        QString     activeEnv    = obj.value( "active_environment" ).toString( "production" );

        if ( environments.contains( activeEnv ) )
        {
            QJsonObject envConfig = environments[activeEnv].toObject();

            if ( envConfig.contains( "connection_string" ) )
            {
                keyValuePairs["connection_string"] = envConfig["connection_string"].toString();
            }

            if ( envConfig.contains( "endpoint" ) )
            {
                keyValuePairs["local_endpoint"] = envConfig["endpoint"].toString();
            }
        }
    }

    // Handle top-level configuration
    if ( obj.contains( "batch_timeout_ms" ) )
    {
        keyValuePairs["batch_timeout_ms"] = QString::number( obj["batch_timeout_ms"].toInt() );
    }

    if ( obj.contains( "max_batch_size" ) )
    {
        keyValuePairs["max_batch_size"] = QString::number( obj["max_batch_size"].toInt() );
    }

    if ( obj.contains( "max_queue_size" ) )
    {
        keyValuePairs["max_queue_size"] = QString::number( obj["max_queue_size"].toInt() );
    }

    if ( obj.contains( "connection_timeout_ms" ) )
    {
        keyValuePairs["connection_timeout_ms"] = QString::number( obj["connection_timeout_ms"].toInt() );
    }

    if ( obj.contains( "memory_threshold_mb" ) )
    {
        keyValuePairs["memory_threshold_mb"] = QString::number( obj["memory_threshold_mb"].toInt() );
    }

    if ( obj.contains( "sampling_rate" ) )
    {
        keyValuePairs["sampling_rate"] = QString::number( obj["sampling_rate"].toDouble() );
    }

    // Handle privacy settings
    if ( obj.contains( "privacy" ) )
    {
        QJsonObject privacy = obj["privacy"].toObject();

        if ( privacy.contains( "filter_file_paths" ) )
        {
            keyValuePairs["filter_file_paths"] = privacy["filter_file_paths"].toBool() ? "true" : "false";
        }

        if ( privacy.contains( "filter_user_data" ) )
        {
            keyValuePairs["filter_user_data"] = privacy["filter_user_data"].toBool() ? "true" : "false";
        }
    }

    // Handle fallback settings
    if ( obj.contains( "fallback" ) )
    {
        QJsonObject fallback = obj["fallback"].toObject();

        if ( fallback.contains( "enable_local_logging" ) )
        {
            keyValuePairs["enable_local_logging"] = fallback["enable_local_logging"].toBool() ? "true" : "false";
        }

        if ( fallback.contains( "failure_threshold" ) )
        {
            keyValuePairs["failure_threshold"] = QString::number( fallback["failure_threshold"].toInt() );
        }

        if ( fallback.contains( "retry_interval_seconds" ) )
        {
            keyValuePairs["retry_interval_seconds"] = QString::number( fallback["retry_interval_seconds"].toInt() );
        }
    }

    // Apply the configuration using the existing setData method
    if ( !keyValuePairs.empty() )
    {
        prefs->setData( keyValuePairs );
    }

    return true;
}
