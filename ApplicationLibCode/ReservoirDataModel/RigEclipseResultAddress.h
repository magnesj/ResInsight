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

#include "RiaDefines.h"

#include <QString>

class RigEclipseResultAddress
{
public:
    RigEclipseResultAddress();
    explicit RigEclipseResultAddress( const QString& resultName );
    explicit RigEclipseResultAddress( RiaDefines::ResultCatType type,
                                      const QString&            resultName,
                                      int                       timeLapseBaseTimeStep = NO_TIME_LAPSE,
                                      int                       differenceCaseId      = NO_CASE_DIFF,
                                      bool                      divideByCellFaceArea  = false );
    explicit RigEclipseResultAddress( RiaDefines::ResultCatType type, RiaDefines::ResultDataType dataType, const QString& resultName );

    bool isValid() const;

    void                       setDataType( RiaDefines::ResultDataType dataType ) { m_resultDataType = dataType; }
    RiaDefines::ResultDataType dataType() const { return m_resultDataType; }

    // Delta Time Step
    bool                 isDeltaTimeStepActive() const;
    void                 setDeltaTimeStepIndex( int timeStepIndex );
    int                  deltaTimeStepIndex() const;
    bool                 representsAllTimeLapses() const;
    static constexpr int allTimeLapsesValue() { return ALL_TIME_LAPSES; }
    static constexpr int noTimeLapseValue() { return NO_TIME_LAPSE; }
    // Delta Grid Case
    bool                 isDeltaCaseActive() const;
    void                 setDeltaCaseId( int caseId );
    int                  deltaCaseId() const;
    static constexpr int noCaseDiffValue() { return NO_CASE_DIFF; }

    // Divide by Cell Face Area
    void enableDivideByCellFaceArea( bool enable );
    bool isDivideByCellFaceAreaActive() const;

    bool operator<( const RigEclipseResultAddress& other ) const;
    bool operator==( const RigEclipseResultAddress& other ) const;

    const QString& resultName() const;
    void           setResultName( QString name );

    RiaDefines::ResultCatType resultCatType() const;
    void                      setResultCatType( RiaDefines::ResultCatType catType );

private:
    RiaDefines::ResultCatType  m_resultCatType;
    RiaDefines::ResultDataType m_resultDataType;
    QString                    m_resultName;
    int                        m_timeLapseBaseFrameIdx;
    int                        m_differenceCaseId;
    bool                       m_divideByCellFaceArea;

    static const int ALL_TIME_LAPSES = -2;
    static const int NO_TIME_LAPSE   = -1;
    static const int NO_CASE_DIFF    = -1;
};
