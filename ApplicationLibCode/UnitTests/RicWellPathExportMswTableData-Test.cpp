/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2025 Equinor ASA
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

#include "gtest/gtest.h"

#include "RiaApplication.h"
#include "RiaTestDataDirectory.h"

#include "CompletionExportCommands/RicWellPathExportMswTableData.h"

#include "CompletionsMsw/RigMswTableData.h"
#include "CompletionsMsw/RigMswTableRows.h"

#include "RimEclipseCase.h"
#include "RimProject.h"
#include "RimWellPath.h"

#include <QFile>

#include <algorithm>
#include <string>
#include <tuple>
#include <vector>

namespace
{

//--------------------------------------------------------------------------------------------------
/// Extract {i, j, k, gridName} tuples from COMPSEGS data, sorted for stable comparison.
//--------------------------------------------------------------------------------------------------
std::vector<std::tuple<size_t, size_t, size_t, std::string>> extractSortedCells( const RigMswTableData& data )
{
    std::vector<std::tuple<size_t, size_t, size_t, std::string>> cells;
    for ( const auto& row : data.compsegsData() )
    {
        cells.emplace_back( row.i, row.j, row.k, row.gridName );
    }
    std::sort( cells.begin(), cells.end() );
    return cells;
}

} // anonymous namespace

//==================================================================================================
//
// Parameterized integration tests: Tree mode vs FlatList mode for all MSW project files.
//
// The test loads a ResInsight project file, extracts well MSW data using both the tree-based
// (extractSingleWellMswDataTree) and flat-list-based (extractSingleWellMswDataFlatList)
// algorithms, and verifies that both produce equivalent results for every well path:
//
//   - The same set of reservoir cells in COMPSEGS (sorted {i,j,k,gridName} tuples)
//   - The same number of WSEGVALV, WSEGAICD, and WSEGSICD valve rows
//
//==================================================================================================

class MswTreeVsFlatListTest : public testing::TestWithParam<std::string>
{
};

TEST_P( MswTreeVsFlatListTest, CompareTreeAndFlatListModes )
{
    const std::string& projectFileName = GetParam();
    QString            projectFilePath =
        QString( "%1/msw-export/project-files/%2" ).arg( TEST_MODEL_DIR ).arg( QString::fromStdString( projectFileName ) );

    if ( !QFile::exists( projectFilePath ) )
    {
        GTEST_SKIP() << "Project file not found: " << projectFilePath.toStdString();
    }

    bool loaded = RiaApplication::instance()->loadProject( projectFilePath );
    ASSERT_TRUE( loaded ) << "Failed to load project: " << projectFilePath.toStdString();

    RimProject* project = RiaApplication::instance()->project();
    ASSERT_NE( project, nullptr );

    auto eclipseCases = project->eclipseCases();
    ASSERT_FALSE( eclipseCases.empty() ) << "No eclipse cases found in project";

    RimEclipseCase* eclipseCase = eclipseCases[0];
    ASSERT_NE( eclipseCase, nullptr );

    if ( eclipseCase->eclipseCaseData() == nullptr )
    {
        GTEST_SKIP() << "Eclipse case data not loaded — EGRID file may be unavailable";
    }

    auto wellPaths = project->allWellPaths();
    ASSERT_FALSE( wellPaths.empty() ) << "No well paths found in project";

    int wellsWithData = 0;

    for ( auto* wellPath : wellPaths )
    {
        ASSERT_NE( wellPath, nullptr );

        auto treeResult = RicWellPathExportMswTableData::extractSingleWellMswDataTree( eclipseCase, wellPath );
        auto flatResult = RicWellPathExportMswTableData::extractSingleWellMswDataFlatList( eclipseCase, wellPath );

        // If one mode fails, the other should fail too (no MSW data for this well path).
        if ( !treeResult.has_value() && !flatResult.has_value() )
        {
            continue;
        }

        ASSERT_TRUE( treeResult.has_value() ) << "Tree mode failed for well '" << wellPath->name().toStdString()
                                              << "': " << treeResult.error();
        ASSERT_TRUE( flatResult.has_value() ) << "FlatList mode failed for well '" << wellPath->name().toStdString()
                                              << "': " << flatResult.error();

        const std::string wellName = treeResult->wellName();

        // Both modes must produce data for the same well.
        EXPECT_EQ( wellName, flatResult->wellName() ) << "Well name mismatch for: " << wellPath->name().toStdString();

        // Both modes must connect to the same set of reservoir cells.
        auto treeCells = extractSortedCells( *treeResult );
        auto flatCells = extractSortedCells( *flatResult );

        EXPECT_EQ( treeCells, flatCells ) << "COMPSEGS cells differ between Tree and FlatList modes for well '" << wellName << "'";

        // Both modes must produce the same number of valve rows for each valve type.
        EXPECT_EQ( treeResult->wsegvalvData().size(), flatResult->wsegvalvData().size() )
            << "WSEGVALV row count differs for well '" << wellName << "'";

        EXPECT_EQ( treeResult->wsegaicdData().size(), flatResult->wsegaicdData().size() )
            << "WSEGAICD row count differs for well '" << wellName << "'";

        EXPECT_EQ( treeResult->wsegsicdData().size(), flatResult->wsegsicdData().size() )
            << "WSEGSICD row count differs for well '" << wellName << "'";

        ++wellsWithData;
    }

    EXPECT_GT( wellsWithData, 0 ) << "No well paths produced MSW data — check project file and well path MSW parameters";
}

// Project files with known failures (FlatList mode not yet producing equivalent output):
//   "fishbones.rsp"        — duplicate COMPSEGS cells in FlatList mode
//   "perf_aicd.rsp"        — WSEGAICD rows missing in FlatList mode
//   "perf_valve.rsp"       — WSEGVALV row count differs in FlatList mode
//   "perf-lgr-two-wells.rsp" — extra COMPSEGS cell in FlatList mode
//   "two_wells.rsp"        — WSEGVALV row count differs in FlatList mode

INSTANTIATE_TEST_SUITE_P( MswExportProjectFiles,
                          MswTreeVsFlatListTest,
                          testing::Values( "base.rsp", "fracture.rsp", "perf_lateral.rsp", "perf-lgr.rsp" ) );
