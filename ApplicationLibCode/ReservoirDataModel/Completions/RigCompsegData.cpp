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

namespace internal
{
//==================================================================================================
///
//==================================================================================================
int completionTypeSortOrder( RigCompletionData::CompletionType type )
{
    switch ( type )
    {
        case RigCompletionData::CompletionType::PERFORATION:
            return 0;
        case RigCompletionData::CompletionType::PERFORATION_ICD:
            return 1;
        case RigCompletionData::CompletionType::PERFORATION_ICV:
            return 2;
        case RigCompletionData::CompletionType::PERFORATION_AICD:
            return 3;
        case RigCompletionData::CompletionType::FISHBONES_ICD:
            return 4;
        case RigCompletionData::CompletionType::FISHBONES:
            return 5;
        case RigCompletionData::CompletionType::FRACTURE:
            return 6;
        case RigCompletionData::CompletionType::CT_UNDEFINED:
        default:
            return 7;
    }
}
} // namespace internal

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

    if ( m_completionType != other.m_completionType )
    {
        return ( internal::completionTypeSortOrder( m_completionType ) < internal::completionTypeSortOrder( other.m_completionType ) );
    }

    if ( m_startLength != other.m_startLength )
    {
        return ( m_startLength < other.m_startLength );
    }

    return m_gridCell < other.m_gridCell;
}

//==================================================================================================
///
//==================================================================================================
const QString& RigCompsegData::wellName() const
{
    return m_wellName;
}

//==================================================================================================
///
//==================================================================================================
const RigCompletionDataGridCell& RigCompsegData::gridCell() const
{
    return m_gridCell;
}

//==================================================================================================
///
//==================================================================================================
int RigCompsegData::branchNumber() const
{
    return m_branchNumber;
}

//==================================================================================================
///
//==================================================================================================
double RigCompsegData::startLength() const
{
    return m_startLength;
}

//==================================================================================================
///
//==================================================================================================
double RigCompsegData::endLength() const
{
    return m_endLength;
}

//==================================================================================================
///
//==================================================================================================
const QString& RigCompsegData::directionPenetration() const
{
    return m_directionPenetration;
}

//==================================================================================================
///
//==================================================================================================
double RigCompsegData::endRange() const
{
    return m_endRange;
}

//==================================================================================================
///
//==================================================================================================
double RigCompsegData::connectionDepth() const
{
    return m_connectionDepth;
}

//==================================================================================================
///
//==================================================================================================
RigCompletionData::CompletionType RigCompsegData::completionType() const
{
    return m_completionType;
}

//==================================================================================================
///
//==================================================================================================
void RigCompsegData::setDirectionPenetration( const QString& dir )
{
    m_directionPenetration = dir;
}

//==================================================================================================
///
//==================================================================================================
void RigCompsegData::setEndRange( double range )
{
    m_endRange = range;
}

//==================================================================================================
///
//==================================================================================================
void RigCompsegData::setConnectionDepth( double depth )
{
    m_connectionDepth = depth;
}

//==================================================================================================
///
//==================================================================================================
void RigCompsegData::setCompletionType( RigCompletionData::CompletionType type )
{
    m_completionType = type;
}

//==================================================================================================
///
//==================================================================================================
bool RigCompsegData::isMainGrid() const
{
    return m_gridCell.isMainGridCell();
}

//==================================================================================================
///
//==================================================================================================
QString RigCompsegData::lgrName() const
{
    return m_gridCell.lgrName();
}
