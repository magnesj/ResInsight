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

    QString realizationName() const { return m_realizationName; }
    void    setRealizationName( const QString& fieldId ) { m_realizationName = fieldId; }

    void                       createSummaryReaderInterface() override;
    RifSummaryReaderInterface* summaryReader() override;

    std::vector<time_t>                  timeSteps( const RifEclipseSummaryAddress& resultAddress ) const override;
    std::pair<bool, std::vector<double>> values( const RifEclipseSummaryAddress& resultAddress ) const override;
    std::string                          unitName( const RifEclipseSummaryAddress& resultAddress ) const override;
    RiaDefines::EclipseUnitSystem        unitSystem() const override;

private:
    caf::PdmField<QString> m_realizationName;

protected:
    QString caseName() const override;
};
