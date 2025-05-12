/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2025 Equinor ASA
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
#include "cafPdmPointer.h"

class RimEnsembleFileset;

//==================================================================================================
///
/// Class to manage a collection of ensemble filesets
///
//==================================================================================================
class RimEnsembleFileSetCollection : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    RimEnsembleFileSetCollection();

    void                             addFileset( RimEnsembleFileset* fileset );
    void                             removeFileset( RimEnsembleFileset* fileset );
    std::vector<RimEnsembleFileset*> filesets() const;
    void                             deleteAllFileSets();

    QList<caf::PdmOptionItemInfo> ensembleFileSetOptions() const;

    void updateConnectedEditors();

private:
    void defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering ) override;
    void defineUiTreeOrdering( caf::PdmUiTreeOrdering& uiTreeOrdering, QString uiConfigName = "" ) override;
    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override;
    void appendMenuItems( caf::CmdFeatureMenuBuilder& menuBuilder ) const override;

private:
    caf::PdmChildArrayField<RimEnsembleFileset*> m_filesets;
};
