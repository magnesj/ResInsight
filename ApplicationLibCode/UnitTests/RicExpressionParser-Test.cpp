#include "gtest/gtest.h"

#include "RimSummaryCalculation.h"

#include "ExpressionParserImpl.h"
#include "expressionparser/ExpressionParser.h"

#include <numeric>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RicExpressionParserTest, BasicUsage )
{
    std::vector<double> a( 10 );
    std::iota( a.begin(), a.end(), 10 );

    std::vector<double> b( 10 );
    std::iota( b.begin(), b.end(), 100 );

    std::vector<double> c( 10 );

    ExpressionParser parser;
    parser.assignVector( "a", a );
    parser.assignVector( "b", b );
    parser.assignVector( "c", c );

    QString expr = "c := a + b";
    EXPECT_TRUE( parser.evaluate( expr ) );

    EXPECT_DOUBLE_EQ( c[0], 110.0 );
    EXPECT_DOUBLE_EQ( c[9], 128.0 );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RicExpressionParserTest, DetectVariables )
{
    QString expr = "c := a + (x / y)";

    std::vector<QString> variables = ExpressionParser::detectReferencedVariables( expr );

    EXPECT_STREQ( variables[0].toStdString().data(), "c" );
    EXPECT_STREQ( variables[1].toStdString().data(), "a" );
    EXPECT_STREQ( variables[2].toStdString().data(), "x" );
    EXPECT_STREQ( variables[3].toStdString().data(), "y" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RicExpressionParserTest, FindLeftHandSide )
{
    {
        QString expr = "c := a";

        QString s = RimSummaryCalculation::findLeftHandSide( expr );

        EXPECT_STREQ( s.toStdString().data(), "c" );
    }

    {
        QString expr = "c:=a";

        QString s = RimSummaryCalculation::findLeftHandSide( expr );

        EXPECT_STREQ( s.toStdString().data(), "c" );
    }

    {
        QString expr = "\na:=b\n\nc:=a";

        QString s = RimSummaryCalculation::findLeftHandSide( expr );

        EXPECT_STREQ( s.toStdString().data(), "c" );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RicExpressionParserTest, ForLoopWithIfStatement )
{
    std::vector<double> a( 10 );
    std::iota( a.begin(), a.end(), 10 );

    std::vector<double> b( 10 );
    std::iota( b.begin(), b.end(), 100 );

    std::vector<double> c( 10 );

    ExpressionParser parser;
    parser.assignVector( "a", a );
    parser.assignVector( "b", b );
    parser.assignVector( "c", c );

    QString expr = "for (var i := 0; i < min(a[],b[],c[]); i += 1)\n"
                   "{                                             \n"
                   "    c[i] := if((a[i] > 13), a[i], b[i]);      \n"
                   "}                                             \n";

    EXPECT_TRUE( parser.evaluate( expr ) );

    EXPECT_DOUBLE_EQ( c[0], 100.0 );
    EXPECT_DOUBLE_EQ( c[9], 19.0 );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RicExpressionParserTest, ExpandedIfStatement )
{
    std::vector<double> a( 10 );
    std::iota( a.begin(), a.end(), 10 );

    std::vector<double> b( 10 );
    std::iota( b.begin(), b.end(), 100 );

    std::vector<double> c( 10 );

    ExpressionParser parser;
    parser.assignVector( "a", a );
    parser.assignVector( "b", b );
    parser.assignVector( "c", c );

    QString expr         = "c := if((a > 13), a, b)";
    auto    expandedText = ExpressionParserImpl::expandIfStatements( expr );

    // std::cout << expandedText.toStdString();

    EXPECT_TRUE( parser.evaluate( expandedText ) );

    EXPECT_DOUBLE_EQ( c[0], 100.0 );
    EXPECT_DOUBLE_EQ( c[9], 19.0 );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RicExpressionParserTest, ExpandIfStatementsAndEvaluate )
{
    std::vector<double> a( 10 );
    std::iota( a.begin(), a.end(), 10 );

    std::vector<double> b( 10 );
    std::iota( b.begin(), b.end(), 100 );

    std::vector<double> c( 10 );

    ExpressionParser parser;
    parser.assignVector( "a", a );
    parser.assignVector( "b", b );
    parser.assignVector( "c", c );

    QString expr = "c := if((a > 13), a, b)";

    EXPECT_TRUE( parser.expandIfStatementsAndEvaluate( expr ) );

    EXPECT_DOUBLE_EQ( c[0], 100.0 );
    EXPECT_DOUBLE_EQ( c[9], 19.0 );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RicExpressionParserTest, ExpandIfWithMinAggregation )
{
    // Test that min(b) in an if-condition is treated as the global minimum of vector b,
    // not as a per-element operation. This was a bug where min(b) was incorrectly expanded
    // to min(b[i]) which just returns b[i] (the identity for a single scalar argument).

    // a = [5, 6, 25], b = [4, 7, 6]
    // min(b) = 4
    // Expected: c[i] = (a[i] > 4) ? a[i] : b[i]
    //   c[0] = (5 > 4) ? 5 : 4  = 5
    //   c[1] = (6 > 4) ? 6 : 7  = 6   <- key: should compare 6 with min(b)=4, not b[1]=7
    //   c[2] = (25 > 4) ? 25 : 6 = 25

    std::vector<double> a = { 5.0, 6.0, 25.0 };
    std::vector<double> b = { 4.0, 7.0, 6.0 };
    std::vector<double> c( 3 );

    ExpressionParser parser;
    parser.assignVector( "a", a );
    parser.assignVector( "b", b );
    parser.assignVector( "c", c );

    QString expr = "c := if(a > min(b), a, b)";

    EXPECT_TRUE( parser.expandIfStatementsAndEvaluate( expr ) );

    EXPECT_DOUBLE_EQ( c[0], 5.0 );
    EXPECT_DOUBLE_EQ( c[1], 6.0 );  // Would be 7.0 with the bug (comparing a[1]=6 with b[1]=7)
    EXPECT_DOUBLE_EQ( c[2], 25.0 );
}
