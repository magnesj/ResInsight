#pragma once

#include "cvfVector2.h"
#include "cvfVector3.h"

namespace cvf::robust
{

//--------------------------------------------------------------------------------------------------
/// Exact geometric predicates based on Shewchuk's adaptive precision arithmetic
/// (see ThirdParty/ShewchukPredicates).
///
/// The sign of the result is always exact for finite input. No tolerances are
/// used and the input coordinates are used as they are (no translation or
/// scaling), as any rounding would change the exact answer. Results are
/// independent of platform and compiler.
///
/// The functions are thread safe. The one-time initialization of the underlying
/// library is handled internally.
//--------------------------------------------------------------------------------------------------
enum class Orientation : int
{
    Negative = -1,
    Zero     = 0,
    Positive = 1
};

//--------------------------------------------------------------------------------------------------
/// Positive if a, b, c are in counterclockwise order, negative if they are in
/// clockwise order, and zero if they are collinear.
//--------------------------------------------------------------------------------------------------
[[nodiscard]] Orientation orient2d( const Vec2d& a, const Vec2d& b, const Vec2d& c );

//--------------------------------------------------------------------------------------------------
/// Positive if d lies below the plane through a, b, c, where "below" means that
/// a, b, c appear in counterclockwise order when viewed from above the plane.
/// Equivalently, the sign of dot( cross( a - d, b - d ), c - d ), which is the
/// signed volume of the tetrahedron ( a, b, c, d ) times 6. Negative if d lies
/// above the plane, and zero if the four points are coplanar.
///
/// Example: a = ( 0, 0, 0 ), b = ( 1, 0, 0 ), c = ( 0, 1, 0 ) is
/// counterclockwise seen from +z, so d = ( 0, 0, -1 ) gives Positive and d = (
/// 0, 0, 1 ) gives Negative.
//--------------------------------------------------------------------------------------------------
[[nodiscard]] Orientation orient3d( const Vec3d& a, const Vec3d& b, const Vec3d& c, const Vec3d& d );

//--------------------------------------------------------------------------------------------------
/// Raw adaptive results of the predicates above. The sign is exact, but the
/// magnitude is only an estimate of the determinant. Use when the magnitude is
/// needed for ranking or diagnostics, never compare the value against a
/// tolerance to decide on orientation.
//--------------------------------------------------------------------------------------------------
[[nodiscard]] double orient2dValue( const Vec2d& a, const Vec2d& b, const Vec2d& c );
[[nodiscard]] double orient3dValue( const Vec3d& a, const Vec3d& b, const Vec3d& c, const Vec3d& d );

} // namespace cvf::robust
