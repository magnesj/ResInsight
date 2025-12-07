/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2024- Equinor ASA
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

#include <map>
#include <set>
#include <string>

//==================================================================================================
//
// Privacy filter for OpenTelemetry data
// Sanitizes sensitive information from telemetry data before transmission
//
//==================================================================================================
class RiaOpenTelemetryPrivacyFilter
{
public:
    struct FilterRules
    {
        bool                 filterFilePaths = true;
        bool                 filterUserData  = true;
        std::set<std::string> allowedAttributes;
        
        FilterRules();
    };

    static std::string                        sanitizeStackTrace( const std::string& stackTrace, const FilterRules& rules = FilterRules() );
    static std::map<std::string, std::string> filterAttributes( const std::map<std::string, std::string>& attributes, const FilterRules& rules = FilterRules() );
    static std::string                        sanitizeFilePath( const std::string& filePath );
    static std::string                        sanitizeMessage( const std::string& message, const FilterRules& rules = FilterRules() );

private:
    static std::string removePersonalPaths( const std::string& path );
    static std::string removeUsernames( const std::string& text );
    static std::string removeEnvironmentVariables( const std::string& text );
    static bool        isSensitiveAttribute( const std::string& key );
    static bool        containsSensitiveKeywords( const std::string& value );
    
    static const std::set<std::string>& getSensitiveAttributes();
    static const std::set<std::string>& getSensitiveKeywords();
    static const std::set<std::string>& getPathKeywords();
};