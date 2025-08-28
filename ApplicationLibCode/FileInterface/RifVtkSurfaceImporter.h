/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2025-     Equinor ASA
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

#include <filesystem>
#include <memory>

#include "pugixml.hpp"

class RigTriangleMeshData;

//==================================================================================================
///
//==================================================================================================
class RifVtkSurfaceImporter
{
public:
    static std::unique_ptr<RigTriangleMeshData> importFromFile( const std::filesystem::path& filepath );

private:
    static std::unique_ptr<RigTriangleMeshData> importFromXmlDoc( const pugi::xml_document& doc );
};
