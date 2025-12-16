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

#include "cafPdmField.h"
#include "cafPdmObject.h"

#include <QString>
#include <map>
#include <vector>

//==================================================================================================
//
// OpenTelemetry preferences for ResInsight
// Follows the same pattern as RiaPreferencesOsdu
//
//==================================================================================================
class RiaPreferencesOpenTelemetry : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    RiaPreferencesOpenTelemetry();
    ~RiaPreferencesOpenTelemetry() override;

    static RiaPreferencesOpenTelemetry* current();

    void setData( const std::map<QString, QString>& keyValuePairs );
    void setFieldsReadOnly();

    // Service name and version are hardcoded, not configurable
    QString serviceName() const { return "ResInsight"; }
    QString serviceVersion() const; // Read from ResInsightVersion.cmake

    // Getters for configuration values
    QString activeEnvironment() const;
    QString connectionString() const;
    QString localEndpoint() const;
    int     batchTimeoutMs() const;
    int     maxBatchSize() const;
    int     maxQueueSize() const;
    int     memoryThresholdMb() const;
    double  samplingRate() const;
    int     connectionTimeoutMs() const;
    bool    filterFilePaths() const;
    bool    filterUserData() const;
    bool    enableLocalLogging() const;
    int     failureThreshold() const;
    int     retryIntervalSeconds() const;

    // Configuration validation
    struct ValidationResult
    {
        bool                 isValid;
        QString              errorMessage;
        std::vector<QString> warnings;
    };
    ValidationResult validate() const;
    bool             testConnection() const;

protected:
    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override;
    QList<caf::PdmOptionItemInfo> calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions ) override;

private:
    caf::PdmField<QString> m_activeEnvironment;
    caf::PdmField<QString> m_connectionString;
    caf::PdmField<QString> m_localEndpoint;
    caf::PdmField<int>     m_batchTimeoutMs;
    caf::PdmField<int>     m_maxBatchSize;
    caf::PdmField<int>     m_maxQueueSize;
    caf::PdmField<int>     m_memoryThresholdMb;
    caf::PdmField<double>  m_samplingRate;
    caf::PdmField<int>     m_connectionTimeoutMs;
    caf::PdmField<bool>    m_filterFilePaths;
    caf::PdmField<bool>    m_filterUserData;
    caf::PdmField<bool>    m_enableLocalLogging;
    caf::PdmField<int>     m_failureThreshold;
    caf::PdmField<int>     m_retryIntervalSeconds;
};