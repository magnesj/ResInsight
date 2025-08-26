/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2019-     Equinor ASA
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

#include "RigEclipseResultAddress.h"

#include "RiaResultNames.h"

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RigEclipseResultAddress::RigEclipseResultAddress( RiaDefines::ResultCatType type, RiaDefines::ResultDataType dataType, const QString& resultName )
    : m_resultCatType( type )
    , m_resultDataType( dataType )
    , m_resultName( resultName )
    , m_timeLapseBaseFrameIdx( NO_TIME_LAPSE )
    , m_differenceCaseId( NO_CASE_DIFF )
    , m_divideByCellFaceArea( false )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RigEclipseResultAddress::RigEclipseResultAddress( RiaDefines::ResultCatType type,
                                                  const QString&            resultName,
                                                  int                       timeLapseBaseTimeStep /*= NO_TIME_LAPSE*/,
                                                  int                       differenceCaseId /*= NO_CASE_DIFF*/,
                                                  bool                      divideByCellFaceArea /*= false */ )
    : m_resultCatType( type )
    , m_resultDataType( RiaDefines::ResultDataType::UNKNOWN )
    , m_resultName( resultName )
    , m_timeLapseBaseFrameIdx( timeLapseBaseTimeStep )
    , m_differenceCaseId( differenceCaseId )
    , m_divideByCellFaceArea( divideByCellFaceArea )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RigEclipseResultAddress::RigEclipseResultAddress( const QString& resultName )
    : m_resultCatType( RiaDefines::ResultCatType::UNDEFINED )
    , m_resultDataType( RiaDefines::ResultDataType::UNKNOWN )
    , m_resultName( resultName )
    , m_timeLapseBaseFrameIdx( NO_TIME_LAPSE )
    , m_differenceCaseId( NO_CASE_DIFF )
    , m_divideByCellFaceArea( false )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RigEclipseResultAddress::RigEclipseResultAddress()
    : m_resultCatType( RiaDefines::ResultCatType::UNDEFINED )
    , m_resultDataType( RiaDefines::ResultDataType::UNKNOWN )
    , m_timeLapseBaseFrameIdx( NO_TIME_LAPSE )
    , m_differenceCaseId( NO_CASE_DIFF )
    , m_divideByCellFaceArea( false )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RigEclipseResultAddress::isValid() const
{
    return !( m_resultName.isEmpty() || m_resultName == RiaResultNames::undefinedResultName() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RigEclipseResultAddress::isDeltaTimeStepActive() const
{
    return m_timeLapseBaseFrameIdx > NO_TIME_LAPSE;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigEclipseResultAddress::setDeltaTimeStepIndex( int timeStepIndex )
{
    m_timeLapseBaseFrameIdx = timeStepIndex;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RigEclipseResultAddress::deltaTimeStepIndex() const
{
    return m_timeLapseBaseFrameIdx;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RigEclipseResultAddress::representsAllTimeLapses() const
{
    return m_timeLapseBaseFrameIdx == ALL_TIME_LAPSES;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RigEclipseResultAddress::isDeltaCaseActive() const
{
    return m_differenceCaseId > NO_CASE_DIFF;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigEclipseResultAddress::setDeltaCaseId( int caseId )
{
    m_differenceCaseId = caseId;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RigEclipseResultAddress::deltaCaseId() const
{
    return m_differenceCaseId;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigEclipseResultAddress::enableDivideByCellFaceArea( bool enable )
{
    m_divideByCellFaceArea = enable;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RigEclipseResultAddress::isDivideByCellFaceAreaActive() const
{
    return m_divideByCellFaceArea;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RigEclipseResultAddress::operator<( const RigEclipseResultAddress& other ) const
{
    if ( m_divideByCellFaceArea != other.m_divideByCellFaceArea )
    {
        return ( m_divideByCellFaceArea < other.m_divideByCellFaceArea );
    }

    if ( m_differenceCaseId != other.m_differenceCaseId )
    {
        return ( m_differenceCaseId < other.m_differenceCaseId );
    }

    if ( m_timeLapseBaseFrameIdx != other.m_timeLapseBaseFrameIdx )
    {
        return ( m_timeLapseBaseFrameIdx < other.m_timeLapseBaseFrameIdx );
    }

    if ( m_resultCatType != other.m_resultCatType )
    {
        return ( m_resultCatType < other.m_resultCatType );
    }

    return ( m_resultName < other.m_resultName );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const QString& RigEclipseResultAddress::resultName() const
{
    return m_resultName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigEclipseResultAddress::setResultName( QString name )
{
    m_resultName = name;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaDefines::ResultCatType RigEclipseResultAddress::resultCatType() const
{
    return m_resultCatType;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigEclipseResultAddress::setResultCatType( RiaDefines::ResultCatType catType )
{
    m_resultCatType = catType;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RigEclipseResultAddress::operator==( const RigEclipseResultAddress& other ) const
{
    return m_resultCatType == other.m_resultCatType && m_resultName == other.m_resultName &&
           m_timeLapseBaseFrameIdx == other.m_timeLapseBaseFrameIdx && m_differenceCaseId == other.m_differenceCaseId &&
           m_divideByCellFaceArea == other.m_divideByCellFaceArea;
}
