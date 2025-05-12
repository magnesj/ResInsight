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

#include "RicCreateEnsembleFromFilesetFeature.h"

#include "Summary/RiaSummaryPlotTools.h"
#include "Summary/RiaSummaryTools.h"

#include "Ensemble/RimPathPatternEnsemble.h"
#include "EnsembleFileset/RimEnsembleFileSet.h"
#include "RimSummaryCaseMainCollection.h"

#include "cafSelectionManagerTools.h"

#include <QAction>

CAF_CMD_SOURCE_INIT( RicCreateEnsembleFromFilesetFeature, "RicCreateEnsembleFromFilesetFeature" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicCreateEnsembleFromFilesetFeature::isCommandEnabled() const
{
    return true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicCreateEnsembleFromFilesetFeature::onActionTriggered( bool isChecked )
{
    for ( auto fileSet : caf::selectedObjectsByType<RimEnsembleFileSet*>() )
    {
        auto ensemble = new RimPathPatternEnsemble();
        ensemble->setEnsembleFileSet( fileSet );
        RiaSummaryTools::summaryCaseMainCollection()->addEnsemble( ensemble );
        ensemble->loadDataAndUpdate();

        RiaSummaryPlotTools::createAndAppendDefaultSummaryMultiPlot( {}, { ensemble } );
    }

    RiaSummaryTools::summaryCaseMainCollection()->updateConnectedEditors();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicCreateEnsembleFromFilesetFeature::setupActionLook( QAction* actionToSetup )
{
    actionToSetup->setIcon( QIcon( ":/Cases16x16.png" ) );
    actionToSetup->setText( "Create Summary Ensemble" );
}
