/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2024-  Equinor ASA
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

#include "cafAppEnum.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"

#include "RigWellTargetCandidatesGenerator.h"

//==================================================================================================
///
///
//==================================================================================================
class RimWellTargetCandidatesGenerator : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    RimWellTargetCandidatesGenerator();
    ~RimWellTargetCandidatesGenerator() override;

protected:
    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override;
    void defineEditorAttribute( const caf::PdmFieldHandle* field, QString uiConfigName, caf::PdmUiEditorAttribute* attribute ) override;
    QList<caf::PdmOptionItemInfo> calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions ) override;
    void                          defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering ) override;

private:
    void generateCandidates();
    void updateAllBoundaries();

    caf::PdmField<int> m_timeStep;

    caf::PdmField<caf::AppEnum<RigWellTargetCandidatesGenerator::VolumeType>>       m_volumeType;
    caf::PdmField<caf::AppEnum<RigWellTargetCandidatesGenerator::VolumeResultType>> m_volumeResultType;
    caf::PdmField<caf::AppEnum<RigWellTargetCandidatesGenerator::VolumesType>>      m_volumesType;

    caf::PdmField<double> m_volume;
    caf::PdmField<double> m_pressure;
    caf::PdmField<double> m_permeability;
    caf::PdmField<double> m_transmissibility;

    caf::PdmField<int> m_maxIterations;
    caf::PdmField<int> m_maxClusters;

    double m_minimumVolume;
    double m_maximumVolume;

    double m_minimumPressure;
    double m_maximumPressure;

    double m_minimumPermeability;
    double m_maximumPermeability;

    double m_minimumTransmissibility;
    double m_maximumTransmissibility;
};
