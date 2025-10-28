/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2024 Equinor ASA
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

#include "RicWellSegmentData.h"

#include <QString>

#include <limits>

//==================================================================================================
///
//==================================================================================================
RicWellSegmentData::RicWellSegmentData( const QString& wellName, int segmentNumber )
    : m_wellName( wellName )
    , m_segmentNumber( segmentNumber )
    , m_branch( 0 )
    , m_joinSegment( 0 )
    , m_length( std::numeric_limits<double>::infinity() )
    , m_depth( std::numeric_limits<double>::infinity() )
    , m_diameter( std::numeric_limits<double>::infinity() )
    , m_roughness( std::numeric_limits<double>::infinity() )
    , m_area( std::numeric_limits<double>::infinity() )
    , m_volume( std::numeric_limits<double>::infinity() )
    , m_lengthX( std::nullopt )
    , m_lengthY( std::nullopt )
{
}

//==================================================================================================
///
//==================================================================================================
RicWellSegmentData::~RicWellSegmentData()
{
}

//==================================================================================================
///
//==================================================================================================
bool RicWellSegmentData::operator<( const RicWellSegmentData& other ) const
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
void RicWellSegmentData::setSegmentProperties( int    branch,
                                                int    joinSegment,
                                                double length,
                                                double depth,
                                                double diameter,
                                                double roughness,
                                                double area,
                                                double volume )
{
    m_branch     = branch;
    m_joinSegment = joinSegment;
    m_length     = length;
    m_depth      = depth;
    m_diameter   = diameter;
    m_roughness  = roughness;
    m_area       = area;
    m_volume     = volume;
}

//==================================================================================================
///
//==================================================================================================
void RicWellSegmentData::setSegmentCoordinates( double lengthX, double lengthY )
{
    m_lengthX = lengthX;
    m_lengthY = lengthY;
}

//==================================================================================================
///
//==================================================================================================
void RicWellSegmentData::setComment( const QString& comment )
{
    m_comment = comment;
}

//==================================================================================================
///
//==================================================================================================
double RicWellSegmentData::defaultValue()
{
    return std::numeric_limits<double>::infinity();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicWellSegmentData::isDefaultValue( double num )
{
    return num == defaultValue();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const QString& RicWellSegmentData::wellName() const
{
    return m_wellName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RicWellSegmentData::segmentNumber() const
{
    return m_segmentNumber;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RicWellSegmentData::branch() const
{
    return m_branch;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RicWellSegmentData::joinSegment() const
{
    return m_joinSegment;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RicWellSegmentData::length() const
{
    return m_length;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RicWellSegmentData::depth() const
{
    return m_depth;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RicWellSegmentData::diameter() const
{
    return m_diameter;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RicWellSegmentData::roughness() const
{
    return m_roughness;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RicWellSegmentData::area() const
{
    return m_area;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RicWellSegmentData::volume() const
{
    return m_volume;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::optional<double> RicWellSegmentData::lengthX() const
{
    return m_lengthX;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::optional<double> RicWellSegmentData::lengthY() const
{
    return m_lengthY;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const QString& RicWellSegmentData::comment() const
{
    return m_comment;
}

