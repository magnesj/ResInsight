/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2025 Equinor ASA
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

#include "RicMswUnifiedData.h"

#include <algorithm>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicMswUnifiedData::addWellData( RicMswTableData wellData )
{
    // Check if well name already exists
    const QString& wellName = wellData.wellName();
    auto           it       = std::find_if( m_wellDataList.begin(),
                            m_wellDataList.end(),
                            [&wellName]( const RicMswTableData& data ) { return data.wellName() == wellName; } );

    if ( it != m_wellDataList.end() )
    {
        // Replace existing data
        *it = std::move( wellData );
    }
    else
    {
        // Add new data
        m_wellDataList.push_back( std::move( wellData ) );
    }

    updateIndex();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicMswUnifiedData::clear()
{
    m_wellDataList.clear();
    m_wellNameToIndex.clear();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const RicMswTableData* RicMswUnifiedData::getWellData( const QString& wellName ) const
{
    auto it = m_wellNameToIndex.find( wellName );
    if ( it != m_wellNameToIndex.end() && it->second < m_wellDataList.size() )
    {
        return &m_wellDataList[it->second];
    }
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<WelsegsHeader> RicMswUnifiedData::getAllWelsegsHeaders() const
{
    std::vector<WelsegsHeader> allHeaders;

    for ( const auto& wellData : m_wellDataList )
    {
        if ( wellData.welsegsHeader().has_value() )
        {
            allHeaders.push_back( wellData.welsegsHeader().value() );
        }
    }

    return allHeaders;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<WelsegsRow> RicMswUnifiedData::getAllWelsegsRows() const
{
    std::vector<WelsegsRow> allRows;

    for ( const auto& wellData : m_wellDataList )
    {
        const auto& wellRows = wellData.welsegsData();
        allRows.insert( allRows.end(), wellRows.begin(), wellRows.end() );
    }

    return allRows;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<CompsegsRow> RicMswUnifiedData::getAllCompsegsRows( bool lgrOnly ) const
{
    std::vector<CompsegsRow> allRows;

    for ( const auto& wellData : m_wellDataList )
    {
        std::vector<CompsegsRow> wellRows;

        if ( lgrOnly )
        {
            wellRows = wellData.lgrCompsegsData();
        }
        else
        {
            wellRows = wellData.mainGridCompsegsData();
        }

        allRows.insert( allRows.end(), wellRows.begin(), wellRows.end() );
    }

    return allRows;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<WsegvalvRow> RicMswUnifiedData::getAllWsegvalvRows() const
{
    std::vector<WsegvalvRow> allRows;

    for ( const auto& wellData : m_wellDataList )
    {
        const auto& wellRows = wellData.wsegvalvData();
        allRows.insert( allRows.end(), wellRows.begin(), wellRows.end() );
    }

    return allRows;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<WsegaicdRow> RicMswUnifiedData::getAllWsegaicdRows() const
{
    std::vector<WsegaicdRow> allRows;

    for ( const auto& wellData : m_wellDataList )
    {
        const auto& wellRows = wellData.wsegaicdData();
        allRows.insert( allRows.end(), wellRows.begin(), wellRows.end() );
    }

    return allRows;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicMswUnifiedData::hasAnyLgrData() const
{
    return std::any_of( m_wellDataList.begin(), m_wellDataList.end(), []( const RicMswTableData& wellData ) { return wellData.hasLgrData(); } );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<QString> RicMswUnifiedData::wellNames() const
{
    std::vector<QString> names;
    names.reserve( m_wellDataList.size() );

    std::transform( m_wellDataList.begin(),
                    m_wellDataList.end(),
                    std::back_inserter( names ),
                    []( const RicMswTableData& wellData ) { return wellData.wellName(); } );

    return names;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicMswUnifiedData::isValid() const
{
    return validationErrors().empty();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<QString> RicMswUnifiedData::validationErrors() const
{
    std::vector<QString> errors;

    if ( m_wellDataList.empty() )
    {
        errors.push_back( "No well data available" );
        return errors;
    }

    // Check each well's data validity
    for ( const auto& wellData : m_wellDataList )
    {
        if ( !wellData.isValid() )
        {
            auto wellErrors = wellData.validationErrors();
            for ( const auto& error : wellErrors )
            {
                errors.push_back( QString( "%1: %2" ).arg( wellData.wellName(), error ) );
            }
        }
    }

    // Check for duplicate well names
    std::vector<QString> names = wellNames();
    std::sort( names.begin(), names.end() );
    auto it = std::adjacent_find( names.begin(), names.end() );
    if ( it != names.end() )
    {
        errors.push_back( QString( "Duplicate well name: %1" ).arg( *it ) );
    }

    return errors;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicMswUnifiedData::updateIndex()
{
    m_wellNameToIndex.clear();

    for ( size_t i = 0; i < m_wellDataList.size(); ++i )
    {
        m_wellNameToIndex[m_wellDataList[i].wellName()] = i;
    }
}