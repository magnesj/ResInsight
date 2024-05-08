/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2024     Equinor ASA
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

#include "RigVfpTables.h"

#include "RiaOpmParserTools.h"

#include "RiaEclipseUnitTools.h"
#include "cafAppEnum.h"
#include "opm/input/eclipse/Schedule/VFPInjTable.hpp"
#include "opm/input/eclipse/Schedule/VFPProdTable.hpp"

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigVfpTables::clearTables()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
VfpPlotData RigVfpTables::populatePlotData( const Opm::VFPInjTable&                 table,
                                            RimVfpDefines::InterpolatedVariableType interpolatedVariable,
                                            RimVfpDefines::FlowingPhaseType         flowingPhase )
{
    VfpPlotData plotData;

    QString xAxisTitle = axisTitle( RimVfpDefines::ProductionVariableType::FLOW_RATE, flowingPhase );
    plotData.setXAxisTitle( xAxisTitle );

    QString yAxisTitle = QString( "%1 %2" ).arg( caf::AppEnum<RimVfpDefines::InterpolatedVariableType>::uiText( interpolatedVariable ),
                                                 getDisplayUnitWithBracket( RimVfpDefines::ProductionVariableType::THP ) );
    plotData.setYAxisTitle( yAxisTitle );

    std::vector<double> thpValues = table.getTHPAxis();

    for ( size_t thp = 0; thp < thpValues.size(); thp++ )
    {
        size_t              numValues = table.getFloAxis().size();
        std::vector<double> xVals     = table.getFloAxis();
        std::vector<double> yVals( numValues, 0.0 );
        for ( size_t y = 0; y < numValues; y++ )
        {
            yVals[y] = table( thp, y );
            if ( interpolatedVariable == RimVfpDefines::InterpolatedVariableType::BHP_THP_DIFF )
            {
                yVals[y] -= thpValues[thp];
            }
        }

        double  value = convertToDisplayUnit( thpValues[thp], RimVfpDefines::ProductionVariableType::THP );
        QString unit  = getDisplayUnit( RimVfpDefines::ProductionVariableType::THP );
        QString title = QString( "%1 [%2]: %3" )
                            .arg( caf::AppEnum<RimVfpDefines::ProductionVariableType>::uiText( RimVfpDefines::ProductionVariableType::THP ) )
                            .arg( unit )
                            .arg( value );

        convertToDisplayUnit( yVals, RimVfpDefines::ProductionVariableType::THP );
        convertToDisplayUnit( xVals, RimVfpDefines::ProductionVariableType::FLOW_RATE );

        plotData.appendCurve( title, xVals, yVals );
    }

    return plotData;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
VfpPlotData RigVfpTables::populatePlotData( const Opm::VFPProdTable&                table,
                                            RimVfpDefines::ProductionVariableType   primaryVariable,
                                            RimVfpDefines::ProductionVariableType   familyVariable,
                                            RimVfpDefines::InterpolatedVariableType interpolatedVariable,
                                            RimVfpDefines::FlowingPhaseType         flowingPhase,
                                            const VfpTableSelection&                tableSelection )
{
    VfpPlotData plotData;

    QString xAxisTitle = axisTitle( primaryVariable, flowingPhase );
    plotData.setXAxisTitle( xAxisTitle );

    QString yAxisTitle = QString( "%1 %2" ).arg( caf::AppEnum<RimVfpDefines::InterpolatedVariableType>::uiText( interpolatedVariable ),
                                                 getDisplayUnitWithBracket( RimVfpDefines::ProductionVariableType::THP ) );
    plotData.setYAxisTitle( yAxisTitle );

    size_t numFamilyValues = getProductionTableData( table, familyVariable ).size();
    for ( size_t familyIdx = 0; familyIdx < numFamilyValues; familyIdx++ )
    {
        std::vector<double> primaryAxisValues    = getProductionTableData( table, primaryVariable );
        std::vector<double> familyVariableValues = getProductionTableData( table, familyVariable );
        std::vector<double> thpValues            = getProductionTableData( table, RimVfpDefines::ProductionVariableType::THP );

        size_t              numValues = primaryAxisValues.size();
        std::vector<double> yVals( numValues, 0.0 );

        for ( size_t y = 0; y < numValues; y++ )
        {
            size_t wfr_idx =
                getVariableIndex( table, RimVfpDefines::ProductionVariableType::WATER_CUT, primaryVariable, y, familyVariable, familyIdx, tableSelection );
            size_t gfr_idx = getVariableIndex( table,
                                               RimVfpDefines::ProductionVariableType::GAS_LIQUID_RATIO,
                                               primaryVariable,
                                               y,
                                               familyVariable,
                                               familyIdx,
                                               tableSelection );
            size_t alq_idx = getVariableIndex( table,
                                               RimVfpDefines::ProductionVariableType::ARTIFICIAL_LIFT_QUANTITY,
                                               primaryVariable,
                                               y,
                                               familyVariable,
                                               familyIdx,
                                               tableSelection );
            size_t flo_idx =
                getVariableIndex( table, RimVfpDefines::ProductionVariableType::FLOW_RATE, primaryVariable, y, familyVariable, familyIdx, tableSelection );
            size_t thp_idx =
                getVariableIndex( table, RimVfpDefines::ProductionVariableType::THP, primaryVariable, y, familyVariable, familyIdx, tableSelection );

            yVals[y] = table( thp_idx, wfr_idx, gfr_idx, alq_idx, flo_idx );
            if ( interpolatedVariable == RimVfpDefines::InterpolatedVariableType::BHP_THP_DIFF )
            {
                yVals[y] -= thpValues[thp_idx];
            }
        }

        double  familyValue = convertToDisplayUnit( familyVariableValues[familyIdx], familyVariable );
        QString familyUnit  = getDisplayUnit( familyVariable );
        QString familyTitle = QString( "%1: %2 %3" )
                                  .arg( caf::AppEnum<RimVfpDefines::ProductionVariableType>::uiText( familyVariable ) )
                                  .arg( familyValue )
                                  .arg( familyUnit );

        convertToDisplayUnit( yVals, RimVfpDefines::ProductionVariableType::THP );
        convertToDisplayUnit( primaryAxisValues, primaryVariable );

        plotData.appendCurve( familyTitle, primaryAxisValues, yVals );
    }

    return plotData;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RigVfpTables::axisTitle( RimVfpDefines::ProductionVariableType variableType, RimVfpDefines::FlowingPhaseType flowingPhase )
{
    QString title;

    if ( flowingPhase == RimVfpDefines::FlowingPhaseType::GAS )
    {
        title = "Gas ";
    }
    else
    {
        title = "Liquid ";
    }
    title += QString( "%1 %2" ).arg( caf::AppEnum<RimVfpDefines::ProductionVariableType>::uiText( variableType ),
                                     getDisplayUnitWithBracket( variableType ) );

    return title;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RigVfpTables::getDisplayUnit( RimVfpDefines::ProductionVariableType variableType )
{
    if ( variableType == RimVfpDefines::ProductionVariableType::THP ) return "Bar";

    if ( variableType == RimVfpDefines::ProductionVariableType::FLOW_RATE ) return "Sm3/day";

    return "";
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RigVfpTables::getDisplayUnitWithBracket( RimVfpDefines::ProductionVariableType variableType )
{
    QString unit = getDisplayUnit( variableType );
    if ( !unit.isEmpty() ) return QString( "[%1]" ).arg( unit );

    return {};
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
double RigVfpTables::convertToDisplayUnit( double value, RimVfpDefines::ProductionVariableType variableType )
{
    if ( variableType == RimVfpDefines::ProductionVariableType::THP )
    {
        return RiaEclipseUnitTools::pascalToBar( value );
    }

    if ( variableType == RimVfpDefines::ProductionVariableType::FLOW_RATE )
    {
        // Convert to m3/sec to m3/day
        return value * static_cast<double>( 24 * 60 * 60 );
    }

    return value;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigVfpTables::convertToDisplayUnit( std::vector<double>& values, RimVfpDefines::ProductionVariableType variableType )
{
    for ( double& value : values )
        value = convertToDisplayUnit( value, variableType );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RigVfpTables::textForPlotData( const VfpPlotData& plotData )
{
    QString dataText;

    if ( plotData.size() > 0 )
    {
        // The curves should have same dimensions
        const size_t curveSize = plotData.curveSize( 0 );

        // Generate the headers for the columns
        // First column is the primary variable
        QString columnTitleLine( plotData.xAxisTitle() );

        // Then one column per "family"
        for ( size_t s = 0; s < plotData.size(); s++ )
        {
            columnTitleLine.append( QString( "\t%1" ).arg( plotData.curveTitle( s ) ) );
        }
        columnTitleLine.append( "\n" );

        dataText.append( columnTitleLine );

        // Add the rows: one row per primary variable value
        for ( size_t idx = 0; idx < curveSize; idx++ )
        {
            QString line;

            // First item on each line is the primary variable
            line.append( QString( "%1" ).arg( plotData.xData( 0 )[idx] ) );

            for ( size_t s = 0; s < plotData.size(); s++ )
            {
                line.append( QString( "\t%1" ).arg( plotData.yData( s )[idx] ) );
            }
            dataText.append( line );
            dataText.append( "\n" );
        }
    }

    return dataText;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<double> RigVfpTables::getProductionTableData( const Opm::VFPProdTable& table, RimVfpDefines::ProductionVariableType variableType )
{
    std::vector<double> xVals;
    if ( variableType == RimVfpDefines::ProductionVariableType::WATER_CUT )
    {
        xVals = table.getWFRAxis();
    }
    else if ( variableType == RimVfpDefines::ProductionVariableType::GAS_LIQUID_RATIO )
    {
        xVals = table.getGFRAxis();
    }
    else if ( variableType == RimVfpDefines::ProductionVariableType::ARTIFICIAL_LIFT_QUANTITY )
    {
        xVals = table.getALQAxis();
    }
    else if ( variableType == RimVfpDefines::ProductionVariableType::FLOW_RATE )
    {
        xVals = table.getFloAxis();
    }
    else if ( variableType == RimVfpDefines::ProductionVariableType::THP )
    {
        xVals = table.getTHPAxis();
    }

    return xVals;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
size_t RigVfpTables::getVariableIndex( const Opm::VFPProdTable&              table,
                                       RimVfpDefines::ProductionVariableType targetVariable,
                                       RimVfpDefines::ProductionVariableType primaryVariable,
                                       size_t                                primaryValue,
                                       RimVfpDefines::ProductionVariableType familyVariable,
                                       size_t                                familyValue,
                                       const VfpTableSelection&              tableSelection )
{
    if ( targetVariable == primaryVariable ) return primaryValue;
    if ( targetVariable == familyVariable ) return familyValue;
    if ( targetVariable == RimVfpDefines::ProductionVariableType::WATER_CUT ) return tableSelection.m_waterCutIdx;
    if ( targetVariable == RimVfpDefines::ProductionVariableType::GAS_LIQUID_RATIO ) return tableSelection.m_gasLiquidRatioIdx;
    if ( targetVariable == RimVfpDefines::ProductionVariableType::ARTIFICIAL_LIFT_QUANTITY )
        return tableSelection.m_articifialLiftQuantityIdx;
    if ( targetVariable == RimVfpDefines::ProductionVariableType::FLOW_RATE ) return tableSelection.m_flowRateIdx;
    if ( targetVariable == RimVfpDefines::ProductionVariableType::THP ) return tableSelection.m_thpIdx;

    return getProductionTableData( table, targetVariable ).size() - 1;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::optional<Opm::VFPInjTable> RigVfpTables::injectionTable( int tableNumber ) const
{
    for ( const auto& table : m_injectionTables )
    {
        if ( table.getTableNum() == tableNumber )
        {
            return table;
        }
    }

    return std::nullopt;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::optional<Opm::VFPProdTable> RigVfpTables::productionTable( int tableNumber ) const
{
    for ( const auto& table : m_productionTables )
    {
        if ( table.getTableNum() == tableNumber )
        {
            return table;
        }
    }

    return std::nullopt;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigVfpTables::importFromTextFiles( const std::vector<std::string>& filenames )
{
    for ( const std::string& filePath : filenames )
    {
        const std::vector<Opm::VFPInjTable> injTables = RiaOpmParserTools::extractVfpInjectionTables( filePath );
        m_injectionTables.insert( m_injectionTables.end(), injTables.begin(), injTables.end() );

        const std::vector<Opm::VFPProdTable> prodTables = RiaOpmParserTools::extractVfpProductionTables( filePath );
        m_productionTables.insert( m_productionTables.end(), prodTables.begin(), prodTables.end() );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigVfpTables::importFromSimulatorInputFile( const std::string& filename )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigVfpTables::addInjectionTable( const Opm::VFPInjTable& table )
{
    m_injectionTables.push_back( table );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RigVfpTables::addProductionTable( const Opm::VFPProdTable& table )
{
    m_productionTables.push_back( table );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RigVfpTables::asciiDataForProductionTable( int                                     tableNumber,
                                                   RimVfpDefines::ProductionVariableType   primaryVariable,
                                                   RimVfpDefines::ProductionVariableType   familyVariable,
                                                   RimVfpDefines::InterpolatedVariableType interpolatedVariable,
                                                   RimVfpDefines::FlowingPhaseType         flowingPhase,
                                                   const VfpTableSelection&                tableSelection ) const
{
    QString wellName = "Injection table well";

    auto tableContainer = productionTable( tableNumber );
    if ( !tableContainer.has_value() ) return {};

    auto plotData = populatePlotData( *tableContainer, primaryVariable, familyVariable, interpolatedVariable, flowingPhase, tableSelection );

    return textForPlotData( plotData );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RigVfpTables::asciiDataForInjectionTable( int                                     tableNumber,
                                                  RimVfpDefines::InterpolatedVariableType interpolatedVariable,
                                                  RimVfpDefines::FlowingPhaseType         flowingPhase ) const
{
    QString wellName = "Injection table well";

    auto injTable = injectionTable( tableNumber );
    if ( !injTable.has_value() ) return {};

    auto plotData = populatePlotData( *injTable, interpolatedVariable, flowingPhase );

    return textForPlotData( plotData );
}
