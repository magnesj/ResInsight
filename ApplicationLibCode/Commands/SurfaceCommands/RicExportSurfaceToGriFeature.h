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

#include "RifSurfio.h"

#include "cafCmdFeature.h"

#include <optional>
#include <vector>

class RimSurface;

//==================================================================================================
///
//==================================================================================================
class RicExportSurfaceToGriFeature : public caf::CmdFeature
{
    CAF_CMD_HEADER_INIT;

public:
    // Determines the regular grid to use for all surfaces.
    // For a single RimRegularSurface the stored params are returned directly (no dialog).
    // For multiple surfaces or any unstructured surface, the union bounding box is computed
    // and a dialog is shown once so the user can confirm/adjust the grid parameters.
    // Returns nullopt if the user cancels.
    static std::optional<RigRegularSurfaceData> resolveGridParams( const std::vector<RimSurface*>& surfaces );

    // Resamples one surface onto the given regular grid and returns depth values
    // in row-major order (index = j * nx + i). Returns an empty vector on failure.
    static std::vector<float> resampleToGrid( RimSurface* surf, const RigRegularSurfaceData& gridParams );

    enum class ExportFormat
    {
        GRI,
        IRAP
    };

    // Shared export pipeline: resolve grid, ask for folder, resample and write each surface.
    static void exportToFolder( const std::vector<RimSurface*>& surfaces, ExportFormat format );

protected:
    bool isCommandEnabled() const override;
    void onActionTriggered( bool isChecked ) override;
    void setupActionLook( QAction* actionToSetup ) override;
};
