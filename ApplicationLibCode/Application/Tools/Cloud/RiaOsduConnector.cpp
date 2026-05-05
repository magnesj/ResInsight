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

#include "RiaOsduConnector.h"
#include "RiaCloudDefines.h"
#include "RiaLogging.h"
#include "RiaOsduDefines.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegularExpression>

#include <limits>

#include "cafAssert.h"

namespace
{
//--------------------------------------------------------------------------------------------------
/// Parse an OSDU UnitOfMeasure reference id (e.g. "data:reference-data--UnitOfMeasure:ft:") and return
/// the multiplier that converts the value into meters. Returns 1.0 (and sets recognized=false) when the
/// id is empty or the symbol is unknown, so callers can log a single warning at the call site.
//--------------------------------------------------------------------------------------------------
double unitOfMeasureToMeters( const QString& unitId, bool* recognized = nullptr )
{
    if ( recognized ) *recognized = false;

    QString symbol;
    if ( unitId.endsWith( ':' ) )
    {
        int lastColon = unitId.lastIndexOf( ':', unitId.length() - 2 );
        if ( lastColon >= 0 ) symbol = unitId.mid( lastColon + 1, unitId.length() - lastColon - 2 );
    }
    if ( symbol.isEmpty() ) return 1.0;

    if ( symbol.compare( "m", Qt::CaseInsensitive ) == 0 )
    {
        if ( recognized ) *recognized = true;
        return 1.0;
    }
    if ( symbol.compare( "ft", Qt::CaseInsensitive ) == 0 )
    {
        if ( recognized ) *recognized = true;
        return 0.3048;
    }
    return 1.0;
}

//--------------------------------------------------------------------------------------------------
/// Extract the linear unit factor (-> meters) from a persistableReferenceCrs JSON string by scanning
/// the embedded WKT. The PROJCS WKT places the projection's linear UNIT after the nested angular GEOGCS
/// UNIT, so the last `UNIT["name", factor]` token is the linear one. Falls back to 1.0 on any parse
/// failure or for non-projected CRSs.
//--------------------------------------------------------------------------------------------------
double linearCrsUnitToMeters( const QString& persistableReferenceCrs )
{
    if ( persistableReferenceCrs.isEmpty() ) return 1.0;

    QJsonDocument doc = QJsonDocument::fromJson( persistableReferenceCrs.toUtf8() );
    if ( !doc.isObject() ) return 1.0;

    QJsonObject obj = doc.object();
    QString     wkt = obj["wkt"].toString();
    if ( wkt.isEmpty() ) wkt = obj["lateBoundCRS"].toObject()["wkt"].toString();
    if ( wkt.isEmpty() ) return 1.0;
    if ( !wkt.startsWith( "PROJCS", Qt::CaseInsensitive ) ) return 1.0;

    QRegularExpression re( "UNIT\\[\"[^\"]+\"\\s*,\\s*([0-9.eE+\\-]+)\\]" );
    auto               matches = re.globalMatch( wkt );
    double             factor  = 1.0;
    while ( matches.hasNext() )
    {
        QRegularExpressionMatch m  = matches.next();
        bool                    ok = false;
        double                  v  = m.captured( 1 ).toDouble( &ok );
        if ( ok ) factor = v;
    }
    return factor;
}
} // namespace

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaOsduConnector::RiaOsduConnector( QObject*       parent,
                                    const QString& server,
                                    const QString& dataPartitionId,
                                    const QString& authority,
                                    const QString& scopes,
                                    const QString& clientId,
                                    unsigned int   port )
    : RiaCloudConnector( parent, server, authority, scopes, clientId, port )
    , m_dataPartitionId( dataPartitionId )
{
    connect( this,
             SIGNAL( parquetDownloadFinished( const QByteArray&, const QString&, const QString& ) ),
             this,
             SLOT( parquetDownloadComplete( const QByteArray&, const QString&, const QString& ) ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaOsduConnector::~RiaOsduConnector()
{
    // Abort all pending network replies to prevent threading issues during destruction
    QMutexLocker lock( &m_repliesMutex );
    for ( auto& [id, reply] : m_replies )
    {
        if ( !reply.isNull() )
        {
            reply->disconnect();
            reply->abort();
        }
    }
    m_replies.clear();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOsduConnector::clearCachedData()
{
    QMutexLocker lock( &m_mutex );
    m_fields.clear();
    m_wellbores.clear();
    m_wellboreTrajectories.clear();
    m_wellLogs.clear();
    m_parquetData.clear();
    m_parquetErrors.clear();
    m_wellSurfaceLocations.clear();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOsduConnector::requestFieldsByName( const QString& token, const QString& fieldName )
{
    requestFieldsByName( m_server, m_dataPartitionId, token, fieldName );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOsduConnector::requestFieldsByName( const QString& fieldName )
{
    requestFieldsByName( token(), fieldName );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOsduConnector::requestFieldsByName( const QString& server, const QString& dataPartitionId, const QString& token, const QString& fieldName )
{
    std::map<QString, QString> params;
    params["kind"]  = RiaOsduDefines::osduFieldKind();
    params["limit"] = "10000";
    params["query"] = "data.FieldName:" + fieldName;

    auto reply = makeSearchRequest( params, server, dataPartitionId, token );
    connect( reply,
             &QNetworkReply::finished,
             [this, reply, fieldName]()
             {
                 if ( reply->error() == QNetworkReply::NoError )
                 {
                     parseFields( reply );
                 }
                 else
                 {
                     QString errorMessage =
                         QString( "Download failed for fields by name (%1). Error: %2" ).arg( fieldName ).arg( reply->errorString() );
                     RiaLogging::error( errorMessage );
                 }
             } );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOsduConnector::requestWellboresByFieldId( const QString& fieldId )
{
    requestWellboresByFieldId( m_server, m_dataPartitionId, token(), fieldId );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOsduConnector::requestWellboresByFieldId( const QString& server, const QString& dataPartitionId, const QString& token, const QString& fieldId )
{
    std::map<QString, QString> params;
    params["kind"]  = RiaOsduDefines::osduWellboreKind();
    params["limit"] = "10000";
    params["query"] = QString( "nested(data.GeoContexts, (FieldID:\"%1\"))" ).arg( fieldId );

    auto reply = makeSearchRequest( params, server, dataPartitionId, token );
    connect( reply,
             &QNetworkReply::finished,
             [this, reply, fieldId]()
             {
                 if ( reply->error() == QNetworkReply::NoError )
                 {
                     parseWellboresByFieldId( reply, fieldId );
                 }
                 else
                 {
                     QString errorMessage =
                         QString( "Request failed for wellbores for field (%1). Error: %2" ).arg( fieldId ).arg( reply->errorString() );
                     RiaLogging::error( errorMessage );
                 }
             } );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<OsduWellLog> RiaOsduConnector::requestWellLogsByWellboreIdBlocking( const QString& wellboreId )
{
    QString token = requestTokenBlocking();

    QEventLoop loop;
    connect( this, SIGNAL( wellLogsFinished( const QString& ) ), &loop, SLOT( quit() ) );
    requestWellLogsByWellboreId( m_server, m_dataPartitionId, token, wellboreId );
    loop.exec();

    return m_wellLogs[wellboreId];
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOsduConnector::requestWellLogsByWellboreId( const QString& wellboreId )
{
    requestWellLogsByWellboreId( m_server, m_dataPartitionId, token(), wellboreId );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOsduConnector::requestWellLogsByWellboreId( const QString& server,
                                                    const QString& dataPartitionId,
                                                    const QString& token,
                                                    const QString& wellboreId )
{
    std::map<QString, QString> params;
    params["kind"]  = RiaOsduDefines::osduWellLogKind();
    params["limit"] = "10000";
    params["query"] = "data.WellboreID: \"" + wellboreId + "\"";

    auto reply = makeSearchRequest( params, server, dataPartitionId, token );
    connect( reply,
             &QNetworkReply::finished,
             [this, reply, wellboreId]()
             {
                 if ( reply->error() == QNetworkReply::NoError )
                 {
                     parseWellLogs( reply, wellboreId );
                 }
                 else
                 {
                     QString errorMessage =
                         QString( "Request failed for well logs by wellbore (%1). Error: %2" ).arg( wellboreId ).arg( reply->errorString() );
                     RiaLogging::error( errorMessage );
                 }
             } );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOsduConnector::requestWellboreTrajectoryByWellboreId( const QString& wellboreId )
{
    requestWellboreTrajectoryByWellboreId( m_server, m_dataPartitionId, token(), wellboreId );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOsduConnector::requestWellboreTrajectoryByWellboreId( const QString& server,
                                                              const QString& dataPartitionId,
                                                              const QString& token,
                                                              const QString& wellboreId )
{
    std::map<QString, QString> params;
    params["kind"]  = RiaOsduDefines::osduWellboreTrajectoryKind();
    params["limit"] = "10000";
    params["query"] = "data.WellboreID: \"" + wellboreId + "\"";

    auto reply = makeSearchRequest( params, server, dataPartitionId, token );
    connect( reply,
             &QNetworkReply::finished,
             [this, reply, wellboreId]()
             {
                 if ( reply->error() == QNetworkReply::NoError )
                 {
                     parseWellTrajectory( reply, wellboreId );
                 }
                 else
                 {
                     QString errorMessage =
                         QString( "Request failed for well trajectory by wellbore (%1). Error: %2" ).arg( wellboreId ).arg( reply->errorString() );
                     RiaLogging::error( errorMessage );
                 }
             } );
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
QString RiaOsduConnector::constructSearchUrl( const QString& server )
{
    return server + "/api/search/v2/query";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiaOsduConnector::constructFileDownloadUrl( const QString& server, const QString& fileId )
{
    return server + "/api/file/v2/files/" + fileId + "/downloadURL";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiaOsduConnector::constructWellLogDownloadUrl( const QString& server, const QString& wellLogId )
{
    return server + "/api/os-wellbore-ddms/ddms/v3/welllogs/" + wellLogId + "/data";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiaOsduConnector::constructWellboreTrajectoriesDownloadUrl( const QString& server, const QString& wellboreTrajectoryId )
{
    return server + "/api/os-wellbore-ddms/ddms/v3/wellboretrajectories/" + wellboreTrajectoryId + "/data";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QNetworkReply* RiaOsduConnector::makeSearchRequest( const std::map<QString, QString>& parameters,
                                                    const QString&                    server,
                                                    const QString&                    dataPartitionId,
                                                    const QString&                    token )
{
    QNetworkRequest networkRequest;
    networkRequest.setUrl( QUrl( constructSearchUrl( server ) ) );

    addStandardHeader( networkRequest, token, dataPartitionId, RiaCloudDefines::contentTypeJson() );

    QJsonObject obj;
    for ( auto [key, value] : parameters )
    {
        obj.insert( key, value );
    }

    QJsonDocument doc( obj );
    QString       strJson( doc.toJson( QJsonDocument::Compact ) );

    auto reply = m_networkAccessManager->post( networkRequest, strJson.toUtf8() );
    return reply;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOsduConnector::parseFields( QNetworkReply* reply )
{
    QByteArray result = reply->readAll();
    reply->deleteLater();

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJsonDocument doc          = QJsonDocument::fromJson( result );
        QJsonObject   jsonObj      = doc.object();
        QJsonArray    resultsArray = jsonObj["results"].toArray();

        {
            QMutexLocker lock( &m_mutex );
            m_fields.clear();

            for ( const QJsonValue& value : resultsArray )
            {
                QJsonObject resultObj = value.toObject();

                QString id        = resultObj["id"].toString();
                QString kind      = resultObj["kind"].toString();
                QString fieldName = resultObj["data"].toObject()["FieldName"].toString();
                m_fields.push_back( OsduField{ id, kind, fieldName } );
            }

            RiaLogging::debug( QString( "Found %1 fields." ).arg( m_fields.size() ) );
        }

        emit fieldsFinished();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOsduConnector::parseWellboresByFieldId( QNetworkReply* reply, const QString& fieldId )
{
    QByteArray result = reply->readAll();
    reply->deleteLater();

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJsonDocument doc          = QJsonDocument::fromJson( result );
        QJsonObject   jsonObj      = doc.object();
        QJsonArray    resultsArray = jsonObj["results"].toArray();

        {
            QMutexLocker lock( &m_mutex );
            m_wellbores[fieldId].clear();
            for ( const QJsonValue& value : resultsArray )
            {
                QJsonObject resultObj = value.toObject();
                QString     id        = resultObj["id"].toString();
                QString     kind      = resultObj["kind"].toString();
                QString     name      = resultObj["data"].toObject()["FacilityName"].toString();
                QString     wellId    = resultObj["data"].toObject()["WellID"].toString();

                // Extract datum elevation. The DefaultVerticalMeasurementID is probably the datum elevation needed.
                // Default to 0.0 if nothing is found, but finding nothing is suspicious.
                double     datumElevation               = std::numeric_limits<double>::infinity();
                QString    defaultVerticalMeasurementId = resultObj["data"].toObject()["DefaultVerticalMeasurementID"].toString();
                QJsonArray verticalMeasurementsArray    = resultObj["data"].toObject()["VerticalMeasurements"].toArray();
                for ( const QJsonValue& vma : verticalMeasurementsArray )
                {
                    QString verticalMeasurementId = vma["VerticalMeasurementID"].toString();
                    if ( verticalMeasurementId == defaultVerticalMeasurementId )
                    {
                        double  verticalMeasurement = vma["VerticalMeasurement"].toDouble( 0.0 );
                        QString unitId              = vma["VerticalMeasurementUnitOfMeasureID"].toString();
                        bool    unitRecognized      = false;
                        double  factor              = unitOfMeasureToMeters( unitId, &unitRecognized );
                        if ( !unitRecognized && !unitId.isEmpty() )
                        {
                            RiaLogging::warning(
                                QString( "Unrecognized datum elevation unit '%1' for well bore '%2'; assuming meters." ).arg( unitId ).arg( name ) );
                        }
                        datumElevation = verticalMeasurement * factor;
                    }
                }

                if ( std::isinf( datumElevation ) )
                {
                    RiaLogging::warning( QString( "Missing datum elevation for well bore '%1'. Id: %2" ).arg( name ).arg( id ) );
                    datumElevation = 0.0;
                }

                // Wellbore records typically do not carry SpatialLocation; the surface point lives on the parent
                // Well record. Surface easting/northing/crs are populated separately via requestWellSurfaceLocationBlocking.
                m_wellbores[fieldId].push_back( OsduWellbore{ id, kind, name, wellId, fieldId, datumElevation } );
            }
        }

        emit wellboresByFieldIdFinished( fieldId );
    }
    else
    {
        RiaLogging::error( "Failed to download wellbores for field with id " + fieldId + ": " + reply->errorString() );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOsduConnector::parseWellTrajectory( QNetworkReply* reply, const QString& wellboreId )
{
    QByteArray result = reply->readAll();
    reply->deleteLater();

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJsonDocument doc          = QJsonDocument::fromJson( result );
        QJsonObject   jsonObj      = doc.object();
        QJsonArray    resultsArray = jsonObj["results"].toArray();

        {
            QMutexLocker lock( &m_mutex );
            m_wellboreTrajectories[wellboreId].clear();
            for ( const QJsonValue& value : resultsArray )
            {
                QJsonObject resultObj = value.toObject();
                if ( resultObj.isEmpty() ) continue;

                QString id   = resultObj["id"].toString();
                QString kind = resultObj["kind"].toString();
                QString existenceKind;
                QString crs;

                // Safely extract existenceKind from nested data object
                QJsonObject dataObj = resultObj["data"].toObject();
                if ( !dataObj.isEmpty() && dataObj.contains( "ExistenceKind" ) )
                {
                    existenceKind = dataObj["ExistenceKind"].toString();
                }

                QJsonObject spatialLocation = dataObj["SpatialLocation"].toObject();
                QJsonObject ingested        = spatialLocation["AsIngestedCoordinates"].toObject();
                if ( !ingested.isEmpty() )
                {
                    crs = ingested["persistableReferenceCrs"].toString();
                }

                // The MD entry in AvailableTrajectoryStationProperties advertises a length unit that the parquet
                // values are stored in. Treat it as the canonical length unit for the geometric columns
                // (MD/TVD/X/Y) and use it to derive a multiplier into meters, so downstream code can combine the
                // trajectory with surface origin and datum elevation (which are stored as meters).
                //
                // Note: in real-world OSDU records the X/Y entries sometimes advertise a different unit than
                // MD/TVD (e.g. "dega" while the values are clearly meters/feet). Trusting MD's unit and applying
                // it uniformly to all four columns gives the right result for those datasets too.
                QString    mdUnitId;
                QJsonArray availableProps = dataObj["AvailableTrajectoryStationProperties"].toArray();
                for ( const QJsonValue& propValue : availableProps )
                {
                    QJsonObject propObj = propValue.toObject();
                    if ( propObj["Name"].toString() == "MD" )
                    {
                        mdUnitId = propObj["StationPropertyUnitID"].toString();
                        break;
                    }
                }
                bool   unitRecognized = false;
                double unitToMeters   = unitOfMeasureToMeters( mdUnitId, &unitRecognized );
                if ( !unitRecognized && !mdUnitId.isEmpty() )
                {
                    RiaLogging::warning( QString( "Unrecognized MD unit '%1' for trajectory %2; assuming meters." ).arg( mdUnitId ).arg( id ) );
                }

                m_wellboreTrajectories[wellboreId].push_back( OsduWellboreTrajectory{ id, kind, wellboreId, existenceKind, crs, unitToMeters } );
            }
        }

        emit wellboreTrajectoryFinished( wellboreId, resultsArray.size(), "" );
    }
    else
    {
        emit wellboreTrajectoryFinished( wellboreId, 0, "Failed to download: " + reply->errorString() );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOsduConnector::parseWellLogs( QNetworkReply* reply, const QString& wellboreId )
{
    QByteArray result = reply->readAll();
    reply->deleteLater();

    if ( reply->error() == QNetworkReply::NoError )
    {
        QJsonDocument doc          = QJsonDocument::fromJson( result );
        QJsonObject   jsonObj      = doc.object();
        QJsonArray    resultsArray = jsonObj["results"].toArray();

        {
            QMutexLocker lock( &m_mutex );
            m_wellLogs[wellboreId].clear();
            for ( const QJsonValue& value : resultsArray )
            {
                QJsonObject resultObj = value.toObject();
                QString     id        = resultObj["id"].toString();
                QString     kind      = resultObj["kind"].toString();

                QJsonObject dataObj       = resultObj["data"].toObject();
                QString     name          = dataObj["Name"].toString();
                QString     description   = dataObj["Description"].toString();
                double      samplingStart = dataObj["SamplingStart"].toDouble( std::numeric_limits<double>::infinity() );
                double      samplingStop  = dataObj["SamplingStop"].toDouble( std::numeric_limits<double>::infinity() );

                QJsonArray curvesArray = dataObj["Curves"].toArray();
                RiaLogging::debug( QString( "Curves for '%1':" ).arg( id ) );

                std::vector<OsduWellLogChannel> channels;
                for ( const QJsonValue& curve : curvesArray )
                {
                    QString mnemonic         = curve["Mnemonic"].toString();
                    QString curveId          = curve["CurveID"].toString();
                    QString curveDescription = curve["CurveDescription"].toString();
                    double  curveBaseDepth   = curve["BaseDepth"].toDouble( std::numeric_limits<double>::infinity() );
                    double  curveTopDepth    = curve["TopDepth"].toDouble( std::numeric_limits<double>::infinity() );
                    QString interpreterName  = curve["InterpreterName"].toString();
                    QString quality          = curve["CurveQuality"].toString();
                    QString unit             = curve["CurveUnit"].toString();
                    QString depthUnit        = curve["DepthUnit"].toString();

                    RiaLogging::debug(
                        QString( "%1: '%2' (%3 - %4)" ).arg( curveId ).arg( curveDescription ).arg( curveTopDepth ).arg( curveBaseDepth ) );
                    channels.push_back( OsduWellLogChannel{ .id              = curveId,
                                                            .mnemonic        = mnemonic,
                                                            .description     = curveDescription,
                                                            .topDepth        = curveTopDepth,
                                                            .baseDepth       = curveBaseDepth,
                                                            .interpreterName = interpreterName,
                                                            .quality         = quality,
                                                            .unit            = unit,
                                                            .depthUnit       = depthUnit } );
                }

                m_wellLogs[wellboreId].push_back( OsduWellLog{ .id            = id,
                                                               .kind          = kind,
                                                               .name          = name,
                                                               .description   = description,
                                                               .samplingStart = samplingStart,
                                                               .samplingStop  = samplingStop,
                                                               .wellboreId    = wellboreId,
                                                               .channels      = channels } );
            }
        }

        emit wellLogsFinished( wellboreId );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOsduConnector::addStandardHeader( QNetworkRequest& networkRequest,
                                          const QString&   token,
                                          const QString&   dataPartitionId,
                                          const QString&   contentType )
{
    networkRequest.setHeader( QNetworkRequest::ContentTypeHeader, contentType );
    networkRequest.setRawHeader( "Authorization", "Bearer " + token.toUtf8() );
    networkRequest.setRawHeader( QByteArray( "Data-Partition-Id" ), dataPartitionId.toUtf8() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QNetworkReply*
    RiaOsduConnector::makeDownloadRequest( const QString& url, const QString& dataPartitionId, const QString& token, const QString& contentType )
{
    QNetworkRequest networkRequest;
    networkRequest.setUrl( QUrl( url ) );

    addStandardHeader( networkRequest, token, dataPartitionId, contentType );

    auto reply = m_networkAccessManager->get( networkRequest );
    return reply;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiaOsduConnector::dataPartition() const
{
    return m_dataPartitionId;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<OsduField> RiaOsduConnector::fields() const
{
    QMutexLocker lock( &m_mutex );
    return m_fields;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<OsduWellLog> RiaOsduConnector::wellLogs( const QString& wellboreId ) const
{
    QMutexLocker lock( &m_mutex );

    auto it = m_wellLogs.find( wellboreId );
    if ( it != m_wellLogs.end() ) return it->second;

    return {};
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<OsduWellbore> RiaOsduConnector::wellboresByFieldId( const QString& fieldId ) const
{
    QMutexLocker lock( &m_mutex );

    auto it = m_wellbores.find( fieldId );
    if ( it != m_wellbores.end() ) return it->second;

    return {};
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::optional<OsduWellbore> RiaOsduConnector::wellboreById( const QString& wellboreId ) const
{
    QMutexLocker lock( &m_mutex );

    for ( const auto& [fieldId, wellbores] : m_wellbores )
    {
        auto it = std::find_if( wellbores.begin(), wellbores.end(), [wellboreId]( const OsduWellbore& w ) { return w.id == wellboreId; } );
        if ( it != wellbores.end() )
        {
            return *it;
        }
    }

    return std::nullopt;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<OsduWellboreTrajectory> RiaOsduConnector::wellboreTrajectories( const QString& wellboreId ) const
{
    QMutexLocker lock( &m_mutex );

    auto it = m_wellboreTrajectories.find( wellboreId );
    if ( it != m_wellboreTrajectories.end() ) return it->second;

    return {};
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOsduConnector::requestWellboreTrajectoryParquetDataById( const QString& wellboreTrajectoryId )
{
    QString url = constructWellboreTrajectoriesDownloadUrl( m_server, wellboreTrajectoryId );
    RiaLogging::debug( "Wellbore trajectory URL: " + url );
    requestParquetDataByUrl( url, wellboreTrajectoryId );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOsduConnector::requestWellLogParquetDataById( const QString& wellLogId )
{
    QString url = constructWellLogDownloadUrl( m_server, wellLogId );
    RiaLogging::debug( "Well log URL: " + url );

    requestParquetDataByUrl( url, wellLogId );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOsduConnector::requestParquetDataByUrl( const QString& url, const QString& id )
{
    requestParquetData( url, m_dataPartitionId, token(), id );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::pair<QByteArray, QString> RiaOsduConnector::requestWellboreTrajectoryParquetDataByIdBlocking( const QString& wellboreTrajectoryId )
{
    QString url = constructWellboreTrajectoriesDownloadUrl( m_server, wellboreTrajectoryId );
    RiaLogging::debug( "Wellbore trajectory URL: " + url );

    return requestParquetDataByUrlBlocking( url, wellboreTrajectoryId );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::pair<QByteArray, QString> RiaOsduConnector::requestWellLogParquetDataByIdBlocking( const QString& wellLogId )
{
    QString url = constructWellLogDownloadUrl( m_server, wellLogId );
    RiaLogging::debug( "Well log URL: " + url );

    return requestParquetDataByUrlBlocking( url, wellLogId );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::pair<QByteArray, QString> RiaOsduConnector::requestParquetDataByUrlBlocking( const QString& url, const QString& id )
{
    QString token = requestTokenBlocking();

    QEventLoop loop;
    connect( this, SIGNAL( parquetDownloadFinished( const QByteArray&, const QString&, const QString& ) ), &loop, SLOT( quit() ) );
    requestParquetData( url, m_dataPartitionId, token, id );
    loop.exec();

    QMutexLocker lock( &m_mutex );
    return { m_parquetData[id], m_parquetErrors[id] };
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOsduConnector::requestParquetData( const QString& url, const QString& dataPartitionId, const QString& token, const QString& id )
{
    RiaLogging::info( "Requesting download of parquet from: " + url );

    auto reply = makeDownloadRequest( url, dataPartitionId, token, RiaCloudDefines::contentTypeParquet() );
    m_repliesMutex.lock();
    m_replies[id] = reply;
    m_repliesMutex.unlock();

    connect( reply,
             &QNetworkReply::finished,
             [this, reply, url, id]()
             {
                 if ( reply->error() == QNetworkReply::NoError )
                 {
                     QByteArray contents = reply->readAll();
                     RiaLogging::info( QString( "Download succeeded: %1 bytes." ).arg( contents.length() ) );
                     emit parquetDownloadFinished( contents, "", id );
                 }
                 else
                 {
                     QString errorMessage = "Request failed: " + url + " failed." + reply->errorString();
                     RiaLogging::error( errorMessage );
                     emit parquetDownloadFinished( QByteArray(), errorMessage, id );
                 }
             } );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOsduConnector::parquetDownloadComplete( const QByteArray& contents, const QString& errorMessage, const QString& id )
{
    CAF_ASSERT( !id.isEmpty() );
    QMutexLocker lock( &m_mutex );
    m_parquetData[id]   = contents;
    m_parquetErrors[id] = errorMessage;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaOsduConnector::cancelRequestForId( const QString& id )
{
    QMutexLocker lock( &m_repliesMutex );
    auto         it = m_replies.find( id );
    if ( it != m_replies.end() )
    {
        if ( !it->second.isNull() )
        {
            it->second->abort();
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaOsduConnector::WellSurfaceLocation RiaOsduConnector::requestWellSurfaceLocationBlocking( const QString& wellId )
{
    if ( wellId.isEmpty() ) return {};

    // OSDU stores record references with a trailing colon (e.g. "data:master-data--Well:abcd:"). The storage
    // API expects the id without it.
    QString recordId = wellId;
    while ( recordId.endsWith( ':' ) )
        recordId.chop( 1 );

    {
        QMutexLocker lock( &m_mutex );
        auto         it = m_wellSurfaceLocations.find( recordId );
        if ( it != m_wellSurfaceLocations.end() ) return it->second;
    }

    QString token = requestTokenBlocking();
    QString url   = m_server + "/api/storage/v2/records/" + recordId;

    QNetworkRequest networkRequest;
    networkRequest.setUrl( QUrl( url ) );
    addStandardHeader( networkRequest, token, m_dataPartitionId, RiaCloudDefines::contentTypeJson() );

    QNetworkReply* reply = m_networkAccessManager->get( networkRequest );

    QEventLoop loop;
    connect( reply, &QNetworkReply::finished, &loop, &QEventLoop::quit );
    loop.exec();

    WellSurfaceLocation location;

    if ( reply->error() == QNetworkReply::NoError )
    {
        QByteArray    body            = reply->readAll();
        QJsonDocument doc             = QJsonDocument::fromJson( body );
        QJsonObject   data            = doc.object()["data"].toObject();
        QJsonObject   spatialLocation = data["SpatialLocation"].toObject();
        QJsonObject   ingested        = spatialLocation["AsIngestedCoordinates"].toObject();

        if ( !ingested.isEmpty() )
        {
            location.crs             = ingested["persistableReferenceCrs"].toString();
            const double crsToMeters = linearCrsUnitToMeters( location.crs );
            QJsonArray   features    = ingested["features"].toArray();
            if ( !features.isEmpty() )
            {
                QJsonArray coordinates = features[0].toObject()["geometry"].toObject()["coordinates"].toArray();
                if ( coordinates.size() >= 2 )
                {
                    // CRS WKT may declare a non-meter linear unit (e.g. US survey foot for state-plane CRSs).
                    // Convert here so downstream code can rely on the surface origin always being meters.
                    location.easting  = coordinates[0].toDouble() * crsToMeters;
                    location.northing = coordinates[1].toDouble() * crsToMeters;
                    location.isValid  = true;
                }
            }
        }

        if ( !location.isValid )
        {
            RiaLogging::warning( QString( "No SpatialLocation found on Well record '%1'." ).arg( recordId ) );
        }
    }
    else
    {
        RiaLogging::error( QString( "Failed to download Well record '%1': %2" ).arg( recordId ).arg( reply->errorString() ) );
    }

    reply->deleteLater();

    {
        QMutexLocker lock( &m_mutex );
        m_wellSurfaceLocations[recordId] = location;
    }

    return location;
}
