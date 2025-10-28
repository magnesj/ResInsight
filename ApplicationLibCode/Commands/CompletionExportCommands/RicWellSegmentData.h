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

#pragma once

#include <QString>

#include <optional>
#include <vector>

//==================================================================================================
///
//==================================================================================================
class RicWellSegmentData
{
public:
    RicWellSegmentData( const QString& wellName, int segmentNumber );
    ~RicWellSegmentData();

    bool operator<( const RicWellSegmentData& other ) const;

    void setSegmentProperties( int    branch,
                               int    joinSegment,
                               double length,
                               double depth,
                               double diameter,
                               double roughness,
                               double area,
                               double volume );

    void setSegmentCoordinates( double lengthX, double lengthY );
    void setComment( const QString& comment );

    static double                                   defaultValue();
    static bool                                     isDefaultValue( double num );
    const QString&                                  wellName() const;
    int                                             segmentNumber() const;
    int                                             branch() const;
    int                                             joinSegment() const;
    double                                          length() const;
    double                                          depth() const;
    double                                          diameter() const;
    double                                          roughness() const;
    double                                          area() const;
    double                                          volume() const;
    std::optional<double>                           lengthX() const;
    std::optional<double>                           lengthY() const;
    const QString&                                  comment() const;

private:
    QString        m_wellName;
    int            m_segmentNumber;
    int            m_branch;
    int            m_joinSegment;
    double         m_length;
    double         m_depth;
    double         m_diameter;
    double         m_roughness;
    double         m_area;
    double         m_volume;
    QString        m_comment;

    std::optional<double> m_lengthX;
    std::optional<double> m_lengthY;
};