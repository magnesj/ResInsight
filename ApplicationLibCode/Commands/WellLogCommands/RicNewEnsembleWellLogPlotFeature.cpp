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

#include "RicNewEnsembleWellLogPlotFeature.h"

#include "RicNewWellLogPlotFeatureImpl.h"

#include "RiaColorTables.h"

#include "RimEnsembleWellLogCurveSet.h"
#include "RimEnsembleWellLogs.h"
#include "RimWellLogChannel.h"
#include "RimWellLogLasFile.h"
#include "RimWellLogPlot.h"
#include "RimWellLogTrack.h"

#include "RiuPlotMainWindowTools.h"

#include "cafSelectionManager.h"

#include <QAction>

#include <set>

CAF_CMD_SOURCE_INIT( RicNewEnsembleWellLogPlotFeature, "RicNewEnsembleWellLogPlotFeature" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicNewEnsembleWellLogPlotFeature::isCommandEnabled() const
{
    return caf::SelectionManager::instance()->selectedItemOfType<RimEnsembleWellLogs>() != nullptr;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicNewEnsembleWellLogPlotFeature::onActionTriggered( bool isChecked )
{
    auto* ensembleWellLogs = caf::SelectionManager::instance()->selectedItemOfType<RimEnsembleWellLogs>();
    if ( !ensembleWellLogs ) return;

    // Collect available channels (alphabetically, as in the channel list), excluding depth channels
    const QStringList depthChannels = { "DEPT", "DEPTH", "MD", "MDEPTH", "TVDMSL", "TVDRKB" };
    std::set<QString> channelNames;
    for ( auto* wellLogFile : ensembleWellLogs->wellLogFiles() )
    {
        for ( auto* channel : wellLogFile->wellLogChannels() )
        {
            if ( !depthChannels.contains( channel->name().toUpper() ) ) channelNames.insert( channel->name() );
        }
    }

    constexpr size_t maxChannelCount = 5;

    RimDepthTrackPlot*          plot          = nullptr;
    RimEnsembleWellLogCurveSet* firstCurveSet = nullptr;
    int                         trackIndex    = 0;
    for ( const QString& channelName : channelNames )
    {
        if ( trackIndex >= static_cast<int>( maxChannelCount ) ) break;

        RimWellLogTrack* plotTrack = RicNewWellLogPlotFeatureImpl::createWellLogPlotTrack( true, channelName, plot );
        plot                       = plotTrack->firstAncestorOrThisOfType<RimDepthTrackPlot>();

        auto* curveSet = new RimEnsembleWellLogCurveSet();
        curveSet->setEnsembleWellLogs( ensembleWellLogs );
        curveSet->setColor( RiaColorTables::normalPaletteColors().cycledColor3f( trackIndex ) );
        curveSet->setWellLogChannelName( channelName );
        plotTrack->setEnsembleWellLogCurveSet( curveSet );
        curveSet->loadDataAndUpdate( true );

        if ( !firstCurveSet ) firstCurveSet = curveSet;
        trackIndex++;
    }

    if ( !plot )
    {
        // No channels found: create an empty track with a curve set the user can configure
        RimWellLogTrack* plotTrack = RicNewWellLogPlotFeatureImpl::createWellLogPlotTrack();
        plot                       = plotTrack->firstAncestorOrThisOfType<RimDepthTrackPlot>();

        firstCurveSet = new RimEnsembleWellLogCurveSet();
        firstCurveSet->setEnsembleWellLogs( ensembleWellLogs );
        firstCurveSet->setColor( RiaColorTables::normalPaletteColors().cycledColor3f( 0 ) );
        plotTrack->setEnsembleWellLogCurveSet( firstCurveSet );
    }

    plot->updateConnectedEditors();
    if ( auto* wellLogPlot = dynamic_cast<RimWellLogPlot*>( plot ) ) wellLogPlot->zoomAll();

    RiuPlotMainWindowTools::onObjectAppended( firstCurveSet );
    RiuPlotMainWindowTools::refreshToolbars();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicNewEnsembleWellLogPlotFeature::setupActionLook( QAction* actionToSetup )
{
    actionToSetup->setText( "New Well Log Plot" );
    actionToSetup->setIcon( QIcon( ":/WellLogPlot16x16.png" ) );
}
