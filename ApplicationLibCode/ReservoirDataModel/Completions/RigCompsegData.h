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

#include "RigCompletionData.h"
#include "RigCompletionDataGridCell.h"

#include <QString>

//==================================================================================================
///  Minimal data structure for COMPSEGS table export
//==================================================================================================
class RigCompsegData
{
public:
    RigCompsegData( const QString&                    wellName,
                    const RigCompletionDataGridCell&  gridCell,
                    int                               branchNumber,
                    double                            startLength,
                    double                            endLength,
                    RigCompletionData::CompletionType completionType = RigCompletionData::CompletionType::CT_UNDEFINED );

    bool operator<( const RigCompsegData& other ) const;

    // Required COMPSEGS fields
    const QString&                   wellName() const;
    const RigCompletionDataGridCell& gridCell() const;
    int                              branchNumber() const;
    double                           startLength() const;
    double                           endLength() const;

    // Optional COMPSEGS fields
    const QString& directionPenetration() const;
    double         endRange() const;
    double         connectionDepth() const;
    RigCompletionData::CompletionType completionType() const;

    void setDirectionPenetration( const QString& dir );
    void setEndRange( double range );
    void setConnectionDepth( double depth );
    void setCompletionType( RigCompletionData::CompletionType type );

    // LGR support
    bool isMainGrid() const;
    QString lgrName() const;

private:
    // Required fields
    QString                   m_wellName;
    RigCompletionDataGridCell m_gridCell;
    int                       m_branchNumber;
    double                    m_startLength;
    double                    m_endLength;

    // Optional fields
    QString                           m_directionPenetration;
    double                            m_endRange;
    double                            m_connectionDepth;
    RigCompletionData::CompletionType m_completionType;
};