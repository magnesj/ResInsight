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

#include "RicWellSegmentCompletionData.h"

#include <QString>
#include <limits>

//==================================================================================================
///
//==================================================================================================
RicWellSegmentCompletionData::RicWellSegmentCompletionData( const QString& wellName, const QString& gridName )
    : m_wellName( wellName )
    , m_gridName( gridName )
    , m_i( -1 )
    , m_j( -1 )
    , m_k( -1 )
    , m_branchNumber( -1 )
    , m_startLength( std::numeric_limits<double>::infinity() )
    , m_endLength( std::numeric_limits<double>::infinity() )
    , m_connectionDepth( std::numeric_limits<double>::infinity() )
    , m_endRange( std::numeric_limits<double>::infinity() )
{
}

//==================================================================================================
///
//==================================================================================================
RicWellSegmentCompletionData::~RicWellSegmentCompletionData()
{
}

//==================================================================================================
///
//==================================================================================================
bool RicWellSegmentCompletionData::operator<( const RicWellSegmentCompletionData& other ) const
{
    if ( m_wellName != other.m_wellName )
    {
        return ( m_wellName < other.m_wellName );
    }

    if ( m_gridName != other.m_gridName )
    {
        return ( m_gridName < other.m_gridName );
    }

    if ( m_i != other.m_i )
    {
        return ( m_i < other.m_i );
    }

    if ( m_j != other.m_j )
    {
        return ( m_j < other.m_j );
    }

    return m_k < other.m_k;
}

//==================================================================================================
///
//==================================================================================================
void RicWellSegmentCompletionData::setGridCell( int i, int j, int k )
{
    m_i = i;
    m_j = j;
    m_k = k;
}

//==================================================================================================
///
//==================================================================================================
void RicWellSegmentCompletionData::setBranchNumber( int branchNumber )
{
    m_branchNumber = branchNumber;
}

//==================================================================================================
///
//==================================================================================================
void RicWellSegmentCompletionData::setSegmentRange( double startLength, double endLength )
{
    m_startLength = startLength;
    m_endLength = endLength;
}

//==================================================================================================
///
//==================================================================================================
void RicWellSegmentCompletionData::setConnectionDepth( double depth )
{
    m_connectionDepth = depth;
}

//==================================================================================================
///
//==================================================================================================
void RicWellSegmentCompletionData::setDirectionPenetration( const QString& dirPen )
{
    m_directionPenetration = dirPen;
}

//==================================================================================================
///
//==================================================================================================
void RicWellSegmentCompletionData::setEndRange( double endRange )
{
    m_endRange = endRange;
}

//==================================================================================================
///
//==================================================================================================
void RicWellSegmentCompletionData::setComment( const QString& comment )
{
    m_comment = comment;
}

//==================================================================================================
///
//==================================================================================================
double RicWellSegmentCompletionData::defaultValue()
{
    return std::numeric_limits<double>::infinity();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicWellSegmentCompletionData::isDefaultValue( double num )
{
    return num == defaultValue();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const QString& RicWellSegmentCompletionData::wellName() const
{
    return m_wellName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const QString& RicWellSegmentCompletionData::gridName() const
{
    return m_gridName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RicWellSegmentCompletionData::i() const
{
    return m_i;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RicWellSegmentCompletionData::j() const
{
    return m_j;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RicWellSegmentCompletionData::k() const
{
    return m_k;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RicWellSegmentCompletionData::branchNumber() const
{
    return m_branchNumber;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RicWellSegmentCompletionData::startLength() const
{
    return m_startLength;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RicWellSegmentCompletionData::endLength() const
{
    return m_endLength;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RicWellSegmentCompletionData::connectionDepth() const
{
    return m_connectionDepth;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const QString& RicWellSegmentCompletionData::directionPenetration() const
{
    return m_directionPenetration;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RicWellSegmentCompletionData::endRange() const
{
    return m_endRange;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const QString& RicWellSegmentCompletionData::comment() const
{
    return m_comment;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicWellSegmentCompletionData::isLgr() const
{
    return !m_gridName.isEmpty();
}