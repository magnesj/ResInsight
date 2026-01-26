/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2015-     Statoil ASA
//  Copyright (C) 2015-     Ceetron Solutions AS
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

#include "RimWellLogTrackStackedCurves.h"

#include "RimWellLogTrack.h"

#include "RiaDefines.h"

#include "Well/RigWellLogCurveData.h"

#include "RimDepthTrackPlot.h"
#include "RimWellFlowRateCurve.h"
#include "RimWellLogCurve.h"
#include "RimWellLogPlot.h"

#include <Qt>

#include <algorithm>
#include <cmath>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimWellLogTrackStackedCurves::RimWellLogTrackStackedCurves( RimWellLogTrack* track )
    : m_track( track )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellLogTrackStackedCurves::updateStackedCurveData()
{
    // See RimSummaryPlot::updateStackedCurveDataForAxis() horizontal plots

    RimDepthTrackPlot* wellLogPlot = m_track->firstAncestorOrThisOfTypeAsserted<RimDepthTrackPlot>();

    RimWellLogPlot::DepthTypeEnum depthType   = wellLogPlot->depthType();
    RiaDefines::DepthUnitType     displayUnit = wellLogPlot->depthUnit();
    if ( depthType == RiaDefines::DepthTypeEnum::CONNECTION_NUMBER )
    {
        displayUnit = RiaDefines::DepthUnitType::UNIT_NONE;
    }

    std::map<RiaDefines::PhaseType, size_t> curvePhaseCount;

    // Stack the curves that are meant to be stacked
    std::map<int, std::vector<RimWellLogCurve*>> stackedCurves = visibleStackedCurves();

    // Reset all stacked curves
    for ( auto groupCurvePair : stackedCurves )
    {
        const std::vector<RimWellLogCurve*>& stackedCurvesInGroup = groupCurvePair.second;
        for ( auto curve : stackedCurvesInGroup )
        {
            curve->loadDataAndUpdate( false );
            curvePhaseCount[curve->phaseType()]++;
        }
    }

    for ( auto groupCurvePair : stackedCurves )
    {
        int                                  groupId              = groupCurvePair.first;
        const std::vector<RimWellLogCurve*>& stackedCurvesInGroup = groupCurvePair.second;
        if ( stackedCurvesInGroup.empty() ) continue;

        // Z-position of curve, to draw them in correct order
        double zPos = -10000.0 + 100.0 * static_cast<double>( groupId );

        // We use the depths from the curve with the largest depth range.
        // Trying to merge them is difficult since they may not be in order.
        std::pair<double, double> maxDepthRange;
        std::vector<double>       allDepthValues;

        for ( auto curve : stackedCurvesInGroup )
        {
            auto depths = curve->curveData()->depths( depthType );
            if ( depths.empty() ) continue;

            if ( allDepthValues.empty() )
            {
                auto minmaxit = std::minmax_element( depths.begin(), depths.end() );
                maxDepthRange = std::make_pair( *minmaxit.first, *minmaxit.second );
                allDepthValues.insert( allDepthValues.end(), depths.begin(), depths.end() );
            }
            else
            {
                auto                      minmaxit   = std::minmax_element( depths.begin(), depths.end() );
                std::pair<double, double> depthRange = std::make_pair( *minmaxit.first, *minmaxit.second );
                if ( std::fabs( depthRange.second - depthRange.first ) > std::fabs( maxDepthRange.second - maxDepthRange.first ) )
                {
                    maxDepthRange  = depthRange;
                    allDepthValues = depths;
                }
            }
        }

        if ( allDepthValues.empty() ) continue;

        size_t              stackIndex = 0u;
        std::vector<double> allStackedValues( allDepthValues.size(), 0.0 );
        for ( auto curve : stackedCurvesInGroup )
        {
            auto interpolatedCurveValues = curve->curveData()->calculateResampledCurveData( depthType, allDepthValues );
            auto xValues                 = interpolatedCurveValues->propertyValues();
            for ( size_t i = 0; i < xValues.size(); ++i )
            {
                if ( xValues[i] != HUGE_VAL )
                {
                    allStackedValues[i] += xValues[i];
                }
            }

            RigWellLogCurveData tempCurveData;
            tempCurveData.setValuesAndDepths( allStackedValues, allDepthValues, depthType, 0.0, displayUnit, false, m_track->isLogarithmicScale() );

            auto plotDepthValues          = tempCurveData.depths( depthType );
            auto polyLineStartStopIndices = tempCurveData.polylineStartStopIndices();

            curve->setOverrideCurveData( allStackedValues, plotDepthValues, polyLineStartStopIndices );
            curve->setZOrder( zPos );

            if ( !dynamic_cast<RimWellFlowRateCurve*>( curve ) )
            {
                // Apply a area filled style if it isn't already set
                if ( curve->fillStyle() == Qt::NoBrush )
                {
                    curve->setFillStyle( Qt::SolidPattern );
                }

                if ( curve->isStackedWithPhaseColors() )
                {
                    curve->assignStackColor( stackIndex, curvePhaseCount[curve->phaseType()] );
                }
            }
            zPos -= 1.0;
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::map<int, std::vector<RimWellLogCurve*>> RimWellLogTrackStackedCurves::visibleStackedCurves() const
{
    std::map<int, std::vector<RimWellLogCurve*>> stackedCurves;

    for ( RimWellLogCurve* curve : m_track->curves() )
    {
        if ( curve && curve->isChecked() )
        {
            RimWellFlowRateCurve* wfrCurve = dynamic_cast<RimWellFlowRateCurve*>( curve );
            if ( wfrCurve != nullptr ) // Flow rate curves are always stacked
            {
                stackedCurves[wfrCurve->groupId()].push_back( wfrCurve );
            }
            else if ( curve->isStacked() )
            {
                stackedCurves[-1].push_back( curve );
            }
        }
    }

    return stackedCurves;
}
