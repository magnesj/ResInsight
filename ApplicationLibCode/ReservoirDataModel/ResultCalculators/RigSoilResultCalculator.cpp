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

#include "RiaResultNames.h"

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
    m_resultsData->testAndComputeSgasForTimeStep( timeStepIndex );

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
