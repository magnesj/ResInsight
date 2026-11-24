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

#include "RiaSumoExploreCache.h"

//--------------------------------------------------------------------------------------------------
/// Null until assets have been stored. An empty answer is not stored by callers, as it cannot be told from
/// a failed request.
//--------------------------------------------------------------------------------------------------
const std::vector<SumoAsset>* RiaSumoExploreCache::assets() const
{
    return m_assets ? &m_assets.value() : nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoExploreCache::setAssets( const std::vector<SumoAsset>& assets )
{
    m_assets = assets;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const std::vector<SumoCase>* RiaSumoExploreCache::cases( const QString& assetName ) const
{
    auto it = m_cases.find( assetName );
    return it != m_cases.end() ? &it->second.cases : nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoExploreCache::setCases( const QString& assetName, const std::vector<SumoCase>& cases )
{
    m_cases[assetName] = { cases, QDateTime::currentDateTime() };
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::optional<QDateTime> RiaSumoExploreCache::casesUpdated( const QString& assetName ) const
{
    auto it = m_cases.find( assetName );
    if ( it == m_cases.end() ) return std::nullopt;
    return it->second.updated;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const std::vector<QString>* RiaSumoExploreCache::ensembleNames( const QString& caseId ) const
{
    auto it = m_ensembleNames.find( caseId );
    return it != m_ensembleNames.end() ? &it->second : nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoExploreCache::setEnsembleNames( const QString& caseId, const std::vector<QString>& names )
{
    m_ensembleNames[caseId] = names;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const QString& RiaSumoExploreCache::userName() const
{
    return m_userName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoExploreCache::setUserName( const QString& userName )
{
    m_userName = userName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoExploreCache::clearCases()
{
    m_cases.clear();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoExploreCache::clearCases( const QString& assetName )
{
    m_cases.erase( assetName );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoExploreCache::clearEnsembleNames()
{
    m_ensembleNames.clear();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaSumoExploreCache::clear()
{
    m_assets.reset();
    m_cases.clear();
    m_ensembleNames.clear();
    m_userName.clear();
}
