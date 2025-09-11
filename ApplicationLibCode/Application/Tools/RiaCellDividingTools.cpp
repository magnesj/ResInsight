/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2018-     Equinor ASA
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

#include "RiaCellDividingTools.h"

#include "cvfAssert.h"

#include <cmath>
#include <limits>

//--------------------------------------------------------------------------------------------------
/// Internal functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
/// Splits a line in a number of equal parts
//--------------------------------------------------------------------------------------------------
std::vector<cvf::Vec3d> splitLine( cvf::Vec3d ptStart, cvf::Vec3d ptEnd, size_t partCount );

//--------------------------------------------------------------------------------------------------
/// Calculates all points on a face described by edge points from all four edges.
/// The result is a grid of points including the given edge points
///
///                    edgeXPtsHigh
///                  |-------------|
///                  |             |
///   edgeYPtsLow    |             |   edgeYPtsHigh
///                  |             |
///                  |-------------|
///                    edgeXPtsLow
///
//--------------------------------------------------------------------------------------------------
std::vector<std::vector<cvf::Vec3d>> calcFacePoints( const std::vector<cvf::Vec3d> edgeXPtsLow,
                                                     const std::vector<cvf::Vec3d> edgeXPtsHigh,
                                                     const std::vector<cvf::Vec3d> edgeYPtsLow,
                                                     const std::vector<cvf::Vec3d> edgeYPtsHigh );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<cvf::Vec3d> RiaCellDividingTools::createHexCornerCoords( std::array<cvf::Vec3d, 8> mainCellCorners, size_t nx, size_t ny, size_t nz )
{
    std::array<std::pair<size_t, size_t>, 12> edgeCorners = {
        std::make_pair( 0, 1 ),
        std::make_pair( 3, 2 ),
        std::make_pair( 4, 5 ),
        std::make_pair( 7, 6 ), // X
        std::make_pair( 0, 3 ),
        std::make_pair( 4, 7 ),
        std::make_pair( 1, 2 ),
        std::make_pair( 5, 6 ), // Y
        std::make_pair( 0, 4 ),
        std::make_pair( 1, 5 ),
        std::make_pair( 3, 7 ),
        std::make_pair( 2, 6 ), // Z
    };

    std::array<size_t, 3>                   nxyz = { nx, ny, nz };
    std::array<std::vector<cvf::Vec3d>, 12> edgePoints;

    for ( int i = 0; i < 12; i++ )
    {
        int partCountsIndex = i / 4;
        edgePoints[i] = splitLine( mainCellCorners[edgeCorners[i].first], mainCellCorners[edgeCorners[i].second], nxyz[partCountsIndex] );
    }

    // lowIJ, highIJ, lowJK, highKJ,
    std::vector<std::vector<std::vector<cvf::Vec3d>>> nodes;
    nodes.reserve( ( nx + 1 ) * ( ny + 1 ) * ( nz + 1 ) );

    auto xyFacePtsLow  = calcFacePoints( edgePoints[0], edgePoints[1], edgePoints[4], edgePoints[6] );
    auto xyFacePtsHigh = calcFacePoints( edgePoints[2], edgePoints[3], edgePoints[5], edgePoints[7] );
    auto yzFacePtsLow  = calcFacePoints( edgePoints[4], edgePoints[5], edgePoints[8], edgePoints[10] );
    auto yzFacePtsHigh = calcFacePoints( edgePoints[6], edgePoints[7], edgePoints[9], edgePoints[11] );
    auto xzFacePtsLow  = calcFacePoints( edgePoints[0], edgePoints[2], edgePoints[8], edgePoints[9] );
    auto xzFacePtsHigh = calcFacePoints( edgePoints[1], edgePoints[3], edgePoints[10], edgePoints[11] );

    nodes.push_back( xyFacePtsLow );

    for ( size_t z = 1; z < nz; z++ )
    {
        auto xyFacePoints = calcFacePoints( xzFacePtsLow[z], xzFacePtsHigh[z], yzFacePtsLow[z], yzFacePtsHigh[z] );
        nodes.push_back( xyFacePoints );
    }

    nodes.push_back( xyFacePtsHigh );

    std::vector<cvf::Vec3d> coords;
    coords.reserve( nx * ny * nz * 8 );

    for ( size_t z = 1; z < nz + 1; z++ )
    {
        for ( size_t y = 1; y < ny + 1; y++ )
        {
            for ( size_t x = 1; x < nx + 1; x++ )
            {
                std::array<cvf::Vec3d, 8> cs;

                cs[0] = nodes[z - 1][y - 1][x - 1];
                cs[1] = nodes[z - 1][y - 1][x];
                cs[2] = nodes[z - 1][y][x];
                cs[3] = nodes[z - 1][y][x - 1];

                cs[4] = nodes[z][y - 1][x - 1];
                cs[5] = nodes[z][y - 1][x];
                cs[6] = nodes[z][y][x];
                cs[7] = nodes[z][y][x - 1];

                coords.insert( coords.end(), cs.begin(), cs.end() );
            }
        }
    }
    return coords;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<cvf::Vec3d> RiaCellDividingTools::subdivideCylindricalCell( std::array<cvf::Vec3d, 8> cellCornersCylindricalCoords,
                                                                        size_t                    nRadial,
                                                                        size_t                    nTheta,
                                                                        size_t                    nz )
{
    if ( nRadial == 0 || nTheta == 0 || nz == 0 ) return {};

    // Extract radial and angular bounds from the 8 corners
    double rMin     = std::numeric_limits<double>::max();
    double rMax     = std::numeric_limits<double>::lowest();
    double thetaMin = std::numeric_limits<double>::max();
    double thetaMax = std::numeric_limits<double>::lowest();

    // Find the bounds in cylindrical coordinates
    for ( const auto& corner : cellCornersCylindricalCoords )
    {
        double r     = corner.x();
        double theta = corner.y();

        rMin     = std::min( rMin, r );
        rMax     = std::max( rMax, r );
        thetaMin = std::min( thetaMin, theta );
        thetaMax = std::max( thetaMax, theta );
    }

    // Helper function to interpolate z values from the original 8 corners
    auto interpolateZ = [&]( double r, double theta, double zFraction ) -> double
    {
        // Bilinear interpolation in r-theta space, then linear interpolation in z
        double rFraction     = ( r - rMin ) / ( rMax - rMin );
        double thetaFraction = ( theta - thetaMin ) / ( thetaMax - thetaMin );

        // Clamp fractions to [0,1]
        rFraction     = std::max( 0.0, std::min( 1.0, rFraction ) );
        thetaFraction = std::max( 0.0, std::min( 1.0, thetaFraction ) );

        // Get z values from bottom face (corners 0-3) and top face (corners 4-7)
        // Assuming hex corner ordering: 0,1,2,3 are bottom, 4,5,6,7 are top
        // and within each face they form a quad in r-theta space

        // Bottom face interpolation
        double z_bottom_00 = cellCornersCylindricalCoords[0].z(); // rMin, thetaMin
        double z_bottom_10 = cellCornersCylindricalCoords[1].z(); // rMax, thetaMin
        double z_bottom_11 = cellCornersCylindricalCoords[2].z(); // rMax, thetaMax
        double z_bottom_01 = cellCornersCylindricalCoords[3].z(); // rMin, thetaMax

        double z_bottom = ( 1.0 - rFraction ) * ( 1.0 - thetaFraction ) * z_bottom_00 + rFraction * ( 1.0 - thetaFraction ) * z_bottom_10 +
                          rFraction * thetaFraction * z_bottom_11 + ( 1.0 - rFraction ) * thetaFraction * z_bottom_01;

        // Top face interpolation
        double z_top_00 = cellCornersCylindricalCoords[4].z(); // rMin, thetaMin
        double z_top_10 = cellCornersCylindricalCoords[5].z(); // rMax, thetaMin
        double z_top_11 = cellCornersCylindricalCoords[6].z(); // rMax, thetaMax
        double z_top_01 = cellCornersCylindricalCoords[7].z(); // rMin, thetaMax

        double z_top = ( 1.0 - rFraction ) * ( 1.0 - thetaFraction ) * z_top_00 + rFraction * ( 1.0 - thetaFraction ) * z_top_10 +
                       rFraction * thetaFraction * z_top_11 + ( 1.0 - rFraction ) * thetaFraction * z_top_01;

        // Linear interpolation between bottom and top
        return ( 1.0 - zFraction ) * z_bottom + zFraction * z_top;
    };

    std::vector<cvf::Vec3d> subCellCorners;
    subCellCorners.reserve( nRadial * nTheta * nz * 8 );

    // Generate subdivided cells in cylindrical coordinates
    for ( size_t iz = 0; iz < nz; iz++ )
    {
        for ( size_t it = 0; it < nTheta; it++ )
        {
            for ( size_t ir = 0; ir < nRadial; ir++ )
            {
                // Calculate the cylindrical bounds for this sub-cell
                double r0     = rMin + ( rMax - rMin ) * ir / nRadial;
                double r1     = rMin + ( rMax - rMin ) * ( ir + 1 ) / nRadial;
                double theta0 = thetaMin + ( thetaMax - thetaMin ) * it / nTheta;
                double theta1 = thetaMin + ( thetaMax - thetaMin ) * ( it + 1 ) / nTheta;

                double zFraction0 = static_cast<double>( iz ) / nz;
                double zFraction1 = static_cast<double>( iz + 1 ) / nz;

                // Interpolate z values for each corner of the subcell
                double z00_bottom = interpolateZ( r0, theta0, zFraction0 );
                double z10_bottom = interpolateZ( r1, theta0, zFraction0 );
                double z11_bottom = interpolateZ( r1, theta1, zFraction0 );
                double z01_bottom = interpolateZ( r0, theta1, zFraction0 );

                double z00_top = interpolateZ( r0, theta0, zFraction1 );
                double z10_top = interpolateZ( r1, theta0, zFraction1 );
                double z11_top = interpolateZ( r1, theta1, zFraction1 );
                double z01_top = interpolateZ( r0, theta1, zFraction1 );

                // Generate 8 corners for this sub-cell in cylindrical coordinates
                // Bottom face (4 corners)
                subCellCorners.emplace_back( r0, theta0, z00_bottom ); // corner 0
                subCellCorners.emplace_back( r1, theta0, z10_bottom ); // corner 1
                subCellCorners.emplace_back( r1, theta1, z11_bottom ); // corner 2
                subCellCorners.emplace_back( r0, theta1, z01_bottom ); // corner 3

                // Top face (4 corners)
                subCellCorners.emplace_back( r0, theta0, z00_top ); // corner 4
                subCellCorners.emplace_back( r1, theta0, z10_top ); // corner 5
                subCellCorners.emplace_back( r1, theta1, z11_top ); // corner 6
                subCellCorners.emplace_back( r0, theta1, z01_top ); // corner 7
            }
        }
    }

    return subCellCorners;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RiaCellDividingTools::computeFlowDistance( const std::array<cvf::Vec3d, 8>& cellVertices, const cvf::Vec3d& areaCenter )
{
    auto subCellCorners = createHexCornerCoords( cellVertices, 2, 2, 2 );

    double weightedDistanceTotal = 0.0;
    double weightTotal           = 0.0;

    for ( size_t c = 0; c < 8; c++ )
    {
        double weight = 1.0;
        weightTotal += weight;

        cvf::Vec3d centerOfSubCell = cvf::Vec3d::ZERO;
        {
            cvf::Vec3d vertexSum = cvf::Vec3d::ZERO;
            for ( size_t v = 0; v < 8; v++ )
                vertexSum += subCellCorners[c * 8 + v];
            centerOfSubCell = vertexSum / 8;
        }

        auto dist = ( centerOfSubCell - areaCenter ).length();
        weightedDistanceTotal += ( dist * weight );
    }

    return weightedDistanceTotal / weightTotal;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<cvf::Vec3d> splitLine( cvf::Vec3d ptStart, cvf::Vec3d ptEnd, size_t partCount )
{
    std::vector<cvf::Vec3d> pts = { ptStart };

    for ( size_t i = 1; i < partCount; i++ )
    {
        pts.push_back( cvf::Vec3d( ptStart.x() + ( ptEnd.x() - ptStart.x() ) * i / partCount,
                                   ptStart.y() + ( ptEnd.y() - ptStart.y() ) * i / partCount,
                                   ptStart.z() + ( ptEnd.z() - ptStart.z() ) * i / partCount ) );
    }
    pts.push_back( ptEnd );
    return pts;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<std::vector<cvf::Vec3d>> calcFacePoints( const std::vector<cvf::Vec3d> edgeXPtsLow,
                                                     const std::vector<cvf::Vec3d> edgeXPtsHigh,
                                                     const std::vector<cvf::Vec3d> edgeYPtsLow,
                                                     const std::vector<cvf::Vec3d> edgeYPtsHigh )
{
    CVF_ASSERT( edgeXPtsLow.size() == edgeXPtsHigh.size() && edgeYPtsLow.size() == edgeYPtsHigh.size() );

    size_t xSize = edgeXPtsLow.size();
    size_t ySize = edgeYPtsLow.size();

    std::vector<std::vector<cvf::Vec3d>> pts;

    // Add low edge points
    pts.push_back( edgeXPtsLow );

    // Interior points
    for ( size_t y = 1; y < ySize - 1; y++ )
    {
        auto interiorPts = splitLine( edgeYPtsLow[y], edgeYPtsHigh[y], xSize - 1 );
        pts.push_back( interiorPts );
    }

    // Add low edge points
    pts.push_back( edgeXPtsHigh );
    return pts;
}
