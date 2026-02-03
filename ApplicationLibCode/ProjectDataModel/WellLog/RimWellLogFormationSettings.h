/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2025-     Equinor ASA
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

#include "Well/RigWellPathFormations.h"

#include "cafAppEnum.h"
#include "cafPdmField.h"
#include "cafPdmObject.h"
#include "cafPdmPtrField.h"

class RimCase;
class RimWellPath;
class RimWellLogTrack;

//==================================================================================================
///
/// Settings class for formation/trajectory configuration in well log tracks
/// Note: Uses TrajectoryType and FormationSource enums defined in RimWellLogTrack for backward compatibility
///
//==================================================================================================
class RimWellLogFormationSettings : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    RimWellLogFormationSettings();
    ~RimWellLogFormationSettings() override;

    // Formation source (uses int internally, converted to/from RimWellLogTrack::FormationSource)
    int  formationSource() const;
    void setFormationSource( int source );

    // Formation case
    RimCase* formationCase() const;
    void     setFormationCase( RimCase* rimCase );

    // Trajectory type (uses int internally, converted to/from RimWellLogTrack::TrajectoryType)
    int  trajectoryType() const;
    void setTrajectoryType( int trajectoryType );

    // Well paths
    RimWellPath* wellPathForSourceCase() const;
    void         setWellPathForSourceCase( RimWellPath* wellPath );

    RimWellPath* wellPathForSourceWellPath() const;
    void         setWellPathForSourceWellPath( RimWellPath* wellPath );

    // Simulation well
    QString simWellName() const;
    void    setSimWellName( const QString& simWellName );

    int  branchIndex() const;
    void setBranchIndex( int branchIndex );

    bool branchDetection() const;
    void setBranchDetection( bool branchDetection );

    // Formation level
    RigWellPathFormations::FormationLevel formationLevel() const;
    void                                  setFormationLevel( RigWellPathFormations::FormationLevel level );

    // Show fluids
    bool showFormationFluids() const;
    void setShowFormationFluids( bool show );

    // UI ordering helper
    void uiOrdering( const QString& uiConfigName, caf::PdmUiOrdering& uiOrdering, bool formationsForCaseWithSimWellOnly );

protected:
    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override;
    QList<caf::PdmOptionItemInfo> calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions ) override;

private:
    caf::PdmField<int>                                                 m_formationSource;
    caf::PdmPtrField<RimCase*>                                         m_formationCase;
    caf::PdmField<int>                                                 m_formationTrajectoryType;
    caf::PdmPtrField<RimWellPath*>                                     m_formationWellPathForSourceCase;
    caf::PdmPtrField<RimWellPath*>                                     m_formationWellPathForSourceWellPath;
    caf::PdmField<QString>                                             m_formationSimWellName;
    caf::PdmField<int>                                                 m_formationBranchIndex;
    caf::PdmField<bool>                                                m_formationBranchDetection;
    caf::PdmField<caf::AppEnum<RigWellPathFormations::FormationLevel>> m_formationLevel;
    caf::PdmField<bool>                                                m_showFormationFluids;
};
