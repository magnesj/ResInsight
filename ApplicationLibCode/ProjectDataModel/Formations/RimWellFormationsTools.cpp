/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2026-     Equinor ASA
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

#include "RimWellFormationsTools.h"

#include "EnsembleFileSet/RimEnsembleFileSet.h"
#include "RimOilField.h"
#include "RimProject.h"
#include "RimSummaryEnsemble.h"
#include "RimWellFormationsCollection.h"
#include "RimWellFormationsFile.h"

#include "RiaLogging.h"
#include "RifCaseRealizationParametersReader.h"

#include <QFileInfo>

namespace RimWellFormationsTools
{
//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimWellFormationsFile* formationsForEnsemble( RimSummaryEnsemble* ensemble )
{
    if ( !ensemble ) return nullptr;

    auto fileSet = ensemble->ensembleFileSet();
    if ( !fileSet ) return nullptr;

    return fileSet->wellFormations();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void discoverWellFormations( RimEnsembleFileSet* fileSet )
{
    if ( !fileSet || fileSet->wellFormations() ) return;

    // Use a file extension known to exist for most FMU realizations to resolve the realization
    // paths. Fall back to a grid file extension for grid-only ensembles.
    QStringList paths = fileSet->createPaths( ".SMSPEC" );
    if ( paths.isEmpty() ) paths = fileSet->createPaths( ".EGRID" );
    if ( paths.isEmpty() ) return;

    QString foundPath;
    int     missingCount = 0;
    for ( const auto& path : paths )
    {
        QString candidate = RifFmuFormationsFileLocator::locate( path );
        if ( candidate.isEmpty() )
        {
            missingCount++;
            continue;
        }

        if ( foundPath.isEmpty() )
        {
            foundPath = candidate;
        }
        else if ( QFileInfo( candidate ).absoluteFilePath() != QFileInfo( foundPath ).absoluteFilePath() )
        {
            RiaLogging::warning(
                QString( "Well formations: realization file '%1' differs from the first match '%2'" ).arg( candidate ).arg( foundPath ).toStdString() );
        }
    }

    if ( foundPath.isEmpty() ) return;

    if ( missingCount > 0 )
    {
        RiaLogging::warning(
            QString( "Well formations: %1 of %2 realizations are missing formations.csv" ).arg( missingCount ).arg( paths.size() ).toStdString() );
    }

    auto project = RimProject::current();
    if ( !project || !project->activeOilField() ) return;

    auto collection = project->activeOilField()->wellFormationsCollection();
    if ( !collection ) return;

    auto file = collection->findOrCreate( foundPath );
    fileSet->setWellFormations( file );

    RiaLogging::info(
        QString( "Well formations: auto-discovered '%1' for ensemble file set '%2'" ).arg( foundPath ).arg( fileSet->name() ).toStdString() );
}
} // namespace RimWellFormationsTools
