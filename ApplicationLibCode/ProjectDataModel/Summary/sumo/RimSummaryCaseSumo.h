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

#include "RifSummaryReaderInterface.h"
#include "RimSummaryCase.h"

class RimSummaryEnsembleSumo;

//==================================================================================================
//
//
//
//==================================================================================================

class RimSummaryCaseSumo : public RimSummaryCase, public RifSummaryReaderInterface
{
    CAF_PDM_HEADER_INIT;

public:
    RimSummaryCaseSumo();

    void setEnsemble( RimSummaryEnsembleSumo* ensemble );

    void setValues( const std::vector<time_t>& timeSteps, const std::string& vectorName, const std::vector<float>& values );

    int32_t realizationNumber() const { return m_realizationNumber; }
    void    setRealizationNumber( int32_t realizationNumber ) { m_realizationNumber = realizationNumber; }

    QString realizationName() const { return m_realizationName; }
    void    setRealizationName( const QString& realizationName ) { m_realizationName = realizationName; }

    void                       createSummaryReaderInterface() override;
    RifSummaryReaderInterface* summaryReader() override;

    std::vector<time_t>                  timeSteps( const RifEclipseSummaryAddress& resultAddress ) const override;
    std::pair<bool, std::vector<double>> values( const RifEclipseSummaryAddress& resultAddress ) const override;
    std::string                          unitName( const RifEclipseSummaryAddress& resultAddress ) const override;
    RiaDefines::EclipseUnitSystem        unitSystem() const override;

protected:
    QString caseName() const override;
    void    buildMetaData() override;

private:
    caf::PdmField<QString> m_realizationName;

    RimSummaryEnsembleSumo* m_ensemble;

    int32_t                                   m_realizationNumber;
    std::vector<time_t>                       m_timeSteps;
    std::map<std::string, std::vector<float>> m_values;
};
