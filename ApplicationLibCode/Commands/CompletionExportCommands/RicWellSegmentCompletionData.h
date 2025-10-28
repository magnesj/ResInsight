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
class RicWellSegmentCompletionData
{
public:
    RicWellSegmentCompletionData( const QString& wellName, const QString& gridName = QString() );
    ~RicWellSegmentCompletionData();

    bool operator<( const RicWellSegmentCompletionData& other ) const;

    void setGridCell( int i, int j, int k );
    void setBranchNumber( int branchNumber );
    void setSegmentRange( double startLength, double endLength );
    void setConnectionDepth( double depth );
    void setDirectionPenetration( const QString& dirPen );
    void setEndRange( double endRange );
    void setComment( const QString& comment );

    const QString& wellName() const;
    const QString& gridName() const;
    int            i() const;
    int            j() const;
    int            k() const;
    int            branchNumber() const;
    double         startLength() const;
    double         endLength() const;
    double         connectionDepth() const;
    const QString& directionPenetration() const;
    double         endRange() const;
    const QString& comment() const;

    bool isLgr() const;

    static double defaultValue();
    static bool   isDefaultValue( double num );

private:
    QString m_wellName;
    QString m_gridName;
    int     m_i;
    int     m_j;
    int     m_k;
    int     m_branchNumber;
    double  m_startLength;
    double  m_endLength;
    double  m_connectionDepth;
    QString m_directionPenetration;
    double  m_endRange;
    QString m_comment;
};