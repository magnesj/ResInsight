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
#include "RigHexIntersectionTools.h"

#include "RigCellGeometryTools.h"

#include "cvfBoundingBox.h"
#include "cvfGeometryTools.h"
#include "cvfRobustPredicates.h"

#include "cafAssert.h"
#include "cafHexGridIntersectionTools/cafHexGridIntersectionTools.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool isLexicographicallyLess( const cvf::Vec3d& a, const cvf::Vec3d& b )
{
    if ( a.x() != b.x() ) return a.x() < b.x();
    if ( a.y() != b.y() ) return a.y() < b.y();
    return a.z() < b.z();
}

using SegmentHit = RigHexIntersectionTools::SegmentHit;

//--------------------------------------------------------------------------------------------------
/// Exact ordering on (t, entering, face, point)
//--------------------------------------------------------------------------------------------------
bool isHitLess( const SegmentHit& a, const SegmentHit& b )
{
    if ( a.t != b.t ) return a.t < b.t;
    if ( a.isEntering != b.isEntering ) return a.isEntering < b.isEntering;
    if ( a.face != b.face ) return a.face < b.face;
    return isLexicographicallyLess( a.point, b.point );
}

//--------------------------------------------------------------------------------------------------
/// Floating point filters in front of the exact predicates. The error bounds are several times larger than the
/// bounds of the adaptive predicates, so the result is also certain if the compiler contracts to fused multiply-add.
/// A sign that is not certain is decided by the exact predicate.
//--------------------------------------------------------------------------------------------------
int filteredOrient2d( const cvf::Vec2d& a, const cvf::Vec2d& b, const cvf::Vec2d& c )
{
    const double detLeft  = ( a.x() - c.x() ) * ( b.y() - c.y() );
    const double detRight = ( a.y() - c.y() ) * ( b.x() - c.x() );
    const double det      = detLeft - detRight;
    const double errBound = 8.0 * std::numeric_limits<double>::epsilon() * ( std::abs( detLeft ) + std::abs( detRight ) );

    if ( det > errBound ) return 1;
    if ( det < -errBound ) return -1;
    return static_cast<int>( cvf::robust::orient2d( a, b, c ) );
}

//--------------------------------------------------------------------------------------------------
/// The value has the exact sign. The magnitude is exact only if the filter could not decide, otherwise it is an
/// approximation that is accurate enough to interpolate along a segment.
//--------------------------------------------------------------------------------------------------
double filteredOrient3dValue( const cvf::Vec3d& a, const cvf::Vec3d& b, const cvf::Vec3d& c, const cvf::Vec3d& d )
{
    const double adx = a.x() - d.x();
    const double bdx = b.x() - d.x();
    const double cdx = c.x() - d.x();
    const double ady = a.y() - d.y();
    const double bdy = b.y() - d.y();
    const double cdy = c.y() - d.y();
    const double adz = a.z() - d.z();
    const double bdz = b.z() - d.z();
    const double cdz = c.z() - d.z();

    const double bdxcdy = bdx * cdy;
    const double cdxbdy = cdx * bdy;
    const double cdxady = cdx * ady;
    const double adxcdy = adx * cdy;
    const double adxbdy = adx * bdy;
    const double bdxady = bdx * ady;

    const double det = adz * ( bdxcdy - cdxbdy ) + bdz * ( cdxady - adxcdy ) + cdz * ( adxbdy - bdxady );

    const double permanent = ( std::abs( bdxcdy ) + std::abs( cdxbdy ) ) * std::abs( adz ) +
                             ( std::abs( cdxady ) + std::abs( adxcdy ) ) * std::abs( bdz ) +
                             ( std::abs( adxbdy ) + std::abs( bdxady ) ) * std::abs( cdz );
    const double errBound = 16.0 * std::numeric_limits<double>::epsilon() * permanent;

    if ( det > errBound || det < -errBound ) return det;
    return cvf::robust::orient3dValue( a, b, c, d );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int filteredOrient3d( const cvf::Vec3d& a, const cvf::Vec3d& b, const cvf::Vec3d& c, const cvf::Vec3d& d )
{
    const double value = filteredOrient3dValue( a, b, c, d );
    return value > 0.0 ? 1 : ( value < 0.0 ? -1 : 0 );
}

//--------------------------------------------------------------------------------------------------
/// Conservative floating point test that is true only if the segment, given as a ball with a center and a radius,
/// certainly does not touch the four triangles ( corner i, corner i + 1, faceCenter ) of a face. The triangles are
/// inside the slab of points with a distance to the plane through the face center, with normal along the face
/// diagonals' cross product, that is at most the largest corner distance. Rounding errors are bounded and added,
/// so a face that is not rejected is decided by the exact predicates.
//--------------------------------------------------------------------------------------------------
bool isSegmentOutsideFaceSlab( const cvf::Vec3d&                       segmentCenter,
                               double                                  segmentRadius,
                               const std::array<const cvf::Vec3d*, 4>& corners,
                               const cvf::Vec3d&                       faceCenter )
{
    const cvf::Vec3d normal = ( *corners[2] - *corners[0] ) ^ ( *corners[3] - *corners[1] );
    const cvf::Vec3d absNormal( std::abs( normal.x() ), std::abs( normal.y() ), std::abs( normal.z() ) );
    const double     normalL1 = absNormal.x() + absNormal.y() + absNormal.z();
    if ( normalL1 == 0.0 ) return false;

    // Bound of the rounding error of normal . ( x - faceCenter ), with the computed normal taken as exact
    const double errorFactor = 8.0 * std::numeric_limits<double>::epsilon();
    auto         errorBound  = [&]( const cvf::Vec3d& x )
    {
        return errorFactor * ( absNormal.x() * ( std::abs( x.x() ) + std::abs( faceCenter.x() ) ) +
                               absNormal.y() * ( std::abs( x.y() ) + std::abs( faceCenter.y() ) ) +
                               absNormal.z() * ( std::abs( x.z() ) + std::abs( faceCenter.z() ) ) );
    };

    double slabHalfWidth = 0.0;
    for ( const cvf::Vec3d* corner : corners )
    {
        slabHalfWidth = std::max( slabHalfWidth, std::abs( normal * ( *corner - faceCenter ) ) + errorBound( *corner ) );
    }

    // The L1 norm of the normal bounds the L2 norm, so the radius term bounds the distance of any segment point
    const double centerDistance = std::abs( normal * ( segmentCenter - faceCenter ) ) - errorBound( segmentCenter );
    return centerDistance > ( segmentRadius * normalL1 + slabHalfWidth ) * ( 1.0 + errorFactor );
}

//--------------------------------------------------------------------------------------------------
/// Outcodes of the cell corners relative to the segment p - q. Bits 0-5 are set when a corner is below or above the
/// bounding box of the segment along x, y and z, and are exact. Bits 6-9 are set when a corner is certainly on one
/// side of one of two planes containing the segment, with a margin many times larger than the rounding errors.
/// A set of corners with a common bit is separated from the segment, and so is their convex hull.
//--------------------------------------------------------------------------------------------------
std::array<unsigned, 8> segmentOutcodes( const cvf::Vec3d&                p,
                                         const cvf::Vec3d&                q,
                                         const cvf::Vec3d&                segmentMin,
                                         const cvf::Vec3d&                segmentMax,
                                         const std::array<cvf::Vec3d, 8>& hexCorners )
{
    // Two directions perpendicular to the segment, built from the axis with the smallest direction component
    const cvf::Vec3d direction = q - p;
    const cvf::Vec3d absDirection( std::abs( direction.x() ), std::abs( direction.y() ), std::abs( direction.z() ) );
    cvf::Vec3d       axis = cvf::Vec3d::X_AXIS;
    if ( absDirection.y() < absDirection.x() && absDirection.y() <= absDirection.z() )
        axis = cvf::Vec3d::Y_AXIS;
    else if ( absDirection.z() < absDirection.x() && absDirection.z() < absDirection.y() )
        axis = cvf::Vec3d::Z_AXIS;

    const cvf::Vec3d u = direction ^ axis;
    const cvf::Vec3d v = direction ^ u;

    auto l1      = []( const cvf::Vec3d& w ) { return std::abs( w.x() ) + std::abs( w.y() ) + std::abs( w.z() ); };
    auto maxNorm = []( const cvf::Vec3d& w ) { return std::max( { std::abs( w.x() ), std::abs( w.y() ), std::abs( w.z() ) } ); };

    double magnitude = std::max( maxNorm( p ), maxNorm( q ) );
    for ( const auto& corner : hexCorners )
        magnitude = std::max( magnitude, maxNorm( corner ) );

    // Bound of the rounding errors of ( x - p ) . n for the segment points and the corners, with n taken as exact
    const double errorFactor = 64.0 * std::numeric_limits<double>::epsilon() * magnitude;
    const double marginU     = errorFactor * l1( u );
    const double marginV     = errorFactor * l1( v );

    const double qU = direction * u;
    const double qV = direction * v;

    const double maxU = std::max( 0.0, qU ) + marginU;
    const double minU = std::min( 0.0, qU ) - marginU;
    const double maxV = std::max( 0.0, qV ) + marginV;
    const double minV = std::min( 0.0, qV ) - marginV;

    std::array<unsigned, 8> outcodes;
    for ( size_t i = 0; i < 8; ++i )
    {
        const cvf::Vec3d& corner = hexCorners[i];

        unsigned code = 0;
        for ( int axisIdx = 0; axisIdx < 3; ++axisIdx )
        {
            if ( corner[axisIdx] < segmentMin[axisIdx] ) code |= 1u << ( 2 * axisIdx );
            if ( corner[axisIdx] > segmentMax[axisIdx] ) code |= 2u << ( 2 * axisIdx );
        }

        const cvf::Vec3d w      = corner - p;
        const double     valueU = w * u;
        const double     valueV = w * v;
        if ( valueU < minU ) code |= 1u << 6;
        if ( valueU > maxU ) code |= 1u << 7;
        if ( valueV < minV ) code |= 1u << 8;
        if ( valueV > maxV ) code |= 1u << 9;

        outcodes[i] = code;
    }

    return outcodes;
}

//--------------------------------------------------------------------------------------------------
/// Calls handler( face, a, b, c, valueP, valueQ ) for every triangle of the canonical cell surface that is hit by
/// the closed segment p - q, where valueP and valueQ are the orient3d values of p and q relative to the triangle
/// (a, b, c). The decisions are exact. The three triangle edges are tested with the sign of orient3d( p, q, u, v ),
/// which is shared between triangles with a common edge, so a segment crossing the surface hits at least one
/// triangle and there are no gaps between neighbour triangles. A segment in the plane of a triangle is not a hit.
/// The handler returns false to stop the iteration, and the function returns false if it was stopped.
//--------------------------------------------------------------------------------------------------
template <typename Handler>
bool forEachSegmentTriangleHit( const cvf::Vec3d& p, const cvf::Vec3d& q, const std::array<cvf::Vec3d, 8>& hexCorners, Handler&& handler )
{
    auto sign = []( double value ) { return value > 0.0 ? 1 : ( value < 0.0 ? -1 : 0 ); };

    cvf::Vec3d segmentMin( std::min( p.x(), q.x() ), std::min( p.y(), q.y() ), std::min( p.z(), q.z() ) );
    cvf::Vec3d segmentMax( std::max( p.x(), q.x() ), std::max( p.y(), q.y() ), std::max( p.z(), q.z() ) );

    // Ball around the segment for the face slab test. The radius bounds the half length and the rounding of the center
    const double     epsilon       = std::numeric_limits<double>::epsilon();
    const cvf::Vec3d segmentCenter = 0.5 * ( p + q );
    const double     segmentRadius =
        0.5 * ( ( segmentMax.x() - segmentMin.x() ) + ( segmentMax.y() - segmentMin.y() ) + ( segmentMax.z() - segmentMin.z() ) ) *
            ( 1.0 + 8.0 * epsilon ) +
        2.0 * epsilon * ( std::abs( segmentCenter.x() ) + std::abs( segmentCenter.y() ) + std::abs( segmentCenter.z() ) );

    const std::array<unsigned, 8> outcodes = segmentOutcodes( p, q, segmentMin, segmentMax, hexCorners );
    if ( ( outcodes[0] & outcodes[1] & outcodes[2] & outcodes[3] & outcodes[4] & outcodes[5] & outcodes[6] & outcodes[7] ) != 0 )
    {
        return true;
    }

    // The cell edges are shared by two faces, the sign is computed once. 2 = not computed yet
    std::array<std::array<int, 8>, 8> cellEdgeSigns;
    for ( auto& row : cellEdgeSigns )
        row.fill( 2 );

    auto edgeSign = [&]( int cornerU, int cornerV )
    {
        int& cached = cellEdgeSigns[cornerU][cornerV];
        if ( cached == 2 )
        {
            cached                          = filteredOrient3d( p, q, hexCorners[cornerU], hexCorners[cornerV] );
            cellEdgeSigns[cornerV][cornerU] = -cached;
        }
        return cached;
    };

    for ( int faceIdx = 0; faceIdx < 6; ++faceIdx )
    {
        const auto face = static_cast<cvf::StructGridInterface::FaceType>( faceIdx );
        cvf::ubyte faceVertexIndices[4];
        cvf::StructGridInterface::cellFaceVertexIndices( face, faceVertexIndices );

        // Culling of faces away from the segment. The face center is inside the convex hull of the corners
        if ( ( outcodes[faceVertexIndices[0]] & outcodes[faceVertexIndices[1]] & outcodes[faceVertexIndices[2]] &
               outcodes[faceVertexIndices[3]] ) != 0 )
        {
            continue;
        }

        const std::array<const cvf::Vec3d*, 4> corners = { &hexCorners[faceVertexIndices[0]],
                                                           &hexCorners[faceVertexIndices[1]],
                                                           &hexCorners[faceVertexIndices[2]],
                                                           &hexCorners[faceVertexIndices[3]] };

        const cvf::Vec3d faceCenter = RigHexIntersectionTools::canonicalFaceCenter( *corners[0], *corners[1], *corners[2], *corners[3] );
        if ( isSegmentOutsideFaceSlab( segmentCenter, segmentRadius, corners, faceCenter ) ) continue;

        // Sign of the spoke edge from corner k to the face center. The edge in the opposite direction has the opposite sign
        std::array<int, 4> spokeSigns;
        for ( int k = 0; k < 4; ++k )
            spokeSigns[k] = filteredOrient3d( p, q, *corners[k], faceCenter );

        for ( int i = 0; i < 4; ++i )
        {
            const int next = i < 3 ? i + 1 : 0;

            const int s1 = edgeSign( faceVertexIndices[i], faceVertexIndices[next] );
            const int s2 = spokeSigns[next];
            const int s3 = -spokeSigns[i];

            // The line through p and q passes through the closed triangle if no edge is on the opposite side
            const bool hasPositive = s1 > 0 || s2 > 0 || s3 > 0;
            const bool hasNegative = s1 < 0 || s2 < 0 || s3 < 0;
            if ( hasPositive == hasNegative ) continue;

            const double valueP = filteredOrient3dValue( *corners[i], *corners[next], faceCenter, p );
            const double valueQ = filteredOrient3dValue( *corners[i], *corners[next], faceCenter, q );

            const int signP = sign( valueP );
            const int signQ = sign( valueQ );
            if ( signP * signQ > 0 || ( signP == 0 && signQ == 0 ) ) continue;

            if ( !handler( face, *corners[i], *corners[next], faceCenter, valueP, valueQ ) ) return false;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
/// Exact sign of orient2d( a, b, c ) where c is perturbed symbolically to ( c.x + e, c.y + e^2 ), e infinitesimal.
/// The sign is never zero unless a and b coincide, so a point on an edge belongs to exactly one of the two sides.
//--------------------------------------------------------------------------------------------------
int perturbedOrient2d( const cvf::Vec2d& a, const cvf::Vec2d& b, const cvf::Vec2d& c )
{
    const int sign = filteredOrient2d( a, b, c );
    if ( sign != 0 ) return sign;

    if ( b.y() != a.y() ) return b.y() > a.y() ? -1 : 1;
    if ( b.x() != a.x() ) return b.x() > a.x() ? 1 : -1;
    return 0;
}

//--------------------------------------------------------------------------------------------------
/// Exact orientation of the triangle projected on the plane given by two of the axes
//--------------------------------------------------------------------------------------------------
int projectedOrientation( const cvf::Vec3d& a, const cvf::Vec3d& b, const cvf::Vec3d& c, int axis0, int axis1 )
{
    return static_cast<int>(
        cvf::robust::orient2d( cvf::Vec2d( a[axis0], a[axis1] ), cvf::Vec2d( b[axis0], b[axis1] ), cvf::Vec2d( c[axis0], c[axis1] ) ) );
}

//--------------------------------------------------------------------------------------------------
/// Does the triangle cross the ray from p in the +z direction? The point is perturbed symbolically to
/// ( p.x + e, p.y + e^2, p.z + e^3 ), so every crossing is decided exactly, with no tolerance and no ties.
/// Edges and vertices shared by neighbour triangles are therefore counted once.
//--------------------------------------------------------------------------------------------------
bool isTriangleAbovePoint( const cvf::Vec3d& p, const cvf::Vec3d& a, const cvf::Vec3d& b, const cvf::Vec3d& c )
{
    // The faces that cannot contain the point are culled by the caller
    const cvf::Vec2d p2( p.x(), p.y() );
    const cvf::Vec2d a2( a.x(), a.y() );
    const cvf::Vec2d b2( b.x(), b.y() );
    const cvf::Vec2d c2( c.x(), c.y() );

    // The perturbed point is inside the projected triangle when it is on the same side of all three edges.
    // The common sign is the orientation of the projected triangle.
    const int side = perturbedOrient2d( a2, b2, p2 );
    if ( side == 0 ) return false;
    if ( perturbedOrient2d( b2, c2, p2 ) != side ) return false;
    if ( perturbedOrient2d( c2, a2, p2 ) != side ) return false;

    // orient3d is positive when p is below the plane of a, b, c seen counter-clockwise from above
    const int below = filteredOrient3d( a, b, c, p );
    if ( below != 0 ) return below * side > 0;

    // p is in the plane. Decide on the perturbation, which moves p by n.( e, e^2, e^3 ) relative to the plane
    // with the normal n. The triangle is above the perturbed point when the sign of that term is opposite to nz.
    const int nx = projectedOrientation( a, b, c, 1, 2 );
    if ( nx != 0 ) return nx * side < 0;

    const int ny = projectedOrientation( a, b, c, 2, 0 );
    if ( ny != 0 ) return ny * side < 0;

    // Horizontal triangle in the plane of p, which is below the perturbed point
    return false;
}

} // namespace

//--------------------------------------------------------------------------------------------------
/// Face corners are listed in cyclic order. Any rotation or reversal of that order keeps the two diagonals as the
/// same pairs of corners, and floating point addition is commutative, so summing the diagonals first gives a
/// bit-identical result for every listing of the same face.
//--------------------------------------------------------------------------------------------------
cvf::Vec3d RigHexIntersectionTools::canonicalFaceCenter( const cvf::Vec3d& v0, const cvf::Vec3d& v1, const cvf::Vec3d& v2, const cvf::Vec3d& v3 )
{
    return ( ( v0 + v2 ) + ( v1 + v3 ) ) * 0.25;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RigHexIntersectionTools::lineHexCellIntersection( const cvf::Vec3d&                 p1,
                                                      const cvf::Vec3d&                 p2,
                                                      const std::array<cvf::Vec3d, 8>&  hexCorners,
                                                      const size_t                      hexIndex,
                                                      std::vector<HexIntersectionInfo>* intersections )
{
    CAF_ASSERT( intersections != nullptr );

    const cvf::Vec3d direction     = p2 - p1;
    const double     lengthSquared = direction.lengthSquared();

    // At most one hit per surface triangle
    std::array<SegmentHit, 24> hits;
    size_t                     hitCount = 0;

    forEachSegmentTriangleHit( p1,
                               p2,
                               hexCorners,
                               [&]( cvf::StructGridInterface::FaceType face,
                                    const cvf::Vec3d&,
                                    const cvf::Vec3d&,
                                    const cvf::Vec3d&,
                                    double valueP,
                                    double valueQ )
                               {
                                   // The values have opposite signs or one of them is zero. The segment enters the cell if
                                   // it moves to the side of the triangle plane that has the larger orient3d value
                                   const bool isEntering = valueQ > valueP;

                                   double t = 0.0;
                                   if ( valueP == 0.0 )
                                       t = 0.0;
                                   else if ( valueQ == 0.0 )
                                       t = 1.0;
                                   else
                                       t = std::abs( valueP ) / ( std::abs( valueP ) + std::abs( valueQ ) );

                                   const cvf::Vec3d point = t == 1.0 ? p2 : p1 + t * direction;
                                   hits[hitCount++]       = { t, point, isEntering, face };
                                   return true;
                               } );

    if ( hitCount == 0 ) return 0;

    // Hits on a shared triangle or face edge are reported once per triangle. Merge hits in the same direction
    // that are closer than a tolerance relative to the segment length. The tolerance is never below what the
    // coordinate magnitude allows to resolve.
    double mergeTolerance = intersectionMergeTolerance;
    if ( hitCount > 1 && lengthSquared > 0.0 )
    {
        double coordinateMagnitude = 0.0;
        for ( int i = 0; i < 3; ++i )
        {
            coordinateMagnitude = std::max( { coordinateMagnitude, std::abs( p1[i] ), std::abs( p2[i] ) } );
        }
        const double resolvableTolerance = 64.0 * std::numeric_limits<double>::epsilon() * coordinateMagnitude / std::sqrt( lengthSquared );
        mergeTolerance                   = std::max( mergeTolerance, resolvableTolerance );
    }

    return sortMergeAndAppendHits( std::span<SegmentHit>( hits.data(), hitCount ), mergeTolerance, hexIndex, intersections );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RigHexIntersectionTools::sortMergeAndAppendHits( std::span<SegmentHit>             hits,
                                                     double                            mergeTolerance,
                                                     size_t                            hexIndex,
                                                     std::vector<HexIntersectionInfo>* intersections )
{
    CAF_ASSERT( intersections != nullptr );

    // Insertion sort, as there are only a few hits
    for ( size_t i = 1; i < hits.size(); ++i )
    {
        SegmentHit hit = hits[i];
        size_t     j   = i;
        for ( ; j > 0 && isHitLess( hit, hits[j - 1] ); --j )
        {
            hits[j] = hits[j - 1];
        }
        hits[j] = hit;
    }

    // The last kept hit is the first hit of the current run, so comparing to it avoids chaining
    const SegmentHit* runStart = nullptr;

    int intersectionCount = 0;
    for ( const SegmentHit& hit : hits )
    {
        if ( runStart && runStart->isEntering == hit.isEntering && hit.t - runStart->t <= mergeTolerance ) continue;

        runStart = &hit;
        intersections->emplace_back( hit.point, hit.isEntering, hit.face, hexIndex );
        ++intersectionCount;
    }

    return intersectionCount;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RigHexIntersectionTools::lineIntersectsHexCell( const cvf::Vec3d& p1, const cvf::Vec3d& p2, const std::array<cvf::Vec3d, 8>& hexCorners )
{
    const bool noHit =
        forEachSegmentTriangleHit( p1,
                                   p2,
                                   hexCorners,
                                   []( cvf::StructGridInterface::FaceType, const cvf::Vec3d&, const cvf::Vec3d&, const cvf::Vec3d&, double, double )
                                   { return false; } );
    return !noHit;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RigHexIntersectionTools::isPointInCell( const cvf::Vec3d& point, const std::array<cvf::Vec3d, 8>& hexCorners )
{
    // The perturbation of the point is positive, so a coordinate equal to the maximum is outside
    cvf::Vec3d minCorner = hexCorners[0];
    cvf::Vec3d maxCorner = hexCorners[0];
    for ( size_t i = 1; i < hexCorners.size(); ++i )
    {
        for ( int axis = 0; axis < 3; ++axis )
        {
            minCorner[axis] = std::min( minCorner[axis], hexCorners[i][axis] );
            maxCorner[axis] = std::max( maxCorner[axis], hexCorners[i][axis] );
        }
    }
    for ( int axis = 0; axis < 3; ++axis )
    {
        if ( point[axis] >= maxCorner[axis] || point[axis] < minCorner[axis] ) return false;
    }

    size_t intersections = 0;

    for ( int faceIdx = 0; faceIdx < 6; ++faceIdx )
    {
        cvf::ubyte faceVertexIndices[4];
        cvf::StructGridInterface::cellFaceVertexIndices( static_cast<cvf::StructGridInterface::FaceType>( faceIdx ), faceVertexIndices );

        const cvf::Vec3d& v0 = hexCorners[faceVertexIndices[0]];
        const cvf::Vec3d& v1 = hexCorners[faceVertexIndices[1]];
        const cvf::Vec3d& v2 = hexCorners[faceVertexIndices[2]];
        const cvf::Vec3d& v3 = hexCorners[faceVertexIndices[3]];

        // Exact culling of the faces that cannot be above the point, the face center is inside the corners' bounding box
        bool isCulled = std::max( { v0.z(), v1.z(), v2.z(), v3.z() } ) < point.z();
        for ( int axis = 0; axis < 2 && !isCulled; ++axis )
        {
            isCulled = point[axis] >= std::max( { v0[axis], v1[axis], v2[axis], v3[axis] } ) ||
                       point[axis] < std::min( { v0[axis], v1[axis], v2[axis], v3[axis] } );
        }
        if ( isCulled ) continue;

        const cvf::Vec3d  faceCenter = canonicalFaceCenter( v0, v1, v2, v3 );
        const cvf::Vec3d* corners[4] = { &v0, &v1, &v2, &v3 };
        for ( int i = 0; i < 4; ++i )
        {
            if ( isTriangleAbovePoint( point, *corners[i], *corners[i < 3 ? i + 1 : 0], faceCenter ) ) ++intersections;
        }
    }

    return intersections % 2 == 1;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RigHexIntersectionTools::planeHexCellIntersection( const std::array<cvf::Vec3d, 8>&              hexCorners,
                                                        const cvf::Plane&                             fracturePlane,
                                                        std::list<std::pair<cvf::Vec3d, cvf::Vec3d>>& intersectionLineSegments )
{
    bool                                  isCellIntersected = false;
    caf::HexGridIntersectionTools::ClipVx triangleIntersectionPoint1;
    caf::HexGridIntersectionTools::ClipVx triangleIntersectionPoint2;
    bool                                  isMostVxesOnPositiveSideOfP1 = false;

    forEachCellSurfaceTriangle( hexCorners,
                                [&]( cvf::StructGridInterface::FaceType, const cvf::Vec3d& t0, const cvf::Vec3d& t1, const cvf::Vec3d& t2 )
                                {
                                    bool isIntersectingPlane =
                                        caf::HexGridIntersectionTools::planeTriangleIntersection( fracturePlane,
                                                                                                  t0,
                                                                                                  0,
                                                                                                  t1,
                                                                                                  1,
                                                                                                  t2,
                                                                                                  2,
                                                                                                  &triangleIntersectionPoint1,
                                                                                                  &triangleIntersectionPoint2,
                                                                                                  &isMostVxesOnPositiveSideOfP1 );

                                    if ( isIntersectingPlane )
                                    {
                                        isCellIntersected = true;
                                        intersectionLineSegments.emplace_back( triangleIntersectionPoint1.vx, triangleIntersectionPoint2.vx );
                                    }
                                    return true;
                                } );

    return isCellIntersected;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RigHexIntersectionTools::planeHexIntersectionPolygons( const std::array<cvf::Vec3d, 8>&      hexCorners,
                                                            cvf::Mat4d                            transformMatrixForPlane,
                                                            std::vector<std::vector<cvf::Vec3d>>& polygons )
{
    bool isCellIntersected = false;

    cvf::Plane fracturePlane;
    fracturePlane.setFromPointAndNormal( transformMatrixForPlane.translation(), static_cast<cvf::Vec3d>( transformMatrixForPlane.col( 2 ) ) );

    // Find line-segments where cell and fracture plane intersects
    std::list<std::pair<cvf::Vec3d, cvf::Vec3d>> intersectionLineSegments;

    isCellIntersected = planeHexCellIntersection( hexCorners, fracturePlane, intersectionLineSegments );

    RigCellGeometryTools::createPolygonFromLineSegments( intersectionLineSegments, polygons );

    return isCellIntersected;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool operator<( const HexIntersectionInfo& hi1, const HexIntersectionInfo& hi2 )
{
    if ( hi1.m_hexIndex != hi2.m_hexIndex ) return hi1.m_hexIndex < hi2.m_hexIndex;
    if ( hi1.m_face != hi2.m_face ) return hi1.m_face < hi2.m_face;
    if ( hi1.m_isIntersectionEntering != hi2.m_isIntersectionEntering ) return hi1.m_isIntersectionEntering < hi2.m_isIntersectionEntering;

    return isLexicographicallyLess( hi1.m_intersectionPoint, hi2.m_intersectionPoint );
}
