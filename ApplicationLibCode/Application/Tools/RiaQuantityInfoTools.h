/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2024     Equinor ASA
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

#include <string>
#include <unordered_map>

//==================================================================================================
//
//==================================================================================================
namespace RiaQuantityInfoTools
{
void writeEclipseKeywordToFile( const QString& filePath );
void write6XKeywordToFile( const QString& filePath );

void importEclipseKeywords( const QString& filePath );
void import6XKeywords( const QString& filePath );
void importKeywords( const QString& keywordEclipseFilePath, const QString& keyword6XFilePath );

std::unordered_map<std::string, std::pair<std::string, std::string>> importFromFile( const QString& filePath );
void writeToFile( const QString& filePath, const std::unordered_map<std::string, std::pair<std::string, std::string>>& data );

} // namespace RiaQuantityInfoTools
