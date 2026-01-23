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

#include "RimWellLogTrackTools.h"

#include "RimWellLogTrack.h"

#include "RiaSimWellBranchTools.h"

#include "RigEclipseCaseData.h"
#include "RigFemPartResultsCollection.h"
#include "RigFemResultAddress.h"
#include "RigGeoMechCaseData.h"
#include "Well/RigEclipseWellLogExtractor.h"
#include "Well/RigGeoMechWellLogExtractor.h"
#include "Well/RigWellPath.h"

#include "RimCase.h"
#include "RimDepthTrackPlot.h"
#include "RimEclipseCase.h"
#include "RimGeoMechCase.h"
#include "RimWellAllocationPlot.h"
#include "RimWellLogCurve.h"
#include "RimWellLogPlotCollection.h"

#include "RiaWellLogCurveMerger.h"

#include "Well/RigWellLogCurveData.h"

#include "cafPdmUiItem.h"

#include "cvfAssert.h"

#include <cmath>
#include <limits>
#include <set>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
CurveSamplingPointData RimWellLogTrackTools::curveSamplingPointData( RigEclipseWellLogExtractor* extractor, RigResultAccessor* resultAccessor )
{
    CurveSamplingPointData curveData;

    curveData.md      = extractor->cellIntersectionMDs();
    curveData.tvd     = extractor->cellIntersectionTVDs();
    curveData.rkbDiff = extractor->wellPathGeometry()->rkbDiff();

    extractor->curveData( resultAccessor, &curveData.data );

    return curveData;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
CurveSamplingPointData RimWellLogTrackTools::curveSamplingPointData( RigGeoMechWellLogExtractor* extractor,
                                                                      const RigFemResultAddress&  resultAddress )
{
    CurveSamplingPointData curveData;

    curveData.md      = extractor->cellIntersectionMDs();
    curveData.tvd     = extractor->cellIntersectionTVDs();
    curveData.rkbDiff = extractor->wellPathGeometry()->rkbDiff();

    extractor->curveData( resultAddress, 0, 0, &curveData.data );
    return curveData;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellLogTrackTools::findRegionNamesToPlot( const CurveSamplingPointData&           curveData,
                                                   const std::vector<QString>&             regionNamesVector,
                                                   RimWellLogPlot::DepthTypeEnum           depthType,
                                                   std::vector<QString>*                   regionNamesToPlot,
                                                   std::vector<std::pair<double, double>>* yValues )
{
    if ( regionNamesVector.empty() ) return;

    std::vector<size_t> regionNameIndicesFromCurve;

    for ( double nameIdx : curveData.data )
    {
        if ( nameIdx != std::numeric_limits<double>::infinity() )
        {
            regionNameIndicesFromCurve.push_back( static_cast<size_t>( round( nameIdx ) ) );
        }
        else
        {
            regionNameIndicesFromCurve.push_back( std::numeric_limits<size_t>::max() );
        }
    }

    if ( regionNameIndicesFromCurve.empty() ) return;

    std::vector<double> depthVector;

    if ( depthType == RiaDefines::DepthTypeEnum::MEASURED_DEPTH || depthType == RiaDefines::DepthTypeEnum::PSEUDO_LENGTH )
    {
        depthVector = curveData.md;
    }
    else if ( depthType == RiaDefines::DepthTypeEnum::TRUE_VERTICAL_DEPTH || depthType == RiaDefines::DepthTypeEnum::TRUE_VERTICAL_DEPTH_RKB )
    {
        depthVector = curveData.tvd;
        if ( depthType == RiaDefines::DepthTypeEnum::TRUE_VERTICAL_DEPTH_RKB )
        {
            for ( double& depthValue : depthVector )
            {
                depthValue += curveData.rkbDiff;
            }
        }
    }

    if ( depthVector.empty() ) return;

    double currentYStart = depthVector[0];
    size_t prevNameIndex = regionNameIndicesFromCurve[0];
    size_t currentNameIndex;

    for ( size_t i = 1; i < regionNameIndicesFromCurve.size(); i++ )
    {
        currentNameIndex = regionNameIndicesFromCurve[i];
        if ( currentNameIndex != std::numeric_limits<size_t>::max() && currentNameIndex != prevNameIndex )
        {
            if ( prevNameIndex < regionNamesVector.size() )
            {
                regionNamesToPlot->push_back( regionNamesVector[prevNameIndex] );
                yValues->push_back( std::make_pair( currentYStart, depthVector[i - 1] ) );
            }

            currentYStart = depthVector[i];
            prevNameIndex = currentNameIndex;
        }
    }

    size_t lastFormationIdx = regionNameIndicesFromCurve.back();
    if ( lastFormationIdx < regionNamesVector.size() )
    {
        regionNamesToPlot->push_back( regionNamesVector[lastFormationIdx] );
        yValues->push_back( std::make_pair( currentYStart, depthVector.back() ) );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<QString> RimWellLogTrackTools::formationNamesVector( RimCase* rimCase )
{
    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>( rimCase );
    RimGeoMechCase* geoMechCase = dynamic_cast<RimGeoMechCase*>( rimCase );

    if ( eclipseCase )
    {
        return eclipseCase->eclipseCaseData()->formationNames();
    }
    else if ( geoMechCase )
    {
        return geoMechCase->geoMechData()->femPartResults()->formationNames();
    }

    return std::vector<QString>();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellLogTrackTools::addOverburden( std::vector<QString>& namesVector, CurveSamplingPointData& curveData, double height )
{
    if ( !curveData.data.empty() )
    {
        namesVector.push_back( "Overburden" );

        // Prepend the new "fake" depth for start of overburden
        double tvdTop = curveData.tvd[0];
        curveData.tvd.insert( curveData.tvd.begin(), tvdTop );
        curveData.tvd.insert( curveData.tvd.begin(), tvdTop - height );

        // TODO: this is not always correct
        double mdTop = curveData.md[0];
        curveData.md.insert( curveData.md.begin(), mdTop );
        curveData.md.insert( curveData.md.begin(), mdTop - height );

        curveData.data.insert( curveData.data.begin(), namesVector.size() - 1 );
        curveData.data.insert( curveData.data.begin(), namesVector.size() - 1 );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellLogTrackTools::addUnderburden( std::vector<QString>& namesVector, CurveSamplingPointData& curveData, double height )
{
    if ( !curveData.data.empty() )
    {
        namesVector.push_back( "Underburden" );

        size_t lastIndex = curveData.tvd.size() - 1;

        // Append the new "fake" depth for start of underburden
        double tvdBottom = curveData.tvd[lastIndex];
        curveData.tvd.push_back( tvdBottom );
        curveData.tvd.push_back( tvdBottom + height );

        // TODO: this is not always correct
        double mdBottom = curveData.md[lastIndex];
        curveData.md.push_back( mdBottom );
        curveData.md.push_back( mdBottom + height );

        curveData.data.push_back( namesVector.size() - 1 );
        curveData.data.push_back( namesVector.size() - 1 );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellLogTrackTools::simWellOptionItems( QList<caf::PdmOptionItemInfo>* options, RimCase* rimCase )
{
    CVF_ASSERT( options );
    if ( !options ) return;

    std::set<QString> sortedWellNames;

    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>( rimCase );

    if ( eclipseCase && eclipseCase->eclipseCaseData() )
    {
        sortedWellNames = eclipseCase->eclipseCaseData()->findSortedWellNames();
    }

    caf::IconProvider simWellIcon( ":/Well.svg" );
    for ( const QString& wname : sortedWellNames )
    {
        options->push_back( caf::PdmOptionItemInfo( wname, wname, false, simWellIcon ) );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RigEclipseWellLogExtractor* RimWellLogTrackTools::createSimWellExtractor( RimWellLogPlotCollection* wellLogCollection,
                                                                           RimCase*                  rimCase,
                                                                           const QString&            simWellName,
                                                                           int                       branchIndex,
                                                                           bool                      useBranchDetection )
{
    if ( !wellLogCollection ) return nullptr;

    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>( rimCase );
    if ( !eclipseCase ) return nullptr;

    std::vector<const RigWellPath*> wellPaths = RiaSimWellBranchTools::simulationWellBranches( simWellName, useBranchDetection );

    if ( wellPaths.empty() ) return nullptr;

    CVF_ASSERT( branchIndex >= 0 && branchIndex < static_cast<int>( wellPaths.size() ) );

    return ( wellLogCollection->findOrCreateSimWellExtractor( simWellName,
                                                              QString( "Find or create sim well extractor" ),
                                                              wellPaths[branchIndex],
                                                              eclipseCase->eclipseCaseData() ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::pair<double, double> RimWellLogTrackTools::adjustXRange( double minValue, double maxValue, double tickInterval )
{
    double minRemainder = std::fmod( minValue, tickInterval );
    double maxRemainder = std::fmod( maxValue, tickInterval );
    double adjustedMin  = minValue - minRemainder;
    double adjustedMax  = maxValue + ( tickInterval - maxRemainder );
    return std::make_pair( adjustedMin, adjustedMax );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::pair<double, double> RimWellLogTrackTools::extendMinMaxRange( double minValue, double maxValue, double factor )
{
    if ( minValue == std::numeric_limits<double>::infinity() || maxValue == std::numeric_limits<double>::infinity() )
    {
        return { minValue, maxValue };
    }

    auto modifiedMin = minValue;
    auto modifiedMax = maxValue;

    auto range = std::fabs( maxValue - minValue );
    if ( range < 1e-6 )
    {
        // If min and max are equal, the curve is not visible. Make sure the range is larger than zero.
        if ( maxValue != 0.0 )
        {
            range = maxValue * 0.01;
        }
        else
        {
            range = 1.0;
        }
    }

    modifiedMax += factor * range;

    auto candidateMinValue = minValue - factor * range;
    if ( std::signbit( minValue ) == std::signbit( candidateMinValue ) )
    {
        modifiedMin = candidateMinValue;
    }
    else
    {
        // If the sign of the adjusted minimum changes, set minimum to zero to make sure that zero is located properly
        modifiedMin = 0.0;
    }

    return { modifiedMin, modifiedMax };
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString RimWellLogTrackTools::asciiDataForPlotExport( const QString&                       trackDescription,
                                                       RimDepthTrackPlot*                   depthTrackPlot,
                                                       const std::vector<RimWellLogCurve*>& curves )
{
    QString out = "\n" + trackDescription + "\n";

    std::vector<QString>             curveNames;
    std::vector<double>              curveDepths;
    std::vector<std::vector<double>> curvesPlotXValues;

    auto depthType             = depthTrackPlot->depthType();
    auto depthUnit             = depthTrackPlot->depthUnit();
    bool isWellAllocInflowPlot = false;
    {
        auto wapl = depthTrackPlot->firstAncestorOfType<RimWellAllocationPlot>();
        if ( wapl )
        {
            isWellAllocInflowPlot = ( wapl->flowType() == RimWellAllocationPlot::INFLOW );
        }
    }

    RiaWellLogCurveMerger curveMerger;
    bool                  foundNonMatchingDepths = false;

    for ( RimWellLogCurve* curve : curves )
    {
        if ( !curve->isChecked() ) continue;

        const RigWellLogCurveData* curveData = curve->curveData();
        if ( !curveData ) continue;
        curveNames.push_back( curve->curveName() );

        if ( curveNames.size() == 1 )
        {
            curveDepths = curveData->depthValuesByIntervals( depthType, depthUnit );
        }

        std::vector<double> xPlotValues = curveData->propertyValuesByIntervals();
        if ( xPlotValues.empty() )
        {
            curveNames.pop_back();

            if ( curveNames.empty() )
            {
                curveDepths.clear();
            }
            continue;
        }

        if ( curveDepths.size() != xPlotValues.size() )
        {
            foundNonMatchingDepths = true;
        }

        std::vector<double> depths = curveData->depthValuesByIntervals( depthType, depthUnit );
        curveMerger.addCurveData( depths, xPlotValues );

        curvesPlotXValues.push_back( xPlotValues );
    }

    // Header

    if ( depthType == RiaDefines::DepthTypeEnum::CONNECTION_NUMBER )
    {
        out += "Connection";
    }
    else if ( depthType == RiaDefines::DepthTypeEnum::MEASURED_DEPTH )
    {
        out += "MD   ";
    }
    else if ( depthType == RiaDefines::DepthTypeEnum::PSEUDO_LENGTH )
    {
        out += "PL   ";
    }
    else if ( depthType == RiaDefines::DepthTypeEnum::TRUE_VERTICAL_DEPTH )
    {
        out += "TVDMSL  ";
    }
    else if ( depthType == RiaDefines::DepthTypeEnum::TRUE_VERTICAL_DEPTH_RKB )
    {
        out += "TVDRKB  ";
    }

    for ( QString name : curveNames )
    {
        out += "  \t" + name;
    }
    out += "\n";

    // Resample when curves have different depth
    if ( foundNonMatchingDepths )
    {
        curvesPlotXValues.clear();
        curveDepths.clear();

        curveMerger.computeLookupValues();

        const std::vector<double>& allDepths = curveMerger.allXValues();
        curveDepths                          = allDepths;
        for ( size_t curveIdx = 0; curveIdx < curveMerger.curveCount(); ++curveIdx )
        {
            const std::vector<double>& curveValues = curveMerger.lookupYValuesForAllXValues( curveIdx );
            curvesPlotXValues.push_back( curveValues );
        }
    }

    for ( size_t dIdx = 0; dIdx < curveDepths.size(); ++dIdx )
    {
        size_t i          = dIdx;
        double curveDepth = curveDepths[i];

        if ( depthType == RiaDefines::DepthTypeEnum::CONNECTION_NUMBER )
        {
            if ( dIdx == 0 )
                continue; // Skip the first line. (shallow depth, which is last)
                          // as it is a fictitious value added to make
                          // the plot easier to read

            i = curveDepths.size() - 1 - dIdx; // Reverse the order, since the connections are coming bottom to top

            if ( i == 0 )
            {
                if ( curveDepths.size() > 1 && curveDepths[i] == curveDepths[i + 1] )
                {
                    continue; // Skip double depth at last connection
                }
            }

            curveDepth = curveDepths[i];

            if ( isWellAllocInflowPlot )
            {
                curveDepth -= 0.5; // To shift the values that was shifted to get the numbers between the changes
            }
        }

        out += QString::number( curveDepth, 'f', 3 );
        for ( std::vector<double> plotVector : curvesPlotXValues )
        {
            out += QString( " \t%1" ).arg( QString::number( plotVector[i], 'f', 3 ), 12 );
        }
        out += "\n";
    }

    return out;
}
