#include "gtest/gtest.h"

#include "cvfBase.h"
#include "cvfRobustPredicates.h"
#include "cvfVector2.h"
#include "cvfVector3.h"

#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <random>
#include <thread>
#include <vector>

#ifdef CVF_HAS_FMA_PROBE
// Same source as the vendored predicates.c, compiled with FMA contraction
// enabled and with renamed symbols (see CMakeLists.txt)
extern "C"
{
    void   exactinit_contracted();
    double orient2d_contracted( double* pa, double* pb, double* pc );
    double orient3d_contracted( double* pa, double* pb, double* pc, double* pd );
}
#endif

using namespace cvf;
using cvf::robust::Orientation;

namespace robustPredicatesTest
{
int toInt( Orientation o )
{
    return static_cast<int>( o );
}

Orientation negate( Orientation o )
{
    return static_cast<Orientation>( -toInt( o ) );
}

Orientation signOf( int64_t v )
{
    if ( v > 0 ) return Orientation::Positive;
    if ( v < 0 ) return Orientation::Negative;
    return Orientation::Zero;
}

double nextUp( double v, int steps )
{
    const double inf = std::numeric_limits<double>::infinity();
    for ( int i = 0; i < std::abs( steps ); i++ )
    {
        v = std::nextafter( v, steps > 0 ? inf : -inf );
    }
    return v;
}

// Exact reference for small integer coordinates
Orientation referenceOrient2d( const Vec2d& a, const Vec2d& b, const Vec2d& c )
{
    auto i = []( double v ) { return static_cast<int64_t>( v ); };

    const int64_t det = ( i( a.x() ) - i( c.x() ) ) * ( i( b.y() ) - i( c.y() ) ) -
                        ( i( a.y() ) - i( c.y() ) ) * ( i( b.x() ) - i( c.x() ) );
    return signOf( det );
}

Orientation referenceOrient3d( const Vec3d& a, const Vec3d& b, const Vec3d& c, const Vec3d& d )
{
    auto i = []( double v ) { return static_cast<int64_t>( v ); };

    const int64_t adx = i( a.x() ) - i( d.x() ), ady = i( a.y() ) - i( d.y() ), adz = i( a.z() ) - i( d.z() );
    const int64_t bdx = i( b.x() ) - i( d.x() ), bdy = i( b.y() ) - i( d.y() ), bdz = i( b.z() ) - i( d.z() );
    const int64_t cdx = i( c.x() ) - i( d.x() ), cdy = i( c.y() ) - i( d.y() ), cdz = i( c.z() ) - i( d.z() );

    const int64_t det = adx * ( bdy * cdz - bdz * cdy ) + bdx * ( cdy * adz - cdz * ady ) + cdx * ( ady * bdz - adz * bdy );
    return signOf( det );
}

// Points exactly on the plane z = z0 + p * ( x - x0 ) + q * ( y - y0 ). All
// values are dyadic rationals of moderate size, so the evaluation is exact in
// double precision.
struct UtmPlane
{
    static constexpr double x0 = 450000.0;
    static constexpr double y0 = 6700000.0;
    static constexpr double z0 = -2000.0;

    double p = 0.5;
    double q = -0.25;

    Vec3d pointOnPlane( double dx, double dy ) const { return Vec3d( x0 + dx, y0 + dy, z0 + p * dx + q * dy ); }
};
} // namespace robustPredicatesTest

using namespace robustPredicatesTest;

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RobustPredicates, Orient2dBasicSigns )
{
    const Vec2d a( 0, 0 );
    const Vec2d b( 1, 0 );

    EXPECT_EQ( Orientation::Positive, robust::orient2d( a, b, Vec2d( 0, 1 ) ) );
    EXPECT_EQ( Orientation::Negative, robust::orient2d( a, b, Vec2d( 0, -1 ) ) );
    EXPECT_EQ( Orientation::Zero, robust::orient2d( a, b, Vec2d( 2, 0 ) ) );
    EXPECT_EQ( Orientation::Zero, robust::orient2d( a, b, Vec2d( -3, 0 ) ) );

    // Degenerate input
    EXPECT_EQ( Orientation::Zero, robust::orient2d( a, a, b ) );
    EXPECT_EQ( Orientation::Zero, robust::orient2d( a, a, a ) );

    EXPECT_GT( robust::orient2dValue( a, b, Vec2d( 0, 1 ) ), 0.0 );
    EXPECT_LT( robust::orient2dValue( a, b, Vec2d( 0, -1 ) ), 0.0 );
    EXPECT_EQ( 0.0, robust::orient2dValue( a, b, Vec2d( 2, 0 ) ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RobustPredicates, Orient3dSignConvention )
{
    // a, b, c are counterclockwise seen from +z
    const Vec3d a( 0, 0, 0 );
    const Vec3d b( 1, 0, 0 );
    const Vec3d c( 0, 1, 0 );

    // Positive if d is below the plane
    EXPECT_EQ( Orientation::Positive, robust::orient3d( a, b, c, Vec3d( 0, 0, -1 ) ) );
    EXPECT_EQ( Orientation::Negative, robust::orient3d( a, b, c, Vec3d( 0, 0, 1 ) ) );
    EXPECT_EQ( Orientation::Zero, robust::orient3d( a, b, c, Vec3d( 5, -7, 0 ) ) );

    // Clockwise seen from +z flips the meaning
    EXPECT_EQ( Orientation::Negative, robust::orient3d( a, c, b, Vec3d( 0, 0, -1 ) ) );
    EXPECT_EQ( Orientation::Positive, robust::orient3d( a, c, b, Vec3d( 0, 0, 1 ) ) );

    // Documented identity: sign of dot( cross( a - d, b - d ), c - d )
    const Vec3d  d( 0.25, 0.25, -2.0 );
    const double expected = ( ( a - d ) ^ ( b - d ) ) * ( c - d );
    EXPECT_GT( expected, 0.0 );
    EXPECT_EQ( Orientation::Positive, robust::orient3d( a, b, c, d ) );
    EXPECT_GT( robust::orient3dValue( a, b, c, d ), 0.0 );
    EXPECT_LT( robust::orient3dValue( a, b, c, Vec3d( 0.25, 0.25, 2.0 ) ), 0.0 );

    // Degenerate input
    EXPECT_EQ( Orientation::Zero, robust::orient3d( a, a, b, c ) );
    EXPECT_EQ( Orientation::Zero, robust::orient3d( a, b, c, a ) );
}

//--------------------------------------------------------------------------------------------------
/// Kettner et al., "Classroom examples of robustness problems in geometric
/// computations". The points p are on a grid with the spacing of one ulp around
/// (0.5, 0.5), q and r are on the line y = x. The exact determinant is 12 * (
/// p.y - p.x ), so the exact orientation is sign( j - i ).
//--------------------------------------------------------------------------------------------------
TEST( RobustPredicates, Orient2dUlpGridIsConsistent )
{
    const double ulp = std::ldexp( 1.0, -53 );
    const Vec2d  q( 12.0, 12.0 );
    const Vec2d  r( 24.0, 24.0 );

    const int gridSize = 256;
    for ( int j = 0; j < gridSize; j++ )
    {
        for ( int i = 0; i < gridSize; i++ )
        {
            const Vec2d p( 0.5 + i * ulp, 0.5 + j * ulp );

            const Orientation expected = signOf( j - i );
            EXPECT_EQ( expected, robust::orient2d( p, q, r ) ) << "i=" << i << " j=" << j;

            // The cyclic permutations are the same orientation, swapping flips it
            EXPECT_EQ( expected, robust::orient2d( q, r, p ) );
            EXPECT_EQ( negate( expected ), robust::orient2d( q, p, r ) );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RobustPredicates, Orient2dCollinearAtUtmCoordinates )
{
    const double x0 = 450000.0;

    // y = y0 + 0.5 * ( x - x0 ), exact for all dyadic x
    auto onLine = []( double x ) { return Vec2d( x, 6700000.0 + 0.5 * ( x - 450000.0 ) ); };

    const Vec2d a = onLine( x0 + 0.125 );
    const Vec2d b = onLine( x0 + 100.375 );
    const Vec2d c = onLine( x0 + 957.625 );

    EXPECT_EQ( Orientation::Zero, robust::orient2d( a, b, c ) );
    EXPECT_EQ( Orientation::Zero, robust::orient2d( c, a, b ) );
    EXPECT_EQ( Orientation::Zero, robust::orient2d( c, b, a ) );

    // One ulp above the line is to the left of a -> b, one ulp below is to the
    // right
    for ( int steps = 1; steps <= 4; steps++ )
    {
        const Vec2d above( c.x(), nextUp( c.y(), steps ) );
        const Vec2d below( c.x(), nextUp( c.y(), -steps ) );

        EXPECT_EQ( Orientation::Positive, robust::orient2d( a, b, above ) );
        EXPECT_EQ( Orientation::Negative, robust::orient2d( a, b, below ) );
        EXPECT_EQ( Orientation::Negative, robust::orient2d( b, a, above ) );
    }

    // Perturbing in x moves the point to the right/left of the line
    EXPECT_EQ( Orientation::Negative, robust::orient2d( a, b, Vec2d( nextUp( c.x(), 1 ), c.y() ) ) );
    EXPECT_EQ( Orientation::Positive, robust::orient2d( a, b, Vec2d( nextUp( c.x(), -1 ), c.y() ) ) );

    // Also for a line that is not representable exactly in direction, the point
    // itself is on the line
    EXPECT_EQ( Orientation::Zero, robust::orient2d( a, b, a ) );
    EXPECT_EQ( Orientation::Zero, robust::orient2d( a, b, b ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RobustPredicates, Orient3dCoplanarAtUtmCoordinates )
{
    // Horizontal plane
    {
        const double z = -2000.0;
        const Vec3d  a( 450000.5, 6700000.25, z );
        const Vec3d  b( 450100.75, 6700000.5, z );
        const Vec3d  c( 450050.125, 6700123.375, z );
        const Vec3d  d( 450033.0, 6700044.0, z );

        EXPECT_EQ( Orientation::Zero, robust::orient3d( a, b, c, d ) );
        EXPECT_EQ( Orientation::Zero, robust::orient3d( d, c, b, a ) );

        ASSERT_EQ( Orientation::Positive,
                   robust::orient2d( Vec2d( a.x(), a.y() ), Vec2d( b.x(), b.y() ), Vec2d( c.x(), c.y() ) ) );

        const Vec3d dUp( d.x(), d.y(), nextUp( z, 1 ) );
        const Vec3d dDown( d.x(), d.y(), nextUp( z, -1 ) );
        EXPECT_EQ( Orientation::Negative, robust::orient3d( a, b, c, dUp ) );
        EXPECT_EQ( Orientation::Positive, robust::orient3d( a, b, c, dDown ) );
    }

    // Tilted plane, exactly representable
    {
        UtmPlane plane;

        const Vec3d a = plane.pointOnPlane( 0.5, 0.25 );
        const Vec3d b = plane.pointOnPlane( 100.75, 3.5 );
        const Vec3d c = plane.pointOnPlane( 40.125, 123.375 );
        const Vec3d d = plane.pointOnPlane( 33.0, 44.0 );

        ASSERT_EQ( Orientation::Positive,
                   robust::orient2d( Vec2d( a.x(), a.y() ), Vec2d( b.x(), b.y() ), Vec2d( c.x(), c.y() ) ) );
        EXPECT_EQ( Orientation::Zero, robust::orient3d( a, b, c, d ) );

        for ( int steps = 1; steps <= 4; steps++ )
        {
            const Vec3d dUp( d.x(), d.y(), nextUp( d.z(), steps ) );
            const Vec3d dDown( d.x(), d.y(), nextUp( d.z(), -steps ) );
            EXPECT_EQ( Orientation::Negative, robust::orient3d( a, b, c, dUp ) );
            EXPECT_EQ( Orientation::Positive, robust::orient3d( a, b, c, dDown ) );
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// Random points on an exactly representable plane, with the fourth point moved
/// a few ulp off the plane
//--------------------------------------------------------------------------------------------------
TEST( RobustPredicates, Orient3dNearlyCoplanarRandom )
{
    std::mt19937_64                    rng( 4711 );
    std::uniform_int_distribution<int> coord( -8000, 8000 ); // multiples of 1/8 => +-1000 m
    std::uniform_int_distribution<int> ulps( -3, 3 );

    UtmPlane plane;

    auto randomPoint = [&]() { return plane.pointOnPlane( coord( rng ) / 8.0, coord( rng ) / 8.0 ); };

    int checked = 0;
    for ( int n = 0; n < 20000; n++ )
    {
        const Vec3d a = randomPoint();
        const Vec3d b = randomPoint();
        const Vec3d c = randomPoint();
        Vec3d       d = randomPoint();

        const Orientation xy = robust::orient2d( Vec2d( a.x(), a.y() ), Vec2d( b.x(), b.y() ), Vec2d( c.x(), c.y() ) );
        if ( xy == Orientation::Zero ) continue;

        const int k = ulps( rng );
        d           = Vec3d( d.x(), d.y(), nextUp( d.z(), k ) );

        // d above the plane (k > 0) is Negative for counterclockwise a, b, c seen
        // from above
        Orientation expected = negate( signOf( k ) );
        if ( xy == Orientation::Negative ) expected = negate( expected );

        EXPECT_EQ( expected, robust::orient3d( a, b, c, d ) );
        EXPECT_EQ( negate( expected ), robust::orient3d( b, a, c, d ) );
        EXPECT_EQ( expected, robust::orient3d( b, c, a, d ) );
        checked++;
    }

    EXPECT_GT( checked, 15000 );
}

//--------------------------------------------------------------------------------------------------
/// Compare with an exact integer reference. The small range gives many exact
/// zeros, the large range gives mostly nonzero results. Also checks symmetry
/// and exact translation invariance.
//--------------------------------------------------------------------------------------------------
TEST( RobustPredicates, Orient2dRandomAgainstIntegerReference )
{
    std::mt19937_64 rng( 1 );

    for ( int range : { 3, 40, 1 << 20 } )
    {
        std::uniform_int_distribution<int> coord( -range, range );
        const double                       shift = 1 << 24; // exact translation for integer input

        int zeros = 0;
        for ( int n = 0; n < 20000; n++ )
        {
            const Vec2d a( coord( rng ), coord( rng ) );
            const Vec2d b( coord( rng ), coord( rng ) );
            const Vec2d c( coord( rng ), coord( rng ) );

            const Orientation expected = referenceOrient2d( a, b, c );
            if ( expected == Orientation::Zero ) zeros++;

            EXPECT_EQ( expected, robust::orient2d( a, b, c ) );
            EXPECT_EQ( negate( expected ), robust::orient2d( b, a, c ) );
            EXPECT_EQ( negate( expected ), robust::orient2d( a, c, b ) );
            EXPECT_EQ( expected, robust::orient2d( b, c, a ) );

            const Vec2d t( shift, -shift );
            EXPECT_EQ( expected, robust::orient2d( a + t, b + t, c + t ) );
        }

        if ( range == 3 ) EXPECT_GT( zeros, 0 );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RobustPredicates, Orient3dRandomAgainstIntegerReference )
{
    std::mt19937_64 rng( 2 );

    // Differences up to 2^16, a product of three stays below 2^63
    for ( int range : { 2, 12, 1 << 15 } )
    {
        std::uniform_int_distribution<int> coord( -range, range );
        const double                       shift = 1 << 24;

        int zeros = 0;
        for ( int n = 0; n < 20000; n++ )
        {
            const Vec3d a( coord( rng ), coord( rng ), coord( rng ) );
            const Vec3d b( coord( rng ), coord( rng ), coord( rng ) );
            const Vec3d c( coord( rng ), coord( rng ), coord( rng ) );
            const Vec3d d( coord( rng ), coord( rng ), coord( rng ) );

            const Orientation expected = referenceOrient3d( a, b, c, d );
            if ( expected == Orientation::Zero ) zeros++;

            EXPECT_EQ( expected, robust::orient3d( a, b, c, d ) );
            EXPECT_EQ( negate( expected ), robust::orient3d( b, a, c, d ) );
            EXPECT_EQ( negate( expected ), robust::orient3d( a, c, b, d ) );
            EXPECT_EQ( negate( expected ), robust::orient3d( a, b, d, c ) );
            EXPECT_EQ( expected, robust::orient3d( b, c, a, d ) );

            const Vec3d t( shift, -shift, shift );
            EXPECT_EQ( expected, robust::orient3d( a + t, b + t, c + t, d + t ) );
        }

        if ( range == 2 ) EXPECT_GT( zeros, 0 );
    }
}

//--------------------------------------------------------------------------------------------------
/// All threads start at the same time, the first call to the predicates
/// initializes the library
//--------------------------------------------------------------------------------------------------
TEST( RobustPredicates, ConcurrentCalls )
{
    const int                threadCount = 16;
    std::atomic<bool>        start( false );
    std::atomic<int>         failures( 0 );
    std::vector<std::thread> threads;

    UtmPlane    plane;
    const Vec3d a = plane.pointOnPlane( 0.5, 0.25 );
    const Vec3d b = plane.pointOnPlane( 100.75, 3.5 );
    const Vec3d c = plane.pointOnPlane( 40.125, 123.375 );
    const Vec3d d = plane.pointOnPlane( 33.0, 44.0 );
    const Vec3d dUp( d.x(), d.y(), nextUp( d.z(), 1 ) );

    for ( int t = 0; t < threadCount; t++ )
    {
        threads.emplace_back(
            [&]()
            {
                while ( !start.load() )
                {
                }

                for ( int i = 0; i < 2000; i++ )
                {
                    if ( robust::orient3d( a, b, c, d ) != Orientation::Zero ) failures++;
                    if ( robust::orient3d( a, b, c, dUp ) != Orientation::Negative ) failures++;
                    if ( robust::orient2d( Vec2d( 0, 0 ), Vec2d( 1, 0 ), Vec2d( 0, 1 ) ) != Orientation::Positive )
                        failures++;
                }
            } );
    }

    start = true;
    for ( auto& thread : threads )
    {
        thread.join();
    }

    EXPECT_EQ( 0, failures.load() );
}

#ifdef CVF_HAS_FMA_PROBE
//--------------------------------------------------------------------------------------------------
/// The vendored predicates.c is compiled with FMA contraction disabled. Compare
/// with a build of the same source with contraction enabled. The results must
/// be bit identical, otherwise results would depend on compiler and platform
/// (see OPM/ResInsight#15088).
//--------------------------------------------------------------------------------------------------
TEST( RobustPredicates, SameSignWithAndWithoutFmaContraction )
{
    exactinit_contracted();

    auto signOfValue = []( double v ) { return ( v > 0.0 ) - ( v < 0.0 ); };

    std::mt19937_64 rng( 99 );

    UtmPlane                               plane;
    std::uniform_int_distribution<int>     coord( -8000, 8000 );
    std::uniform_int_distribution<int>     ulps( -3, 3 );
    std::uniform_real_distribution<double> real( -1.0, 1.0 );

    int mismatches = 0;
    int cases      = 0;

    auto check2d = [&]( const Vec2d& a, const Vec2d& b, const Vec2d& c )
    {
        double pa[2] = { a.x(), a.y() };
        double pb[2] = { b.x(), b.y() };
        double pc[2] = { c.x(), c.y() };

        cases++;
        if ( signOfValue( robust::orient2dValue( a, b, c ) ) != signOfValue( orient2d_contracted( pa, pb, pc ) ) )
            mismatches++;
    };

    auto check3d = [&]( const Vec3d& a, const Vec3d& b, const Vec3d& c, const Vec3d& d )
    {
        double pa[3] = { a.x(), a.y(), a.z() };
        double pb[3] = { b.x(), b.y(), b.z() };
        double pc[3] = { c.x(), c.y(), c.z() };
        double pd[3] = { d.x(), d.y(), d.z() };

        cases++;
        if ( signOfValue( robust::orient3dValue( a, b, c, d ) ) != signOfValue( orient3d_contracted( pa, pb, pc, pd ) ) )
            mismatches++;
    };

    for ( int n = 0; n < 20000; n++ )
    {
        // Generic double input
        const Vec3d ra( real( rng ), real( rng ), real( rng ) );
        const Vec3d rb( real( rng ), real( rng ), real( rng ) );
        const Vec3d rc( real( rng ), real( rng ), real( rng ) );
        const Vec3d rd( real( rng ), real( rng ), real( rng ) );
        check3d( ra, rb, rc, rd );
        check2d( Vec2d( ra.x(), ra.y() ), Vec2d( rb.x(), rb.y() ), Vec2d( rc.x(), rc.y() ) );

        // Nearly degenerate input at UTM coordinates, forces the exact stages
        const Vec3d a = plane.pointOnPlane( coord( rng ) / 8.0, coord( rng ) / 8.0 );
        const Vec3d b = plane.pointOnPlane( coord( rng ) / 8.0, coord( rng ) / 8.0 );
        const Vec3d c = plane.pointOnPlane( coord( rng ) / 8.0, coord( rng ) / 8.0 );
        Vec3d       d = plane.pointOnPlane( coord( rng ) / 8.0, coord( rng ) / 8.0 );
        d             = Vec3d( d.x(), d.y(), nextUp( d.z(), ulps( rng ) ) );
        check3d( a, b, c, d );

        // Nearly collinear
        const Vec2d la( a.x(), 6700000.0 + 0.5 * ( a.x() - 450000.0 ) );
        const Vec2d lb( b.x(), 6700000.0 + 0.5 * ( b.x() - 450000.0 ) );
        const Vec2d lc( c.x(), nextUp( 6700000.0 + 0.5 * ( c.x() - 450000.0 ), ulps( rng ) ) );
        check2d( la, lb, lc );
    }

    EXPECT_GT( cases, 0 );
    EXPECT_EQ( 0, mismatches );
}
#endif
