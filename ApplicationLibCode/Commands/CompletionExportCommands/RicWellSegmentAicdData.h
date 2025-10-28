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

#pragma once

#include <QString>
#include <array>

#include "RimWellPathAicdParameters.h"

//==================================================================================================
///
//==================================================================================================
class RicWellSegmentAicdData
{
public:
    RicWellSegmentAicdData( const QString& wellName, int segmentNumber );

    QString wellName() const;
    int     segmentNumber() const;

    void setDeviceOpen( bool deviceOpen );
    bool deviceOpen() const;

    void   setLength( double length );
    double length() const;

    void   setFlowScalingFactor( double scalingFactor );
    double flowScalingFactor() const;

    void                                       setParameters( const std::array<double, AICD_NUM_PARAMS>& parameters );
    const std::array<double, AICD_NUM_PARAMS>& parameters() const;
    std::array<double, AICD_NUM_PARAMS>&       parameters();

    void    setComment( const QString& comment );
    QString comment() const;

    static double defaultValue();
    static bool   isDefaultValue( double value );

private:
    QString m_wellName;
    int     m_segmentNumber;
    bool    m_deviceOpen;
    double  m_length;
    double  m_flowScalingFactor;
    QString m_comment;

    std::array<double, AICD_NUM_PARAMS> m_parameters;
};