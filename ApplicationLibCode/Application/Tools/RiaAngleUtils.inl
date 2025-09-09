/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2025     Equinor ASA
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

#include <cmath>
#include <type_traits>

namespace Ria
{

//==================================================================================================
// Degrees normalization
//==================================================================================================
template <typename T>
inline Degrees<T> Degrees<T>::normalized() const
{
    T normalized = cvf::Math::fmod( m_value, T( 360 ) );
    if ( normalized < T( 0 ) )
    {
        normalized += T( 360 );
    }
    return Degrees<T>( normalized );
}

//==================================================================================================
// Radians normalization
//==================================================================================================
template <typename T>
inline Radians<T> Radians<T>::normalized() const
{
    T twoPi      = T( 2 ) * ( std::is_same_v<T, float> ? cvf::PI_F : cvf::PI_D );
    T normalized = cvf::Math::fmod( m_value, twoPi );
    if ( normalized < T( 0 ) )
    {
        normalized += twoPi;
    }
    return Radians<T>( normalized );
}

//==================================================================================================
// Coordinate conversion implementations
//==================================================================================================

template <typename T>
inline cvf::Vector3<T> CoordinateConverter::cylindricalToCartesian( T radius, const Radians<T>& angle, T height )
{
    T x = radius * cvf::Math::cos( angle.value() );
    T y = radius * cvf::Math::sin( angle.value() );
    return cvf::Vector3<T>( x, y, height );
}

template <typename T>
inline cvf::Vector3<T> CoordinateConverter::cylindricalToCartesian( T radius, const Degrees<T>& angle, T height )
{
    T angleRad = cvf::Math::toRadians( angle.value() );
    T x        = radius * cvf::Math::cos( angleRad );
    T y        = radius * cvf::Math::sin( angleRad );
    return cvf::Vector3<T>( x, y, height );
}

template <typename T>
inline cvf::Vector3<T> CoordinateConverter::cylindricalToCartesian( const CylindricalCoordinate& coord )
{
    T x = static_cast<T>( coord.radius * cvf::Math::cos( coord.angle ) );
    T y = static_cast<T>( coord.radius * cvf::Math::sin( coord.angle ) );
    T z = static_cast<T>( coord.height );
    return cvf::Vector3<T>( x, y, z );
}

template <typename T>
inline CylindricalCoordinate CoordinateConverter::cartesianToCylindrical( const cvf::Vector3<T>& cartesian )
{
    return cartesianToCylindrical( cartesian.x(), cartesian.y(), cartesian.z() );
}

template <typename T>
inline CylindricalCoordinate CoordinateConverter::cartesianToCylindrical( T x, T y, T z )
{
    double radius = cvf::Math::sqrt( static_cast<double>( x * x + y * y ) );
    double angle  = std::atan2( static_cast<double>( y ), static_cast<double>( x ) );
    double height = static_cast<double>( z );

    return CylindricalCoordinate( radius, Radiansd( angle ), height );
}

} // namespace Ria
