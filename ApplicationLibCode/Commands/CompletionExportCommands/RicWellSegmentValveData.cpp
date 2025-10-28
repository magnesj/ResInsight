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

#include "RicWellSegmentValveData.h"

#include <QString>
#include <limits>

//==================================================================================================
///
//==================================================================================================
RicWellSegmentValveData::RicWellSegmentValveData( const QString& wellName, int segmentNumber )
    : m_wellName( wellName )
    , m_segmentNumber( segmentNumber )
    , m_flowCoefficient( std::numeric_limits<double>::infinity() )
    , m_area( std::numeric_limits<double>::infinity() )
    , m_additionalPipeLength( std::numeric_limits<double>::infinity() )
{
}

//==================================================================================================
///
//==================================================================================================
RicWellSegmentValveData::~RicWellSegmentValveData()
{
}

//==================================================================================================
///
//==================================================================================================
bool RicWellSegmentValveData::operator<( const RicWellSegmentValveData& other ) const
{
    if ( m_wellName != other.m_wellName )
    {
        return ( m_wellName < other.m_wellName );
    }

    return m_segmentNumber < other.m_segmentNumber;
}

//==================================================================================================
///
//==================================================================================================
void RicWellSegmentValveData::setFlowCoefficient( double flowCoefficient )
{
    m_flowCoefficient = flowCoefficient;
}

//==================================================================================================
///
//==================================================================================================
void RicWellSegmentValveData::setArea( double area )
{
    m_area = area;
}

//==================================================================================================
///
//==================================================================================================
void RicWellSegmentValveData::setAdditionalPipeLength( double length )
{
    m_additionalPipeLength = length;
}

//==================================================================================================
///
//==================================================================================================
void RicWellSegmentValveData::setValveType( const QString& valveType )
{
    m_valveType = valveType;
}

//==================================================================================================
///
//==================================================================================================
void RicWellSegmentValveData::setComment( const QString& comment )
{
    m_comment = comment;
}

//==================================================================================================
///
//==================================================================================================
double RicWellSegmentValveData::defaultValue()
{
    return std::numeric_limits<double>::infinity();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicWellSegmentValveData::isDefaultValue( double num )
{
    return num == defaultValue();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const QString& RicWellSegmentValveData::wellName() const
{
    return m_wellName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RicWellSegmentValveData::segmentNumber() const
{
    return m_segmentNumber;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RicWellSegmentValveData::flowCoefficient() const
{
    return m_flowCoefficient;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RicWellSegmentValveData::area() const
{
    return m_area;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RicWellSegmentValveData::additionalPipeLength() const
{
    return m_additionalPipeLength;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const QString& RicWellSegmentValveData::valveType() const
{
    return m_valveType;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const QString& RicWellSegmentValveData::comment() const
{
    return m_comment;
}