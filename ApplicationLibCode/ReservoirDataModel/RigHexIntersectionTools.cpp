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
///
//--------------------------------------------------------------------------------------------------
int filteredOrient3d( const cvf::Vec3d& a, const cvf::Vec3d& b, const cvf::Vec3d& c, const cvf::Vec3d& d )
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

    if ( det > errBound ) return 1;
    if ( det < -errBound ) return -1;
    return static_cast<int>( cvf::robust::orient3d( a, b, c, d ) );
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
                                                      std::vector<HexIntersectionInfo>* intersections,
                                                      double                            parametricTolerance )
{
    CAF_ASSERT( intersections != nullptr );

    const cvf::Vec3d direction     = p2 - p1;
    const double     lengthSquared = direction.lengthSquared();

    // At most one hit per surface triangle
    std::array<SegmentHit, 24> hits;
    size_t                     hitCount = 0;

    forEachCellSurfaceTriangle( hexCorners,
                                [&]( cvf::StructGridInterface::FaceType face, const cvf::Vec3d& t0, const cvf::Vec3d& t1, const cvf::Vec3d& t2 )
                                {
                                    cvf::Vec3d intersection;
                                    bool       isEntering = false;

                                    int intsStatus = cvf::GeometryTools::intersectLineSegmentTriangle( p1,
                                                                                                       p2,
                                                                                                       t0,
                                                                                                       t1,
                                                                                                       t2,
                                                                                                       &intersection,
                                                                                                       &isEntering,
                                                                                                       parametricTolerance );
                                    if ( intsStatus == 1 )
                                    {
                                        const double t = lengthSquared > 0.0 ? ( intersection - p1 ).dot( direction ) / lengthSquared : 0.0;
                                        hits[hitCount++] = { t, intersection, isEntering, face };
                                    }
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
    const bool allVisited =
        forEachCellSurfaceTriangle( hexCorners,
                                    [&]( cvf::StructGridInterface::FaceType, const cvf::Vec3d& t0, const cvf::Vec3d& t1, const cvf::Vec3d& t2 )
                                    {
                                        cvf::Vec3d intersection;
                                        bool       isEntering = false;
                                        return cvf::GeometryTools::intersectLineSegmentTriangle( p1, p2, t0, t1, t2, &intersection, &isEntering ) !=
                                               1;
                                    } );
    return !allVisited;
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
