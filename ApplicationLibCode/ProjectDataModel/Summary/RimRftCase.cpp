/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2022- Equinor ASA
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

#include "RimRftCase.h"

//==================================================================================================
//
//
//
//==================================================================================================
CAF_PDM_SOURCE_INIT( RimRftCase, "RimRftCase" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimRftCase::RimRftCase()
{
    CAF_PDM_InitObject( "RFT Case ", ":/SummaryCases16x16.png", "", "" );

    CAF_PDM_InitFieldNoDefault( &m_rftFilePath, "RftFilePath", "Rft File" );
    m_rftFilePath.uiCapability()->setUiReadOnly( true );

    CAF_PDM_InitFieldNoDefault( &m_dataDeckFilePath, "DataDeckFilePath", "Data Deck File" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimRftCase::setDataDeckFileName( const QString& fileName )
{
    m_dataDeckFilePath.v().setPath( fileName );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimRftCase::setRftFileName( const QString& fileName )
{
    m_rftFilePath.v().setPath( fileName );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimRftCase::rftFilePath() const
{
    return m_rftFilePath().path();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimRftCase::dataDeckFilePath() const
{
    return m_dataDeckFilePath().path();
}