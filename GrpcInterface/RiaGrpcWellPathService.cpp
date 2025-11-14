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
//////////////////////////////////////////////////////////////////////////////////
#include "RiaGrpcWellPathService.h"

#include "RiaGrpcCallbacks.h"
#include "RiaGrpcHelper.h"

#include "../Commands/CompletionExportCommands/RicWellPathExportCompletionDataFeatureImpl.h"
#include "../Commands/CompletionExportCommands/RicWellPathExportMswTableData.h"

#include "CompletionsMsw/RigMswTableData.h"
#include "RigEclipseCaseData.h"
#include "RigGridBase.h"
#include "RigMainGrid.h"

#include "RimCase.h"
#include "RimEclipseCase.h"
#include "RimProject.h"
#include "RimWellPath.h"
#include "RimWellPathCompletionSettings.h"

using namespace rips;

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
grpc::Status RiaGrpcWellPathService::GetCompletionData( grpc::ServerContext*               context,
                                                        const rips::SimulatorTableRequest* request,
                                                        rips::SimulatorTableData*          reply )

{
    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>( RiaGrpcHelper::findCase( request->case_id() ) );
    if ( !eclipseCase )
    {
        return grpc::Status( grpc::NOT_FOUND, "Case not found" );
    }

    RimWellPath* wellPath = nullptr;

    if ( auto proj = RimProject::current() )
    {
        wellPath = proj->wellPathByName( QString::fromStdString( request->wellpath_name() ) );
    }

    if ( !wellPath )
    {
        return grpc::Status( grpc::NOT_FOUND, "Well path " + request->wellpath_name() + " not found" );
    }

    auto compdata = RicWellPathExportCompletionDataFeatureImpl::completionDataForWellPath( wellPath, eclipseCase );
    for ( const auto& cd : compdata )
    {
        SimulatorCompdatEntry* grpcData = reply->add_compdat();
        RiaGrpcWellPathService::copyCompdatToGrpc( cd, grpcData );
    }

    auto ijPos = RicWellPathExportCompletionDataFeatureImpl::wellPathUpperGridIntersectionIJ( eclipseCase, wellPath );
    auto compSettings = wellPath->completionSettings();

    SimulatorWelspecsEntry* grpcData = reply->add_welspecs();
    RiaGrpcWellPathService::copyWelspecsToGrpc( compSettings, grpcData, eclipseCase, ijPos.second.x(), ijPos.second.y() );

    // Multisegment wells

    int  timeStep         = 0;
    auto mswDataContainer = RicWellPathExportMswTableData::extractSingleWellMswData( eclipseCase, wellPath, timeStep );
    if ( mswDataContainer.has_value() )
    {
        auto tables = mswDataContainer.value();

        if ( tables.hasWelsegsData() )
        {
            if ( SimulatorWelsegsEntry* grpcSegData = reply->add_welsegs() )
            {
                RiaGrpcWellPathService::copyWelsegsToGrpc( tables, grpcSegData );
            }
        }

        if ( tables.hasCompsegsData() )
        {
            RiaGrpcWellPathService::copyCompsegsToGrpc( tables, reply );
        }

        if ( tables.hasWsegvalvData() )
        {
            RiaGrpcWellPathService::copyWsegvalvToGrpc( tables, reply );
        }

        if ( tables.hasWsegaicdData() )
        {
            RiaGrpcWellPathService::copyWsegaicdToGrpc( tables, reply );
        }
    }

    return grpc::Status::OK;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaGrpcWellPathService::copyWelspecsToGrpc( const RimWellPathCompletionSettings* compSettings,
                                                 rips::SimulatorWelspecsEntry*        grpcData,
                                                 RimEclipseCase*                      eclipseCase,
                                                 int                                  gridI,
                                                 int                                  gridJ )
{
    grpcData->set_well_name( compSettings->wellNameForExport().toStdString() );
    grpcData->set_group_name( compSettings->groupNameForExport().toStdString() );

    // Convert to 1-based indexing
    grpcData->set_grid_i( gridI + 1 );
    grpcData->set_grid_j( gridJ + 1 );

    if ( compSettings->referenceDepth().has_value() )
    {
        grpcData->set_bhp_depth( compSettings->referenceDepth().value() );
    }
    grpcData->set_phase( compSettings->wellTypeNameForExport().toStdString() );
    if ( compSettings->drainageRadius().has_value() )
    {
        grpcData->set_drainage_radius( compSettings->drainageRadius().value() );
    }
    grpcData->set_inflow_equation( compSettings->gasInflowEquationForExport().toStdString() );
    grpcData->set_auto_shut_in( compSettings->automaticWellShutInForExport().toStdString() );
    grpcData->set_cross_flow( compSettings->allowWellCrossFlowForExport().toStdString() );
    grpcData->set_pvt_num( compSettings->wellBoreFluidPVT() );
    grpcData->set_hydrostatic_density_calc( compSettings->hydrostaticDensityForExport().toStdString() );
    grpcData->set_fip_region( compSettings->fluidInPlaceRegion() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaGrpcWellPathService::copyWelsegsToGrpc( const RigMswTableData& mswTableData, rips::SimulatorWelsegsEntry* grpcData )
{
    if ( !mswTableData.hasWelsegsData() ) return;

    auto* headerEntry   = grpcData->mutable_header();
    auto  welsegsHeader = mswTableData.welsegsHeader();
    headerEntry->set_well_name( welsegsHeader.well );
    headerEntry->set_top_depth( welsegsHeader.topDepth );
    headerEntry->set_top_length( welsegsHeader.topLength );
    if ( welsegsHeader.wellboreVolume.has_value() )
    {
        headerEntry->set_wellbore_volume( welsegsHeader.wellboreVolume.value() );
    }
    headerEntry->set_info_type( welsegsHeader.infoType );
    if ( welsegsHeader.pressureComponents.has_value() )
    {
        headerEntry->set_pressure_omponents( welsegsHeader.pressureComponents.value() );
    }
    if ( welsegsHeader.flowModel.has_value() )
    {
        headerEntry->set_flow_model( welsegsHeader.flowModel.value() );
    }

    for ( const auto& welsegsRow : mswTableData.welsegsData() )
    {
        auto* rowEntry = grpcData->add_row();
        rowEntry->set_segment_1( welsegsRow.segment1 );
        rowEntry->set_segment_2( welsegsRow.segment2 );
        rowEntry->set_branch( welsegsRow.branch );
        rowEntry->set_join_segment( welsegsRow.joinSegment );
        rowEntry->set_length( welsegsRow.length );
        rowEntry->set_depth( welsegsRow.depth );
        if ( welsegsRow.diameter.has_value() )
        {
            rowEntry->set_diameter( welsegsRow.diameter.value() );
        }
        if ( welsegsRow.roughness.has_value() )
        {
            rowEntry->set_roughness( welsegsRow.roughness.value() );
        }
        if ( !welsegsRow.description.empty() )
        {
            rowEntry->set_description( welsegsRow.description );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaGrpcWellPathService::copyCompsegsToGrpc( const RigMswTableData& mswTableData, rips::SimulatorTableData* reply )
{
    if ( !mswTableData.hasCompsegsData() ) return;

    for ( const auto& compsegsRow : mswTableData.compsegsData() )
    {
        auto* grpcCompData = reply->add_compsegs();

        // Convert to 1-based indexing for grid coordinates
        grpcCompData->set_i( static_cast<int32_t>( compsegsRow.i + 1 ) );
        grpcCompData->set_j( static_cast<int32_t>( compsegsRow.j + 1 ) );
        grpcCompData->set_k( static_cast<int32_t>( compsegsRow.k + 1 ) );

        grpcCompData->set_branch( compsegsRow.branch );
        grpcCompData->set_distance_start( compsegsRow.distanceStart );
        grpcCompData->set_distance_end( compsegsRow.distanceEnd );

        if ( !compsegsRow.gridName.empty() )
        {
            grpcCompData->set_grid_name( compsegsRow.gridName );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaGrpcWellPathService::copyWsegvalvToGrpc( const RigMswTableData& mswTableData, rips::SimulatorTableData* reply )
{
    if ( !mswTableData.hasWsegvalvData() ) return;

    for ( const auto& wsegvalvRow : mswTableData.wsegvalvData() )
    {
        auto* grpcValvData = reply->add_wsegvalv();

        grpcValvData->set_well_name( wsegvalvRow.well );
        grpcValvData->set_segment_number( wsegvalvRow.segmentNumber );
        grpcValvData->set_cv( wsegvalvRow.cv );
        grpcValvData->set_area( wsegvalvRow.area );

        if ( wsegvalvRow.extraLength.has_value() )
        {
            grpcValvData->set_extra_length( wsegvalvRow.extraLength.value() );
        }
        if ( wsegvalvRow.pipeD.has_value() )
        {
            grpcValvData->set_pipe_d( wsegvalvRow.pipeD.value() );
        }
        if ( wsegvalvRow.roughness.has_value() )
        {
            grpcValvData->set_roughness( wsegvalvRow.roughness.value() );
        }
        if ( wsegvalvRow.pipeA.has_value() )
        {
            grpcValvData->set_pipe_a( wsegvalvRow.pipeA.value() );
        }
        if ( wsegvalvRow.status.has_value() )
        {
            grpcValvData->set_status( wsegvalvRow.status.value() );
        }
        if ( wsegvalvRow.maxA.has_value() )
        {
            grpcValvData->set_max_a( wsegvalvRow.maxA.value() );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaGrpcWellPathService::copyWsegaicdToGrpc( const RigMswTableData& mswTableData, rips::SimulatorTableData* reply )
{
    if ( !mswTableData.hasWsegaicdData() ) return;

    for ( const auto& wsegaicdRow : mswTableData.wsegaicdData() )
    {
        auto* grpcAicdData = reply->add_wsegaicd();

        grpcAicdData->set_well_name( wsegaicdRow.well );
        grpcAicdData->set_segment_1( wsegaicdRow.segment1 );
        grpcAicdData->set_segment_2( wsegaicdRow.segment2 );
        grpcAicdData->set_strength( wsegaicdRow.strength );
        grpcAicdData->set_max_abs_rate( wsegaicdRow.maxAbsRate );
        grpcAicdData->set_flow_rate_exponent( wsegaicdRow.flowRateExponent );
        grpcAicdData->set_visc_exponent( wsegaicdRow.viscExponent );

        if ( wsegaicdRow.length.has_value() )
        {
            grpcAicdData->set_length( wsegaicdRow.length.value() );
        }
        if ( wsegaicdRow.densityCali.has_value() )
        {
            grpcAicdData->set_density_cali( wsegaicdRow.densityCali.value() );
        }
        if ( wsegaicdRow.viscosityCali.has_value() )
        {
            grpcAicdData->set_viscosity_cali( wsegaicdRow.viscosityCali.value() );
        }
        if ( wsegaicdRow.criticalValue.has_value() )
        {
            grpcAicdData->set_critical_value( wsegaicdRow.criticalValue.value() );
        }
        if ( wsegaicdRow.widthTrans.has_value() )
        {
            grpcAicdData->set_width_trans( wsegaicdRow.widthTrans.value() );
        }
        if ( wsegaicdRow.maxViscRatio.has_value() )
        {
            grpcAicdData->set_max_visc_ratio( wsegaicdRow.maxViscRatio.value() );
        }
        if ( wsegaicdRow.methodScalingFactor.has_value() )
        {
            grpcAicdData->set_method_scaling_factor( wsegaicdRow.methodScalingFactor.value() );
        }
        if ( wsegaicdRow.status.has_value() )
        {
            grpcAicdData->set_status( wsegaicdRow.status.value() );
        }
        if ( wsegaicdRow.oilFlowFraction.has_value() )
        {
            grpcAicdData->set_oil_flow_fraction( wsegaicdRow.oilFlowFraction.value() );
        }
        if ( wsegaicdRow.waterFlowFraction.has_value() )
        {
            grpcAicdData->set_water_flow_fraction( wsegaicdRow.waterFlowFraction.value() );
        }
        if ( wsegaicdRow.gasFlowFraction.has_value() )
        {
            grpcAicdData->set_gas_flow_fraction( wsegaicdRow.gasFlowFraction.value() );
        }
        if ( wsegaicdRow.oilViscFraction.has_value() )
        {
            grpcAicdData->set_oil_visc_fraction( wsegaicdRow.oilViscFraction.value() );
        }
        if ( wsegaicdRow.waterViscFraction.has_value() )
        {
            grpcAicdData->set_water_visc_fraction( wsegaicdRow.waterViscFraction.value() );
        }
        if ( wsegaicdRow.gasViscFraction.has_value() )
        {
            grpcAicdData->set_gas_visc_fraction( wsegaicdRow.gasViscFraction.value() );
        }
        if ( !wsegaicdRow.description.empty() )
        {
            grpcAicdData->set_description( wsegaicdRow.description );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RiaGrpcWellPathService::copyCompdatToGrpc( const RigCompletionData& inputData, rips::SimulatorCompdatEntry* compDat )
{
    compDat->set_comment( inputData.metaDataString().toStdString() );
    compDat->set_well_name( inputData.wellName().toStdString() );

    // Convert to 1-based indexing
    compDat->set_grid_i( inputData.completionDataGridCell().localCellIndexI() + 1 );
    compDat->set_grid_j( inputData.completionDataGridCell().localCellIndexJ() + 1 );
    compDat->set_upper_k( inputData.completionDataGridCell().localCellIndexK() + 1 );
    compDat->set_lower_k( inputData.completionDataGridCell().localCellIndexK() + 1 );

    compDat->set_open_shut_flag( "OPEN" );
    if ( inputData.saturation() != inputData.defaultValue() )
    {
        compDat->set_saturation( inputData.saturation() );
    }
    if ( inputData.transmissibility() != inputData.defaultValue() )
    {
        compDat->set_transmissibility( inputData.transmissibility() );
    }
    if ( inputData.diameter() != inputData.defaultValue() )
    {
        compDat->set_diameter( inputData.diameter() );
    }
    if ( inputData.kh() != inputData.defaultValue() )
    {
        compDat->set_kh( inputData.kh() );
    }
    if ( inputData.skinFactor() != inputData.defaultValue() )
    {
        compDat->set_skin_factor( inputData.skinFactor() );
    }
    if ( inputData.dFactor() != inputData.defaultValue() )
    {
        compDat->set_d_factor( inputData.dFactor() );
    }
    compDat->set_direction( inputData.directionStringXYZ().toStdString() );
    if ( inputData.startMD().has_value() )
    {
        compDat->set_start_md( inputData.startMD().value() );
        compDat->set_end_md( inputData.endMD().value() );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RiaGrpcCallbackInterface*> RiaGrpcWellPathService::createCallbacks()
{
    using Self = RiaGrpcWellPathService;

    return { new RiaGrpcUnaryCallback<Self, SimulatorTableRequest, SimulatorTableData>( this,
                                                                                        &Self::GetCompletionData,
                                                                                        &Self::RequestGetCompletionData ) };
}

static bool RiaGrpcWellPathService_init = RiaGrpcServiceFactory::instance()->registerCreator<RiaGrpcWellPathService>(
    typeid( RiaGrpcWellPathService ).hash_code() );
