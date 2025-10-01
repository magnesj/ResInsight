/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2016-     Statoil ASA
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

#include "RigFlowDiagDefines.h"

#include "cafAppEnum.h"

#include <set>
#include <string>

class RigFlowDiagResultAddress
{
public:
    enum PhaseSelection
    {
        PHASE_ALL = 0b111,
        PHASE_OIL = 0b001,
        PHASE_GAS = 0b010,
        PHASE_WAT = 0b100,
    };

    using PhaseSelectionEnum = caf::AppEnum<PhaseSelection>;

    RigFlowDiagResultAddress( const std::string& aVariableName, PhaseSelection phaseSelection, const std::set<std::string>& someSelectedTracerNames )
        : variableName( aVariableName )
        , selectedTracerNames( someSelectedTracerNames )
        , phaseSelection( phaseSelection )
    {
    }

    RigFlowDiagResultAddress( const std::string& aVariableName, PhaseSelection phaseSelection, const std::string& tracerName )
        : variableName( aVariableName )
        , phaseSelection( phaseSelection )
    {
        selectedTracerNames.insert( tracerName );
    }

    bool isNativeResult() const;

    std::string uiText() const;
    std::string uiShortText() const;

    std::string           variableName;
    std::set<std::string> selectedTracerNames;
    PhaseSelection        phaseSelection;

    bool operator<( const RigFlowDiagResultAddress& other ) const
    {
        if ( selectedTracerNames != other.selectedTracerNames )
        {
            return selectedTracerNames < other.selectedTracerNames;
        }
        if ( phaseSelection != other.phaseSelection )
        {
            return phaseSelection < other.phaseSelection;
        }

        return variableName < other.variableName;
    }
};
