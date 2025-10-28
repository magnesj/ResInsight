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
#include <optional>
#include <vector>

//==================================================================================================
///
//==================================================================================================
class RicWellSegmentValveData
{
public:
    RicWellSegmentValveData( const QString& wellName, int segmentNumber );
    ~RicWellSegmentValveData();

    bool operator<( const RicWellSegmentValveData& other ) const;

    void setFlowCoefficient( double flowCoefficient );
    void setArea( double area );
    void setAdditionalPipeLength( double length );
    void setValveType( const QString& valveType );
    void setComment( const QString& comment );

    const QString& wellName() const;
    int            segmentNumber() const;
    double         flowCoefficient() const;
    double         area() const;
    double         additionalPipeLength() const;
    const QString& valveType() const;
    const QString& comment() const;

    static double defaultValue();
    static bool   isDefaultValue( double num );

private:
    QString m_wellName;
    int     m_segmentNumber;
    double  m_flowCoefficient;
    double  m_area;
    double  m_additionalPipeLength;
    QString m_valveType;
    QString m_comment;
};