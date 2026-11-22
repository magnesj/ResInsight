#include "gtest/gtest.h"

#include "RifEclipseSummaryAddress.h"

#include "RimCurveStyleRule.h"
#include "RimCurveStyleRules.h"
#include "RimCurveStyleSettings.h"
#include "RimNamedCurveStyleRule.h"
#include "RimPlotCurveAppearance.h"
#include "RimSavedCurveAppearance.h"
#include "RimSummaryCurve.h"

using D = RimCurveStyleSettings::Dimension;

//--------------------------------------------------------------------------------------------------
/// Rules for the selected dimensions are returned ordered by dimension, independent of creation order
//--------------------------------------------------------------------------------------------------
TEST( RimCurveStyleRulesTest, RulesForOrderedByDimension )
{
    RimCurveStyleSettings settings;
    settings.setColorDimension( D::WELL );
    settings.setSymbolDimension( D::CASE );

    auto wellRule = settings.findOrCreateRule( D::WELL, "A-1" );
    wellRule->setCustom( true );
    settings.findOrCreateRule( D::WELL, "B-2" )->setCustom( true );
    settings.findOrCreateRule( D::VECTOR, "WOPR" )->setCustom( true );
    settings.findOrCreateRule( D::CASE, "auto case" );
    auto caseRule = settings.findOrCreateRule( D::CASE, "case" );
    caseRule->setCustom( true );

    EXPECT_EQ( wellRule, settings.findOrCreateRule( D::WELL, "A-1" ) );

    const auto address  = RifEclipseSummaryAddress::wellAddress( "WOPR", "A-1" );
    const auto matching = RimCurveStyleSettings::rulesFor( settings.resolved(), address, "case" );

    // The two base styles come first, followed by the item rules ordered by dimension
    ASSERT_EQ( 4u, matching.size() );
    EXPECT_EQ( caseRule, matching[2].rule );
    EXPECT_TRUE( matching[2].applySymbol && !matching[2].applyColor );
    EXPECT_EQ( wellRule, matching[3].rule );
    EXPECT_TRUE( matching[3].applyColor && !matching[3].applySymbol );

    // The base styles and the well rule apply, as there is no custom rule for the case
    EXPECT_EQ( 3u, RimCurveStyleSettings::rulesFor( settings.resolved(), address, "auto case" ).size() );
}

//--------------------------------------------------------------------------------------------------
/// There is always a global setting, and settings can be added and deleted
//--------------------------------------------------------------------------------------------------
TEST( RimCurveStyleRulesTest, AlwaysAtLeastOnePreset )
{
    RimCurveStyleRules rules;
    ASSERT_EQ( 1u, rules.presets().size() );

    auto second = rules.addPreset( "Second" );
    EXPECT_EQ( 2u, rules.presets().size() );
    EXPECT_EQ( "Second", second->name() );

    rules.deletePreset( second );
    rules.deletePreset( rules.presets().front() );
    EXPECT_EQ( 1u, rules.presets().size() );
}

//--------------------------------------------------------------------------------------------------
/// The global setting assigns color and symbol by its own dimensions, but named rules take precedence
//--------------------------------------------------------------------------------------------------
TEST( RimCurveStyleRulesTest, NamedRulesTakePrecedence )
{
    RimCurveStyleRules rules;

    auto global = rules.defaultSettings();
    global->setColorDimension( D::WELL );
    global->setSymbolDimension( D::NONE );

    auto vectorSettings = rules.addPreset( "Vectors" );
    vectorSettings->setColorDimension( D::VECTOR );
    vectorSettings->setSymbolDimension( D::CASE );

    auto colorRule = rules.addNamedRule( "Color by vector" );
    colorRule->setDimension( D::VECTOR );
    colorRule->setSettings( vectorSettings );
    colorRule->setAppliesTo( RimNamedCurveStyleRule::AppliesTo::COLOR );

    auto symbolRule = rules.addNamedRule( "Symbol by case" );
    symbolRule->setDimension( D::CASE );
    symbolRule->setSettings( vectorSettings );
    symbolRule->setAppliesTo( RimNamedCurveStyleRule::AppliesTo::SYMBOL );

    const auto wellAddress  = RifEclipseSummaryAddress::wellAddress( "WOPR", "A-1" );
    const auto fieldAddress = RifEclipseSummaryAddress::fieldAddress( "FOPT" );

    // Only the global setting
    auto resolved = RimCurveStyleRules::resolve( global, {}, wellAddress, "case" );
    EXPECT_EQ( D::WELL, resolved.color.dimension );
    EXPECT_EQ( global, resolved.color.settings );
    EXPECT_FALSE( resolved.symbol.isSet() );

    // No value for the well dimension
    EXPECT_FALSE( RimCurveStyleRules::resolve( global, {}, fieldAddress, "case" ).color.isSet() );

    // The color rule replaces the color of the global setting
    resolved = RimCurveStyleRules::resolve( global, { colorRule }, wellAddress, "case" );
    EXPECT_EQ( D::VECTOR, resolved.color.dimension );
    EXPECT_EQ( vectorSettings, resolved.color.settings );
    EXPECT_FALSE( resolved.symbol.isSet() );

    // The symbol rule only affects the symbol
    resolved = RimCurveStyleRules::resolve( global, { symbolRule }, wellAddress, "case" );
    EXPECT_EQ( D::WELL, resolved.color.dimension );
    EXPECT_EQ( D::CASE, resolved.symbol.dimension );

    // Named rules work without a global setting
    resolved = RimCurveStyleRules::resolve( nullptr, { colorRule, symbolRule }, wellAddress, "case" );
    EXPECT_EQ( D::VECTOR, resolved.color.dimension );
    EXPECT_EQ( D::CASE, resolved.symbol.dimension );
    EXPECT_FALSE( RimCurveStyleRules::resolve( nullptr, {}, wellAddress, "case" ).color.isSet() );
}

//--------------------------------------------------------------------------------------------------
/// A named rule with selected items only applies to those items. Later rules win over earlier ones.
//--------------------------------------------------------------------------------------------------
TEST( RimCurveStyleRulesTest, NamedRuleItemsAndOrder )
{
    RimCurveStyleRules rules;

    auto first = rules.addPreset( "First" );
    first->setColorDimension( D::WELL );
    auto second = rules.addPreset( "Second" );
    second->setColorDimension( D::CASE );

    auto wellRule = rules.addNamedRule( "Wells" );
    wellRule->setDimension( D::WELL );
    wellRule->setSettings( first );
    wellRule->setItems( { "A-1" } );

    auto caseRule = rules.addNamedRule( "Cases" );
    caseRule->setDimension( D::CASE );
    caseRule->setSettings( second );

    const auto a1 = RifEclipseSummaryAddress::wellAddress( "WOPR", "A-1" );
    const auto b2 = RifEclipseSummaryAddress::wellAddress( "WOPR", "B-2" );

    EXPECT_TRUE( wellRule->matches( a1, "case" ) );
    EXPECT_FALSE( wellRule->matches( b2, "case" ) );

    EXPECT_EQ( D::WELL, RimCurveStyleRules::resolve( nullptr, { caseRule, wellRule }, a1, "case" ).color.dimension );
    EXPECT_EQ( D::CASE, RimCurveStyleRules::resolve( nullptr, { wellRule, caseRule }, a1, "case" ).color.dimension );
    EXPECT_EQ( D::CASE, RimCurveStyleRules::resolve( nullptr, { caseRule, wellRule }, b2, "case" ).color.dimension );
}

//--------------------------------------------------------------------------------------------------
/// Registered names keep their index when new names are added
//--------------------------------------------------------------------------------------------------
TEST( RimCurveStyleRulesTest, ItemIndicesStableWhenNamesAdded )
{
    RimCurveStyleRules rules;

    rules.registerItemNames( D::CASE, { "B", "D" } );
    auto indices = rules.itemIndices( D::CASE );
    EXPECT_EQ( 0, indices["B"] );
    EXPECT_EQ( 1, indices["D"] );

    rules.registerItemNames( D::CASE, { "A", "B", "C", "D" } );
    indices = rules.itemIndices( D::CASE );
    ASSERT_EQ( 4u, indices.size() );
    EXPECT_EQ( 0, indices["B"] );
    EXPECT_EQ( 1, indices["D"] );
    EXPECT_EQ( 2, indices["A"] );
    EXPECT_EQ( 3, indices["C"] );

    EXPECT_TRUE( rules.itemIndices( D::WELL ).empty() );
    EXPECT_TRUE( rules.itemIndices( D::VECTOR ).empty() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RimCurveStyleRulesTest, CopyAppearance )
{
    RimPlotCurveAppearance source;
    source.setColor( cvf::Color3f::RED );
    source.setSymbol( RiuPlotCurveSymbol::SYMBOL_DIAMOND );
    source.setLineThickness( 4 );
    source.setSymbolLabel( "label" );

    RimPlotCurveAppearance target;
    target.copyFrom( source );

    EXPECT_TRUE( target.color() == cvf::Color3f::RED );
    EXPECT_EQ( RiuPlotCurveSymbol::SYMBOL_DIAMOND, target.symbol() );
    EXPECT_EQ( 4, target.lineThickness() );
    EXPECT_EQ( "label", target.symbolLabel() );
}

//--------------------------------------------------------------------------------------------------
/// The saved appearance is restored, and not applied after the curve is deleted
//--------------------------------------------------------------------------------------------------
TEST( RimCurveStyleRulesTest, SavedAppearanceRestore )
{
    auto curve = new RimSummaryCurve;
    curve->setColor( cvf::Color3f::RED );

    RimSavedCurveAppearance saved;
    saved.setCurve( curve );

    curve->setColor( cvf::Color3f::BLUE );
    EXPECT_EQ( curve, saved.restore() );
    EXPECT_TRUE( curve->color() == cvf::Color3f::RED );

    delete curve;
    EXPECT_EQ( nullptr, saved.restore() );
}
