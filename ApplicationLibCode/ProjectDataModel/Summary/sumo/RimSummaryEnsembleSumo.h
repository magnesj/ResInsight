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

//==================================================================================================
//
//
//
//==================================================================================================

class RimSummaryEnsembleSumo : public RimSummaryCaseCollection
{
    CAF_PDM_HEADER_INIT;

public:
    RimSummaryEnsembleSumo();

    QString fieldId() const { return m_sumoFieldId; }
    void    setFieldId( const QString& fieldId ) { m_sumoFieldId = fieldId; }

    QString caseId() const { return m_sumoCaseId; }
    void    setCaseId( const QString& caseId ) { m_sumoCaseId = caseId; }

    QString ensembleId() const { return m_sumoEnsembleId; }
    void    setEnsembleId( const QString& ensembleId ) { m_sumoEnsembleId = ensembleId; }

    // To be called by the RimSummaryCaseSumo
    std::vector<time_t>                  timeSteps( const RifEclipseSummaryAddress& resultAddress ) const;
    std::pair<bool, std::vector<double>> values( const RifEclipseSummaryAddress& resultAddress ) const;
    std::string                          unitName( const RifEclipseSummaryAddress& resultAddress ) const;
    RiaDefines::EclipseUnitSystem        unitSystem() const;

    bool loadSummaryData( const RifEclipseSummaryAddress& resultAddress );

private:
    caf::PdmField<QString> m_sumoFieldId;
    caf::PdmField<QString> m_sumoCaseId;
    caf::PdmField<QString> m_sumoEnsembleId;

protected:
    void onLoadDataAndUpdate() override;
};
