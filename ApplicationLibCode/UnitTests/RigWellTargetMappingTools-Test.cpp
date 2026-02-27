/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2024-  Equinor ASA
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

#include "Well/RigWellTargetMappingTools.h"

#include "RigActiveCellInfo.h"
#include "RigMainGrid.h"
#include "RigNNCData.h"
#include "RigNncConnection.h"

#include "cvfStructGrid.h"

#include <limits>
#include <vector>

using FaceType    = cvf::StructGridInterface::FaceType;
using VolumesType = RigWellTargetMapping::VolumesType;
using VolumeType  = RigWellTargetMapping::VolumeType;

//--------------------------------------------------------------------------------------------------
// getValueForFace tests
//--------------------------------------------------------------------------------------------------

TEST( RigWellTargetMappingToolsTest, GetValueForFace_PosI_ReturnsXValue )
{
    std::vector<double> x = { 1.0, 2.0, 3.0 };
    std::vector<double> y = { 10.0, 20.0, 30.0 };
    std::vector<double> z = { 100.0, 200.0, 300.0 };

    EXPECT_DOUBLE_EQ( 2.0, RigWellTargetMappingTools::getValueForFace( x, y, z, FaceType::POS_I, 1 ) );
}

TEST( RigWellTargetMappingToolsTest, GetValueForFace_NegI_ReturnsXValue )
{
    std::vector<double> x = { 1.0, 2.0, 3.0 };
    std::vector<double> y = { 10.0, 20.0, 30.0 };
    std::vector<double> z = { 100.0, 200.0, 300.0 };

    EXPECT_DOUBLE_EQ( 1.0, RigWellTargetMappingTools::getValueForFace( x, y, z, FaceType::NEG_I, 0 ) );
}

TEST( RigWellTargetMappingToolsTest, GetValueForFace_PosJ_ReturnsYValue )
{
    std::vector<double> x = { 1.0, 2.0, 3.0 };
    std::vector<double> y = { 10.0, 20.0, 30.0 };
    std::vector<double> z = { 100.0, 200.0, 300.0 };

    EXPECT_DOUBLE_EQ( 30.0, RigWellTargetMappingTools::getValueForFace( x, y, z, FaceType::POS_J, 2 ) );
}

TEST( RigWellTargetMappingToolsTest, GetValueForFace_NegJ_ReturnsYValue )
{
    std::vector<double> x = { 1.0, 2.0, 3.0 };
    std::vector<double> y = { 10.0, 20.0, 30.0 };
    std::vector<double> z = { 100.0, 200.0, 300.0 };

    EXPECT_DOUBLE_EQ( 20.0, RigWellTargetMappingTools::getValueForFace( x, y, z, FaceType::NEG_J, 1 ) );
}

TEST( RigWellTargetMappingToolsTest, GetValueForFace_PosK_ReturnsZValue )
{
    std::vector<double> x = { 1.0, 2.0, 3.0 };
    std::vector<double> y = { 10.0, 20.0, 30.0 };
    std::vector<double> z = { 100.0, 200.0, 300.0 };

    EXPECT_DOUBLE_EQ( 100.0, RigWellTargetMappingTools::getValueForFace( x, y, z, FaceType::POS_K, 0 ) );
}

TEST( RigWellTargetMappingToolsTest, GetValueForFace_NegK_ReturnsZValue )
{
    std::vector<double> x = { 1.0, 2.0, 3.0 };
    std::vector<double> y = { 10.0, 20.0, 30.0 };
    std::vector<double> z = { 100.0, 200.0, 300.0 };

    EXPECT_DOUBLE_EQ( 300.0, RigWellTargetMappingTools::getValueForFace( x, y, z, FaceType::NEG_K, 2 ) );
}

TEST( RigWellTargetMappingToolsTest, GetValueForFace_NoFace_ReturnsInfinity )
{
    std::vector<double> x = { 1.0 };
    std::vector<double> y = { 10.0 };
    std::vector<double> z = { 100.0 };

    double result = RigWellTargetMappingTools::getValueForFace( x, y, z, FaceType::NO_FACE, 0 );
    EXPECT_TRUE( std::isinf( result ) );
}

//--------------------------------------------------------------------------------------------------
// getTransmissibilityValueForFace tests
//--------------------------------------------------------------------------------------------------

TEST( RigWellTargetMappingToolsTest, GetTransmissibilityValueForFace_PosI_UsesResultIndex )
{
    std::vector<double> x = { 5.0, 15.0, 25.0 };
    std::vector<double> y = { 6.0, 16.0, 26.0 };
    std::vector<double> z = { 7.0, 17.0, 27.0 };

    // For positive face, should use resultIndex (0) not neighborResultIndex (2)
    double result = RigWellTargetMappingTools::getTransmissibilityValueForFace( x, y, z, FaceType::POS_I, 0, 2 );
    EXPECT_DOUBLE_EQ( 5.0, result );
}

TEST( RigWellTargetMappingToolsTest, GetTransmissibilityValueForFace_NegI_UsesNeighborIndex )
{
    std::vector<double> x = { 5.0, 15.0, 25.0 };
    std::vector<double> y = { 6.0, 16.0, 26.0 };
    std::vector<double> z = { 7.0, 17.0, 27.0 };

    // For negative face, should use neighborResultIndex (2) not resultIndex (0)
    double result = RigWellTargetMappingTools::getTransmissibilityValueForFace( x, y, z, FaceType::NEG_I, 0, 2 );
    EXPECT_DOUBLE_EQ( 25.0, result );
}

TEST( RigWellTargetMappingToolsTest, GetTransmissibilityValueForFace_PosJ_UsesResultIndex )
{
    std::vector<double> x = { 5.0, 15.0 };
    std::vector<double> y = { 6.0, 16.0 };
    std::vector<double> z = { 7.0, 17.0 };

    double result = RigWellTargetMappingTools::getTransmissibilityValueForFace( x, y, z, FaceType::POS_J, 0, 1 );
    EXPECT_DOUBLE_EQ( 6.0, result );
}

TEST( RigWellTargetMappingToolsTest, GetTransmissibilityValueForFace_NegJ_UsesNeighborIndex )
{
    std::vector<double> x = { 5.0, 15.0 };
    std::vector<double> y = { 6.0, 16.0 };
    std::vector<double> z = { 7.0, 17.0 };

    double result = RigWellTargetMappingTools::getTransmissibilityValueForFace( x, y, z, FaceType::NEG_J, 0, 1 );
    EXPECT_DOUBLE_EQ( 16.0, result );
}

TEST( RigWellTargetMappingToolsTest, GetTransmissibilityValueForFace_PosK_UsesResultIndex )
{
    std::vector<double> x = { 5.0, 15.0 };
    std::vector<double> y = { 6.0, 16.0 };
    std::vector<double> z = { 7.0, 17.0 };

    double result = RigWellTargetMappingTools::getTransmissibilityValueForFace( x, y, z, FaceType::POS_K, 0, 1 );
    EXPECT_DOUBLE_EQ( 7.0, result );
}

TEST( RigWellTargetMappingToolsTest, GetTransmissibilityValueForFace_NegK_UsesNeighborIndex )
{
    std::vector<double> x = { 5.0, 15.0 };
    std::vector<double> y = { 6.0, 16.0 };
    std::vector<double> z = { 7.0, 17.0 };

    double result = RigWellTargetMappingTools::getTransmissibilityValueForFace( x, y, z, FaceType::NEG_K, 0, 1 );
    EXPECT_DOUBLE_EQ( 17.0, result );
}

//--------------------------------------------------------------------------------------------------
// getOilVectorName tests
//--------------------------------------------------------------------------------------------------

TEST( RigWellTargetMappingToolsTest, GetOilVectorName_ReservoirVolumes )
{
    EXPECT_EQ( QString( "RFIPOIL" ), RigWellTargetMappingTools::getOilVectorName( VolumesType::RESERVOIR_VOLUMES ) );
}

TEST( RigWellTargetMappingToolsTest, GetOilVectorName_SurfaceVolumesSfip )
{
    EXPECT_EQ( QString( "SFIPOIL" ), RigWellTargetMappingTools::getOilVectorName( VolumesType::SURFACE_VOLUMES_SFIP ) );
}

TEST( RigWellTargetMappingToolsTest, GetOilVectorName_SurfaceVolumesFip )
{
    EXPECT_EQ( QString( "FIPOIL" ), RigWellTargetMappingTools::getOilVectorName( VolumesType::SURFACE_VOLUMES_FIP ) );
}

TEST( RigWellTargetMappingToolsTest, GetOilVectorName_ReservoirVolumesComputed )
{
    EXPECT_EQ( QString( "riPORV*SOIL" ), RigWellTargetMappingTools::getOilVectorName( VolumesType::RESERVOIR_VOLUMES_COMPUTED ) );
}

//--------------------------------------------------------------------------------------------------
// getGasVectorName tests
//--------------------------------------------------------------------------------------------------

TEST( RigWellTargetMappingToolsTest, GetGasVectorName_ReservoirVolumes )
{
    EXPECT_EQ( QString( "RFIPGAS" ), RigWellTargetMappingTools::getGasVectorName( VolumesType::RESERVOIR_VOLUMES ) );
}

TEST( RigWellTargetMappingToolsTest, GetGasVectorName_SurfaceVolumesSfip )
{
    EXPECT_EQ( QString( "SFIPGAS" ), RigWellTargetMappingTools::getGasVectorName( VolumesType::SURFACE_VOLUMES_SFIP ) );
}

TEST( RigWellTargetMappingToolsTest, GetGasVectorName_SurfaceVolumesFip )
{
    EXPECT_EQ( QString( "FIPGAS" ), RigWellTargetMappingTools::getGasVectorName( VolumesType::SURFACE_VOLUMES_FIP ) );
}

TEST( RigWellTargetMappingToolsTest, GetGasVectorName_ReservoirVolumesComputed )
{
    EXPECT_EQ( QString( "riPORV*SGAS" ), RigWellTargetMappingTools::getGasVectorName( VolumesType::RESERVOIR_VOLUMES_COMPUTED ) );
}

//--------------------------------------------------------------------------------------------------
// isSaturationSufficient tests
//--------------------------------------------------------------------------------------------------

TEST( RigWellTargetMappingToolsTest, IsSaturationSufficient_OilAboveThreshold_ReturnsTrue )
{
    RigWellTargetMapping::DataContainer data;
    data.saturationOil = { 0.3, 0.1 };
    data.saturationGas = { 0.0, 0.0 };

    RigWellTargetMapping::ClusteringLimits limits;
    limits.saturationOil = 0.2;
    limits.saturationGas = 0.1;

    EXPECT_TRUE( RigWellTargetMappingTools::isSaturationSufficient( VolumeType::OIL, data, limits, 0 ) );
}

TEST( RigWellTargetMappingToolsTest, IsSaturationSufficient_OilBelowThreshold_ReturnsFalse )
{
    RigWellTargetMapping::DataContainer data;
    data.saturationOil = { 0.3, 0.1 };
    data.saturationGas = { 0.0, 0.0 };

    RigWellTargetMapping::ClusteringLimits limits;
    limits.saturationOil = 0.2;
    limits.saturationGas = 0.1;

    EXPECT_FALSE( RigWellTargetMappingTools::isSaturationSufficient( VolumeType::OIL, data, limits, 1 ) );
}

TEST( RigWellTargetMappingToolsTest, IsSaturationSufficient_OilAtThreshold_ReturnsTrue )
{
    RigWellTargetMapping::DataContainer data;
    data.saturationOil = { 0.2 };
    data.saturationGas = { 0.0 };

    RigWellTargetMapping::ClusteringLimits limits;
    limits.saturationOil = 0.2;
    limits.saturationGas = 0.1;

    EXPECT_TRUE( RigWellTargetMappingTools::isSaturationSufficient( VolumeType::OIL, data, limits, 0 ) );
}

TEST( RigWellTargetMappingToolsTest, IsSaturationSufficient_GasAboveThreshold_ReturnsTrue )
{
    RigWellTargetMapping::DataContainer data;
    data.saturationOil = { 0.0 };
    data.saturationGas = { 0.5 };

    RigWellTargetMapping::ClusteringLimits limits;
    limits.saturationOil = 0.2;
    limits.saturationGas = 0.3;

    EXPECT_TRUE( RigWellTargetMappingTools::isSaturationSufficient( VolumeType::GAS, data, limits, 0 ) );
}

TEST( RigWellTargetMappingToolsTest, IsSaturationSufficient_GasBelowThreshold_ReturnsFalse )
{
    RigWellTargetMapping::DataContainer data;
    data.saturationOil = { 0.0 };
    data.saturationGas = { 0.1 };

    RigWellTargetMapping::ClusteringLimits limits;
    limits.saturationOil = 0.2;
    limits.saturationGas = 0.3;

    EXPECT_FALSE( RigWellTargetMappingTools::isSaturationSufficient( VolumeType::GAS, data, limits, 0 ) );
}

TEST( RigWellTargetMappingToolsTest, IsSaturationSufficient_Hydrocarbon_OilSufficient_ReturnsTrue )
{
    RigWellTargetMapping::DataContainer data;
    data.saturationOil = { 0.4 };
    data.saturationGas = { 0.05 };

    RigWellTargetMapping::ClusteringLimits limits;
    limits.saturationOil = 0.2;
    limits.saturationGas = 0.3;

    EXPECT_TRUE( RigWellTargetMappingTools::isSaturationSufficient( VolumeType::HYDROCARBON, data, limits, 0 ) );
}

TEST( RigWellTargetMappingToolsTest, IsSaturationSufficient_Hydrocarbon_GasSufficient_ReturnsTrue )
{
    RigWellTargetMapping::DataContainer data;
    data.saturationOil = { 0.05 };
    data.saturationGas = { 0.5 };

    RigWellTargetMapping::ClusteringLimits limits;
    limits.saturationOil = 0.2;
    limits.saturationGas = 0.3;

    EXPECT_TRUE( RigWellTargetMappingTools::isSaturationSufficient( VolumeType::HYDROCARBON, data, limits, 0 ) );
}

TEST( RigWellTargetMappingToolsTest, IsSaturationSufficient_Hydrocarbon_NeitherSufficient_ReturnsFalse )
{
    RigWellTargetMapping::DataContainer data;
    data.saturationOil = { 0.05 };
    data.saturationGas = { 0.05 };

    RigWellTargetMapping::ClusteringLimits limits;
    limits.saturationOil = 0.2;
    limits.saturationGas = 0.3;

    EXPECT_FALSE( RigWellTargetMappingTools::isSaturationSufficient( VolumeType::HYDROCARBON, data, limits, 0 ) );
}

TEST( RigWellTargetMappingToolsTest, IsSaturationSufficient_OilType_GasAbove_ReturnsFalse )
{
    // VolumeType::OIL only checks oil saturation, not gas
    RigWellTargetMapping::DataContainer data;
    data.saturationOil = { 0.05 };
    data.saturationGas = { 0.9 };

    RigWellTargetMapping::ClusteringLimits limits;
    limits.saturationOil = 0.2;
    limits.saturationGas = 0.3;

    EXPECT_FALSE( RigWellTargetMappingTools::isSaturationSufficient( VolumeType::OIL, data, limits, 0 ) );
}

TEST( RigWellTargetMappingToolsTest, IsSaturationSufficient_GasType_OilAbove_ReturnsFalse )
{
    // VolumeType::GAS only checks gas saturation, not oil
    RigWellTargetMapping::DataContainer data;
    data.saturationOil = { 0.9 };
    data.saturationGas = { 0.05 };

    RigWellTargetMapping::ClusteringLimits limits;
    limits.saturationOil = 0.2;
    limits.saturationGas = 0.3;

    EXPECT_FALSE( RigWellTargetMappingTools::isSaturationSufficient( VolumeType::GAS, data, limits, 0 ) );
}

//--------------------------------------------------------------------------------------------------
// assignClusterIdToCells tests
//--------------------------------------------------------------------------------------------------

TEST( RigWellTargetMappingToolsTest, AssignClusterIdToCells_SetsCorrectIds )
{
    RigActiveCellInfo activeCellInfo;
    activeCellInfo.setReservoirCellCount( 5 );
    activeCellInfo.setCellResultIndex( 0, 0 );
    activeCellInfo.setCellResultIndex( 1, 1 );
    activeCellInfo.setCellResultIndex( 2, 2 );

    std::vector<int>    clusters( 3, 0 );
    std::vector<size_t> cells = { 0, 1, 2 };

    RigWellTargetMappingTools::assignClusterIdToCells( activeCellInfo, cells, clusters, 5 );

    EXPECT_EQ( 5, clusters[0] );
    EXPECT_EQ( 5, clusters[1] );
    EXPECT_EQ( 5, clusters[2] );
}

TEST( RigWellTargetMappingToolsTest, AssignClusterIdToCells_SkipsUndefinedResultIndex )
{
    RigActiveCellInfo activeCellInfo;
    activeCellInfo.setReservoirCellCount( 3 );
    activeCellInfo.setCellResultIndex( 0, 0 );
    // reservoir cells 1 and 2 have no result index (UNDEFINED_SIZE_T)

    std::vector<int>    clusters( 1, 0 );
    std::vector<size_t> cells = { 0, 1, 2 };

    RigWellTargetMappingTools::assignClusterIdToCells( activeCellInfo, cells, clusters, 7 );

    EXPECT_EQ( 7, clusters[0] );
}

TEST( RigWellTargetMappingToolsTest, AssignClusterIdToCells_EmptyCells_NoChange )
{
    RigActiveCellInfo activeCellInfo;
    activeCellInfo.setReservoirCellCount( 3 );
    activeCellInfo.setCellResultIndex( 0, 0 );
    activeCellInfo.setCellResultIndex( 1, 1 );

    std::vector<int>    clusters( 2, 99 );
    std::vector<size_t> cells;

    RigWellTargetMappingTools::assignClusterIdToCells( activeCellInfo, cells, clusters, 1 );

    EXPECT_EQ( 99, clusters[0] );
    EXPECT_EQ( 99, clusters[1] );
}

TEST( RigWellTargetMappingToolsTest, AssignClusterIdToCells_OverwritesExistingId )
{
    RigActiveCellInfo activeCellInfo;
    activeCellInfo.setReservoirCellCount( 2 );
    activeCellInfo.setCellResultIndex( 0, 0 );
    activeCellInfo.setCellResultIndex( 1, 1 );

    std::vector<int>    clusters = { 3, 3 };
    std::vector<size_t> cells    = { 0, 1 };

    RigWellTargetMappingTools::assignClusterIdToCells( activeCellInfo, cells, clusters, 9 );

    EXPECT_EQ( 9, clusters[0] );
    EXPECT_EQ( 9, clusters[1] );
}

//--------------------------------------------------------------------------------------------------
// nncConnectionCellAndResult tests
//--------------------------------------------------------------------------------------------------

TEST( RigWellTargetMappingToolsTest, NncConnectionCellAndResult_NoConnections_ReturnsEmpty )
{
    cvf::ref<RigMainGrid> mainGrid = new RigMainGrid;

    auto result = RigWellTargetMappingTools::nncConnectionCellAndResult( 0, mainGrid.p() );

    EXPECT_TRUE( result.empty() );
}

TEST( RigWellTargetMappingToolsTest, NncConnectionCellAndResult_MatchingCell_ReturnsConnection )
{
    cvf::ref<RigMainGrid> mainGrid = new RigMainGrid;
    mainGrid->nncData()->allConnections().push_back(
        RigConnection( (size_t)3, (size_t)7, cvf::StructGridInterface::FaceType::POS_I ) );

    auto result = RigWellTargetMappingTools::nncConnectionCellAndResult( 3, mainGrid.p() );

    ASSERT_EQ( 1u, result.size() );
    auto& [cellInfo, nncIdx] = result.front();
    EXPECT_EQ( 7u, cellInfo.first );
    EXPECT_EQ( cvf::StructGridInterface::FaceType::POS_I, cellInfo.second );
    EXPECT_EQ( 0u, nncIdx );
}

TEST( RigWellTargetMappingToolsTest, NncConnectionCellAndResult_NonMatchingCell_ReturnsEmpty )
{
    cvf::ref<RigMainGrid> mainGrid = new RigMainGrid;
    mainGrid->nncData()->allConnections().push_back(
        RigConnection( (size_t)5, (size_t)8, cvf::StructGridInterface::FaceType::NEG_J ) );

    auto result = RigWellTargetMappingTools::nncConnectionCellAndResult( 0, mainGrid.p() );

    EXPECT_TRUE( result.empty() );
}

TEST( RigWellTargetMappingToolsTest, NncConnectionCellAndResult_MultipleConnections_ReturnsOnlyMatching )
{
    cvf::ref<RigMainGrid> mainGrid = new RigMainGrid;
    mainGrid->nncData()->allConnections().push_back(
        RigConnection( (size_t)2, (size_t)10, cvf::StructGridInterface::FaceType::POS_I ) );
    mainGrid->nncData()->allConnections().push_back(
        RigConnection( (size_t)4, (size_t)11, cvf::StructGridInterface::FaceType::POS_J ) );
    mainGrid->nncData()->allConnections().push_back(
        RigConnection( (size_t)2, (size_t)12, cvf::StructGridInterface::FaceType::NEG_K ) );

    auto result = RigWellTargetMappingTools::nncConnectionCellAndResult( 2, mainGrid.p() );

    ASSERT_EQ( 2u, result.size() );

    auto it = result.begin();
    EXPECT_EQ( 10u, it->first.first );
    EXPECT_EQ( 0u, it->second );
    ++it;
    EXPECT_EQ( 12u, it->first.first );
    EXPECT_EQ( 2u, it->second );
}
