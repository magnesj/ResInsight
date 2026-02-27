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

#include "RigWellTargetMappingTools.h"

#include "RiaResultNames.h"

#include "cafAssert.h"
#include "cvfStructGrid.h"

#include <limits>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RigWellTargetMappingTools::getValueForFace( const std::vector<double>& x,
                                                   const std::vector<double>& y,
                                                   const std::vector<double>& z,
                                                   CellFaceType               face,
                                                   size_t                     resultIndex )
{
    if ( face == cvf::StructGridInterface::FaceType::POS_I || face == cvf::StructGridInterface::FaceType::NEG_I ) return x[resultIndex];
    if ( face == cvf::StructGridInterface::FaceType::POS_J || face == cvf::StructGridInterface::FaceType::NEG_J ) return y[resultIndex];
    if ( face == cvf::StructGridInterface::FaceType::POS_K || face == cvf::StructGridInterface::FaceType::NEG_K ) return z[resultIndex];
    return std::numeric_limits<double>::infinity();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RigWellTargetMappingTools::getTransmissibilityValueForFace( const std::vector<double>& x,
                                                                   const std::vector<double>& y,
                                                                   const std::vector<double>& z,
                                                                   CellFaceType               face,
                                                                   size_t                     resultIndex,
                                                                   size_t                     neighborResultIndex )
{
    // For negative directions (NEG_I, NEG_J, NEG_K) use the value from the neighbor cell
    bool isPos = face == cvf::StructGridInterface::FaceType::POS_I || face == cvf::StructGridInterface::FaceType::POS_J ||
                 face == cvf::StructGridInterface::FaceType::POS_K;
    size_t index = isPos ? resultIndex : neighborResultIndex;
    return getValueForFace( x, y, z, face, index );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RigWellTargetMappingTools::getOilVectorName( VolumesType volumesType )
{
    switch ( volumesType )
    {
        case VolumesType::RESERVOIR_VOLUMES_COMPUTED:
            return RiaResultNames::riPorvSoil();
        case VolumesType::RESERVOIR_VOLUMES:
            return "RFIPOIL";
        case VolumesType::SURFACE_VOLUMES_SFIP:
            return "SFIPOIL";
        case VolumesType::SURFACE_VOLUMES_FIP:
            return "FIPOIL";
        default:
        {
            CAF_ASSERT( false );
            return "";
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RigWellTargetMappingTools::getGasVectorName( VolumesType volumesType )
{
    switch ( volumesType )
    {
        case VolumesType::RESERVOIR_VOLUMES_COMPUTED:
            return RiaResultNames::riPorvSgas();
        case VolumesType::RESERVOIR_VOLUMES:
            return "RFIPGAS";
        case VolumesType::SURFACE_VOLUMES_SFIP:
            return "SFIPGAS";
        case VolumesType::SURFACE_VOLUMES_FIP:
            return "FIPGAS";
        default:
        {
            CAF_ASSERT( false );
            return "";
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RigWellTargetMappingTools::isSaturationSufficient( VolumeType                                volumeType,
                                                        const RigWellTargetMapping::DataContainer& data,
                                                        const RigWellTargetMapping::ClusteringLimits& limits,
                                                        size_t                                    idx )
{
    bool needsValidOil = volumeType == VolumeType::OIL || volumeType == VolumeType::HYDROCARBON;
    bool needsValidGas = volumeType == VolumeType::GAS || volumeType == VolumeType::HYDROCARBON;
    // For hydrocarbon it is enough that one of the saturations is above the limit.
    if ( needsValidOil && data.saturationOil[idx] >= limits.saturationOil ) return true;
    if ( needsValidGas && data.saturationGas[idx] >= limits.saturationGas ) return true;
    return false;
}
