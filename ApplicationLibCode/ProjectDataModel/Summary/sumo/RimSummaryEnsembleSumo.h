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

#include "RimSummaryCaseCollection.h"

#include "Sumo/RiaSumoConnector.h"

//==================================================================================================
//
//
//
//==================================================================================================

struct ParquetKey
{
    QString fieldName;
    QString caseId;
    QString ensembleId;
    QString vectorName;

    auto operator<=>( const ParquetKey& other ) const
    {
        return std::tie( fieldName, caseId, ensembleId, vectorName ) <=>
               std::tie( other.fieldName, other.caseId, other.ensembleId, other.vectorName );
    }
};

namespace arrow
{
class Table;
}

class RimSummaryEnsembleSumo : public RimSummaryCaseCollection
{
    CAF_PDM_HEADER_INIT;

public:
    RimSummaryEnsembleSumo();

    QString fieldName() const { return m_sumoFieldName; }
    void    setFieldName( const QString& fieldId ) { m_sumoFieldName = fieldId; }

    QString caseId() const { return m_sumoCaseId; }
    void    setCaseId( const QString& caseId ) { m_sumoCaseId = caseId; }

    QString ensembleId() const { return m_ensembleId; }
    void    setEnsembleId( const QString& ensembleId ) { m_ensembleId = ensembleId; }

    // To be called by the RimSummaryCaseSumo
    std::vector<time_t>                timeSteps( const QString& realizationName, const RifEclipseSummaryAddress& resultAddress );
    std::vector<double>                values( const QString& realizationName, const RifEclipseSummaryAddress& resultAddress );
    std::string                        unitName( const RifEclipseSummaryAddress& resultAddress );
    RiaDefines::EclipseUnitSystem      unitSystem() const;
    std::set<RifEclipseSummaryAddress> allResultAddresses() const;

protected:
    void onLoadDataAndUpdate() override;

private:
    void                          defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering ) override;
    QList<caf::PdmOptionItemInfo> calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions ) override;
    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override;

    void createSumoConnector();
    void getAvailableVectorNames();
    void clearCachedData();

    QByteArray loadSummaryData( const RifEclipseSummaryAddress& resultAddress );
    QByteArray loadParquetData( const ParquetKey& parquetKey );

    void distributeDataToRealizations( const RifEclipseSummaryAddress& resultAddress, std::shared_ptr<arrow::Table> table );

    std::vector<double> dataForColumn( std::shared_ptr<arrow::Table> table, const QString& realizationName, const QString& columnName );

private:
    caf::PdmField<QString> m_sumoFieldName;
    caf::PdmField<QString> m_sumoCaseId;
    caf::PdmField<QString> m_ensembleId;

    QPointer<RiaSumoConnector> m_sumoConnector;

    const QString m_registryKeyBearerToken_DEBUG_ONLY = "PrivateBearerToken";

    // summary data
    std::set<RifEclipseSummaryAddress>                  m_resultAddresses;
    std::map<ParquetKey, QByteArray>                    m_parquetData;
    std::map<ParquetKey, std::shared_ptr<arrow::Table>> m_parquetTable;
};
