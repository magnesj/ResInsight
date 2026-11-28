/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2025- Equinor ASA
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

#include "RiaSumoExplore.h"

#include <QDateTime>
#include <QString>

#include <map>
#include <optional>
#include <vector>

//==================================================================================================
/// What Sumo answered about assets, cases and ensembles, kept so the property editor and the selection
/// dialog do not ask again. Nothing expires by itself: it is replaced by an explicit refresh, and each
/// list remembers when it was stored.
//==================================================================================================
class RiaSumoExploreCache
{
public:
    const std::vector<SumoAsset>* assets() const;
    void                          setAssets( const std::vector<SumoAsset>& assets );

    const std::vector<SumoCase>* cases( const QString& assetName ) const;
    void                         setCases( const QString& assetName, const std::vector<SumoCase>& cases );
    std::optional<QDateTime>     casesUpdated( const QString& assetName ) const;

    const std::vector<QString>* ensembleNames( const QString& caseId ) const;
    void                        setEnsembleNames( const QString& caseId, const std::vector<QString>& names );

    // The signed-in user, empty until known
    const QString& userName() const;
    void           setUserName( const QString& userName );

    void clearCases();
    void clearCases( const QString& assetName );
    void clearEnsembleNames();
    void clear();

private:
    struct CaseEntry
    {
        std::vector<SumoCase> cases;
        QDateTime             updated;
    };

    std::optional<std::vector<SumoAsset>>   m_assets;
    std::map<QString, CaseEntry>            m_cases;
    std::map<QString, std::vector<QString>> m_ensembleNames;
    QString                                 m_userName;
};
