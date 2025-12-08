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

#include "RiaVersionInfo.h"

#include "cafPdmUiLineEditor.h"
#include "cafPdmUiSpinBoxEditor.h"
#include "cafPdmUiTextEditor.h"

CAF_PDM_SOURCE_INIT( RiaPreferencesOpenTelemetry, "RiaPreferencesOpenTelemetry" );

namespace
{
RiaPreferencesOpenTelemetry* openTelemetryPrefs = nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaPreferencesOpenTelemetry::RiaPreferencesOpenTelemetry()
{
    CAF_PDM_InitObject( "OpenTelemetry Configuration", "", "", "Configuration for OpenTelemetry crash reporting and telemetry" );

    CAF_PDM_InitField( &m_activeEnvironment, "activeEnvironment", QString( "production" ), "Active Environment" );
    CAF_PDM_InitField( &m_connectionString, "connectionString", QString(), "Azure Connection String" );
    CAF_PDM_InitField( &m_localEndpoint, "localEndpoint", QString( "http://localhost:4317" ), "Local OTLP Endpoint" );

    CAF_PDM_InitField( &m_batchTimeoutMs, "batchTimeoutMs", 5000, "Batch Timeout (ms)" );
    m_batchTimeoutMs.uiCapability()->setUiEditorTypeName( caf::PdmUiSpinBoxEditor::uiEditorTypeName() );

    CAF_PDM_InitField( &m_maxBatchSize, "maxBatchSize", 512, "Max Batch Size" );
    m_maxBatchSize.uiCapability()->setUiEditorTypeName( caf::PdmUiSpinBoxEditor::uiEditorTypeName() );

    CAF_PDM_InitField( &m_maxQueueSize, "maxQueueSize", 10000, "Max Queue Size" );
    m_maxQueueSize.uiCapability()->setUiEditorTypeName( caf::PdmUiSpinBoxEditor::uiEditorTypeName() );

    CAF_PDM_InitField( &m_memoryThresholdMb, "memoryThresholdMb", 50, "Memory Threshold (MB)" );
    m_memoryThresholdMb.uiCapability()->setUiEditorTypeName( caf::PdmUiSpinBoxEditor::uiEditorTypeName() );

    CAF_PDM_InitField( &m_samplingRate, "samplingRate", 1.0, "Sampling Rate" );

    CAF_PDM_InitField( &m_connectionTimeoutMs, "connectionTimeoutMs", 10000, "Connection Timeout (ms)" );
    m_connectionTimeoutMs.uiCapability()->setUiEditorTypeName( caf::PdmUiSpinBoxEditor::uiEditorTypeName() );

    // Privacy settings
    CAF_PDM_InitField( &m_filterFilePaths, "filterFilePaths", true, "Filter File Paths" );
    CAF_PDM_InitField( &m_filterUserData, "filterUserData", true, "Filter User Data" );

    // Fallback settings
    CAF_PDM_InitField( &m_enableLocalLogging, "enableLocalLogging", true, "Enable Local Logging Fallback" );
    CAF_PDM_InitField( &m_failureThreshold, "failureThreshold", 3, "Failure Threshold" );
    m_failureThreshold.uiCapability()->setUiEditorTypeName( caf::PdmUiSpinBoxEditor::uiEditorTypeName() );

    CAF_PDM_InitField( &m_retryIntervalSeconds, "retryIntervalSeconds", 300, "Retry Interval (seconds)" );
    m_retryIntervalSeconds.uiCapability()->setUiEditorTypeName( caf::PdmUiSpinBoxEditor::uiEditorTypeName() );
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
    if ( !openTelemetryPrefs )
    {
        openTelemetryPrefs = new RiaPreferencesOpenTelemetry;
    }
    return openTelemetryPrefs;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaPreferencesOpenTelemetry::setData( const std::map<QString, QString>& keyValuePairs )
{
    for ( const auto& pair : keyValuePairs )
    {
        if ( pair.first == "active_environment" )
        {
            m_activeEnvironment = pair.second;
        }
        else if ( pair.first == "connection_string" )
        {
            m_connectionString = pair.second;
        }
        else if ( pair.first == "local_endpoint" )
        {
            m_localEndpoint = pair.second;
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
QString RiaPreferencesOpenTelemetry::activeEnvironment() const
{
    return m_activeEnvironment;
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
QString RiaPreferencesOpenTelemetry::localEndpoint() const
{
    return m_localEndpoint;
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

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiaPreferencesOpenTelemetry::ValidationResult RiaPreferencesOpenTelemetry::validate() const
{
    ValidationResult result;
    result.isValid = true;

    if ( activeEnvironment() == "production" )
    {
        if ( connectionString().isEmpty() )
        {
            result.isValid      = false;
            result.errorMessage = "Connection string is required for production environment";
            return result;
        }

        if ( !connectionString().contains( "InstrumentationKey" ) )
        {
            result.warnings.push_back( "Connection string may be invalid - missing InstrumentationKey" );
        }
    }
    else if ( activeEnvironment() == "development" )
    {
        if ( localEndpoint().isEmpty() )
        {
            result.isValid      = false;
            result.errorMessage = "Local endpoint is required for development environment";
            return result;
        }
    }

    if ( samplingRate() < 0.0 || samplingRate() > 1.0 )
    {
        result.isValid      = false;
        result.errorMessage = "Sampling rate must be between 0.0 and 1.0";
        return result;
    }

    if ( batchTimeoutMs() < 100 || batchTimeoutMs() > 60000 )
    {
        result.warnings.push_back( "Batch timeout should be between 100ms and 60000ms" );
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RiaPreferencesOpenTelemetry::testConnection() const
{
    // TODO: Implement actual connection test
    // This would attempt to send a test span to the configured endpoint
    return true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaPreferencesOpenTelemetry::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    // Validate configuration when fields change
    ValidationResult validation = validate();
    if ( !validation.isValid )
    {
        // Could show warning to user here
    }
}