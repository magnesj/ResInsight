//##################################################################################################
//
//   Custom Visualization Core library
//   Copyright (C) 2025 Ceetron Solutions AS
//
//   This library may be used under the terms of either the GNU General Public License or
//   the GNU Lesser General Public License as follows:
//
//   GNU General Public License Usage
//   This library is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//   See the GNU General Public License at <<http://www.gnu.org/licenses/gpl.html>>
//   for more details.
//
//   GNU Lesser General Public License Usage
//   This library is free software; you can redistribute it and/or modify
//   it under the terms of the GNU Lesser General Public License as published by
//   the Free Software Foundation; either version 2.1 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//   See the GNU Lesser General Public License at <<http://www.gnu.org/licenses/lgpl-2.1.html>>
//   for more details.
//
//##################################################################################################

#include "cafPdmLogging.h"

#include <algorithm>

namespace caf
{

//--------------------------------------------------------------------------------------------------
// Static member definitions
//--------------------------------------------------------------------------------------------------
std::vector<std::shared_ptr<PdmLogger>> PdmLogging::s_loggers;
std::mutex                              PdmLogging::s_loggerMutex;

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void PdmLogging::registerLogger( std::shared_ptr<PdmLogger> logger )
{
    if ( !logger ) return;

    std::lock_guard<std::mutex> lock( s_loggerMutex );

    // Avoid duplicate registrations
    auto it = std::find( s_loggers.begin(), s_loggers.end(), logger );
    if ( it == s_loggers.end() )
    {
        s_loggers.push_back( logger );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void PdmLogging::unregisterLogger( std::shared_ptr<PdmLogger> logger )
{
    if ( !logger ) return;

    std::lock_guard<std::mutex> lock( s_loggerMutex );

    auto it = std::find( s_loggers.begin(), s_loggers.end(), logger );
    if ( it != s_loggers.end() )
    {
        s_loggers.erase( it );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void PdmLogging::clearAllLoggers()
{
    std::lock_guard<std::mutex> lock( s_loggerMutex );
    s_loggers.clear();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void PdmLogging::error( const QString& message )
{
    std::lock_guard<std::mutex> lock( s_loggerMutex );

    for ( auto& logger : s_loggers )
    {
        if ( logger && logger->level() >= static_cast<int>( PdmLogLevel::PDM_LL_ERROR ) )
        {
            logger->error( message );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void PdmLogging::warning( const QString& message )
{
    std::lock_guard<std::mutex> lock( s_loggerMutex );

    for ( auto& logger : s_loggers )
    {
        if ( logger && logger->level() >= static_cast<int>( PdmLogLevel::PDM_LL_WARNING ) )
        {
            logger->warning( message );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void PdmLogging::info( const QString& message )
{
    std::lock_guard<std::mutex> lock( s_loggerMutex );

    for ( auto& logger : s_loggers )
    {
        if ( logger && logger->level() >= static_cast<int>( PdmLogLevel::PDM_LL_INFO ) )
        {
            logger->info( message );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void PdmLogging::debug( const QString& message )
{
    std::lock_guard<std::mutex> lock( s_loggerMutex );

    for ( auto& logger : s_loggers )
    {
        if ( logger && logger->level() >= static_cast<int>( PdmLogLevel::PDM_LL_DEBUG ) )
        {
            logger->debug( message );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool PdmLogging::hasLoggers()
{
    std::lock_guard<std::mutex> lock( s_loggerMutex );
    return !s_loggers.empty();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int PdmLogging::effectiveLogLevel()
{
    std::lock_guard<std::mutex> lock( s_loggerMutex );

    if ( s_loggers.empty() )
    {
        return 0; // No logging
    }

    // Return the highest log level among all registered loggers
    int maxLevel = 0;
    for ( const auto& logger : s_loggers )
    {
        if ( logger )
        {
            maxLevel = std::max( maxLevel, logger->level() );
        }
    }

    return maxLevel;
}

} // namespace caf
