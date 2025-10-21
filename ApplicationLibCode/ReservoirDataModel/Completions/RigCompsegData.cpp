/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2025-     Equinor ASA
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

#include "RigCompsegData.h"

#include <limits>

//==================================================================================================
///
//==================================================================================================
RigCompsegData::RigCompsegData( const QString&                    wellName,
                                const RigCompletionDataGridCell&  gridCell,
                                int                               branchNumber,
                                double                            startLength,
                                double                            endLength,
                                RigCompletionData::CompletionType completionType )
    : m_wellName( wellName )
    , m_gridCell( gridCell )
    , m_branchNumber( branchNumber )
    , m_startLength( startLength )
    , m_endLength( endLength )
    , m_endRange( std::numeric_limits<double>::infinity() )
    , m_connectionDepth( std::numeric_limits<double>::infinity() )
    , m_completionType( completionType )
{
}

//==================================================================================================
///
//==================================================================================================
bool RigCompsegData::operator<( const RigCompsegData& other ) const
{
    if ( m_wellName != other.m_wellName )
    {
        return ( m_wellName < other.m_wellName );
    }

    if ( m_branchNumber != other.m_branchNumber )
    {
        return ( m_branchNumber < other.m_branchNumber );
    }

    if ( m_startLength != other.m_startLength )
    {
        return ( m_startLength < other.m_startLength );
    }

    return m_gridCell < other.m_gridCell;
}