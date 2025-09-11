/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2017     Statoil ASA
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

#include "RiaCellDividingTools.h"
#include "RigMainGrid.h"

//--------------------------------------------------------------------------------------------------
/// Helper
//--------------------------------------------------------------------------------------------------
std::array<cvf::Vec3d, 8> createRegularCellCoords( cvf::Vec3d refPt, double xLen, double yLen, double zLen )
{
    std::array<cvf::Vec3d, 8> coords = {
        refPt + cvf::Vec3d( 0, 0, 0 ),
        refPt + cvf::Vec3d( xLen, 0, 0 ),
        refPt + cvf::Vec3d( xLen, yLen, 0 ),
        refPt + cvf::Vec3d( 0, yLen, 0 ),
        refPt + cvf::Vec3d( 0, 0, zLen ),
        refPt + cvf::Vec3d( xLen, 0, zLen ),
        refPt + cvf::Vec3d( xLen, yLen, zLen ),
        refPt + cvf::Vec3d( 0, yLen, zLen ),
    };
    return coords;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RiaCellDividingTools, flowDistanceCubicMainCell_AreaPointInCenter )
{
    std::array<cvf::Vec3d, 8> mainCellCorners = createRegularCellCoords( cvf::Vec3d( 10, 10, 10 ), 10, 10, 10 );
    cvf::Vec3d                point( 15, 15, 15 );

    double dist = RiaCellDividingTools::computeFlowDistance( mainCellCorners, point );

    double expectedDist = ( ( cvf::Vec3d( 12.5, 12.5, 12.5 ) - point ).length() + ( cvf::Vec3d( 17.5, 12.5, 12.5 ) - point ).length() +
                            ( cvf::Vec3d( 12.5, 17.5, 12.5 ) - point ).length() + ( cvf::Vec3d( 17.5, 17.5, 12.5 ) - point ).length() +
                            ( cvf::Vec3d( 12.5, 12.5, 17.5 ) - point ).length() + ( cvf::Vec3d( 17.5, 12.5, 17.5 ) - point ).length() +
                            ( cvf::Vec3d( 12.5, 17.5, 17.5 ) - point ).length() + ( cvf::Vec3d( 17.5, 17.5, 17.5 ) - point ).length() ) /
                          8;

    EXPECT_NEAR( expectedDist, dist, 1e-6 );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RiaCellDividingTools, flowDistanceCubicMainCell_AreaPointNearCorner )
{
    std::array<cvf::Vec3d, 8> mainCellCorners = createRegularCellCoords( cvf::Vec3d( 10, 10, 10 ), 10, 10, 10 );
    cvf::Vec3d                point( 11, 11, 11 );

    double dist = RiaCellDividingTools::computeFlowDistance( mainCellCorners, point );

    double expectedDist = ( ( cvf::Vec3d( 12.5, 12.5, 12.5 ) - point ).length() + ( cvf::Vec3d( 17.5, 12.5, 12.5 ) - point ).length() +
                            ( cvf::Vec3d( 12.5, 17.5, 12.5 ) - point ).length() + ( cvf::Vec3d( 17.5, 17.5, 12.5 ) - point ).length() +
                            ( cvf::Vec3d( 12.5, 12.5, 17.5 ) - point ).length() + ( cvf::Vec3d( 17.5, 12.5, 17.5 ) - point ).length() +
                            ( cvf::Vec3d( 12.5, 17.5, 17.5 ) - point ).length() + ( cvf::Vec3d( 17.5, 17.5, 17.5 ) - point ).length() ) /
                          8;

    EXPECT_NEAR( expectedDist, dist, 1e-6 );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RiaCellDividingTools, flowDistanceHighMainCell_AreaPointNearLowerCorner )
{
    std::array<cvf::Vec3d, 8> mainCellCorners = createRegularCellCoords( cvf::Vec3d( 10, 10, 10 ), 10, 10, 100 );
    cvf::Vec3d                point( 11, 11, 11 );

    double dist = RiaCellDividingTools::computeFlowDistance( mainCellCorners, point );

    double expectedDist = ( ( cvf::Vec3d( 12.5, 12.5, 35 ) - point ).length() + ( cvf::Vec3d( 17.5, 12.5, 35 ) - point ).length() +
                            ( cvf::Vec3d( 12.5, 17.5, 35 ) - point ).length() + ( cvf::Vec3d( 17.5, 17.5, 35 ) - point ).length() +
                            ( cvf::Vec3d( 12.5, 12.5, 85 ) - point ).length() + ( cvf::Vec3d( 17.5, 12.5, 85 ) - point ).length() +
                            ( cvf::Vec3d( 12.5, 17.5, 85 ) - point ).length() + ( cvf::Vec3d( 17.5, 17.5, 85 ) - point ).length() ) /
                          8;

    EXPECT_NEAR( expectedDist, dist, 1e-6 );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RigCellGeometryTools, lgrNodesTest )
{
    std::array<cvf::Vec3d, 8> mainCellCorners = createRegularCellCoords( cvf::Vec3d( 10, 10, 10 ), 36, 18, 18 );
    auto                      coords          = RiaCellDividingTools::createHexCornerCoords( mainCellCorners, 6, 3, 3 );
}

//--------------------------------------------------------------------------------------------------
/// Helper function to create cylindrical cell coordinates (r, theta, z)
//--------------------------------------------------------------------------------------------------
std::array<cvf::Vec3d, 8> createCylindricalCellCoords( double rMin, double rMax, double thetaMin, double thetaMax, double zMin, double zMax )
{
    std::array<cvf::Vec3d, 8> coords = {
        cvf::Vec3d( rMin, thetaMin, zMin ),     // corner 0: bottom face
        cvf::Vec3d( rMax, thetaMin, zMin ),     // corner 1
        cvf::Vec3d( rMax, thetaMax, zMin ),     // corner 2
        cvf::Vec3d( rMin, thetaMax, zMin ),     // corner 3
        cvf::Vec3d( rMin, thetaMin, zMax ),     // corner 4: top face
        cvf::Vec3d( rMax, thetaMin, zMax ),     // corner 5
        cvf::Vec3d( rMax, thetaMax, zMax ),     // corner 6
        cvf::Vec3d( rMin, thetaMax, zMax ),     // corner 7
    };
    return coords;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RiaCellDividingTools, subdivideCylindricalCell_BasicSubdivision )
{
    // Create a simple cylindrical cell: r=[1,2], theta=[0,π/2], z=[0,10]
    double pi = 3.14159265359;
    std::array<cvf::Vec3d, 8> cylindricalCorners = createCylindricalCellCoords( 1.0, 2.0, 0.0, pi/2, 0.0, 10.0 );
    
    // Subdivide into 2x2x2 subcells
    auto subCellCorners = RiaCellDividingTools::subdivideCylindricalCell( cylindricalCorners, 2, 2, 2 );
    
    // Should have 2*2*2 = 8 subcells, each with 8 corners = 64 total corners
    EXPECT_EQ( subCellCorners.size(), 64 );
    
    // Check first subcell (ir=0, it=0, iz=0)
    // Expected bounds: r=[1,1.5], theta=[0,π/4], z=[0,5]
    EXPECT_NEAR( subCellCorners[0].x(), 1.0, 1e-10 );       // r0
    EXPECT_NEAR( subCellCorners[0].y(), 0.0, 1e-10 );       // theta0
    EXPECT_NEAR( subCellCorners[0].z(), 0.0, 1e-10 );       // z0
    
    EXPECT_NEAR( subCellCorners[1].x(), 1.5, 1e-10 );       // r1
    EXPECT_NEAR( subCellCorners[1].y(), 0.0, 1e-10 );       // theta0
    EXPECT_NEAR( subCellCorners[1].z(), 0.0, 1e-10 );       // z0
    
    EXPECT_NEAR( subCellCorners[2].x(), 1.5, 1e-10 );       // r1
    EXPECT_NEAR( subCellCorners[2].y(), pi/4, 1e-10 );      // theta1
    EXPECT_NEAR( subCellCorners[2].z(), 0.0, 1e-10 );       // z0
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RiaCellDividingTools, subdivideCylindricalCell_ZInterpolation )
{
    // Create a cylindrical cell with non-uniform z coordinates to test interpolation
    std::array<cvf::Vec3d, 8> cylindricalCorners = {
        cvf::Vec3d( 1.0, 0.0, 0.0 ),     // corner 0: bottom face
        cvf::Vec3d( 2.0, 0.0, 1.0 ),     // corner 1: different z
        cvf::Vec3d( 2.0, 1.0, 2.0 ),     // corner 2: different z
        cvf::Vec3d( 1.0, 1.0, 1.5 ),     // corner 3: different z
        cvf::Vec3d( 1.0, 0.0, 10.0 ),    // corner 4: top face
        cvf::Vec3d( 2.0, 0.0, 11.0 ),    // corner 5: different z
        cvf::Vec3d( 2.0, 1.0, 12.0 ),    // corner 6: different z
        cvf::Vec3d( 1.0, 1.0, 11.5 ),    // corner 7: different z
    };
    
    // Subdivide into 1x1x2 subcells to test z interpolation
    auto subCellCorners = RiaCellDividingTools::subdivideCylindricalCell( cylindricalCorners, 1, 1, 2 );
    
    // Should have 1*1*2 = 2 subcells, each with 8 corners = 16 total corners
    EXPECT_EQ( subCellCorners.size(), 16 );
    
    // Check that z values are properly interpolated
    // First subcell should have z values between bottom and middle
    // Second subcell should have z values between middle and top
    
    // For the first subcell (iz=0), corner 0 should be at rMin, thetaMin with bottom z
    EXPECT_NEAR( subCellCorners[0].x(), 1.0, 1e-10 );       // rMin
    EXPECT_NEAR( subCellCorners[0].y(), 0.0, 1e-10 );       // thetaMin
    EXPECT_NEAR( subCellCorners[0].z(), 0.0, 1e-10 );       // bottom z at (rMin, thetaMin)
    
    // For the first subcell, corner 4 should be at rMin, thetaMin with middle z
    double expectedMiddleZ = (0.0 + 10.0) / 2.0;  // Linear interpolation between bottom and top at (rMin, thetaMin)
    EXPECT_NEAR( subCellCorners[4].x(), 1.0, 1e-10 );       // rMin
    EXPECT_NEAR( subCellCorners[4].y(), 0.0, 1e-10 );       // thetaMin
    EXPECT_NEAR( subCellCorners[4].z(), expectedMiddleZ, 1e-10 );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RiaCellDividingTools, subdivideCylindricalCell_EmptyInput )
{
    std::array<cvf::Vec3d, 8> cylindricalCorners = createCylindricalCellCoords( 1.0, 2.0, 0.0, 1.0, 0.0, 10.0 );
    
    // Test with zero subdivisions
    auto result1 = RiaCellDividingTools::subdivideCylindricalCell( cylindricalCorners, 0, 2, 2 );
    EXPECT_TRUE( result1.empty() );
    
    auto result2 = RiaCellDividingTools::subdivideCylindricalCell( cylindricalCorners, 2, 0, 2 );
    EXPECT_TRUE( result2.empty() );
    
    auto result3 = RiaCellDividingTools::subdivideCylindricalCell( cylindricalCorners, 2, 2, 0 );
    EXPECT_TRUE( result3.empty() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RiaCellDividingTools, subdivideCylindricalCell_SingleSubdivision )
{
    // Create a simple cylindrical cell
    std::array<cvf::Vec3d, 8> cylindricalCorners = createCylindricalCellCoords( 1.0, 3.0, 0.0, 2.0, 5.0, 15.0 );
    
    // Subdivide into 1x1x1 (should return the original cell bounds)
    auto subCellCorners = RiaCellDividingTools::subdivideCylindricalCell( cylindricalCorners, 1, 1, 1 );
    
    // Should have 1*1*1 = 1 subcell with 8 corners
    EXPECT_EQ( subCellCorners.size(), 8 );
    
    // Check that the subdivision preserves the original bounds
    EXPECT_NEAR( subCellCorners[0].x(), 1.0, 1e-10 );   // rMin
    EXPECT_NEAR( subCellCorners[0].y(), 0.0, 1e-10 );   // thetaMin
    EXPECT_NEAR( subCellCorners[0].z(), 5.0, 1e-10 );   // zMin (interpolated)
    
    EXPECT_NEAR( subCellCorners[6].x(), 3.0, 1e-10 );   // rMax
    EXPECT_NEAR( subCellCorners[6].y(), 2.0, 1e-10 );   // thetaMax
    EXPECT_NEAR( subCellCorners[6].z(), 15.0, 1e-10 );  // zMax (interpolated)
}
