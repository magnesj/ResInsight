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
/////////////////////////////////////////////////////////////////////////////////

#include "RimParameterRftCrossPlot.h"

#include "RiaColorTables.h"
#include "RiaPreferences.h"

#include "RifEclipseRftAddress.h"
#include "RifReaderRftInterface.h"

#include "RigEnsembleParameter.h"

#include "RimSummaryCase.h"
#include "RimSummaryEnsemble.h"

#include "RiuContextMenuLauncher.h"
#include "RiuPlotCurve.h"
#include "RiuQwtPlotCurve.h"
#include "RiuQwtPlotWidget.h"
#include "RiuQwtSymbol.h"

#include "cafPdmUiComboBoxEditor.h"

#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_marker.h"
#include "qwt_text.h"

#include <QPaintDevice>

#include <limits>
#include <numeric>

CAF_PDM_SOURCE_INIT( RimParameterRftCrossPlot, "ParameterRftCrossPlot" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimParameterRftCrossPlot::RimParameterRftCrossPlot()
{
    CAF_PDM_InitObject( "Parameter RFT Cross Plot", ":/CorrelationCrossPlot16x16.png" );

    CAF_PDM_InitFieldNoDefault( &m_ensemble, "Ensemble", "Ensemble" );
    CAF_PDM_InitField( &m_wellName, "WellName", QString(), "Well Name" );
    CAF_PDM_InitFieldNoDefault( &m_selectedTimeStep, "TimeStep", "Time Step" );
    CAF_PDM_InitField( &m_depthRangeMin, "DepthRangeMin", 0.0, "Min Depth (MD)" );
    CAF_PDM_InitField( &m_depthRangeMax, "DepthRangeMax", 5000.0, "Max Depth (MD)" );
    CAF_PDM_InitField( &m_ensembleParameter, "EnsembleParameter", QString(), "Ensemble Parameter" );
    m_ensembleParameter.uiCapability()->setUiEditorTypeName( caf::PdmUiComboBoxEditor::uiEditorTypeName() );

    CAF_PDM_InitField( &m_useAutoPlotTitle, "UseAutoPlotTitle", true, "Auto Title" );
    CAF_PDM_InitField( &m_description, "Description", QString( "RFT Cross Plot" ), "Title" );

    CAF_PDM_InitFieldNoDefault( &m_axisTitleFontSize, "AxisTitleFontSize", "Axis Title Font Size" );
    CAF_PDM_InitFieldNoDefault( &m_axisValueFontSize, "AxisValueFontSize", "Axis Value Font Size" );

    m_axisTitleFontSize = caf::FontTools::RelativeSize::Small;
    m_axisValueFontSize = caf::FontTools::RelativeSize::Small;

    m_showPlotLegends = false;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimParameterRftCrossPlot::~RimParameterRftCrossPlot()
{
    cleanupBeforeClose();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimParameterRftCrossPlot::setEnsemble( RimSummaryEnsemble* ensemble )
{
    m_ensemble = ensemble;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimParameterRftCrossPlot::setWellName( const QString& wellName )
{
    m_wellName = wellName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimParameterRftCrossPlot::setTimeStep( const QDateTime& timeStep )
{
    m_selectedTimeStep = timeStep;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimParameterRftCrossPlot::setDepthRange( double minMd, double maxMd )
{
    m_depthRangeMin = minMd;
    m_depthRangeMax = maxMd;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimParameterRftCrossPlot::setEnsembleParameter( const QString& paramName )
{
    m_ensembleParameter = paramName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimParameterRftCrossPlot::ensembleParameter() const
{
    return m_ensembleParameter;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimParameterRftCrossPlot::wellName() const
{
    return m_wellName;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiuQwtPlotWidget* RimParameterRftCrossPlot::viewer()
{
    return m_plotWidget;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RimParameterRftCrossPlot::CaseData> RimParameterRftCrossPlot::createCaseData() const
{
    if ( !m_ensemble() ) return {};
    if ( m_wellName().isEmpty() ) return {};
    if ( !m_selectedTimeStep().isValid() ) return {};
    if ( m_ensembleParameter().isEmpty() ) return {};

    RigEnsembleParameter parameter = m_ensemble->ensembleParameter( m_ensembleParameter );
    if ( !parameter.isNumeric() || !parameter.isValid() ) return {};

    const auto& allCases = m_ensemble->allSummaryCases();

    std::vector<CaseData> result;
    result.reserve( allCases.size() );

    for ( size_t caseIdx = 0; caseIdx < allCases.size(); ++caseIdx )
    {
        RimSummaryCase* summaryCase = allCases[caseIdx];
        if ( !summaryCase ) continue;

        RifReaderRftInterface* reader = summaryCase->rftReader();
        if ( !reader ) continue;

        auto mdAddress =
            RifEclipseRftAddress::createAddress( m_wellName(), m_selectedTimeStep(), RifEclipseRftAddress::RftWellLogChannelType::MD );
        auto pressureAddress =
            RifEclipseRftAddress::createAddress( m_wellName(), m_selectedTimeStep(), RifEclipseRftAddress::RftWellLogChannelType::PRESSURE );

        std::vector<double> depths;
        std::vector<double> pressures;
        reader->values( mdAddress, &depths );
        reader->values( pressureAddress, &pressures );

        if ( depths.empty() || pressures.empty() || depths.size() != pressures.size() ) continue;

        // Collect pressure samples within [depthRangeMin, depthRangeMax]
        std::vector<double> samplesInRange;
        for ( size_t i = 0; i < depths.size(); ++i )
        {
            if ( depths[i] >= m_depthRangeMin() && depths[i] <= m_depthRangeMax() )
            {
                samplesInRange.push_back( pressures[i] );
            }
        }

        if ( samplesInRange.empty() ) continue;

        double meanPressure = std::accumulate( samplesInRange.begin(), samplesInRange.end(), 0.0 ) / samplesInRange.size();

        if ( caseIdx < static_cast<size_t>( parameter.values.size() ) )
        {
            result.push_back( { .parameterValue = parameter.values[caseIdx].toDouble(),
                                .pressureValue  = meanPressure,
                                .summaryCase    = summaryCase } );
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiuPlotWidget* RimParameterRftCrossPlot::plotWidget()
{
    return m_plotWidget;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimParameterRftCrossPlot::updateAxes()
{
    if ( !m_plotWidget ) return;

    const int axisTitleSize = caf::FontTools::absolutePointSize( RiaPreferences::current()->defaultPlotFontSize(), m_axisTitleFontSize() );
    const int axisValueSize = caf::FontTools::absolutePointSize( RiaPreferences::current()->defaultPlotFontSize(), m_axisValueFontSize() );

    const QString depthLabel = QString( "Mean Pressure [MD %1 - %2]" ).arg( m_depthRangeMin() ).arg( m_depthRangeMax() );

    m_plotWidget->setAxisTitleText( RiuPlotAxis::defaultLeft(), depthLabel );
    m_plotWidget->setAxisTitleEnabled( RiuPlotAxis::defaultLeft(), true );
    m_plotWidget->setAxisFontsAndAlignment( RiuPlotAxis::defaultLeft(), axisTitleSize, axisValueSize, false, Qt::AlignCenter );

    if ( m_yValueRange.first != std::numeric_limits<double>::infinity() )
    {
        m_plotWidget->setAxisRange( RiuPlotAxis::defaultLeft(), m_yValueRange.first, m_yValueRange.second );
    }

    m_plotWidget->setAxisTitleText( RiuPlotAxis::defaultBottom(), m_ensembleParameter() );
    m_plotWidget->setAxisTitleEnabled( RiuPlotAxis::defaultBottom(), true );
    m_plotWidget->setAxisFontsAndAlignment( RiuPlotAxis::defaultBottom(), axisTitleSize, axisValueSize, false, Qt::AlignCenter );

    if ( m_xValueRange.first != std::numeric_limits<double>::infinity() )
    {
        m_plotWidget->setAxisRange( RiuPlotAxis::defaultBottom(), m_xValueRange.first, m_xValueRange.second );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimParameterRftCrossPlot::asciiDataForPlotExport() const
{
    QString asciiData;
    asciiData += "Realization\tParameter\tMean Pressure\n";
    for ( const auto& [paramValue, pressureValue, summaryCase] : createCaseData() )
    {
        asciiData += QString( "%1\t%2\t%3\n" ).arg( summaryCase->displayCaseName() ).arg( paramValue ).arg( pressureValue );
    }
    return asciiData;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimParameterRftCrossPlot::detachAllCurves()
{
    if ( m_plotWidget ) m_plotWidget->qwtPlot()->detachItems();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimParameterRftCrossPlot::description() const
{
    return m_description();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QWidget* RimParameterRftCrossPlot::viewWidget()
{
    return m_plotWidget;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimParameterRftCrossPlot::deleteViewWidget()
{
    cleanupBeforeClose();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimParameterRftCrossPlot::doRenderWindowContent( QPaintDevice* paintDevice )
{
    if ( m_plotWidget ) m_plotWidget->render( paintDevice );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RiuPlotWidget* RimParameterRftCrossPlot::doCreatePlotViewWidget( QWidget* parent )
{
    if ( !m_plotWidget )
    {
        m_plotWidget = new RiuQwtPlotWidget( this, parent );
        updatePlotTitle();
        new RiuContextMenuLauncher( m_plotWidget, { "RicShowPlotDataFeature" } );
    }
    return m_plotWidget;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimParameterRftCrossPlot::onLoadDataAndUpdate()
{
    updateMdiWindowVisibility();

    if ( m_plotWidget )
    {
        createPoints();
        updateValueRanges();
        updateAxes();
        updatePlotTitle();
        m_plotWidget->scheduleReplot();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimParameterRftCrossPlot::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    auto* dataGroup = uiOrdering.addNewGroup( "Data" );
    dataGroup->add( &m_ensemble );
    dataGroup->add( &m_wellName );
    dataGroup->add( &m_selectedTimeStep );

    auto* depthGroup = uiOrdering.addNewGroup( "Depth Range" );
    depthGroup->add( &m_depthRangeMin );
    depthGroup->add( &m_depthRangeMax );

    auto* crossPlotGroup = uiOrdering.addNewGroup( "Cross Plot Parameter" );
    crossPlotGroup->add( &m_ensembleParameter );

    auto* plotGroup = uiOrdering.addNewGroup( "Plot Settings" );
    plotGroup->setCollapsedByDefault();
    plotGroup->add( &m_useAutoPlotTitle );
    plotGroup->add( &m_description );
    plotGroup->add( &m_axisTitleFontSize );
    plotGroup->add( &m_axisValueFontSize );

    m_description.uiCapability()->setUiReadOnly( m_useAutoPlotTitle() );

    uiOrdering.skipRemainingFields( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimParameterRftCrossPlot::fieldChangedByUi( const caf::PdmFieldHandle* changedField,
                                                  const QVariant&            oldValue,
                                                  const QVariant&            newValue )
{
    RimPlot::fieldChangedByUi( changedField, oldValue, newValue );
    loadDataAndUpdate();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QList<caf::PdmOptionItemInfo> RimParameterRftCrossPlot::calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions )
{
    QList<caf::PdmOptionItemInfo> options;
    if ( fieldNeedingOptions == &m_ensembleParameter )
    {
        if ( m_ensemble() )
        {
            for ( const auto& param : m_ensemble->variationSortedEnsembleParameters() )
            {
                if ( param.isNumeric() )
                {
                    options.push_back( caf::PdmOptionItemInfo( param.uiName(), param.name ) );
                }
            }
        }
    }
    return options;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimParameterRftCrossPlot::createPoints()
{
    detachAllCurves();

    caf::ColorTable colorTable = RiaColorTables::categoryPaletteColors();

    auto caseData = createCaseData();
    if ( caseData.empty() ) return;

    int idx = 0;
    for ( const auto& [paramValue, pressureValue, summaryCase] : caseData )
    {
        auto* plotCurve = new RiuQwtPlotCurve;
        plotCurve->setSamplesValues( { paramValue }, { pressureValue } );
        plotCurve->setStyle( QwtPlotCurve::NoCurve );

        auto* symbol = new RiuQwtSymbol( RiuPlotCurveSymbol::SYMBOL_ELLIPSE );
        symbol->setSize( 8, 8 );
        symbol->setColor( colorTable.cycledQColor( idx++ ) );
        plotCurve->setSymbol( symbol );

        plotCurve->setTitle( summaryCase->displayCaseName() );
        plotCurve->attach( m_plotWidget->qwtPlot() );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimParameterRftCrossPlot::updatePlotTitle()
{
    if ( !m_plotWidget ) return;

    if ( m_useAutoPlotTitle && m_ensemble() )
    {
        m_description = QString( "%1 vs RFT Pressure [%2 - %3 m], %4" )
                            .arg( m_ensembleParameter() )
                            .arg( m_depthRangeMin() )
                            .arg( m_depthRangeMax() )
                            .arg( m_ensemble->name() );
    }

    m_plotWidget->setPlotTitle( m_description() );
    m_plotWidget->setPlotTitleEnabled( m_showPlotTitle() );
    m_plotWidget->setPlotTitleFontSize( titleFontSize() );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimParameterRftCrossPlot::updateValueRanges()
{
    double xMin = std::numeric_limits<double>::infinity();
    double xMax = -std::numeric_limits<double>::infinity();
    double yMin = std::numeric_limits<double>::infinity();
    double yMax = -std::numeric_limits<double>::infinity();

    for ( const auto& [paramValue, pressureValue, summaryCase] : createCaseData() )
    {
        xMin = std::min( xMin, paramValue );
        xMax = std::max( xMax, paramValue );
        yMin = std::min( yMin, pressureValue );
        yMax = std::max( yMax, pressureValue );
    }

    if ( xMin == std::numeric_limits<double>::infinity() ) return;

    double xRange = xMax - xMin;
    double yRange = yMax - yMin;

    m_xValueRange = { xMin - xRange * 0.1, xMax + xRange * 0.1 };
    m_yValueRange = { yMin - yRange * 0.1, yMax + yRange * 0.1 };
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimParameterRftCrossPlot::cleanupBeforeClose()
{
    detachAllCurves();

    if ( m_plotWidget )
    {
        m_plotWidget->setParent( nullptr );
        delete m_plotWidget;
        m_plotWidget = nullptr;
    }
}
