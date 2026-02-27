/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2024-  Equinor ASA
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

#include "RigWellTargetMapping.h"

#include <vector>

//==================================================================================================
///
///
//==================================================================================================
class RigWellTargetMappingTools
{
public:
    using CellFaceType = RigWellTargetMapping::CellFaceType;
    using VolumeType   = RigWellTargetMapping::VolumeType;
    using VolumesType  = RigWellTargetMapping::VolumesType;

    static double getValueForFace( const std::vector<double>& x,
                                   const std::vector<double>& y,
                                   const std::vector<double>& z,
                                   CellFaceType               face,
                                   size_t                     resultIndex );

    static double getTransmissibilityValueForFace( const std::vector<double>& x,
                                                   const std::vector<double>& y,
                                                   const std::vector<double>& z,
                                                   CellFaceType               face,
                                                   size_t                     resultIndex,
                                                   size_t                     neighborResultIndex );

    static QString getOilVectorName( VolumesType volumesType );
    static QString getGasVectorName( VolumesType volumesType );

    static bool isSaturationSufficient( VolumeType                              volumeType,
                                        const RigWellTargetMapping::DataContainer& data,
                                        const RigWellTargetMapping::ClusteringLimits& limits,
                                        size_t                                  idx );
};
