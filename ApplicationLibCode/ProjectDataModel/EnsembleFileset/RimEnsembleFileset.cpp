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

#include "Ensemble/RiaEnsembleImportTools.h"
#include "RimEnsembleFilesetCollection.h"
#include "RimProject.h"

#include "cafCmdFeatureMenuBuilder.h"
#include "cafPdmUiComboBoxEditor.h"
#include "cafPdmUiLineEditor.h"

CAF_PDM_SOURCE_INIT( RimEnsembleFileset, "EnsembleFileset" );

namespace internal
{
QString placeholderString()
{
    return "*";
}
} // namespace internal

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimEnsembleFileset::RimEnsembleFileset()
    : fileSetChanged( this )

{
    CAF_PDM_InitObject( "Ensemble", ":/Cases16x16.png", "", "" );

    CAF_PDM_InitField( &m_pathPattern, "PathPattern", QString(), "Path Pattern", "", "", "" );
    CAF_PDM_InitField( &m_realizationSubSet, "RealizationSubSet", QString(), "Realization SubSet", "", "", "" );

    setDeletable( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QStringList RimEnsembleFileset::createPaths( const QString& extension ) const
{
    // Append extension to the path pattern and return list of files matching the pattern

    QString pathPattern = m_pathPattern();
    if ( pathPattern.isEmpty() )
    {
        return {};
    }

    pathPattern += extension;

    return RiaEnsembleImportTools::createPathsFromPattern( pathPattern, m_realizationSubSet(), internal::placeholderString() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimEnsembleFileset::findAndSetPathPatternAndRangeString( const QStringList& filePaths )
{
    const auto& [pattern, rangeString] = RiaEnsembleImportTools::findPathPattern( filePaths, internal::placeholderString() );

    // find the pattern without extension by finding . and remove rest of string
    auto noExtension = pattern;
    auto dotIndex    = noExtension.lastIndexOf( '.' );
    if ( dotIndex != -1 )
    {
        noExtension = noExtension.left( dotIndex );
    }

    m_pathPattern       = noExtension;
    m_realizationSubSet = rangeString;
    fileSetChanged.send();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimEnsembleFileset::ensembleFilSetOptions()
{
    return RimProject::current()->ensembleFilesetCollection()->ensembleFileSetOptions();
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
    uiOrdering.add( nameField() );
    uiOrdering.add( &m_pathPattern );
    uiOrdering.add( &m_realizationSubSet );

    uiOrdering.skipRemainingFields( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimEnsembleFileset::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    fileSetChanged.send();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimEnsembleFileset::appendMenuItems( caf::CmdFeatureMenuBuilder& menuBuilder ) const
{
    menuBuilder << "RicCreateEnsembleFromFilesetFeature";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimEnsembleFileset::setPathPattern( const QString& pathPattern )
{
    m_pathPattern = pathPattern;
    fileSetChanged.send();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimEnsembleFileset::setRangeString( const QString& rangeString )
{
    m_realizationSubSet = rangeString;
    fileSetChanged.send();
}
