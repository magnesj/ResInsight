//##################################################################################################
//
//   Custom Visualization Core library
//   Copyright (C) 2011-2013 Ceetron AS
//
//   GNU General Public License Usage
//   This library is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//##################################################################################################

#include "../cafAngleUtils.h"
#include "cvfBase.h"
#include "cvfMath.h"

#include "gtest/gtest.h"

using namespace caf;

TEST(CafAngleUtilsTest, DegreesBasicOperations)
{
    Degreesd d1(90.0);
    Degreesd d2(45.0);
    
    EXPECT_DOUBLE_EQ(90.0, d1.value());
    EXPECT_DOUBLE_EQ(45.0, d2.value());
    
    auto sum = d1 + d2;
    EXPECT_DOUBLE_EQ(135.0, sum.value());
    
    auto diff = d1 - d2;
    EXPECT_DOUBLE_EQ(45.0, diff.value());
    
    auto scaled = d1 * 2.0;
    EXPECT_DOUBLE_EQ(180.0, scaled.value());
    
    auto divided = d1 / 2.0;
    EXPECT_DOUBLE_EQ(45.0, divided.value());
}

TEST(CafAngleUtilsTest, RadiansBasicOperations)
{
    Radiansd r1(cvf::PI_D / 2.0);  // 90 degrees
    Radiansd r2(cvf::PI_D / 4.0);  // 45 degrees
    
    EXPECT_DOUBLE_EQ(cvf::PI_D / 2.0, r1.value());
    EXPECT_DOUBLE_EQ(cvf::PI_D / 4.0, r2.value());
    
    auto sum = r1 + r2;
    EXPECT_DOUBLE_EQ(3.0 * cvf::PI_D / 4.0, sum.value());
    
    auto diff = r1 - r2;
    EXPECT_DOUBLE_EQ(cvf::PI_D / 4.0, diff.value());
    
    auto scaled = r1 * 2.0;
    EXPECT_DOUBLE_EQ(cvf::PI_D, scaled.value());
    
    auto divided = r1 / 2.0;
    EXPECT_DOUBLE_EQ(cvf::PI_D / 4.0, divided.value());
}

TEST(CafAngleUtilsTest, DegreesRadiansConversion)
{
    Degreesd degrees(180.0);
    Radiansd radians(degrees);
    double diff1 = cvf::Math::abs(cvf::PI_D - radians.value());
    EXPECT_LT(diff1, 1e-10);
    
    Radiansd piRadians(cvf::PI_D);
    Degreesd fromRadians(piRadians);
    double diff2 = cvf::Math::abs(180.0 - fromRadians.value());
    EXPECT_LT(diff2, 1e-10);
    
    Degreesd deg90(90.0);
    Radiansd rad90(deg90);
    double diff3 = cvf::Math::abs(cvf::PI_D / 2.0 - rad90.value());
    EXPECT_LT(diff3, 1e-10);
    
    Degreesd original(45.0);
    Radiansd converted(original);
    Degreesd backToOriginal(converted);
    double diff4 = cvf::Math::abs(45.0 - backToOriginal.value());
    EXPECT_LT(diff4, 1e-10);
}

TEST(CafAngleUtilsTest, TrigonometricFunctions)
{
    Degreesd deg90(90.0);
    double sin90 = sin(deg90);
    double cos90 = cos(deg90);
    EXPECT_LT(cvf::Math::abs(1.0 - sin90), 1e-10);
    EXPECT_LT(cvf::Math::abs(0.0 - cos90), 1e-10);
    
    Degreesd deg45(45.0);
    double sin45 = sin(deg45);
    double cos45 = cos(deg45);
    double tan45 = tan(deg45);
    EXPECT_LT(cvf::Math::abs(cvf::SQRT1_2_D - sin45), 1e-10);
    EXPECT_LT(cvf::Math::abs(cvf::SQRT1_2_D - cos45), 1e-10);
    EXPECT_LT(cvf::Math::abs(1.0 - tan45), 1e-10);
    
    Radiansd radPiOver2(cvf::PI_D / 2.0);
    double sinPi2 = sin(radPiOver2);
    double cosPi2 = cos(radPiOver2);
    EXPECT_LT(cvf::Math::abs(1.0 - sinPi2), 1e-10);
    EXPECT_LT(cvf::Math::abs(0.0 - cosPi2), 1e-10);
}

TEST(CafAngleUtilsTest, CylindricalToCartesianConversion)
{
    double radius = 5.0;
    Radiansd angle(cvf::PI_D / 4.0);  // 45 degrees
    double height = 10.0;
    
    auto cartesian = CoordinateConverter::cylindricalToCartesian(radius, angle, height);
    
    double expectedX = radius * cvf::SQRT1_2_D;
    double expectedY = radius * cvf::SQRT1_2_D;
    
    EXPECT_LT(cvf::Math::abs(expectedX - cartesian.x()), 1e-10);
    EXPECT_LT(cvf::Math::abs(expectedY - cartesian.y()), 1e-10);
    EXPECT_LT(cvf::Math::abs(height - cartesian.z()), 1e-10);
    
    Degreesd angleDeg(45.0);
    auto cartesianFromDeg = CoordinateConverter::cylindricalToCartesian(radius, angleDeg, height);
    
    EXPECT_LT(cvf::Math::abs(cartesian.x() - cartesianFromDeg.x()), 1e-10);
    EXPECT_LT(cvf::Math::abs(cartesian.y() - cartesianFromDeg.y()), 1e-10);
    EXPECT_LT(cvf::Math::abs(cartesian.z() - cartesianFromDeg.z()), 1e-10);
}

TEST(CafAngleUtilsTest, CartesianToCylindricalConversion)
{
    cvf::Vec3d cartesian(3.0, 4.0, 5.0);
    
    auto cylindrical = CoordinateConverter::cartesianToCylindrical(cartesian);
    
    EXPECT_LT(cvf::Math::abs(5.0 - cylindrical.radius), 1e-10);  // sqrt(3^2 + 4^2) = 5
    EXPECT_LT(cvf::Math::abs(std::atan2(4.0, 3.0) - cylindrical.angle), 1e-10);
    EXPECT_LT(cvf::Math::abs(5.0 - cylindrical.height), 1e-10);
    
    cvf::Vec3d backToCartesian = CoordinateConverter::cylindricalToCartesian<double>(cylindrical);
    
    EXPECT_LT(cvf::Math::abs(cartesian.x() - backToCartesian.x()), 1e-10);
    EXPECT_LT(cvf::Math::abs(cartesian.y() - backToCartesian.y()), 1e-10);
    EXPECT_LT(cvf::Math::abs(cartesian.z() - backToCartesian.z()), 1e-10);
}

TEST(CafAngleUtilsTest, ComparisonOperators)
{
    Degreesd deg45(45.0);
    Degreesd deg90(90.0);
    Degreesd deg45_2(45.0);
    
    EXPECT_TRUE(deg45 == deg45_2);
    EXPECT_FALSE(deg45 == deg90);
    EXPECT_TRUE(deg45 != deg90);
    EXPECT_FALSE(deg45 != deg45_2);
    
    EXPECT_TRUE(deg45 < deg90);
    EXPECT_FALSE(deg90 < deg45);
    EXPECT_TRUE(deg45 <= deg90);
    EXPECT_TRUE(deg45 <= deg45_2);
    
    EXPECT_TRUE(deg90 > deg45);
    EXPECT_FALSE(deg45 > deg90);
    EXPECT_TRUE(deg90 >= deg45);
    EXPECT_TRUE(deg45 >= deg45_2);
}

TEST(CafAngleUtilsTest, FloatPrecision)
{
    Degreesf degf(90.0f);
    Radiansf radf(degf);
    
    float expectedRad = cvf::PI_F / 2.0f;
    EXPECT_LT(cvf::Math::abs(expectedRad - radf.value()), 1e-6f);
    
    float sinResult = sin(degf);
    EXPECT_LT(cvf::Math::abs(1.0f - sinResult), 1e-6f);
    
    cvf::Vec3f cartesianf = CoordinateConverter::cylindricalToCartesian(5.0f, degf, 10.0f);
    EXPECT_LT(cvf::Math::abs(0.0f - cartesianf.x()), 1e-6f);
    EXPECT_LT(cvf::Math::abs(5.0f - cartesianf.y()), 1e-6f);
    EXPECT_LT(cvf::Math::abs(10.0f - cartesianf.z()), 1e-6f);
}

TEST(CafAngleUtilsTest, AngleLiterals)
{
    using namespace caf;
    
    auto deg90 = 90.0_deg;
    EXPECT_LT(cvf::Math::abs(90.0 - deg90.value()), 1e-10);
    
    auto degFloat = 45.0_degf;
    EXPECT_LT(cvf::Math::abs(45.0f - degFloat.value()), 1e-6f);
    
    auto radPi = 3.14159265_rad;
    EXPECT_LT(cvf::Math::abs(3.14159265 - radPi.value()), 1e-8);
    
    auto radFloat = 1.57079633_radf;
    EXPECT_LT(cvf::Math::abs(1.57079633f - radFloat.value()), 1e-6f);
}