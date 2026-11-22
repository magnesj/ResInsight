#include "gtest/gtest.h"

#include "RifEclipseSummaryAddress.h"

#include "RimCurveStyleRule.h"
#include "RimCurveStyleRules.h"
#include "RimCurveStyleSettings.h"
#include "RimNamedCurveStyleRule.h"
#include "RimPlotCurveAppearance.h"
#include "RimSavedCurveAppearance.h"
#include "RimSummaryCurve.h"

using D = RimNamedCurveStyleRule::Dimension;

//--------------------------------------------------------------------------------------------------
/// The item rules of the rules assigning color and symbol apply to the matching items only, with the
/// property of the rule they belong to
//--------------------------------------------------------------------------------------------------
TEST( RimCurveStyleRulesTest, ItemRulesFollowTheRuleAssigningTheProperty )
{
    RimNamedCurveStyleRule colorRule;
    colorRule.setDimension( D::WELL );
    colorRule.setAppliesTo( RimNamedCurveStyleRule::AppliesTo::COLOR );
    auto wellRule = colorRule.findOrCreateRule( D::WELL, "A-1" );
    wellRule->setCustom( true );
    colorRule.findOrCreateRule( D::WELL, "B-2" )->setCustom( true );
    EXPECT_EQ( wellRule, colorRule.findOrCreateRule( D::WELL, "A-1" ) );

    RimNamedCurveStyleRule symbolRule;
    symbolRule.setDimension( D::CASE );
    symbolRule.setAppliesTo( RimNamedCurveStyleRule::AppliesTo::SYMBOL );
    auto caseRule = symbolRule.findOrCreateRule( D::CASE, "case" );
    caseRule->setCustom( true );
    symbolRule.findOrCreateRule( D::CASE, "auto case" );

    const auto address = RifEclipseSummaryAddress::wellAddress( "WOPR", "A-1" );

    RimNamedCurveStyleRule::Resolved resolved{ &colorRule, &symbolRule };
    const auto                       matching = RimNamedCurveStyleRule::rulesFor( resolved, address, "case" );
    ASSERT_EQ( 2u, matching.size() );
    EXPECT_EQ( wellRule, matching[0].rule );
    EXPECT_TRUE( matching[0].applyColor && !matching[0].applySymbol );
    EXPECT_EQ( caseRule, matching[1].rule );
    EXPECT_TRUE( matching[1].applySymbol && !matching[1].applyColor );

    // The rule for "auto case" is neither custom nor highlighted, and there is no rule for other cases
    EXPECT_EQ( 1u, RimNamedCurveStyleRule::rulesFor( resolved, address, "auto case" ).size() );
    EXPECT_EQ( 1u, RimNamedCurveStyleRule::rulesFor( resolved, address, "other" ).size() );

    // Rules not assigning the property do not apply
    EXPECT_TRUE( RimNamedCurveStyleRule::rulesFor( {}, address, "case" ).empty() );
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
/// A rule only assigns the property it applies to, for curves with a value for its dimension. Later rules win.
//--------------------------------------------------------------------------------------------------
TEST( RimCurveStyleRulesTest, NamedRulesPrecedence )
{
    RimCurveStyleRules rules;

    auto wellColor = rules.addNamedRule( "Color by well" );
    wellColor->setDimension( D::WELL );
    wellColor->setAppliesTo( RimNamedCurveStyleRule::AppliesTo::COLOR );

    auto vectorColor = rules.addNamedRule( "Color by vector" );
    vectorColor->setDimension( D::VECTOR );
    vectorColor->setAppliesTo( RimNamedCurveStyleRule::AppliesTo::COLOR );

    auto caseSymbol = rules.addNamedRule( "Symbol by case" );
    caseSymbol->setDimension( D::CASE );
    caseSymbol->setAppliesTo( RimNamedCurveStyleRule::AppliesTo::SYMBOL );

    const auto wellAddress  = RifEclipseSummaryAddress::wellAddress( "WOPR", "A-1" );
    const auto fieldAddress = RifEclipseSummaryAddress::fieldAddress( "FOPT" );

    EXPECT_TRUE( wellColor->matches( wellAddress, "case" ) );
    EXPECT_FALSE( wellColor->matches( fieldAddress, "case" ) );

    auto resolved = RimCurveStyleRules::resolve( { wellColor, caseSymbol }, wellAddress, "case" );
    EXPECT_EQ( wellColor, resolved.color );
    EXPECT_EQ( caseSymbol, resolved.symbol );

    // No value for the well dimension, but the other rule applies
    resolved = RimCurveStyleRules::resolve( { wellColor, vectorColor }, fieldAddress, "case" );
    EXPECT_EQ( vectorColor, resolved.color );
    EXPECT_EQ( nullptr, resolved.symbol );

    EXPECT_EQ( vectorColor, RimCurveStyleRules::resolve( { wellColor, vectorColor }, wellAddress, "case" ).color );
    EXPECT_EQ( wellColor, RimCurveStyleRules::resolve( { vectorColor, wellColor }, wellAddress, "case" ).color );

    resolved = RimCurveStyleRules::resolve( {}, wellAddress, "case" );
    EXPECT_EQ( nullptr, resolved.color );
    EXPECT_EQ( nullptr, resolved.symbol );
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
/// Only the groups the global appearance controlled are restored, local edits of the others are kept
//--------------------------------------------------------------------------------------------------
TEST( RimCurveStyleRulesTest, SavedAppearanceRestore )
{
    auto curve = new RimSummaryCurve;
    curve->setColor( cvf::Color3f::RED );
    curve->setSymbol( RiuPlotCurveSymbol::SYMBOL_NONE );
    curve->curveAppearance()->setLineThickness( 1 );

    RimSavedCurveAppearance saved;
    saved.setCurve( curve );
    EXPECT_FALSE( saved.isControlled() );

    // The color and the other settings are taken over by the global appearance, the symbol is edited locally
    saved.update( true, false, true );
    EXPECT_TRUE( saved.isControlled() );
    curve->setColor( cvf::Color3f::BLUE );
    curve->curveAppearance()->setLineThickness( 5 );
    curve->setSymbol( RiuPlotCurveSymbol::SYMBOL_DIAMOND );

    // Still controlled, values left by rules that no longer apply are removed
    saved.update( true, false, true );
    EXPECT_TRUE( curve->color() == cvf::Color3f::RED );
    EXPECT_EQ( 1, curve->curveAppearance()->lineThickness() );
    EXPECT_EQ( RiuPlotCurveSymbol::SYMBOL_DIAMOND, curve->symbol() );

    // Only the color is controlled after this, the line thickness stays as it is
    curve->setColor( cvf::Color3f::BLUE );
    curve->curveAppearance()->setLineThickness( 5 );
    saved.update( true, false, false );
    EXPECT_TRUE( curve->color() == cvf::Color3f::RED );
    EXPECT_EQ( 1, curve->curveAppearance()->lineThickness() );

    curve->setColor( cvf::Color3f::BLUE );
    saved.update( false, false, false );
    EXPECT_FALSE( saved.isControlled() );
    EXPECT_TRUE( curve->color() == cvf::Color3f::RED );
    EXPECT_EQ( RiuPlotCurveSymbol::SYMBOL_DIAMOND, curve->symbol() );

    delete curve;
    EXPECT_EQ( nullptr, saved.object() );
}
