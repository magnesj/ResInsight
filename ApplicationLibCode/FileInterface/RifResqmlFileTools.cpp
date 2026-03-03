/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2024-     Equinor ASA
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

#include "RifResqmlFileTools.h"

#include "RiaDefines.h"
#include "RiaLogging.h"

#include "RigActiveCellInfo.h"
#include "RigCaseCellResultsData.h"
#include "RigEclipseCaseData.h"
#include "RigEclipseResultAddress.h"
#include "RigMainGrid.h"

#ifdef RESINSIGHT_HAVE_RESQML
// fesapi includes for reading RESQML EPC files
// These become available when RESINSIGHT_ENABLE_RESQML=ON and fesapi is found
#include "common/EpcDocument.h"
#include "resqml2/AbstractIjkGridRepresentation.h"
#include "resqml2/AbstractValuesProperty.h"
#include "resqml2/ContinuousProperty.h"
#include "resqml2/DiscreteProperty.h"
#endif

#include <QFileInfo>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RifResqmlFileTools::openGridFile( const QString& fileName, RigEclipseCaseData* eclipseCase, QString* errorMessages )
{
#ifdef RESINSIGHT_HAVE_RESQML
    try
    {
        COMMON_NS::EpcDocument epcDoc( fileName.toStdString() );
        const std::string      deserializationMessages = epcDoc.deserializeInto( *epcDoc.createHdf5File() );
        if ( !deserializationMessages.empty() )
        {
            RiaLogging::warning( QString( "RESQML deserialization warnings for %1: %2" )
                                     .arg( fileName )
                                     .arg( QString::fromStdString( deserializationMessages ) ) );
        }

        // Find the first IJK grid representation in the package
        auto* dataObjectRepo = epcDoc.getDataObjectRepository();
        auto  ijkGrids       = dataObjectRepo->getIjkGridRepresentationSet();

        if ( ijkGrids.empty() )
        {
            if ( errorMessages ) *errorMessages = "No IJK grid found in RESQML file: " + fileName;
            return false;
        }

        // Use the first IJK grid
        auto* ijkGrid = ijkGrids[0];

        const uint64_t ni = ijkGrid->getICellCount();
        const uint64_t nj = ijkGrid->getJCellCount();
        const uint64_t nk = ijkGrid->getKCellCount();

        RigMainGrid* mainGrid = eclipseCase->mainGrid();
        mainGrid->setCellCounts( cvf::Vec3st( ni, nj, nk ) );
        mainGrid->setGridName( "Main grid" );

        const uint64_t cellCount = ni * nj * nk;

        RigActiveCellInfo* activeCellInfo          = eclipseCase->activeCellInfo( RiaDefines::PorosityModelType::MATRIX_MODEL );
        RigActiveCellInfo* fractureActiveCellInfo  = eclipseCase->activeCellInfo( RiaDefines::PorosityModelType::FRACTURE_MODEL );

        activeCellInfo->setGridCount( 1 );
        fractureActiveCellInfo->setGridCount( 1 );
        activeCellInfo->setReservoirCellCount( cellCount );
        fractureActiveCellInfo->setReservoirCellCount( cellCount );

        // Reserve room for cells and nodes (8 corner nodes per cell in corner-point layout)
        mainGrid->reservoirCells().reserve( cellCount );
        mainGrid->nodes().reserve( 8 * cellCount );

        RigCell defaultCell;
        defaultCell.setHostGrid( mainGrid );
        mainGrid->reservoirCells().resize( cellCount, defaultCell );
        mainGrid->nodes().resize( 8 * cellCount, cvf::Vec3d( 0, 0, 0 ) );

        // Read corner-point geometry: 8 corners per cell, XYZ per corner
        // RESQML getXyzPointsOfAllBlocksOfPatch returns: 8 * cellCount * 3 doubles
        std::vector<double> xyzPoints( cellCount * 8 * 3 );
        ijkGrid->getXyzPointsOfAllBlocksOfPatch( 0, xyzPoints.data() );

        // Map RESQML corner-point nodes into ResInsight cells
        // RESQML uses depth-positive-down; negate Z for ResInsight right-handed coordinates
        for ( uint64_t cellIdx = 0; cellIdx < cellCount; ++cellIdx )
        {
            RigCell& cell = mainGrid->cell( cellIdx );
            cell.setGridLocalCellIndex( cellIdx );
            cell.setParentCellIndex( cvf::UNDEFINED_SIZE_T );

            for ( int corner = 0; corner < 8; ++corner )
            {
                const uint64_t base                 = ( cellIdx * 8 + corner ) * 3;
                mainGrid->nodes()[cellIdx * 8 + corner] = cvf::Vec3d( xyzPoints[base + 0], xyzPoints[base + 1], -xyzPoints[base + 2] );
                cell.cornerIndices()[corner]             = cellIdx * 8 + corner;
            }

            // Default: all cells active
            activeCellInfo->setCellResultIndex( cellIdx, cellIdx );
        }

        activeCellInfo->computeDerivedData();
        fractureActiveCellInfo->computeDerivedData();

        return true;
    }
    catch ( const std::exception& e )
    {
        if ( errorMessages ) *errorMessages = QString( "Exception reading RESQML file %1: %2" ).arg( fileName ).arg( e.what() );
        return false;
    }
#else
    if ( errorMessages )
    {
        *errorMessages = "RESQML support is not enabled. Rebuild ResInsight with RESINSIGHT_ENABLE_RESQML=ON.";
    }
    return false;
#endif
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::pair<bool, std::map<QString, QString>> RifResqmlFileTools::createInputProperties( const QString& fileName,
                                                                                        RigEclipseCaseData* eclipseCase )
{
    std::map<QString, QString> keywordMapping;

#ifdef RESINSIGHT_HAVE_RESQML
    try
    {
        COMMON_NS::EpcDocument epcDoc( fileName.toStdString() );
        epcDoc.deserializeInto( *epcDoc.createHdf5File() );

        auto* dataObjectRepo = epcDoc.getDataObjectRepository();
        auto  ijkGrids       = dataObjectRepo->getIjkGridRepresentationSet();
        if ( ijkGrids.empty() ) return { false, keywordMapping };

        auto*          ijkGrid  = ijkGrids[0];
        const uint64_t cellCount = ijkGrid->getICellCount() * ijkGrid->getJCellCount() * ijkGrid->getKCellCount();

        // Read all continuous (floating-point) properties
        for ( auto* prop : dataObjectRepo->getDataObjects<RESQML2_NS::ContinuousProperty>() )
        {
            const QString propName = QString::fromStdString( prop->getTitle() );

            std::vector<double> values( cellCount );
            prop->getDoubleValuesOfPatch( 0, values.data() );

            RigEclipseResultAddress resAddr( RiaDefines::ResultCatType::INPUT_PROPERTY, RiaDefines::ResultDataType::FLOAT, propName );
            eclipseCase->results( RiaDefines::PorosityModelType::MATRIX_MODEL )->createResultEntry( resAddr, false );

            auto* newPropertyData = eclipseCase->results( RiaDefines::PorosityModelType::MATRIX_MODEL )->modifiableCellScalarResultTimesteps( resAddr );
            newPropertyData->push_back( std::move( values ) );

            keywordMapping[propName] = propName;
        }

        // Read all discrete (integer) properties
        for ( auto* prop : dataObjectRepo->getDataObjects<RESQML2_NS::DiscreteProperty>() )
        {
            const QString propName = QString::fromStdString( prop->getTitle() );

            std::vector<int> intValues( cellCount );
            prop->getIntValuesOfPatch( 0, intValues.data() );

            std::vector<double> values( intValues.begin(), intValues.end() );

            RigEclipseResultAddress resAddr( RiaDefines::ResultCatType::INPUT_PROPERTY, RiaDefines::ResultDataType::INTEGER, propName );
            eclipseCase->results( RiaDefines::PorosityModelType::MATRIX_MODEL )->createResultEntry( resAddr, false );

            auto* newPropertyData = eclipseCase->results( RiaDefines::PorosityModelType::MATRIX_MODEL )->modifiableCellScalarResultTimesteps( resAddr );
            newPropertyData->push_back( std::move( values ) );

            keywordMapping[propName] = propName;
        }

        return { true, keywordMapping };
    }
    catch ( const std::exception& e )
    {
        RiaLogging::error( QString( "Exception reading RESQML properties from %1: %2" ).arg( fileName ).arg( e.what() ) );
        return { false, keywordMapping };
    }
#else
    return { false, keywordMapping };
#endif
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RifResqmlFileTools::hasGridData( const QString& fileName )
{
#ifdef RESINSIGHT_HAVE_RESQML
    try
    {
        COMMON_NS::EpcDocument epcDoc( fileName.toStdString() );
        epcDoc.deserializeInto( *epcDoc.createHdf5File() );
        auto* dataObjectRepo = epcDoc.getDataObjectRepository();
        return !dataObjectRepo->getIjkGridRepresentationSet().empty();
    }
    catch ( ... )
    {
        return false;
    }
#else
    return false;
#endif
}
