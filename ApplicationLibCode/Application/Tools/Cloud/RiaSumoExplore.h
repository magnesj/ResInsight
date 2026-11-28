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

#pragma once

#include "RiaSumoDefines.h"

#include <QByteArray>
#include <QString>

#include <functional>
#include <vector>

class RiaSumoConnector;

struct SumoAsset
{
    SumoAssetId assetId;

    QString kind;
    QString name;
};

struct SumoCase
{
    SumoCaseId caseId;

    QString kind;
    QString name;

    // Metadata from the cloud service. All of it is optional, an older service leaves it empty.
    QString              author;
    QString              status;
    QString              description;
    QString              date;
    QString              modelName;
    QString              modelRevision;
    std::vector<QString> ensembleNames;
};

//==================================================================================================
/// Finding your way around what Sumo holds: the assets available, the cases of an asset, the ensembles
/// of a case and the realizations of an ensemble. Requests are made through RiaSumoConnector, which owns
/// the connection and does the transfers, and every call returns its result rather than leaving it in
/// shared state.
//==================================================================================================
class RiaSumoExplore
{
public:
    explicit RiaSumoExplore( RiaSumoConnector& connector );

    std::vector<SumoAsset> assets();
    std::vector<SumoCase>  cases( const QString& assetName );
    std::vector<QString>   ensembleNames( const SumoCaseId& caseId );
    std::vector<QString>   realizationIds( const SumoCaseId& caseId, const QString& ensembleName );

    // The signed-in user, empty when unknown. Needs a service that has the whoami endpoint.
    QString userName();

    // Asynchronous variants for dialogs that must stay responsive. onFinished is called once, on the connector
    // thread. Requests made with the same cancelGroup are aborted by RiaSumoConnector::cancelGroup, in which
    // case onFinished receives an empty result.
    void casesAsync( const QString&                                             assetName,
                     const std::function<void( const std::vector<SumoCase>& )>& onFinished,
                     const void*                                                cancelGroup = nullptr );
    void ensembleNamesAsync( const SumoCaseId&                                         caseId,
                             const std::function<void( const std::vector<QString>& )>& onFinished,
                             const void*                                               cancelGroup = nullptr );
    void userNameAsync( const std::function<void( const QString& )>& onFinished, const void* cancelGroup = nullptr );
    void assetsAsync( const std::function<void( const std::vector<SumoAsset>& )>& onFinished, const void* cancelGroup = nullptr );

    static QString parseUserName( const QByteArray& body );

    static std::vector<SumoAsset> parseAssets( const QByteArray& body );
    static std::vector<SumoCase>  parseCases( const QByteArray& body );
    static std::vector<QString>   parseEnsembleNames( const QByteArray& body );
    static std::vector<QString>   parseRealizationIds( const QByteArray& body );

private:
    RiaSumoConnector& m_connector;
};
