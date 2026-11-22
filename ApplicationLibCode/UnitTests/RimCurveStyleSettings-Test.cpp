#include "gtest/gtest.h"

#include "RifEclipseSummaryAddress.h"
#include "RimCurveStyleSettings.h"
#include "RimNamedCurveStyleRule.h"
#include "RimPlotCurveAppearance.h"

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RimCurveStyleSettingsTest, SettingHoldsFullAppearance )
{
    RimCurveStyleSettings source;
    source.setName( "Thick" );
    source.curveAppearance()->setLineThickness( 6 );

    RimCurveStyleSettings target;
    target.copyAppearanceFrom( source );

    EXPECT_EQ( 6, target.curveAppearance()->lineThickness() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RimCurveStyleSettingsTest, DimensionValues )
{
    using D = RimNamedCurveStyleRule::Dimension;

    const auto well   = RifEclipseSummaryAddress::wellAddress( "WOPR", "A-1" );
    const auto region = RifEclipseSummaryAddress::regionAddress( "ROPR", 3 );

    EXPECT_EQ( "A-1", RimNamedCurveStyleRule::dimensionValue( D::WELL, well, "case" ) );
    EXPECT_EQ( "case", RimNamedCurveStyleRule::dimensionValue( D::CASE, well, "case" ) );
    EXPECT_EQ( "WOPR", RimNamedCurveStyleRule::dimensionValue( D::VECTOR, well, "case" ) );
    EXPECT_TRUE( RimNamedCurveStyleRule::dimensionValue( D::REGION, well, "case" ).empty() );
    EXPECT_EQ( "3", RimNamedCurveStyleRule::dimensionValue( D::REGION, region, "case" ) );
}
