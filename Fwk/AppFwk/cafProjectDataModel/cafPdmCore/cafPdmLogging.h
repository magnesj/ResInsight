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

#pragma once

#include <QString>

#include <memory>
#include <mutex>
#include <vector>

namespace caf
{

//==================================================================================================
/// Log levels compatible with application-level logging systems
//==================================================================================================
enum class PdmLogLevel
{
    PDM_LL_ERROR   = 1,
    PDM_LL_WARNING = 2,
    PDM_LL_INFO    = 3,
    PDM_LL_DEBUG   = 4
};

//==================================================================================================
/// Abstract interface for logging from CAF PDM Core
//==================================================================================================
class PdmLogger
{
public:
    virtual ~PdmLogger() = default;

    virtual int  level() const            = 0;
    virtual void setLevel( int logLevel ) = 0;

    virtual void error( const QString& message )   = 0;
    virtual void warning( const QString& message ) = 0;
    virtual void info( const QString& message )    = 0;
    virtual void debug( const QString& message )   = 0;
};

//==================================================================================================
/// Static logging manager for the CAF PDM framework
/// Provides a bridge between CAF and application-level logging systems
//==================================================================================================
class PdmLogging
{
public:
    // Logger registration
    static void registerLogger( std::shared_ptr<PdmLogger> logger );
    static void unregisterLogger( std::shared_ptr<PdmLogger> logger );
    static void clearAllLoggers();

    // Logging methods - these are called from throughout the CAF framework
    static void error( const QString& message );
    static void warning( const QString& message );
    static void info( const QString& message );
    static void debug( const QString& message );

    // Utility methods
    static bool hasLoggers();
    static int  effectiveLogLevel();

private:
    static std::vector<std::shared_ptr<PdmLogger>> s_loggers;
    static std::mutex                              s_loggerMutex;
};

//==================================================================================================
/// Convenience macros for logging from CAF code
//==================================================================================================
#define CAF_PDM_LOG_ERROR( message ) caf::PdmLogging::error( message )
#define CAF_PDM_LOG_WARNING( message ) caf::PdmLogging::warning( message )
#define CAF_PDM_LOG_INFO( message ) caf::PdmLogging::info( message )
#define CAF_PDM_LOG_DEBUG( message ) caf::PdmLogging::debug( message )

// Conditional logging macros that only log if loggers are registered
#define CAF_PDM_LOG_ERROR_IF( message )                                         \
    do                                                                          \
    {                                                                           \
        if ( caf::PdmLogging::hasLoggers() ) caf::PdmLogging::error( message ); \
    } while ( 0 )
#define CAF_PDM_LOG_WARNING_IF( message )                                         \
    do                                                                            \
    {                                                                             \
        if ( caf::PdmLogging::hasLoggers() ) caf::PdmLogging::warning( message ); \
    } while ( 0 )
#define CAF_PDM_LOG_INFO_IF( message )                                         \
    do                                                                         \
    {                                                                          \
        if ( caf::PdmLogging::hasLoggers() ) caf::PdmLogging::info( message ); \
    } while ( 0 )
#define CAF_PDM_LOG_DEBUG_IF( message )                                         \
    do                                                                          \
    {                                                                           \
        if ( caf::PdmLogging::hasLoggers() ) caf::PdmLogging::debug( message ); \
    } while ( 0 )

} // namespace caf
