/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2026-     Equinor ASA
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

#include "RigEclipseCaseData.h"
#include "RigFault.h"
#include "RigMainGrid.h"
#include "RigNNCData.h"
#include "RigNncConnection.h"
#include "RimEclipseResultCase.h"

#include "cvfCollection.h"
#include "cvfStructGrid.h"

#include <QDebug>
#include <QFile>

#include <memory>

//--------------------------------------------------------------------------------------------------
/// Import the DROGON_HIST_NESTED hybrid-grid case and verify that the in-product fault detection
/// (which is augmented with NNC connections inside RigMainGrid::calculateFaults) maps every NNC
/// connection with a resolved face direction to an existing fault face.
///
/// Test is DISABLED by default because it depends on a local model file that is not part of the
/// shared TestModels directory. Enable manually via --gtest_filter when the file is available.
//--------------------------------------------------------------------------------------------------
TEST( RigDrogonHistNestedFaultTest, DISABLED_AddFaultFacesFromNNCs )
{
    const QString filePath( "f:/Models/equinor_azure/hybrid grid/DROGON_HIST_NESTED.EGRID" );
    ASSERT_TRUE( QFile::exists( filePath ) ) << "Test model not found: " << filePath.toStdString();

    std::unique_ptr<RimEclipseResultCase> resultCase( new RimEclipseResultCase );
    resultCase->setGridFileName( filePath );
    ASSERT_TRUE( resultCase->importGridAndResultMetaData( false ) );

    RigEclipseCaseData* caseData = resultCase->eclipseCaseData();
    ASSERT_TRUE( caseData != nullptr );

    RigMainGrid* mainGrid = caseData->mainGrid();
    ASSERT_TRUE( mainGrid != nullptr );

    // Importing the case triggers fault detection. Verify that at least one fault face was
    // detected across all faults (named or auto-generated). The auto-generated faults are also
    // augmented with NNC-derived faces inside RigMainGrid::calculateFaults.
    const size_t faultCount = mainGrid->faults().size();
    EXPECT_GT( faultCount, 0u );

    size_t totalFaultFaces = 0;
    for ( size_t i = 0; i < faultCount; ++i )
    {
        const RigFault* fault = mainGrid->faults().at( i );
        totalFaultFaces += fault->faultFaces().size();
        qDebug() << "Fault" << static_cast<int>( i ) << ":" << fault->name() << "-" << static_cast<int>( fault->faultFaces().size() )
                 << "faces";
    }
    EXPECT_GT( totalFaultFaces, 0u );

    // Make sure NNC data is computed and available
    ASSERT_TRUE( resultCase->ensureNncDataIsComputed() );

    RigNNCData* nncData = mainGrid->nncData();
    ASSERT_TRUE( nncData != nullptr );

    const RigConnectionContainer& nncs = nncData->allConnections();
    qDebug() << "Total NNC connections:" << static_cast<int>( nncs.size() );

    // Every NNC connection with a resolved face direction should now map to an existing fault
    // face, because calculateFaults adds any missing NNC face to the unnamed fault collections.
    size_t nncWithFace             = 0;
    size_t nncWithoutFace          = 0;
    size_t nncWithoutExistingFault = 0;
    for ( size_t i = 0; i < nncs.size(); ++i )
    {
        const RigConnection& conn = nncs[i];
        if ( conn.face() == cvf::StructGridInterface::NO_FACE )
        {
            ++nncWithoutFace;
            continue;
        }

        ++nncWithFace;

        const RigFault* existingFault = mainGrid->findFaultFromCellIndexAndCellFace( conn.c1GlobIdx(), conn.face() );
        if ( !existingFault )
        {
            ++nncWithoutExistingFault;
        }
    }

    qDebug() << "NNCs with resolved face direction:" << static_cast<int>( nncWithFace );
    qDebug() << "NNCs without resolved face direction:" << static_cast<int>( nncWithoutFace );
    qDebug() << "NNCs that did not map to an existing fault face:" << static_cast<int>( nncWithoutExistingFault );

    EXPECT_GT( nncWithFace, 0u );

    // calculateFaults covers eclipse-read NNCs; distributeNNCsToFaults covers NNCs synthesized
    // later by computeAdditionalNncs. Together they should leave no NNC with a resolved face
    // direction unmapped to a fault.
    EXPECT_EQ( nncWithoutExistingFault, 0u );
}
