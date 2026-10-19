#include "gtest/gtest.h"

#include "RifEclipseSummaryAddress.h"
#include "RimSummaryCurveAppearanceSettings.h"

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RimSummaryCurveAppearanceSettingsTest, DefaultsControlColorByCase )
{
    RimSummaryCurveAppearanceSettings settings;

    EXPECT_EQ( RimSummaryCurveAppearanceSettings::Dimension::CASE, settings.colorDimension() );
    EXPECT_EQ( RimSummaryCurveAppearanceSettings::Dimension::NONE, settings.symbolDimension() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RimSummaryCurveAppearanceSettingsTest, DimensionValues )
{
    using D = RimSummaryCurveAppearanceSettings::Dimension;

    const auto well   = RifEclipseSummaryAddress::wellAddress( "WOPR", "A-1" );
    const auto region = RifEclipseSummaryAddress::regionAddress( "ROPR", 3 );

    EXPECT_EQ( "A-1", RimSummaryCurveAppearanceSettings::dimensionValue( D::WELL, well, "case" ) );
    EXPECT_EQ( "case", RimSummaryCurveAppearanceSettings::dimensionValue( D::CASE, well, "case" ) );
    EXPECT_EQ( "WOPR", RimSummaryCurveAppearanceSettings::dimensionValue( D::VECTOR, well, "case" ) );
    EXPECT_TRUE( RimSummaryCurveAppearanceSettings::dimensionValue( D::REGION, well, "case" ).empty() );
    EXPECT_EQ( "3", RimSummaryCurveAppearanceSettings::dimensionValue( D::REGION, region, "case" ) );
}
