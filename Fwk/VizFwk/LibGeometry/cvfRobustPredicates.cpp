#include "cvfBase.h"

#include "cvfRobustPredicates.h"

#include "predicates.h"

#include <cfloat>
#include <limits>
#include <mutex>

// The predicates require plain IEEE-754 double arithmetic. x87 extended
// precision intermediates (FLT_EVAL_METHOD != 0) break the error-free
// transformations.
static_assert( std::numeric_limits<double>::is_iec559, "Robust predicates require IEEE-754 doubles" );
static_assert( FLT_EVAL_METHOD == 0, "Robust predicates require FLT_EVAL_METHOD == 0 (no extended precision)" );

namespace cvf::robust
{

namespace
{
    //--------------------------------------------------------------------------------------------------
    ///
    //--------------------------------------------------------------------------------------------------
    void ensureInitialized()
    {
        static std::once_flag flag;
        std::call_once( flag, []() { exactinit(); } );
    }

    //--------------------------------------------------------------------------------------------------
    ///
    //--------------------------------------------------------------------------------------------------
    Orientation toOrientation( double value )
    {
        if ( value > 0.0 ) return Orientation::Positive;
        if ( value < 0.0 ) return Orientation::Negative;
        return Orientation::Zero;
    }
} // namespace

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double orient2dValue( const Vec2d& a, const Vec2d& b, const Vec2d& c )
{
    ensureInitialized();

    // The C interface takes non-const pointers, copy to keep the interface const
    // correct
    double pa[2] = { a.x(), a.y() };
    double pb[2] = { b.x(), b.y() };
    double pc[2] = { c.x(), c.y() };

    return ::orient2d( pa, pb, pc );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double orient3dValue( const Vec3d& a, const Vec3d& b, const Vec3d& c, const Vec3d& d )
{
    ensureInitialized();

    double pa[3] = { a.x(), a.y(), a.z() };
    double pb[3] = { b.x(), b.y(), b.z() };
    double pc[3] = { c.x(), c.y(), c.z() };
    double pd[3] = { d.x(), d.y(), d.z() };

    return ::orient3d( pa, pb, pc, pd );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
Orientation orient2d( const Vec2d& a, const Vec2d& b, const Vec2d& c )
{
    return toOrientation( orient2dValue( a, b, c ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
Orientation orient3d( const Vec3d& a, const Vec3d& b, const Vec3d& c, const Vec3d& d )
{
    return toOrientation( orient3dValue( a, b, c, d ) );
}

} // namespace cvf::robust
