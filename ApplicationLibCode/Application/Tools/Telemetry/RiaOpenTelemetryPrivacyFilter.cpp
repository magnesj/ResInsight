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

#include "RiaOpenTelemetryPrivacyFilter.h"

#include <regex>
#include <sstream>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaOpenTelemetryPrivacyFilter::FilterRules::FilterRules()
{
    // Default allowed attributes for crash reporting
    allowedAttributes.insert( "crash_type" );
    allowedAttributes.insert( "crash.signal" );
    allowedAttributes.insert( "crash.thread_id" );
    allowedAttributes.insert( "crash.timestamp" );
    allowedAttributes.insert( "crash.stack_trace" );
    allowedAttributes.insert( "test.type" );
    allowedAttributes.insert( "test.thread_id" );
    allowedAttributes.insert( "test.timestamp" );
    allowedAttributes.insert( "test.stack_trace" );
    allowedAttributes.insert( "service.name" );
    allowedAttributes.insert( "service.version" );
    allowedAttributes.insert( "os.type" );
    allowedAttributes.insert( "process.pid" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::string RiaOpenTelemetryPrivacyFilter::sanitizeStackTrace( const std::string& stackTrace, const FilterRules& rules )
{
    if ( !rules.filterFilePaths && !rules.filterUserData )
    {
        return stackTrace;
    }

    std::string sanitized = stackTrace;

    if ( rules.filterFilePaths )
    {
        sanitized = removePersonalPaths( sanitized );
    }

    if ( rules.filterUserData )
    {
        sanitized = removeUsernames( sanitized );
        sanitized = removeEnvironmentVariables( sanitized );
    }

    return sanitized;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::map<std::string, std::string> RiaOpenTelemetryPrivacyFilter::filterAttributes( const std::map<std::string, std::string>& attributes, const FilterRules& rules )
{
    std::map<std::string, std::string> filtered;

    for ( const auto& [key, value] : attributes )
    {
        // Check if attribute is explicitly allowed
        if ( rules.allowedAttributes.find( key ) != rules.allowedAttributes.end() )
        {
            std::string sanitizedValue = value;
            
            // Apply sanitization even to allowed attributes
            if ( key == "crash.stack_trace" || key == "test.stack_trace" )
            {
                sanitizedValue = sanitizeStackTrace( value, rules );
            }
            else if ( rules.filterUserData )
            {
                sanitizedValue = sanitizeMessage( value, rules );
            }
            
            filtered[key] = sanitizedValue;
        }
        else if ( !isSensitiveAttribute( key ) && !containsSensitiveKeywords( value ) )
        {
            // Allow non-sensitive attributes that don't contain sensitive data
            std::string sanitizedValue = rules.filterUserData ? sanitizeMessage( value, rules ) : value;
            filtered[key] = sanitizedValue;
        }
        // Otherwise, skip the attribute (filter it out)
    }

    return filtered;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::string RiaOpenTelemetryPrivacyFilter::sanitizeFilePath( const std::string& filePath )
{
    return removePersonalPaths( filePath );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::string RiaOpenTelemetryPrivacyFilter::sanitizeMessage( const std::string& message, const FilterRules& rules )
{
    std::string sanitized = message;

    if ( rules.filterFilePaths )
    {
        sanitized = removePersonalPaths( sanitized );
    }

    if ( rules.filterUserData )
    {
        sanitized = removeUsernames( sanitized );
        sanitized = removeEnvironmentVariables( sanitized );
    }

    return sanitized;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::string RiaOpenTelemetryPrivacyFilter::removePersonalPaths( const std::string& text )
{
    std::string result = text;

    // Replace Windows user directories
    std::regex userPathRegex( R"(C:\\Users\\[^\\]+)", std::regex_constants::icase );
    result = std::regex_replace( result, userPathRegex, "C:\\Users\\<USER>" );

    // Replace Linux/Mac home directories
    std::regex homePathRegex( R"(/home/[^/]+|/Users/[^/]+)" );
    result = std::regex_replace( result, homePathRegex, "<HOME>" );

    // Replace Windows home environment variable paths
    std::regex winHomeRegex( R"(%USERPROFILE%\\[^\\]*|%HOMEPATH%\\[^\\]*)" );
    result = std::regex_replace( result, winHomeRegex, "<HOME>" );

    // Replace common personal directory patterns
    std::regex personalDirRegex( R"(\\Desktop\\|\\Documents\\|\\Downloads\\|/Desktop/|/Documents/|/Downloads/)" );
    result = std::regex_replace( result, personalDirRegex, "/<PERSONAL_DIR>/" );

    return result;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::string RiaOpenTelemetryPrivacyFilter::removeUsernames( const std::string& text )
{
    std::string result = text;

    // This is a simplified approach - in production, you might want more sophisticated username detection
    // For now, we focus on obvious patterns like paths and environment variables

    return result;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::string RiaOpenTelemetryPrivacyFilter::removeEnvironmentVariables( const std::string& text )
{
    std::string result = text;

    // Remove Windows environment variables
    std::regex winEnvRegex( R"(%[A-Z_]+%)" );
    result = std::regex_replace( result, winEnvRegex, "%<ENV_VAR>%" );

    // Remove Unix environment variables
    std::regex unixEnvRegex( R"(\$[A-Z_][A-Z0-9_]*|\$\{[A-Z_][A-Z0-9_]*\})" );
    result = std::regex_replace( result, unixEnvRegex, "$<ENV_VAR>" );

    return result;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaOpenTelemetryPrivacyFilter::isSensitiveAttribute( const std::string& key )
{
    const auto& sensitiveAttrs = getSensitiveAttributes();
    return sensitiveAttrs.find( key ) != sensitiveAttrs.end();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaOpenTelemetryPrivacyFilter::containsSensitiveKeywords( const std::string& value )
{
    const auto& sensitiveKeywords = getSensitiveKeywords();
    
    std::string lowerValue = value;
    std::transform( lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower );

    for ( const auto& keyword : sensitiveKeywords )
    {
        if ( lowerValue.find( keyword ) != std::string::npos )
        {
            return true;
        }
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const std::set<std::string>& RiaOpenTelemetryPrivacyFilter::getSensitiveAttributes()
{
    static std::set<std::string> sensitiveAttrs = {
        "password",
        "secret",
        "key",
        "token",
        "credential",
        "auth",
        "authorization",
        "bearer",
        "username",
        "user_id",
        "email",
        "personal_id",
        "ssn",
        "social_security",
        "credit_card",
        "api_key"
    };
    return sensitiveAttrs;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const std::set<std::string>& RiaOpenTelemetryPrivacyFilter::getSensitiveKeywords()
{
    static std::set<std::string> sensitiveKeywords = {
        "password",
        "secret",
        "token",
        "credential",
        "api_key",
        "private_key",
        "access_token",
        "refresh_token",
        "bearer",
        "authorization",
        "auth_token"
    };
    return sensitiveKeywords;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
const std::set<std::string>& RiaOpenTelemetryPrivacyFilter::getPathKeywords()
{
    static std::set<std::string> pathKeywords = {
        "users",
        "home",
        "documents",
        "desktop",
        "downloads",
        "appdata",
        "temp",
        "tmp"
    };
    return pathKeywords;
}