/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2023-     Equinor ASA
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

#include "RigSoilResultCalculator.h"
#include "RigCaseCellResultsData.h"
#include "RigEclipseResultInfo.h"
#include "RigEclipseCaseData.h"
#include "RigActiveCellInfo.h"

#include "RiaResultNames.h"
#include "RiaDefines.h"

//==================================================================================================
///
//==================================================================================================
RigSoilResultCalculator::RigSoilResultCalculator( RigCaseCellResultsData& resultsData )
    : RigEclipseResultCalculator( resultsData )
{
}

//==================================================================================================
///
//==================================================================================================
RigSoilResultCalculator::~RigSoilResultCalculator()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RigSoilResultCalculator::isMatching( const RigEclipseResultAddress& resVarAddr ) const
{
    return resVarAddr.resultName() == RiaResultNames::soil() && resVarAddr.resultCatType() == RiaDefines::ResultCatType::DYNAMIC_NATIVE;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigSoilResultCalculator::calculate( const RigEclipseResultAddress& resVarAddr, size_t timeStepIndex )
{
    // See similar function in RifReaderOpmRft::values, but the current implementation is not suitable for merging
    // Compute SGAS based on SWAT if the simulation contains no oil
    testAndComputeSgasForTimeStep( timeStepIndex );

    // Check available phases and create SWAT if only water phase is present and SWAT is missing
    checkAndCreateSwatForWaterOnlySimulation( timeStepIndex );

    // Create result addresses for saturation components
    const RigEclipseResultAddress swatAddr( RiaDefines::ResultCatType::DYNAMIC_NATIVE, RiaResultNames::swat() );
    const RigEclipseResultAddress sgasAddr( RiaDefines::ResultCatType::DYNAMIC_NATIVE, RiaResultNames::sgas() );
    const RigEclipseResultAddress ssolAddr( RiaDefines::ResultCatType::DYNAMIC_NATIVE, "SSOL" );

    // Load scalar indices for each saturation component
    const size_t swatScalarIndex = m_resultsData->findOrLoadKnownScalarResultForTimeStep( swatAddr, timeStepIndex );
    const size_t sgasScalarIndex = m_resultsData->findOrLoadKnownScalarResultForTimeStep( sgasAddr, timeStepIndex );
    const size_t ssolScalarIndex = m_resultsData->findOrLoadKnownScalarResultForTimeStep( ssolAddr, timeStepIndex );

    // Early exit if none of SWAT or SGAS is present
    if ( swatScalarIndex == cvf::UNDEFINED_SIZE_T && sgasScalarIndex == cvf::UNDEFINED_SIZE_T )
    {
        return;
    }

    // Determine result dimensions from available data
    size_t cellCount      = 0;
    size_t totalTimeSteps = 0;

    if ( swatScalarIndex != cvf::UNDEFINED_SIZE_T )
    {
        const std::vector<double>& swatData = m_resultsData->cellScalarResults( swatAddr, timeStepIndex );
        if ( !swatData.empty() )
        {
            cellCount      = swatData.size();
            totalTimeSteps = m_resultsData->infoForEachResultIndex()[swatScalarIndex].timeStepInfos().size();
        }
    }

    if ( sgasScalarIndex != cvf::UNDEFINED_SIZE_T )
    {
        const std::vector<double>& sgasData = m_resultsData->cellScalarResults( sgasAddr, timeStepIndex );
        if ( !sgasData.empty() )
        {
            cellCount = qMax( cellCount, sgasData.size() );

            const size_t sgasTimeSteps = m_resultsData->infoForEachResultIndex()[sgasScalarIndex].timeStepInfos().size();
            totalTimeSteps             = qMax( totalTimeSteps, sgasTimeSteps );
        }
    }

    // Allocate memory for SOIL results
    const size_t soilScalarIndex = m_resultsData->findScalarResultIndexFromAddress( resVarAddr );
    m_resultsData->m_cellScalarResults[soilScalarIndex].resize( totalTimeSteps );

    if ( !m_resultsData->cellScalarResults( resVarAddr, timeStepIndex ).empty() )
    {
        // Data is already computed and allocated
        return;
    }

    m_resultsData->m_cellScalarResults[soilScalarIndex][timeStepIndex].resize( cellCount );

    // Get data vectors for calculation
    const std::vector<double>& swatData = getSaturationDataForCalculation( swatAddr, swatScalarIndex, timeStepIndex );
    const std::vector<double>& sgasData = getSaturationDataForCalculation( sgasAddr, sgasScalarIndex, timeStepIndex );
    const std::vector<double>& ssolData = getSaturationDataForCalculation( ssolAddr, ssolScalarIndex, timeStepIndex );

    std::vector<double>* soilData = m_resultsData->modifiableCellScalarResult( resVarAddr, timeStepIndex );

    // Calculate SOIL using material balance equation: SOIL = 1.0 - SWAT - SGAS - SSOL
    // This ensures that all fluid saturations sum to 1.0 in each cell
#pragma omp parallel for
    for ( int cellIdx = 0; cellIdx < static_cast<int>( cellCount ); cellIdx++ )
    {
        double oilSaturation = 1.0; // Start with total pore space

        // Subtract other phase saturations
        if ( !sgasData.empty() )
        {
            oilSaturation -= sgasData[cellIdx];
        }

        if ( !swatData.empty() )
        {
            oilSaturation -= swatData[cellIdx];
        }

        if ( !ssolData.empty() )
        {
            oilSaturation -= ssolData[cellIdx];
        }

        soilData->at( cellIdx ) = oilSaturation;
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const std::vector<double>& RigSoilResultCalculator::getSaturationDataForCalculation( const RigEclipseResultAddress& address,
                                                                                     size_t                         scalarIndex,
                                                                                     size_t                         timeStepIndex ) const
{
    static const std::vector<double> emptyVector;

    if ( scalarIndex == cvf::UNDEFINED_SIZE_T )
    {
        return emptyVector;
    }

    const std::vector<double>& data = m_resultsData->cellScalarResults( address, timeStepIndex );
    return data.empty() ? emptyVector : data;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigSoilResultCalculator::checkAndCreateSwatForWaterOnlySimulation( size_t timeStepIndex )
{
    // Get available phases from the case data
    const std::set<RiaDefines::PhaseType> availablePhases = m_resultsData->m_ownerCaseData->availablePhases();
    
    // Check if only water phase is present
    if ( availablePhases.size() == 1 && availablePhases.count( RiaDefines::PhaseType::WATER_PHASE ) == 1 )
    {
        // Check if SWAT is already present
        const RigEclipseResultAddress swatAddr( RiaDefines::ResultCatType::DYNAMIC_NATIVE, RiaResultNames::swat() );
        const size_t swatScalarIndex = m_resultsData->findOrLoadKnownScalarResultForTimeStep( swatAddr, timeStepIndex );
        
        if ( swatScalarIndex == cvf::UNDEFINED_SIZE_T )
        {
            // SWAT is not present, create it with 1.0 for all cells and all time steps
            createSwatWithFullSaturation();
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigSoilResultCalculator::createSwatWithFullSaturation()
{
    const RigEclipseResultAddress swatAddr( RiaDefines::ResultCatType::DYNAMIC_NATIVE, RiaResultNames::swat() );
    
    // Create or find the scalar result index for SWAT
    const size_t swatScalarIndex = m_resultsData->findOrCreateScalarResultIndex( swatAddr, false );
    
    // Get the number of time steps and cells from existing data
    size_t timeStepCount = 0;
    size_t cellCount = 0;
    
    // Find an existing result to determine dimensions
    if ( !m_resultsData->m_cellScalarResults.empty() && !m_resultsData->m_cellScalarResults[0].empty() )
    {
        timeStepCount = m_resultsData->m_cellScalarResults[0].size();
        if ( timeStepCount > 0 )
        {
            cellCount = m_resultsData->m_cellScalarResults[0][0].size();
        }
    }
    
    // If we still don't have dimensions, get from active cell count
    if ( cellCount == 0 && m_resultsData->m_activeCellInfo )
    {
        cellCount = m_resultsData->m_activeCellInfo->reservoirCellCount();
    }
    
    // Create SWAT data with 1.0 saturation for all time steps
    if ( timeStepCount > 0 && cellCount > 0 )
    {
        m_resultsData->m_cellScalarResults[swatScalarIndex].resize( timeStepCount );
        
        for ( size_t timeStep = 0; timeStep < timeStepCount; ++timeStep )
        {
            m_resultsData->m_cellScalarResults[swatScalarIndex][timeStep].assign( cellCount, 1.0 );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigSoilResultCalculator::testAndComputeSgasForTimeStep( size_t timeStepIndex )
{
    // Check if SWAT is present
    const RigEclipseResultAddress swatAddr( RiaDefines::ResultCatType::DYNAMIC_NATIVE, RiaResultNames::swat() );
    const size_t swatScalarIndex = m_resultsData->findOrLoadKnownScalarResultForTimeStep( swatAddr, timeStepIndex );
    if ( swatScalarIndex == cvf::UNDEFINED_SIZE_T )
    {
        return;
    }

    // Get available phases from case data
    const std::set<RiaDefines::PhaseType> phases = m_resultsData->m_ownerCaseData->availablePhases();
    
    // Only compute SGAS for gas-water simulations (no oil phase)
    if ( phases.count( RiaDefines::PhaseType::GAS_PHASE ) == 0 ) return;
    if ( phases.count( RiaDefines::PhaseType::OIL_PHASE ) > 0 ) return;

    // Create or find SGAS result
    const RigEclipseResultAddress sgasAddr( RiaDefines::ResultCatType::DYNAMIC_NATIVE, RiaResultNames::sgas() );
    const size_t sgasScalarIndex = m_resultsData->findOrCreateScalarResultIndex( sgasAddr, false );
    
    // Check if SGAS data already exists for this time step
    if ( m_resultsData->m_cellScalarResults[sgasScalarIndex].size() > timeStepIndex )
    {
        std::vector<double>& sgasValues = m_resultsData->m_cellScalarResults[sgasScalarIndex][timeStepIndex];
        if ( !sgasValues.empty() )
        {
            return; // SGAS already computed
        }
    }

    // Get SWAT data dimensions
    size_t cellCount = 0;
    size_t timeStepCount = 0;

    {
        const std::vector<double>& swatData = m_resultsData->m_cellScalarResults[swatScalarIndex][timeStepIndex];
        if ( !swatData.empty() )
        {
            cellCount = swatData.size();
            timeStepCount = m_resultsData->infoForEachResultIndex()[swatScalarIndex].timeStepInfos().size();
        }
    }

    // Allocate SGAS data
    m_resultsData->m_cellScalarResults[sgasScalarIndex].resize( timeStepCount );

    // Check again if data exists after resize
    if ( !m_resultsData->m_cellScalarResults[sgasScalarIndex][timeStepIndex].empty() )
    {
        return;
    }

    m_resultsData->m_cellScalarResults[sgasScalarIndex][timeStepIndex].resize( cellCount );

    // Get data references
    const std::vector<double>* swatData = nullptr;
    {
        swatData = &( m_resultsData->m_cellScalarResults[swatScalarIndex][timeStepIndex] );
        if ( swatData->empty() )
        {
            swatData = nullptr;
        }
    }

    std::vector<double>& sgasData = m_resultsData->m_cellScalarResults[sgasScalarIndex][timeStepIndex];

    // Compute SGAS = 1.0 - SWAT for gas-water simulation
#pragma omp parallel for
    for ( int cellIdx = 0; cellIdx < static_cast<int>( cellCount ); cellIdx++ )
    {
        double gasValue = 1.0; // Start with total pore space

        if ( swatData )
        {
            gasValue -= swatData->at( cellIdx );
        }

        sgasData[cellIdx] = gasValue;
    }
}
