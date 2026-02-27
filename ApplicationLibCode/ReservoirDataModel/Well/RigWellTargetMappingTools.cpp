/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2024-  Equinor ASA
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

#include "RigWellTargetMappingTools.h"

#include "RiaDefines.h"
#include "RiaResultNames.h"

#include "RigCaseCellResultsData.h"
#include "RigMainGrid.h"
#include "RigNNCData.h"
#include "RigNncConnection.h"

#include "RimEclipseCase.h"

#include "cafAssert.h"
#include "cvfMath.h"
#include "cvfStructGrid.h"

#include <algorithm>
#include <limits>
#include <set>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RigWellTargetMappingTools::getValueForFace( const std::vector<double>& x,
                                                   const std::vector<double>& y,
                                                   const std::vector<double>& z,
                                                   CellFaceType               face,
                                                   size_t                     resultIndex )
{
    if ( face == cvf::StructGridInterface::FaceType::POS_I || face == cvf::StructGridInterface::FaceType::NEG_I ) return x[resultIndex];
    if ( face == cvf::StructGridInterface::FaceType::POS_J || face == cvf::StructGridInterface::FaceType::NEG_J ) return y[resultIndex];
    if ( face == cvf::StructGridInterface::FaceType::POS_K || face == cvf::StructGridInterface::FaceType::NEG_K ) return z[resultIndex];
    return std::numeric_limits<double>::infinity();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RigWellTargetMappingTools::getTransmissibilityValueForFace( const std::vector<double>& x,
                                                                   const std::vector<double>& y,
                                                                   const std::vector<double>& z,
                                                                   CellFaceType               face,
                                                                   size_t                     resultIndex,
                                                                   size_t                     neighborResultIndex )
{
    // For negative directions (NEG_I, NEG_J, NEG_K) use the value from the neighbor cell
    bool isPos = face == cvf::StructGridInterface::FaceType::POS_I || face == cvf::StructGridInterface::FaceType::POS_J ||
                 face == cvf::StructGridInterface::FaceType::POS_K;
    size_t index = isPos ? resultIndex : neighborResultIndex;
    return getValueForFace( x, y, z, face, index );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RigWellTargetMappingTools::getOilVectorName( VolumesType volumesType )
{
    switch ( volumesType )
    {
        case VolumesType::RESERVOIR_VOLUMES_COMPUTED:
            return RiaResultNames::riPorvSoil();
        case VolumesType::RESERVOIR_VOLUMES:
            return "RFIPOIL";
        case VolumesType::SURFACE_VOLUMES_SFIP:
            return "SFIPOIL";
        case VolumesType::SURFACE_VOLUMES_FIP:
            return "FIPOIL";
        default:
        {
            CAF_ASSERT( false );
            return "";
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RigWellTargetMappingTools::getGasVectorName( VolumesType volumesType )
{
    switch ( volumesType )
    {
        case VolumesType::RESERVOIR_VOLUMES_COMPUTED:
            return RiaResultNames::riPorvSgas();
        case VolumesType::RESERVOIR_VOLUMES:
            return "RFIPGAS";
        case VolumesType::SURFACE_VOLUMES_SFIP:
            return "SFIPGAS";
        case VolumesType::SURFACE_VOLUMES_FIP:
            return "FIPGAS";
        default:
        {
            CAF_ASSERT( false );
            return "";
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RigWellTargetMappingTools::isSaturationSufficient( VolumeType                                volumeType,
                                                        const RigWellTargetMapping::DataContainer& data,
                                                        const RigWellTargetMapping::ClusteringLimits& limits,
                                                        size_t                                    idx )
{
    bool needsValidOil = volumeType == VolumeType::OIL || volumeType == VolumeType::HYDROCARBON;
    bool needsValidGas = volumeType == VolumeType::GAS || volumeType == VolumeType::HYDROCARBON;
    // For hydrocarbon it is enough that one of the saturations is above the limit.
    if ( needsValidOil && data.saturationOil[idx] >= limits.saturationOil ) return true;
    if ( needsValidGas && data.saturationGas[idx] >= limits.saturationGas ) return true;
    return false;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigWellTargetMappingTools::assignClusterIdToCells( const RigActiveCellInfo&   activeCellInfo,
                                                        const std::vector<size_t>& cells,
                                                        std::vector<int>&          clusters,
                                                        int                        clusterId )
{
    for ( size_t reservoirCellIdx : cells )
    {
        size_t resultIndex = activeCellInfo.cellResultIndex( reservoirCellIdx );
        if ( resultIndex != cvf::UNDEFINED_SIZE_T ) clusters[resultIndex] = clusterId;
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::list<std::pair<std::pair<size_t, RigWellTargetMappingTools::CellFaceType>, size_t>>
    RigWellTargetMappingTools::nncConnectionCellAndResult( size_t cellIdx, RigMainGrid* mainGrid )
{
    std::list<std::pair<std::pair<size_t, CellFaceType>, size_t>> foundCells;

    if ( mainGrid->nncData() == nullptr ) return foundCells;

    auto& connections = mainGrid->nncData()->allConnections();
    for ( size_t i = 0; i < connections.size(); i++ )
    {
        if ( connections[i].c1GlobIdx() == cellIdx )
        {
            foundCells.push_back( std::make_pair( std::make_pair( connections[i].c2GlobIdx(), connections[i].face() ), i ) );
        }
    }

    return foundCells;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigWellTargetMappingTools::createDynamicResultEntry( RigCaseCellResultsData* resultsData, const RigEclipseResultAddress& address )
{
    if ( !resultsData->hasResultEntry( address ) )
    {
        resultsData->createResultEntry( address, false );

        RigEclipseResultAddress addrToMaxTimeStepCountResult;
        resultsData->maxTimeStepCount( &addrToMaxTimeStepCountResult );
        const std::vector<RigEclipseTimeStepInfo> timeStepInfos = resultsData->timeStepInfos( addrToMaxTimeStepCountResult );
        resultsData->setTimeStepInfos( address, timeStepInfos );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigWellTargetMappingTools::createResultVector( RimEclipseCase&         eclipseCase,
                                                    const QString&          resultName,
                                                    const std::vector<int>& clusterIds,
                                                    size_t                  timeStepIdx )
{
    RigEclipseResultAddress resultAddress( RiaDefines::ResultCatType::GENERATED, RiaDefines::ResultDataType::INTEGER, resultName );

    auto resultsData = eclipseCase.results( RiaDefines::PorosityModelType::MATRIX_MODEL );

    createDynamicResultEntry( resultsData, resultAddress );

    std::vector<double>* resultVector = resultsData->modifiableCellScalarResult( resultAddress, timeStepIdx );
    resultVector->resize( clusterIds.size(), std::numeric_limits<double>::infinity() );

    std::fill( resultVector->begin(), resultVector->end(), std::numeric_limits<double>::infinity() );

    for ( size_t idx = 0; idx < clusterIds.size(); idx++ )
    {
        if ( clusterIds[idx] > 0 )
        {
            resultVector->at( idx ) = 1.0 * clusterIds[idx];
        }
    }

    std::set<int> uniqueClusterIds( clusterIds.begin(), clusterIds.end() );
    uniqueClusterIds.erase( 0 ); // Remove unassigned cluster id
    resultsData->recalculateStatistics( resultAddress, uniqueClusterIds );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigWellTargetMappingTools::createResultVector( RimEclipseCase&            eclipseCase,
                                                    const QString&             resultName,
                                                    const std::vector<double>& values,
                                                    size_t                     timeStepIdx )
{
    RigEclipseResultAddress resultAddress( RiaDefines::ResultCatType::GENERATED, resultName );

    auto resultsData = eclipseCase.results( RiaDefines::PorosityModelType::MATRIX_MODEL );

    createDynamicResultEntry( resultsData, resultAddress );

    auto resultVector = resultsData->modifiableCellScalarResult( resultAddress, timeStepIdx );
    resultVector->resize( values.size(), std::numeric_limits<double>::infinity() );
    std::copy( values.begin(), values.end(), resultVector->begin() );

    resultsData->recalculateStatistics( resultAddress );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigWellTargetMappingTools::createStaticResultVector( RimEclipseCase& eclipseCase, const QString& resultName, const std::vector<int>& intValues )
{
    RigEclipseResultAddress resultAddress( RiaDefines::ResultCatType::GENERATED, resultName );

    auto resultsData = eclipseCase.results( RiaDefines::PorosityModelType::MATRIX_MODEL );

    resultsData->addStaticScalarResult( RiaDefines::ResultCatType::GENERATED, resultName, false, intValues.size() );

    std::vector<double>* resultVector = resultsData->modifiableCellScalarResult( resultAddress, 0 );
    resultVector->resize( intValues.size(), std::numeric_limits<double>::infinity() );

    std::fill( resultVector->begin(), resultVector->end(), std::numeric_limits<double>::infinity() );

    for ( size_t idx = 0; idx < intValues.size(); idx++ )
    {
        if ( intValues[idx] > 0 )
        {
            resultVector->at( idx ) = intValues[idx];
        }
    }

    resultsData->recalculateStatistics( resultAddress );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigWellTargetMappingTools::createStaticResultVector( RimEclipseCase& eclipseCase, const QString& resultName, const std::vector<double>& values )
{
    RigEclipseResultAddress resultAddress( RiaDefines::ResultCatType::GENERATED, resultName );

    auto resultsData = eclipseCase.results( RiaDefines::PorosityModelType::MATRIX_MODEL );

    resultsData->addStaticScalarResult( RiaDefines::ResultCatType::GENERATED, resultName, false, values.size() );

    std::vector<double>* resultVector = resultsData->modifiableCellScalarResult( resultAddress, 0 );
    resultVector->resize( values.size(), std::numeric_limits<double>::infinity() );

    std::copy( values.begin(), values.end(), resultVector->begin() );

    resultsData->recalculateStatistics( resultAddress );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigWellTargetMappingTools::createResultVectorIfDefined( RimEclipseCase&            eclipseCase,
                                                             const QString&             resultName,
                                                             const std::vector<double>& values,
                                                             int                        timeStepIdx )
{
    // Avoid creating the result vector if all values are inf/nan
    if ( std::all_of( values.begin(), values.end(), []( auto v ) { return std::isinf( v ) || std::isnan( v ); } ) ) return;

    if ( timeStepIdx < 0 )
    {
        createStaticResultVector( eclipseCase, resultName, values );
    }
    else
    {
        createResultVector( eclipseCase, resultName, values, timeStepIdx );
    }
}
