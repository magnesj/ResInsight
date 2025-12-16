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

#include "RiaPreferencesOpenTelemetry.h"

#include "RiaApplication.h"
#include "RiaPreferences.h"
#include "RiaVersionInfo.h"

#include "cafPdmUiComboBoxEditor.h"
#include "cafPdmUiLineEditor.h"
#include "cafPdmUiTextEditor.h"

CAF_PDM_SOURCE_INIT( RiaPreferencesOpenTelemetry, "RiaPreferencesOpenTelemetry" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaPreferencesOpenTelemetry::RiaPreferencesOpenTelemetry()
{
    CAF_PDM_InitObject( "OpenTelemetry Configuration", "", "", "Configuration for OpenTelemetry crash reporting and telemetry" );

    CAF_PDM_InitField( &m_connectionString, "connectionString", QString(), "Azure Connection String" );

    CAF_PDM_InitField( &m_batchTimeoutMs, "batchTimeoutMs", 5000, "Batch Timeout (ms)" );

    CAF_PDM_InitField( &m_maxBatchSize, "maxBatchSize", 512, "Max Batch Size" );

    CAF_PDM_InitField( &m_maxQueueSize, "maxQueueSize", 10000, "Max Queue Size" );

    CAF_PDM_InitField( &m_memoryThresholdMb, "memoryThresholdMb", 50, "Memory Threshold (MB)" );

    CAF_PDM_InitField( &m_samplingRate, "samplingRate", 1.0, "Sampling Rate" );

    CAF_PDM_InitField( &m_connectionTimeoutMs, "connectionTimeoutMs", 10000, "Connection Timeout (ms)" );

    // Privacy settings
    CAF_PDM_InitField( &m_filterFilePaths, "filterFilePaths", true, "Filter File Paths" );
    CAF_PDM_InitField( &m_filterUserData, "filterUserData", true, "Filter User Data" );

    // Fallback settings
    CAF_PDM_InitField( &m_enableLocalLogging, "enableLocalLogging", true, "Enable Local Logging Fallback" );
    CAF_PDM_InitField( &m_failureThreshold, "failureThreshold", 3, "Failure Threshold" );

    CAF_PDM_InitField( &m_retryIntervalSeconds, "retryIntervalSeconds", 300, "Retry Interval (seconds)" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaPreferencesOpenTelemetry::~RiaPreferencesOpenTelemetry()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaPreferencesOpenTelemetry* RiaPreferencesOpenTelemetry::current()
{
    return RiaApplication::instance()->preferences()->openTelemetryPreferences();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaPreferencesOpenTelemetry::setData( const std::map<QString, QString>& keyValuePairs )
{
    for ( const auto& pair : keyValuePairs )
    {
        if ( pair.first == "connection_string" )
        {
            m_connectionString = pair.second;
        }
        else if ( pair.first == "batch_timeout_ms" )
        {
            m_batchTimeoutMs = pair.second.toInt();
        }
        else if ( pair.first == "max_batch_size" )
        {
            m_maxBatchSize = pair.second.toInt();
        }
        else if ( pair.first == "max_queue_size" )
        {
            m_maxQueueSize = pair.second.toInt();
        }
        else if ( pair.first == "memory_threshold_mb" )
        {
            m_memoryThresholdMb = pair.second.toInt();
        }
        else if ( pair.first == "sampling_rate" )
        {
            m_samplingRate = pair.second.toDouble();
        }
        else if ( pair.first == "connection_timeout_ms" )
        {
            m_connectionTimeoutMs = pair.second.toInt();
        }
        else if ( pair.first == "filter_file_paths" )
        {
            m_filterFilePaths = ( pair.second.toLower() == "true" );
        }
        else if ( pair.first == "filter_user_data" )
        {
            m_filterUserData = ( pair.second.toLower() == "true" );
        }
        else if ( pair.first == "enable_local_logging" )
        {
            m_enableLocalLogging = ( pair.second.toLower() == "true" );
        }
        else if ( pair.first == "failure_threshold" )
        {
            m_failureThreshold = pair.second.toInt();
        }
        else if ( pair.first == "retry_interval_seconds" )
        {
            m_retryIntervalSeconds = pair.second.toInt();
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaPreferencesOpenTelemetry::setFieldsReadOnly()
{
    std::vector<caf::PdmFieldHandle*> fields = this->fields();
    for ( auto field : fields )
    {
        field->uiCapability()->setUiReadOnly( true );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiaPreferencesOpenTelemetry::serviceVersion() const
{
    return QString( STRPRODUCTVER );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RiaPreferencesOpenTelemetry::connectionString() const
{
    return m_connectionString;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RiaPreferencesOpenTelemetry::batchTimeoutMs() const
{
    return m_batchTimeoutMs;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RiaPreferencesOpenTelemetry::maxBatchSize() const
{
    return m_maxBatchSize;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RiaPreferencesOpenTelemetry::maxQueueSize() const
{
    return m_maxQueueSize;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RiaPreferencesOpenTelemetry::memoryThresholdMb() const
{
    return m_memoryThresholdMb;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RiaPreferencesOpenTelemetry::samplingRate() const
{
    return m_samplingRate;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RiaPreferencesOpenTelemetry::connectionTimeoutMs() const
{
    return m_connectionTimeoutMs;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaPreferencesOpenTelemetry::filterFilePaths() const
{
    return m_filterFilePaths;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaPreferencesOpenTelemetry::filterUserData() const
{
    return m_filterUserData;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaPreferencesOpenTelemetry::enableLocalLogging() const
{
    return m_enableLocalLogging;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RiaPreferencesOpenTelemetry::failureThreshold() const
{
    return m_failureThreshold;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
int RiaPreferencesOpenTelemetry::retryIntervalSeconds() const
{
    return m_retryIntervalSeconds;
}
