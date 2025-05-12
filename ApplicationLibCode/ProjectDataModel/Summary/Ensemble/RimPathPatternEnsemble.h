/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2025     Equinor ASA
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

#include "RimSummaryEnsemble.h"
#include "cafPdmPtrField.h"

class RimEnsembleFileSet;

//==================================================================================================
//
//
//
//==================================================================================================
class RimPathPatternEnsemble : public RimSummaryEnsemble
{
    CAF_PDM_HEADER_INIT;

public:
    RimPathPatternEnsemble();

    void setEnsembleFileSet( RimEnsembleFileSet* ensembleFileSet );

private:
    QList<caf::PdmOptionItemInfo> calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions ) override;

    void populatePathPattern();
    void onFilterChanged( const caf::SignalEmitter* emitter );

    void createSummaryCasesFromEnsembleFileSet();

    void initAfterRead() override;

protected:
    void defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering ) override;
    void fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue ) override;

    void onLoadDataAndUpdate() override;

private:
    caf::PdmPtrField<RimEnsembleFileSet*> m_ensembleFileSet;
};
