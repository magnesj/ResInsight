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

#include "RicExportSurfaceToGriFeature.h"

#include "RiaApplication.h"
#include "RiaLogging.h"

#include "RifSurfio.h"

#include "Surface/RigSurface.h"
#include "Surface/RigSurfaceResampler.h"

#include "RimRegularSurface.h"
#include "RimSurface.h"

#include "RicExportSurfaceToGriUi.h"

#include "cafPdmUiPropertyViewDialog.h"

#include "cafSelectionManagerTools.h"
#include "cafUtils.h"

#include "cvfBoundingBox.h"

#include <QAction>
#include <cmath>

CAF_CMD_SOURCE_INIT( RicExportSurfaceToGriFeature, "RicExportSurfaceToGriFeature" );

//--------------------------------------------------------------------------------------------------
/// Computes default grid parameters from the surfaces, shows a PdmUiPropertyViewDialog to let the
/// user confirm/adjust them, and returns the grid params via return value and the chosen folder via
/// exportFolder. Returns nullopt if the user cancels or no valid grid can be computed.
//--------------------------------------------------------------------------------------------------
std::optional<RigRegularSurfaceData> RicExportSurfaceToGriFeature::resolveGridParams( const std::vector<RimSurface*>& surfaces,
                                                                                      const QString&                  defaultFolder,
                                                                                      QString&                        exportFolder )
{
    if ( surfaces.empty() ) return std::nullopt;

    RicExportSurfaceToGriUi ui;

    // Single RimRegularSurface: pre-populate from its stored params
    if ( surfaces.size() == 1 )
    {
        if ( auto* reg = dynamic_cast<RimRegularSurface*>( surfaces[0] ) )
        {
            ui.setDefaults( defaultFolder, reg->nx(), reg->ny(), reg->originX(), reg->originY(), reg->incrementX(), reg->incrementY() );
        }
    }
    else
    {
        // Multiple surfaces or unstructured: compute union bounding box
        cvf::BoundingBox bb;
        size_t           totalVertexCount = 0;
        for ( RimSurface* surf : surfaces )
        {
            RigSurface* rig = surf->surfaceData();
            if ( !rig ) continue;
            for ( const auto& v : rig->vertices() )
                bb.add( v );
            totalVertexCount += rig->vertices().size();
        }

        if ( !bb.isValid() || totalVertexCount == 0 ) return std::nullopt;

        // Estimate grid spacing from total vertex density across all surfaces:
        //   spacing ≈ sqrt( union_area / total_vertex_count )
        const double areaApprox = bb.extent().x() * bb.extent().y();
        const double spacing    = ( areaApprox > 0.0 ) ? std::sqrt( areaApprox / static_cast<double>( totalVertexCount ) ) : 1.0;

        const int nx = std::max( 2, static_cast<int>( std::ceil( bb.extent().x() / spacing ) ) + 1 );
        const int ny = std::max( 2, static_cast<int>( std::ceil( bb.extent().y() / spacing ) ) + 1 );
        ui.setDefaults( defaultFolder, nx, ny, bb.min().x(), bb.min().y(), spacing, spacing );
    }

    caf::PdmUiPropertyViewDialog dialog( nullptr, &ui, "Export Surface to Regular Grid", "" );
    dialog.resize( QSize( 400, 300 ) );
    if ( dialog.exec() != QDialog::Accepted ) return std::nullopt;

    exportFolder = ui.exportFolder();
    return ui.gridParams();
}

//--------------------------------------------------------------------------------------------------
/// For a RimRegularSurface whose stored grid matches gridParams exactly, depth values are returned
/// directly. Otherwise the surface is resampled onto the grid via RigSurfaceResampler.
//--------------------------------------------------------------------------------------------------
std::vector<float> RicExportSurfaceToGriFeature::resampleToGrid( RimSurface* surf, const RigRegularSurfaceData& gridParams )
{
    // Regular surface with matching grid: use stored depth values directly (lossless)
    if ( auto* reg = dynamic_cast<RimRegularSurface*>( surf ) )
    {
        if ( reg->nx() == gridParams.nx && reg->ny() == gridParams.ny && reg->originX() == gridParams.originX &&
             reg->originY() == gridParams.originY && reg->incrementX() == gridParams.incrementX &&
             reg->incrementY() == gridParams.incrementY && reg->rotation() == gridParams.rotation )
        {
            return reg->depthValues();
        }
    }

    // Resample the surface onto the requested grid
    RigSurface* rig = surf->surfaceData();
    if ( !rig || rig->vertices().empty() ) return {};

    auto depthValues = RigSurfaceResampler::resampleToRegularGrid( rig,
                                                                   gridParams.nx,
                                                                   gridParams.ny,
                                                                   gridParams.originX,
                                                                   gridParams.originY,
                                                                   gridParams.incrementX,
                                                                   gridParams.incrementY,
                                                                   gridParams.rotation );

    // RigSurface stores Z as negative depth; IRAP format uses positive depth values
    for ( auto& v : depthValues )
        if ( !std::isnan( v ) ) v = -v;

    return depthValues;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicExportSurfaceToGriFeature::exportToFolder( const std::vector<RimSurface*>& surfaces, ExportFormat format )
{
    RiaApplication* app        = RiaApplication::instance();
    QString         defaultDir = app->lastUsedDialogDirectoryWithFallbackToProjectFolder( "EXPORT_SURFACE" );

    QString exportDir;
    auto    gridParams = resolveGridParams( surfaces, defaultDir, exportDir );
    if ( !gridParams || exportDir.isEmpty() ) return;

    app->setLastUsedDialogDirectory( "EXPORT_SURFACE", exportDir );

    const QString extension = ( format == ExportFormat::GRI ) ? ".gri" : ".irap";

    for ( RimSurface* surf : surfaces )
    {
        const QString fileName =
            caf::Utils::constructFullFileName( exportDir, caf::Utils::makeValidFileBasename( surf->fullName() ), extension );

        const auto depthValues = resampleToGrid( surf, *gridParams );
        if ( depthValues.empty() ) continue;

        bool ok = ( format == ExportFormat::GRI ) ? RifSurfio::exportToGri( fileName.toStdString(), *gridParams, depthValues )
                                                  : RifSurfio::exportToIrap( fileName.toStdString(), *gridParams, depthValues );

        if ( ok )
            RiaLogging::info( QString( "Exported surface to: %1" ).arg( fileName ) );
        else
            RiaLogging::error( QString( "Failed to export surface to: %1" ).arg( fileName ) );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicExportSurfaceToGriFeature::isCommandEnabled() const
{
    return !caf::selectedObjectsByTypeStrict<RimSurface*>().empty();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicExportSurfaceToGriFeature::onActionTriggered( bool isChecked )
{
    auto surfaces = caf::selectedObjectsByTypeStrict<RimSurface*>();
    if ( surfaces.empty() ) return;

    exportToFolder( surfaces, ExportFormat::GRI );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicExportSurfaceToGriFeature::setupActionLook( QAction* actionToSetup )
{
    actionToSetup->setIcon( QIcon( ":/ReservoirSurfaces16x16.png" ) );
    actionToSetup->setText( "Export Surface to IRAP Binary (GRI) file" );
}
