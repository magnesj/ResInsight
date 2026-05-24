/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2024-     Equinor ASA
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

#include "gtest/gtest.h"

#include "RifResqmlFileTools.h"
#include "RigEclipseCaseData.h"
#include "RigMainGrid.h"

#include <QFile>
#include <QString>

static const QString BLOCK_EPC_PATH = "c:/gitroot/resqpy/example_data/block.epc";

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RifResqmlFileTools, HasGridData )
{
    if ( !QFile::exists( BLOCK_EPC_PATH ) )
    {
        GTEST_SKIP() << "Test file not found: " << BLOCK_EPC_PATH.toStdString();
    }

    EXPECT_TRUE( RifResqmlFileTools::hasGridData( BLOCK_EPC_PATH ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RifResqmlFileTools, OpenGridFile )
{
    if ( !QFile::exists( BLOCK_EPC_PATH ) )
    {
        GTEST_SKIP() << "Test file not found: " << BLOCK_EPC_PATH.toStdString();
    }

    auto eclipseCase = std::make_unique<RigEclipseCaseData>( nullptr );

    QString errorMessages;
    bool    success = RifResqmlFileTools::openGridFile( BLOCK_EPC_PATH, eclipseCase.get(), &errorMessages );

    EXPECT_TRUE( success ) << "openGridFile failed: " << errorMessages.toStdString();
    EXPECT_TRUE( errorMessages.isEmpty() ) << errorMessages.toStdString();

    RigMainGrid* mainGrid = eclipseCase->mainGrid();
    ASSERT_NE( nullptr, mainGrid );

    EXPECT_GT( mainGrid->cellCount(), 0u );
    EXPECT_GT( mainGrid->nodes().size(), 0u );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RifResqmlFileTools, CreateInputProperties )
{
    if ( !QFile::exists( BLOCK_EPC_PATH ) )
    {
        GTEST_SKIP() << "Test file not found: " << BLOCK_EPC_PATH.toStdString();
    }

    auto eclipseCase = std::make_unique<RigEclipseCaseData>( nullptr );

    QString errorMessages;
    RifResqmlFileTools::openGridFile( BLOCK_EPC_PATH, eclipseCase.get(), &errorMessages );

    auto [success, keywordMapping] = RifResqmlFileTools::createInputProperties( BLOCK_EPC_PATH, eclipseCase.get() );

    EXPECT_TRUE( success );
    EXPECT_FALSE( keywordMapping.empty() );
}
