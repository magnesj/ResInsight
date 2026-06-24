/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2020-    Equinor ASA
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

#include "RifStimPlanModelExporter.h"

#include "RifStimPlanModelAsymmetricFrkExporter.h"
#include "RifStimPlanModelDeviationFrkExporter.h"
#include "RifStimPlanModelGeologicalFrkExporter.h"
#include "RifStimPlanModelPerfsFrkExporter.h"

#include "RimStimPlanModel.h"
#include "RimWellPath.h"

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RifStimPlanModelExporter::writeToDirectory( RimStimPlanModel* stimPlanModel, bool useDetailedFluidLoss, const QString& directoryPath )
{
    RimWellPath* wellPath = stimPlanModel->wellPath();

    bool isTransverse = ( stimPlanModel->fractureOrientation() == RimStimPlanModel::FractureOrientation::TRANSVERSE_WELL_PATH ||
                          stimPlanModel->fractureOrientation() == RimStimPlanModel::FractureOrientation::AZIMUTH );

    return RifStimPlanModelGeologicalFrkExporter::writeToFile( stimPlanModel, useDetailedFluidLoss, directoryPath + "/Geological.frk" ) &&
           RifStimPlanModelDeviationFrkExporter::writeToFile( wellPath ? wellPath->wellPathGeometry() : nullptr,
                                                              directoryPath + "/Deviation.frk" ) &&
           RifStimPlanModelPerfsFrkExporter::writeToFile( isTransverse,
                                                          stimPlanModel->perforationLength(),
                                                          stimPlanModel->anchorPosition(),
                                                          wellPath ? wellPath->wellPathGeometry() : nullptr,
                                                          directoryPath + "/Perfs.frk" ) &&
           RifStimPlanModelAsymmetricFrkExporter::writeToFile( stimPlanModel->formationDip(),
                                                               stimPlanModel->hasBarrier(),
                                                               stimPlanModel->distanceToBarrier(),
                                                               stimPlanModel->barrierDip(),
                                                               stimPlanModel->wellPenetrationLayer(),
                                                               directoryPath + "/Asymmetric.frk" );
}
