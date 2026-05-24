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

#pragma once

#include <QString>

#include <map>
#include <string>
#include <vector>

class RigEclipseCaseData;

//==================================================================================================
//
// Class for access to RESQML grids.
//
// RESQML is an industry-standard XML-based format for storing reservoir data,
// typically packaged in an EPC (Energy Package Container) file.
//
// Reading support requires the fesapi library (F2I-CONSULTING/fesapi).
// Enable with CMake option: RESINSIGHT_ENABLE_RESQML
//
//==================================================================================================
class RifResqmlFileTools
{
public:
    static bool openGridFile( const QString& fileName, RigEclipseCaseData* eclipseCase, QString* errorMessages );

    static std::pair<bool, std::map<QString, QString>> createInputProperties( const QString& fileName, RigEclipseCaseData* eclipseCase );

    static bool hasGridData( const QString& fileName );
};
