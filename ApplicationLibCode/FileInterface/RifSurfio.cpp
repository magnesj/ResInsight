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

#include "RifSurfio.h"

#include "Surface/RigTriangleMeshData.h"

#include "irap_import.h"

#include <filesystem>
#include <fstream>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::expected<std::unique_ptr<RigTriangleMeshData>, std::string> RifSurfio::importSurface( const std::string& filename )
{
    namespace fs = std::filesystem;

    // Check if file exists and is a regular file
    if ( !fs::exists( filename ) || !fs::is_regular_file( filename ) )
    {
        return std::unexpected( "File does not exist or is not a regular file: " + filename );
    }

    // Check file permissions (best effort, may not be reliable on all platforms)
    fs::perms p = fs::status( filename ).permissions();
    if ( ( p & fs::perms::owner_read ) == fs::perms::none && ( p & fs::perms::group_read ) == fs::perms::none &&
         ( p & fs::perms::others_read ) == fs::perms::none )
    {
        return std::unexpected( "File is not readable (permission denied): " + filename );
    }

    // Try to open the file to confirm readability
    std::ifstream file( filename );
    if ( !file.is_open() )
    {
        return std::unexpected( "Could not open file: " + filename );
    }

    auto fileData = surfio::irap::from_ascii_file( filename );

    return std::make_unique<RigTriangleMeshData>();
}
