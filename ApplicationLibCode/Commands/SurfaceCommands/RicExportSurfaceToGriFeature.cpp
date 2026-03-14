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

#include "RicExportSurfaceToGriFeature.h"

#include "RiaApplication.h"

#include "RifSurfio.h"

#include "Surface/RigSurface.h"
#include "Surface/RigSurfaceResampler.h"

#include "RimRegularSurface.h"
#include "RimSurface.h"

#include "RicExportSurfaceToGriDialog.h"
#include "RiuFileDialogTools.h"

#include "cafSelectionManagerTools.h"
#include "cafUtils.h"

#include "cvfBoundingBox.h"

#include <QAction>
#include <cmath>

CAF_CMD_SOURCE_INIT( RicExportSurfaceToGriFeature, "RicExportSurfaceToGriFeature" );

//--------------------------------------------------------------------------------------------------
/// For a single RimRegularSurface, its stored grid params are returned without showing a dialog.
/// For multiple surfaces or any unstructured surface, the union bounding box across all surfaces
/// is computed and the dialog is shown once so the user can confirm/adjust the shared grid.
//--------------------------------------------------------------------------------------------------
std::optional<RigRegularSurfaceData> RicExportSurfaceToGriFeature::resolveGridParams( const std::vector<RimSurface*>& surfaces )
{
    if ( surfaces.empty() ) return std::nullopt;

    // Single RimRegularSurface: use its stored params directly, no dialog needed
    if ( surfaces.size() == 1 )
    {
        if ( auto* reg = dynamic_cast<RimRegularSurface*>( surfaces[0] ) )
        {
            RigRegularSurfaceData p;
            p.nx         = reg->nx();
            p.ny         = reg->ny();
            p.originX    = reg->originX();
            p.originY    = reg->originY();
            p.incrementX = reg->incrementX();
            p.incrementY = reg->incrementY();
            p.rotation   = reg->rotation();
            return p;
        }
    }

    // Multiple surfaces or unstructured: compute union bounding box and show dialog once
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

    RicGriExportGridParams defaults;
    defaults.originX    = bb.min().x();
    defaults.originY    = bb.min().y();
    defaults.incrementX = spacing;
    defaults.incrementY = spacing;
    defaults.nx         = std::max( 2, static_cast<int>( std::ceil( bb.extent().x() / spacing ) ) + 1 );
    defaults.ny         = std::max( 2, static_cast<int>( std::ceil( bb.extent().y() / spacing ) ) + 1 );

    auto params = RicExportSurfaceToGriDialog::openDialog( nullptr, defaults );
    if ( !params.accepted ) return std::nullopt;

    RigRegularSurfaceData p;
    p.nx         = params.nx;
    p.ny         = params.ny;
    p.originX    = params.originX;
    p.originY    = params.originY;
    p.incrementX = params.incrementX;
    p.incrementY = params.incrementY;
    p.rotation   = 0.0;
    return p;
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
    auto gridParams = resolveGridParams( surfaces );
    if ( !gridParams ) return;

    RiaApplication* app        = RiaApplication::instance();
    QString         defaultDir = app->lastUsedDialogDirectoryWithFallbackToProjectFolder( "EXPORT_SURFACE" );

    QString exportDir = RiuFileDialogTools::getExistingDirectory( nullptr, QObject::tr( "Select Export Folder" ), defaultDir );
    if ( exportDir.isEmpty() ) return;

    app->setLastUsedDialogDirectory( "EXPORT_SURFACE", exportDir );

    const QString extension = ( format == ExportFormat::GRI ) ? ".gri" : ".irap";

    for ( RimSurface* surf : surfaces )
    {
        const QString fileName =
            caf::Utils::constructFullFileName( exportDir, caf::Utils::makeValidFileBasename( surf->fullName() ), extension );

        const auto depthValues = resampleToGrid( surf, *gridParams );
        if ( depthValues.empty() ) continue;

        if ( format == ExportFormat::GRI )
            RifSurfio::exportToGri( fileName.toStdString(), *gridParams, depthValues );
        else
            RifSurfio::exportToIrap( fileName.toStdString(), *gridParams, depthValues );
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
