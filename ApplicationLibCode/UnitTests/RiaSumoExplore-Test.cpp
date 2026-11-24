#include "gtest/gtest.h"

#include "Cloud/RiaSumoExplore.h"
#include "RiuSumoCaseTableModel.h"

namespace
{
std::vector<SumoCase> testCases()
{
    SumoCase a;
    a.caseId        = SumoCaseId( "a" );
    a.name          = "Alpha";
    a.author        = "jdoe";
    a.status        = "keep";
    a.ensembleNames = { "iter-0", "iter-1" };

    SumoCase b;
    b.caseId = SumoCaseId( "b" );
    b.name   = "Beta";
    b.author = "other";
    b.status = "scratch";

    return { a, b };
}
} // namespace

TEST( RiuSumoCaseTableModelTest, FilterProxy )
{
    RiuSumoCaseTableModel model;
    model.setCases( testCases() );

    RiuSumoCaseFilterProxyModel proxy;
    proxy.setSourceModel( &model );
    EXPECT_EQ( 2, proxy.rowCount() );

    proxy.setColumnFilter( RiuSumoCaseTableModel::CaseName, "alp" );
    EXPECT_EQ( 1, proxy.rowCount() );
    proxy.setColumnFilter( RiuSumoCaseTableModel::CaseName, "" );

    proxy.setOnlyUser( "OTHER" );
    EXPECT_EQ( 1, proxy.rowCount() );
    proxy.setOnlyUser( "" );

    proxy.setOnlyOfficial( true );
    EXPECT_EQ( 1, proxy.rowCount() );
    proxy.setOnlyOfficial( false );

    proxy.setAllowedValues( RiuSumoCaseTableModel::Status, { "keep", "scratch" } );
    EXPECT_EQ( 2, proxy.rowCount() );
    proxy.setAllowedValues( RiuSumoCaseTableModel::Status, { "scratch" } );
    EXPECT_EQ( 1, proxy.rowCount() );
}

TEST( RiuSumoCaseTableModelTest, SelectionDiffAndBadge )
{
    RiuSumoCaseTableModel model;
    model.setCases( testCases() );
    model.setExistingEnsembles( { { "a", "iter-0" } } );

    EXPECT_EQ( QString( "1/2" ), model.data( model.index( 0, RiuSumoCaseTableModel::Selection ) ).toString() );

    model.setEnsembleSelected( "a", "iter-0", false );
    model.setEnsembleSelected( "b", "iter-0", true );

    const auto diff = model.selectionDiff();
    ASSERT_EQ( 1u, diff.toAdd.size() );
    EXPECT_EQ( QString( "b" ), diff.toAdd[0].first );
    ASSERT_EQ( 1u, diff.toRemove.size() );
    EXPECT_EQ( QString( "a" ), diff.toRemove[0].first );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RiaSumoExploreTest, ParseCasesWithMetadata )
{
    const QByteArray body = R"([{"id":"c1","name":"case1","user":"jdoe","status":"keep","description":"a case",
        "timestamp":"2024-05-01T10:00:00Z","model_name":"drogon","model_revision":"2","ensemble_names":["iter-0","iter-1"]}])";

    const auto cases = RiaSumoExplore::parseCases( body );
    ASSERT_EQ( 1u, cases.size() );

    EXPECT_EQ( QString( "c1" ), cases[0].caseId.get() );
    EXPECT_EQ( QString( "case1" ), cases[0].name );
    EXPECT_EQ( QString( "jdoe" ), cases[0].author );
    EXPECT_EQ( QString( "keep" ), cases[0].status );
    EXPECT_EQ( QString( "a case" ), cases[0].description );
    EXPECT_EQ( QString( "2024-05-01T10:00:00Z" ), cases[0].date );
    EXPECT_EQ( QString( "drogon" ), cases[0].modelName );
    EXPECT_EQ( QString( "2" ), cases[0].modelRevision );
    ASSERT_EQ( 2u, cases[0].ensembleNames.size() );
    EXPECT_EQ( QString( "iter-1" ), cases[0].ensembleNames[1] );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
TEST( RiaSumoExploreTest, ParseCasesWithMissingMetadata )
{
    const auto cases = RiaSumoExplore::parseCases( R"([{"id":"c1","name":"case1"}])" );
    ASSERT_EQ( 1u, cases.size() );

    EXPECT_EQ( QString( "case1" ), cases[0].name );
    EXPECT_TRUE( cases[0].author.isEmpty() );
    EXPECT_TRUE( cases[0].modelName.isEmpty() );
    EXPECT_TRUE( cases[0].ensembleNames.empty() );
}
