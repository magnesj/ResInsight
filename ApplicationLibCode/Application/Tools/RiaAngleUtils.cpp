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

#include "RiaAngleUtils.h"

#include <cmath>

namespace Ria
{

// Explicit instantiation of commonly used types
template class Degrees<float>;
template class Degrees<double>;
template class Radians<float>;
template class Radians<double>;

// Explicit instantiation of coordinate conversion functions
template cvf::Vector3<float>   CoordinateConverter::cylindricalToCartesian<float>( float, const Radians<float>&, float );
template cvf::Vector3<double>  CoordinateConverter::cylindricalToCartesian<double>( double, const Radians<double>&, double );
template cvf::Vector3<float>   CoordinateConverter::cylindricalToCartesian<float>( float, const Degrees<float>&, float );
template cvf::Vector3<double>  CoordinateConverter::cylindricalToCartesian<double>( double, const Degrees<double>&, double );
template cvf::Vector3<float>   CoordinateConverter::cylindricalToCartesian<float>( const CylindricalCoordinate& );
template cvf::Vector3<double>  CoordinateConverter::cylindricalToCartesian<double>( const CylindricalCoordinate& );
template CylindricalCoordinate CoordinateConverter::cartesianToCylindrical<float>( const cvf::Vector3<float>& );
template CylindricalCoordinate CoordinateConverter::cartesianToCylindrical<double>( const cvf::Vector3<double>& );
template CylindricalCoordinate CoordinateConverter::cartesianToCylindrical<float>( float, float, float );
template CylindricalCoordinate CoordinateConverter::cartesianToCylindrical<double>( double, double, double );

// Explicit instantiation of trigonometric functions
template float  sin<float>( const Degrees<float>& );
template double sin<double>( const Degrees<double>& );
template float  sin<float>( const Radians<float>& );
template double sin<double>( const Radians<double>& );

template float  cos<float>( const Degrees<float>& );
template double cos<double>( const Degrees<double>& );
template float  cos<float>( const Radians<float>& );
template double cos<double>( const Radians<double>& );

template float  tan<float>( const Degrees<float>& );
template double tan<double>( const Degrees<double>& );
template float  tan<float>( const Radians<float>& );
template double tan<double>( const Radians<double>& );

template Radians<float>  asin<float>( float );
template Radians<double> asin<double>( double );
template Radians<float>  acos<float>( float );
template Radians<double> acos<double>( double );
template Radians<float>  atan<float>( float );
template Radians<double> atan<double>( double );
template Radians<float>  atan2<float>( float, float );
template Radians<double> atan2<double>( double, double );

} // namespace Ria
