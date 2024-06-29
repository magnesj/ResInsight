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

#include "RimNamedObject.h"
#include "Sumo/RiaSumoConnector.h"

//==================================================================================================
//
//
//
//==================================================================================================

class RimSummarySumoDataSource : public RimNamedObject
{
    CAF_PDM_HEADER_INIT;

public:
    RimSummarySumoDataSource();

    SumoCaseId caseId() const;
    void       setCaseId( const SumoCaseId& caseId );

    QString caseName() const;
    void    setCaseName( const QString& caseName );

    QString ensembleName() const;
    void    setEnsembleName( const QString& ensembleName );

private:
    void updateName();

private:
    caf::PdmField<QString> m_caseId;
    caf::PdmField<QString> m_caseName;
    caf::PdmField<QString> m_ensembleName;

    caf::PdmField<QString> m_customName;
};
