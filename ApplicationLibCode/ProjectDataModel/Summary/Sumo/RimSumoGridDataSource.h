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

#include "Cloud/RiaSumoConnector.h"

#include "cafPdmField.h"

#include <QPointer>

//==================================================================================================
//
// GUI data source used to select a grid and a set of realizations from a Sumo ensemble, from which
// a set of RimEclipseCaseSumo grid cases can be created (one per selected realization).
//
//==================================================================================================
class RimSumoGridDataSource : public RimNamedObject
{
    CAF_PDM_HEADER_INIT;

public:
    RimSumoGridDataSource();

    SumoCaseId caseId() const;
    void       setCaseId( const SumoCaseId& caseId );

    QString assetName() const;
    void    setAssetName( const QString& assetName );

    QString caseName() const;
    void    setCaseName( const QString& caseName );

    QString ensembleName() const;
    void    setEnsembleName( const QString& ensembleName );

    QString          gridName() const;
    std::vector<int> selectedRealizations() const;

    void updateName();

private:
    void appendMenuItems( caf::CmdFeatureMenuBuilder& menuBuilder ) const override;
    void defineEditorAttribute( const caf::PdmFieldHandle* field, QString uiConfigName, caf::PdmUiEditorAttribute* attribute ) override;
    void defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering ) override;
    QList<caf::PdmOptionItemInfo> calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions ) override;
    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override;

private:
    caf::PdmField<QString> m_caseId;
    caf::PdmField<QString> m_assetName;
    caf::PdmField<QString> m_caseName;
    caf::PdmField<QString> m_ensembleName;
    caf::PdmField<QString> m_customName;

    caf::PdmField<QString>           m_gridName;
    caf::PdmField<std::vector<int>>  m_realizations;

    QPointer<RiaSumoConnector> m_sumoConnector;
};
