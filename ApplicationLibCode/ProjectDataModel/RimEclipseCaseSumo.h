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

#include "RimEclipseCase.h"

#include "cafPdmField.h"

#include <QPointer>

class RiaSumoConnector;

//==================================================================================================
//
// Eclipse grid case backed by a roff grid stored on Sumo. The grid geometry is downloaded as a
// blob through RiaSumoConnector and parsed in memory, so there is no grid file on disk.
//
//==================================================================================================
class RimEclipseCaseSumo : public RimEclipseCase
{
    CAF_PDM_HEADER_INIT;

public:
    RimEclipseCaseSumo();
    ~RimEclipseCaseSumo() override;

    void setSumoCaseId( const QString& sumoCaseId );
    void setEnsembleName( const QString& ensembleName );
    void setGridName( const QString& gridName );
    void setRealization( int realization );

    int realization() const;

    bool openEclipseGridFile() override;

    QString locationOnDisc() const override;

protected:
    void defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering ) override;

private:
    caf::PdmField<QString> m_sumoCaseId;
    caf::PdmField<QString> m_ensembleName;
    caf::PdmField<QString> m_gridName;
    caf::PdmField<int>     m_realization;

    QPointer<RiaSumoConnector> m_sumoConnector;
};
