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

#pragma once

#include <cstddef>
#include <vector>

#include "RigEclipseResultCalculator.h"

class RigCaseCellResultsData;
class RigEclipseResultAddress;

//==================================================================================================
///
//==================================================================================================
class RigSaturationResultCalculator : public RigEclipseResultCalculator
{
public:
    RigSaturationResultCalculator( RigCaseCellResultsData& resultsData );
    ~RigSaturationResultCalculator() override;
    bool isMatching( const RigEclipseResultAddress& resVarAddr ) const override;
    void calculate( const RigEclipseResultAddress& resVarAddr, size_t timeStepIndex ) override;

private:
    const std::vector<double>&
        getSaturationDataForCalculation( const RigEclipseResultAddress& address, size_t scalarIndex, size_t timeStepIndex ) const;
    void checkAndCreateSwatForWaterOnlySimulation( size_t timeStepIndex );
    void createSwatWithFullSaturation();
    void testAndComputeSgasForTimeStep( size_t timeStepIndex );
};
