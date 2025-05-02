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

#include "RimEnsembleFileset.h"

#include "cafPdmUiComboBoxEditor.h"
#include "cafPdmUiLineEditor.h"

CAF_PDM_SOURCE_INIT( RimEnsembleFileset, "EnsembleFileset" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimEnsembleFileset::RimEnsembleFileset()
{
    CAF_PDM_InitObject( "Ensemble Fileset", "", "", "" );

    CAF_PDM_InitField( &m_pathPattern, "PathPattern", QString(), "Path Pattern", "", "", "" );
    CAF_PDM_InitField( &m_realizationSubSet, "RealizationSubSet", QString(), "Realization SubSet", "", "", "" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimEnsembleFileset::defineEditorAttribute( const caf::PdmFieldHandle* field, QString uiConfigName, caf::PdmUiEditorAttribute* attribute )
{
    if ( field == &m_pathPattern )
    {
        auto lineEdAttr = dynamic_cast<caf::PdmUiLineEditorAttribute*>( attribute );
        if ( lineEdAttr )
        {
            lineEdAttr->placeholderText = "Enter path pattern...";
        }
    }
    else if ( field == &m_realizationSubSet )
    {
        auto lineEdAttr = dynamic_cast<caf::PdmUiLineEditorAttribute*>( attribute );
        if ( lineEdAttr )
        {
            lineEdAttr->placeholderText = "E.g. 0,1,2-5,10-20";
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimEnsembleFileset::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    uiOrdering.add( &m_pathPattern );
    uiOrdering.add( &m_realizationSubSet );

    uiOrdering.skipRemainingFields( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimEnsembleFileset::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    // Add field change handling if needed
}
