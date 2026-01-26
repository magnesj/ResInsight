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

#include "RimWellLogTrackPropertyAxis.h"

#include "RimWellLogTrack.h"
#include "RimWellLogTrackTools.h"

#include "RiaDefines.h"
#include "RiaPreferences.h"

#include "RigStatisticsCalculator.h"
#include "Well/RigWellLogCurveData.h"

#include "RimDepthTrackPlot.h"
#include "RimRftTopologyCurve.h"
#include "RimWellLogCurve.h"
#include "RimWellLogPlot.h"

#include "RiuPlotAxis.h"
#include "RiuQwtLinearScaleEngine.h"
#include "RiuQwtPlotWidget.h"

#include "qwt_plot.h"
#include "qwt_scale_div.h"
#include "qwt_scale_engine.h"

#include <cmath>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimWellLogTrackPropertyAxis::RimWellLogTrackPropertyAxis( RimWellLogTrack* track )
    : m_track( track )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellLogTrackPropertyAxis::calculatePropertyValueZoomRange()
{
    m_track->updateStackedCurveData();

    double minValue = HUGE_VAL;
    double maxValue = -HUGE_VAL;

    std::vector<RimWellLogCurve*> curves = m_track->curves();

    size_t topologyCurveCount = 0;
    for ( const auto& curve : curves )
    {
        double minCurveValue = HUGE_VAL;
        double maxCurveValue = -HUGE_VAL;

        if ( curve->isChecked() )
        {
            if ( curve->propertyValueRangeInData( &minCurveValue, &maxCurveValue ) )
            {
                if ( minCurveValue < minValue )
                {
                    minValue = minCurveValue;
                }

                if ( maxCurveValue > maxValue )
                {
                    maxValue = maxCurveValue;
                }
            }
        }

        if ( dynamic_cast<RimRftTopologyCurve*>( curve ) ) topologyCurveCount++;
    }

    if ( topologyCurveCount == curves.size() )
    {
        // The topology track is quite narrow, and to be able to show the curves we add extra space for min/max values
        const double range = maxValue - minValue;
        maxValue += range * 0.5;
        minValue -= range * 0.5;
    }

    if ( minValue == HUGE_VAL )
    {
        // Empty axis when there are no sensible visible curves
        minValue = 0;
        maxValue = 0;
    }
    else if ( m_track->m_minorTickIntervalPropertyAxis != 0.0 )
    {
        std::tie( minValue, maxValue ) = RimWellLogTrackTools::adjustXRange( minValue, maxValue, m_track->m_minorTickIntervalPropertyAxis );
    }
    else
    {
        double adjustmentFactor         = 0.1;
        auto [adjustedMin, adjustedMax] = RimWellLogTrackTools::extendMinMaxRange( minValue, maxValue, adjustmentFactor );

        minValue = adjustedMin;
        maxValue = adjustedMax;
    }

    m_track->m_availablePropertyValueRangeMin = minValue;
    m_track->m_availablePropertyValueRangeMax = maxValue;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellLogTrackPropertyAxis::updatePropertyValueZoom()
{
    RiuQwtPlotWidget* plotWidget = m_track->viewer();
    if ( !plotWidget ) return;

    calculatePropertyValueZoomRange();

    double availableMin = 0.0;
    double availableMax = 0.0;
    m_track->availablePropertyValueRange( &availableMin, &availableMax );

    if ( m_track->m_isAutoScalePropertyValuesEnabled )
    {
        double visibleMin = availableMin;
        double visibleMax = availableMax;

        if ( !m_track->visibleStackedCurves().empty() && !m_track->isLogarithmicScale() )
        {
            // Try to ensure we include the base line whether the values are negative or positive.
            visibleMin = std::min( visibleMin, 0.0 );
            visibleMax = std::max( visibleMax, 0.0 );
        }

        m_track->setVisiblePropertyValueRange( visibleMin, visibleMax );
        m_track->setAutoScalePropertyValuesEnabled( true ); // Re-enable since setVisiblePropertyValueRange disables it

        computeAndSetPropertyValueRangeMinForLogarithmicScale();
        m_track->updateEditors();
    }

    updatePropertyValueAxisAndGridTickIntervals();

    // Attribute range. Fixed range where well components are positioned [-1, 1].
    // Set an extended range here to allow for some label space.
    double componentRangeMax = 2.0 / ( static_cast<double>( m_track->colSpan() ) );
    double componentRangeMin = -0.25;
    if ( m_track->m_showWellPathComponentsBothSides )
    {
        componentRangeMin = -1.5;
        componentRangeMax *= 2.0;
    }
    if ( m_track->m_showWellPathComponentLabels )
    {
        componentRangeMax *= 1.5;
    }

    RimDepthTrackPlot* wellLogPlot = m_track->firstAncestorOrThisOfTypeAsserted<RimDepthTrackPlot>();

    // Attribute components use the opposite axis to the property values
    if ( wellLogPlot->depthOrientation() == RiaDefines::Orientation::VERTICAL )
    {
        plotWidget->setAxisRange( RiuPlotAxis::defaultBottom(), componentRangeMin, componentRangeMax );
    }
    else
    {
        plotWidget->setAxisRange( RiuPlotAxis::defaultRight(), componentRangeMin, componentRangeMax );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellLogTrackPropertyAxis::updatePropertyValueAxisAndGridTickIntervals()
{
    RiuQwtPlotWidget* plotWidget = m_track->viewer();
    if ( !plotWidget ) return;

    RiuPlotAxis valueAxis = m_track->valueAxis();
    RiuPlotAxis depthAxis = m_track->depthAxis();

    bool emptyRange =
        std::abs( m_track->m_visiblePropertyValueRangeMax - m_track->m_visiblePropertyValueRangeMin ) <
        1.0e-6 * std::max( 1.0, std::max( m_track->m_visiblePropertyValueRangeMax.value(), m_track->m_visiblePropertyValueRangeMin.value() ) );
    if ( emptyRange )
    {
        plotWidget->enableGridLines( valueAxis, false, false );
        plotWidget->setAxisRange( valueAxis, 0.0, 1.0 );
        plotWidget->setAxisLabelsAndTicksEnabled( valueAxis, false, false );
    }
    else
    {
        plotWidget->setAxisLabelsAndTicksEnabled( valueAxis, true, true );

        double visibleMin = 0.0;
        double visibleMax = 0.0;
        m_track->visiblePropertyValueRange( &visibleMin, &visibleMax );

        auto rangeBoundaryA = visibleMin;
        auto rangeBoundaryB = visibleMax;
        if ( m_track->m_invertPropertyValueAxis ) std::swap( rangeBoundaryA, rangeBoundaryB );

        if ( m_track->m_propertyAxisMinAndMaxTicksOnly )
        {
            auto roundToDigits = []( double value, int numberOfDigits, bool useFloor )
            {
                if ( value == 0.0 ) return 0.0;

                double factor = std::pow( 10.0, numberOfDigits - std::ceil( std::log10( std::fabs( value ) ) ) );

                if ( useFloor )
                {
                    // Use floor for maximum value to ensure we get a value inside the complete range
                    return std::floor( value * factor ) / factor;
                }

                // Use ceil for minimum value to ensure we get a value inside the complete range
                return std::ceil( value * factor ) / factor;
            };

            auto div = QwtScaleDiv( rangeBoundaryA, rangeBoundaryB );

            QList<double> majorTicks;

            auto min = roundToDigits( rangeBoundaryA, 2, false );
            auto max = roundToDigits( rangeBoundaryB, 2, true );
            if ( min == max )
            {
                min = roundToDigits( rangeBoundaryA, 3, false );
                max = roundToDigits( rangeBoundaryB, 3, true );
            }

            majorTicks.push_back( min );
            majorTicks.push_back( max );

            div.setTicks( QwtScaleDiv::TickType::MajorTick, majorTicks );

            RimDepthTrackPlot* wellLogPlot = m_track->firstAncestorOrThisOfTypeAsserted<RimDepthTrackPlot>();
            if ( wellLogPlot->depthOrientation() == RiaDefines::Orientation::VERTICAL )
            {
                plotWidget->qwtPlot()->setAxisScaleDiv( QwtAxis::XTop, div );
            }
            else
            {
                plotWidget->qwtPlot()->setAxisScaleDiv( QwtAxis::YLeft, div );
            }
        }
        else if ( m_track->m_explicitTickIntervalsPropertyValueAxis )
        {
            plotWidget->setMajorAndMinorTickIntervals( valueAxis,
                                                       m_track->m_majorTickIntervalPropertyAxis,
                                                       m_track->m_minorTickIntervalPropertyAxis,
                                                       rangeBoundaryA,
                                                       rangeBoundaryB );
        }
        else
        {
            int majorTickIntervals = 5;
            int minorTickIntervals = 10;
            plotWidget->setAutoTickIntervalCounts( valueAxis, majorTickIntervals, minorTickIntervals );
            plotWidget->setAxisRange( valueAxis, rangeBoundaryA, rangeBoundaryB );
        }

        plotWidget->enableGridLines( valueAxis,
                                     m_track->m_propertyValueAxisGridVisibility.value() & RimWellLogPlot::AXIS_GRID_MAJOR,
                                     m_track->m_propertyValueAxisGridVisibility.value() & RimWellLogPlot::AXIS_GRID_MINOR );
    }

    RimDepthTrackPlot* wellLogPlot = m_track->firstAncestorOrThisOfType<RimDepthTrackPlot>();
    if ( wellLogPlot )
    {
        plotWidget->enableGridLines( depthAxis,
                                     wellLogPlot->depthAxisGridLinesEnabled() & RimWellLogPlot::AXIS_GRID_MAJOR,
                                     wellLogPlot->depthAxisGridLinesEnabled() & RimWellLogPlot::AXIS_GRID_MINOR );
    }

    plotWidget->enableAxisNumberLabels( valueAxis, m_track->isPropertyAxisEnabled() );

    plotWidget->scheduleReplot();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellLogTrackPropertyAxis::updateAxisScaleEngine()
{
    RiuQwtPlotWidget* plotWidget = m_track->viewer();
    if ( !plotWidget ) return;

    auto wellLogPlot = m_track->firstAncestorOrThisOfType<RimDepthTrackPlot>();
    if ( wellLogPlot )
    {
        if ( wellLogPlot->depthOrientation() == RiaDefines::Orientation::VERTICAL )
        {
            plotWidget->setAxisInverted( RiuPlotAxis::defaultLeft(), true );

            if ( m_track->isLogarithmicScale() )
            {
                plotWidget->qwtPlot()->setAxisScaleEngine( QwtAxis::XTop, new QwtLogScaleEngine );

                // NB! Must assign scale engine to bottom in order to make QwtPlotGrid work
                plotWidget->qwtPlot()->setAxisScaleEngine( QwtAxis::XBottom, new QwtLogScaleEngine );
            }
            else
            {
                plotWidget->qwtPlot()->setAxisScaleEngine( QwtAxis::XTop, new RiuQwtLinearScaleEngine );

                // NB! Must assign scale engine to bottom in order to make QwtPlotGrid work
                plotWidget->qwtPlot()->setAxisScaleEngine( QwtAxis::XBottom, new RiuQwtLinearScaleEngine );
            }
        }
        else
        {
            plotWidget->setAxisInverted( RiuPlotAxis::defaultLeft(), false );

            if ( m_track->isLogarithmicScale() )
            {
                plotWidget->qwtPlot()->setAxisScaleEngine( QwtAxis::YLeft, new QwtLogScaleEngine );

                // NB! Must assign scale engine to bottom in order to make QwtPlotGrid work
                plotWidget->qwtPlot()->setAxisScaleEngine( QwtAxis::YRight, new QwtLogScaleEngine );
            }
            else
            {
                plotWidget->qwtPlot()->setAxisScaleEngine( QwtAxis::YLeft, new RiuQwtLinearScaleEngine );

                // NB! Must assign scale engine to bottom in order to make QwtPlotGrid work
                plotWidget->qwtPlot()->setAxisScaleEngine( QwtAxis::YRight, new RiuQwtLinearScaleEngine );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellLogTrackPropertyAxis::computeAndSetPropertyValueRangeMinForLogarithmicScale()
{
    if ( m_track->m_isAutoScalePropertyValuesEnabled && m_track->isLogarithmicScale() )
    {
        double pos = HUGE_VAL;
        double neg = -HUGE_VAL;

        for ( const auto& curve : m_track->curves() )
        {
            if ( curve->isChecked() && curve->curveData() )
            {
                RigStatisticsCalculator::posNegClosestToZero( curve->curveData()->propertyValuesByIntervals(), pos, neg );
            }
        }

        if ( pos != HUGE_VAL )
        {
            double visibleMin = 0.0;
            double visibleMax = 0.0;
            m_track->visiblePropertyValueRange( &visibleMin, &visibleMax );
            m_track->setVisiblePropertyValueRange( pos, visibleMax );
            m_track->setAutoScalePropertyValuesEnabled( true ); // Re-enable since setVisiblePropertyValueRange disables it
        }
    }
}
