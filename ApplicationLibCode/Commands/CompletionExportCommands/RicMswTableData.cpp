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

#include "RicMswTableData.h"

#include <algorithm>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RicMswTableData::RicMswTableData( const QString& wellName, RiaDefines::EclipseUnitSystem unitSystem )
    : m_wellName( wellName )
    , m_unitSystem( unitSystem )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicMswTableData::setWelsegsHeader( const WelsegsHeader& header )
{
    m_welsegsHeader = header;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicMswTableData::addWelsegsRow( const WelsegsRow& row )
{
    m_welsegsData.push_back( row );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicMswTableData::addCompsegsRow( const CompsegsRow& row )
{
    m_compsegsData.push_back( row );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicMswTableData::addWsegvalvRow( const WsegvalvRow& row )
{
    m_wsegvalvData.push_back( row );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicMswTableData::addWsegaicdRow( const WsegaicdRow& row )
{
    m_wsegaicdData.push_back( row );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicMswTableData::hasLgrData() const
{
    return std::any_of( m_compsegsData.begin(), m_compsegsData.end(), []( const CompsegsRow& row ) { return row.isLgrGrid(); } );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicMswTableData::isEmpty() const
{
    return m_welsegsData.empty() && m_compsegsData.empty() && m_wsegvalvData.empty() && m_wsegaicdData.empty();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<CompsegsRow> RicMswTableData::mainGridCompsegsData() const
{
    std::vector<CompsegsRow> mainGridRows;
    std::copy_if( m_compsegsData.begin(),
                  m_compsegsData.end(),
                  std::back_inserter( mainGridRows ),
                  []( const CompsegsRow& row ) { return row.isMainGrid(); } );
    return mainGridRows;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<CompsegsRow> RicMswTableData::lgrCompsegsData() const
{
    std::vector<CompsegsRow> lgrRows;
    std::copy_if( m_compsegsData.begin(),
                  m_compsegsData.end(),
                  std::back_inserter( lgrRows ),
                  []( const CompsegsRow& row ) { return row.isLgrGrid(); } );
    return lgrRows;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicMswTableData::isValid() const
{
    return validationErrors().empty();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<QString> RicMswTableData::validationErrors() const
{
    std::vector<QString> errors;

    if ( m_wellName.isEmpty() )
    {
        errors.push_back( "Well name is empty" );
    }

    if ( m_welsegsData.empty() )
    {
        errors.push_back( "No WELSEGS data" );
    }

    // Check for duplicate segment numbers in WELSEGS
    std::vector<int> segmentNumbers;
    for ( const auto& row : m_welsegsData )
    {
        segmentNumbers.push_back( row.segmentNumber );
    }
    std::sort( segmentNumbers.begin(), segmentNumbers.end() );
    auto it = std::adjacent_find( segmentNumbers.begin(), segmentNumbers.end() );
    if ( it != segmentNumbers.end() )
    {
        errors.push_back( QString( "Duplicate segment number: %1" ).arg( *it ) );
    }

    return errors;
}