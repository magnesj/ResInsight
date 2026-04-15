/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) Statoil ASA
//  Copyright (C) Ceetron Solutions AS
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

#include "RigResultAccessor.h"

#include <QString>
#include <functional>
#include <vector>

class RigEclipseCaseData;

class RigTimeHistoryResultAccessor
{
public:
    static QString geometrySelectionText( RigEclipseCaseData* eclipseCaseData, size_t m_gridIndex, size_t m_cellIndex );
    static std::vector<double>
        timeHistoryValues( std::function<cvf::ref<RigResultAccessor>( size_t )> accessorFactory, size_t cellIndex, size_t timeStepCount );
};
