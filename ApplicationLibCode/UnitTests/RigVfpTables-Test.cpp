/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2024- Equinor ASA
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

#include "ProjectDataModel/VerticalFlowPerformance/RimVfpDefines.h"
#include "RigVfpTables.h"

#include "opm/input/eclipse/Units/UnitSystem.hpp"

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RigVfpTables, MatchingValues )
{
    std::vector<double> sourceValues   = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    std::vector<double> valuesForMatch = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    std::vector<int>    closestIndices = RigVfpTables::uniqueClosestIndices( sourceValues, valuesForMatch );
    for ( size_t i = 0; i < sourceValues.size(); i++ )
    {
        EXPECT_EQ( i, closestIndices[i] );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RigVfpTables, MoreDestinationValues )
{
    std::vector<double> sourceValues   = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    std::vector<double> valuesForMatch = { 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
    std::vector<int>    closestIndices = RigVfpTables::uniqueClosestIndices( sourceValues, valuesForMatch );
    for ( size_t i = 0; i < sourceValues.size(); i++ )
    {
        EXPECT_EQ( i, closestIndices[i] );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RigVfpTables, MatchInBetweenValues )
{
    std::vector<double> sourceValues   = { 1.0, 2.0, 3.0, 4.0, 5.0 };
    std::vector<double> valuesForMatch = { 1.0, 1.5, 2.0, 3.0, 4.0, 5.0, 6.0 };
    std::vector<int>    closestIndices = RigVfpTables::uniqueClosestIndices( sourceValues, valuesForMatch );

    EXPECT_EQ( 5, (int)closestIndices.size() );

    EXPECT_EQ( 0, closestIndices[0] );
    EXPECT_EQ( 2, closestIndices[1] );
    EXPECT_EQ( 3, closestIndices[2] );
    EXPECT_EQ( 4, closestIndices[3] );
    EXPECT_EQ( 5, closestIndices[4] );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RigVfpTables, SingleValue )
{
    std::vector<double> sourceValues   = { 1.0, 2.0, 3.0 };
    std::vector<double> valuesForMatch = { 2.1 };
    std::vector<int>    closestIndices = RigVfpTables::uniqueClosestIndices( sourceValues, valuesForMatch );

    EXPECT_EQ( 3, (int)closestIndices.size() );

    EXPECT_EQ( -1, closestIndices[0] );
    EXPECT_EQ( 0, closestIndices[1] );
    EXPECT_EQ( -1, closestIndices[2] );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RigVfpTables, FieldUnitDisplayUnits )
{
    // Test field unit system
    Opm::UnitSystem fieldUnitSystem = Opm::UnitSystem::newFIELD();

    QString thpUnit = RigVfpTables::getDisplayUnit( RimVfpDefines::ProductionVariableType::THP, fieldUnitSystem );
    EXPECT_EQ( "psia", thpUnit.toStdString() );

    QString flowRateUnit = RigVfpTables::getDisplayUnit( RimVfpDefines::ProductionVariableType::FLOW_RATE, fieldUnitSystem );
    EXPECT_EQ( "stb/day", flowRateUnit.toStdString() );

    QString thpUnitBracket = RigVfpTables::getDisplayUnitWithBracket( RimVfpDefines::ProductionVariableType::THP, fieldUnitSystem );
    EXPECT_EQ( "[psia]", thpUnitBracket.toStdString() );

    QString flowRateUnitBracket = RigVfpTables::getDisplayUnitWithBracket( RimVfpDefines::ProductionVariableType::FLOW_RATE, fieldUnitSystem );
    EXPECT_EQ( "[stb/day]", flowRateUnitBracket.toStdString() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RigVfpTables, MetricUnitDisplayUnits )
{
    // Test metric unit system
    Opm::UnitSystem metricUnitSystem = Opm::UnitSystem::newMETRIC();

    QString thpUnit = RigVfpTables::getDisplayUnit( RimVfpDefines::ProductionVariableType::THP, metricUnitSystem );
    EXPECT_EQ( "Bar", thpUnit.toStdString() );

    QString flowRateUnit = RigVfpTables::getDisplayUnit( RimVfpDefines::ProductionVariableType::FLOW_RATE, metricUnitSystem );
    EXPECT_EQ( "Sm3/day", flowRateUnit.toStdString() );

    QString thpUnitBracket = RigVfpTables::getDisplayUnitWithBracket( RimVfpDefines::ProductionVariableType::THP, metricUnitSystem );
    EXPECT_EQ( "[Bar]", thpUnitBracket.toStdString() );

    QString flowRateUnitBracket = RigVfpTables::getDisplayUnitWithBracket( RimVfpDefines::ProductionVariableType::FLOW_RATE, metricUnitSystem );
    EXPECT_EQ( "[Sm3/day]", flowRateUnitBracket.toStdString() );
}
