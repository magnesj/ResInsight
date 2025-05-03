
#include "RiaTestDataDirectory.h"
#include "gtest/gtest.h"

#include "opm/input/eclipse/Deck/Deck.hpp"
#include "opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp"
#include "opm/input/eclipse/Parser/Parser.hpp"
#include "opm/io/eclipse/ERst.hpp"
#include "opm/io/eclipse/RestartFileView.hpp"
#include "opm/io/eclipse/rst/state.hpp"
#include "opm/io/eclipse/rst/well.hpp"

#include "RigEclipseCaseData.h"
#include "RigMainGrid.h"

#include <QDir>
#include <QString>

const std::string drogonPath = "e:/gitroot/resinsight-tutorials/model-data/drogon/DROGON-0.UNRST";

TEST( DISABLED_opm_well_data_test, TestImport )
{
    Opm::Deck deck;

    // It is required to create a deck as the input parameter to runspec. The = default() initialization of the runspec keyword does not
    // initialize the object as expected.
    Opm::Runspec runspec( deck );
    Opm::Parser  parser( false );

    try
    {
        QDir baseFolder( TEST_MODEL_DIR );
        bool subFolderExists = baseFolder.cd( "TEST10K_FLT_LGR_NNC" );
        EXPECT_TRUE( subFolderExists );
        QString filename( "TEST10K_FLT_LGR_NNC.UNRST" );
        QString filePath = baseFolder.absoluteFilePath( filename );

        auto stdFilename = baseFolder.absoluteFilePath( filename ).toStdString();

        auto rstFile = std::make_shared<Opm::EclIO::ERst>( drogonPath );
        for ( auto seqNumber : rstFile->listOfReportStepNumbers() )
        {
            auto fileView = std::make_shared<Opm::EclIO::RestartFileView>( rstFile, seqNumber );

            auto state = Opm::RestartIO::RstState::load( fileView, runspec, parser );

            for ( const auto& w : state.wells )
            {
                auto name = w.name;
                std::cout << name << std::endl;
            }
        }
    }
    catch ( std::exception& e )
    {
        std::cout << "Exception: " << e.what() << std::endl;
    }
}

TEST( OpmGridImportGrdecl, TestImport )
{
    try
    {
        /*
                QDir baseFolder( TEST_MODEL_DIR );
                bool subFolderExists = baseFolder.cd( "TEST10K_FLT_LGR_NNC" );
                EXPECT_TRUE( subFolderExists );
                QString filename( "TEST10K_FLT_LGR_NNC.grdecl" );
                QString filePath = baseFolder.absoluteFilePath( filename );
        */

        // auto stdFilename = baseFolder.absoluteFilePath( filename ).toStdString();

        // std::string stdFilename = "c:/gitroot/ResInsight/TestModels/TEST10K_FLT_LGR_NNC/TEST10K_FLT_LGR_NNC.grdecl";
        std::string stdFilename = "f:/Models/equinor_azure/2024.12.2-grdecl-crash/GRID.GRDECL";

        Opm::Parser parser;
        auto        deck = parser.parseFile( stdFilename );

        for ( size_t i = 0; i < deck.size(); i++ )
        {
            auto& keyword = deck[i];
            std::cout << keyword.name();
            if ( keyword.isDataKeyword() )
            {
                std::cout << " Data size: " << keyword.getDataSize();
            }
            else
            {
                std::cout << keyword;
            }

            std::cout << std::endl;
        }

        return;

        Opm::EclipseGrid opmGrid( deck );
        auto             numActive = opmGrid.getNumActive();

        RigEclipseCaseData eclipseCaseData( nullptr );

        auto mainGrid = eclipseCaseData.mainGrid();
        auto nodes    = mainGrid->nodes();

        auto nx = opmGrid.getNX();
        auto ny = opmGrid.getNY();
        auto nz = opmGrid.getNZ();

        auto cellCount = nx * ny * nz;
        nodes.resize( cellCount * 8 );

        auto cells = mainGrid->reservoirCells();

        RigCell defaultCell;
        defaultCell.setHostGrid( mainGrid );
        cells.resize( cellCount, defaultCell );

        std::array<double, 8> opmX{};
        std::array<double, 8> opmY{};
        std::array<double, 8> opmZ{};

        // NB! taken from RoffFileTools
        // Swap i and j to get correct faces
        const size_t cellMappingECLRi[8] = { 2, 3, 1, 0, 6, 7, 5, 4 };

        for ( int opmCellIndex = 0; opmCellIndex < static_cast<int>( cellCount ); opmCellIndex++ )
        {
            RigCell& cell = mainGrid->cell( opmCellIndex );

            cell.setGridLocalCellIndex( opmCellIndex );
            cell.setParentCellIndex( cvf::UNDEFINED_SIZE_T );

            opmGrid.getCellCorners( opmCellIndex, opmX, opmY, opmZ );

            // Each cell has 8 nodes, use reservoir cell index and multiply to find first node index for cell
            auto riNodeStartIndex = opmCellIndex * 8;

            for ( size_t opmNodeIndex = 0; opmNodeIndex < 8; opmNodeIndex++ )
            {
                size_t riNodeIndex = riNodeStartIndex + cellMappingECLRi[opmNodeIndex];

                // The radial grid is specified with (0,0) as center, add grid center to get correct global coordinates
                auto& riNode = nodes[riNodeIndex];
                riNode.x()   = opmX[opmNodeIndex];
                riNode.y()   = opmY[opmNodeIndex];
                riNode.z()   = -opmZ[opmNodeIndex];

                cell.cornerIndices()[riNodeIndex] = riNodeIndex;
            }
        }

        /*
                for ( size_t i = 0; i < nx; i++ )
                {
                    for ( size_t j = 0; j < ny; j++ )
                    {
                        for ( size_t k = 0; k < nz; k++ )
                        {
                            size_t  reservoirIndex = ;
                            RigCell cell;
                            for ( int cornerIdz = 0; cornerIdz < 6; cornerIdz++ )
                            {
                                auto coord = grid.getCornerPos( i, j, k, cornerIdz );
                                nodes.push_back( cvf::Vec3d( coord[0], coord[1], coord[2] ) );
                            }
                        }
                    }
                }
        */
    }
    catch ( std::exception& e )
    {
        std::cout << "Exception: " << e.what() << std::endl;
    }
}
