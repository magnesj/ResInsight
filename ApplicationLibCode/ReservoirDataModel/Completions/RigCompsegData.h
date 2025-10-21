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

#pragma once

#include "RigCompletionDataGridCell.h"

#include <QString>

//==================================================================================================
///  Minimal data structure for COMPSEGS table export
//==================================================================================================
class RigCompsegData
{
public:
    RigCompsegData( const QString&                   wellName,
                    const RigCompletionDataGridCell& gridCell,
                    int                              branchNumber,
                    double                           startLength,
                    double                           endLength );

    bool operator<( const RigCompsegData& other ) const;

    // Required COMPSEGS fields
    const QString&                   wellName() const { return m_wellName; }
    const RigCompletionDataGridCell& gridCell() const { return m_gridCell; }
    int                              branchNumber() const { return m_branchNumber; }
    double                           startLength() const { return m_startLength; }
    double                           endLength() const { return m_endLength; }

    // Optional COMPSEGS fields
    const QString& directionPenetration() const { return m_directionPenetration; }
    double         endRange() const { return m_endRange; }
    double         connectionDepth() const { return m_connectionDepth; }

    void setDirectionPenetration( const QString& dir ) { m_directionPenetration = dir; }
    void setEndRange( double range ) { m_endRange = range; }
    void setConnectionDepth( double depth ) { m_connectionDepth = depth; }

private:
    // Required fields
    QString                   m_wellName;
    RigCompletionDataGridCell m_gridCell;
    int                       m_branchNumber;
    double                    m_startLength;
    double                    m_endLength;
    
    // Optional fields
    QString m_directionPenetration;
    double  m_endRange;
    double  m_connectionDepth;
};