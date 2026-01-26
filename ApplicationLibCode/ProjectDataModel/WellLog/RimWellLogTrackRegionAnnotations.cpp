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

#include "RimWellLogTrackRegionAnnotations.h"

#include "RimWellLogTrack.h"
#include "RimWellLogTrackTools.h"

#include "RiaColorTables.h"
#include "RiaExtractionTools.h"
#include "RiaLogging.h"
#include "RiaPreferences.h"
#include "RiaResultNames.h"
#include "RiaWellLogUnitTools.h"

#include "RigEclipseResultAddress.h"
#include "RigFemPartResultsCollection.h"
#include "RigFemResultAddress.h"
#include "RigGeoMechCaseData.h"
#include "RigResultAccessorFactory.h"
#include "RigWbsParameter.h"
#include "Well/RigEclipseWellLogExtractor.h"
#include "Well/RigGeoMechWellLogExtractor.h"
#include "Well/RigWellPath.h"
#include "Well/RigWellPathFormations.h"

#include "RimCase.h"
#include "RimColorLegend.h"
#include "RimColorLegendItem.h"
#include "RimDepthTrackPlot.h"
#include "RimEclipseCase.h"
#include "RimEclipseResultDefinition.h"
#include "RimGeoMechCase.h"
#include "RimMainPlotCollection.h"
#include "RimWellBoreStabilityPlot.h"
#include "RimWellLogCurveCommonDataSource.h"
#include "RimWellLogExtractionCurve.h"
#include "RimWellLogPlotCollection.h"
#include "RimWellPath.h"

#include "RiuPlotAnnotationTool.h"
#include "RiuQwtPlotWidget.h"

#include "cafFontTools.h"

#include <limits>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimWellLogTrackRegionAnnotations::RimWellLogTrackRegionAnnotations( RimWellLogTrack* track )
    : m_track( track )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimWellLogTrackRegionAnnotations::~RimWellLogTrackRegionAnnotations()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellLogTrackRegionAnnotations::updateRegionAnnotationsOnPlot()
{
    removeRegionAnnotations();

    if ( m_track->annotationType() == RiaDefines::RegionAnnotationType::NO_ANNOTATIONS ) return;

    if ( m_annotationTool == nullptr )
    {
        m_annotationTool = std::make_unique<RiuPlotAnnotationTool>();
    }

    if ( m_track->annotationType() == RiaDefines::RegionAnnotationType::FORMATION_ANNOTATIONS )
    {
        updateFormationNamesOnPlot();
    }
    else if ( m_track->annotationType() == RiaDefines::RegionAnnotationType::RESULT_PROPERTY_ANNOTATIONS )
    {
        updateResultPropertyNamesOnPlot();
    }
    else
    {
        updateCurveDataRegionsOnPlot();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellLogTrackRegionAnnotations::removeRegionAnnotations()
{
    if ( m_annotationTool )
    {
        m_annotationTool->detachAllAnnotations();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<std::pair<double, double>> RimWellLogTrackRegionAnnotations::waterAndRockRegions( RiaDefines::DepthTypeEnum depthType,
                                                                                              const RigGeoMechWellLogExtractor* extractor ) const
{
    double waterEndTVD = extractor->waterDepth();
    if ( waterEndTVD == std::numeric_limits<double>::infinity() )
    {
        waterEndTVD = extractor->estimateWaterDepth();
    }

    if ( depthType == RiaDefines::DepthTypeEnum::MEASURED_DEPTH )
    {
        double waterStartMD = 0.0;
        if ( extractor->wellPathGeometry()->rkbDiff() != std::numeric_limits<double>::infinity() )
        {
            waterStartMD += extractor->wellPathGeometry()->rkbDiff();
        }
        double waterEndMD = extractor->cellIntersectionMDs().front();
        double rockEndMD  = extractor->cellIntersectionMDs().back();
        return { { waterStartMD, waterEndMD }, { waterEndMD, rockEndMD } };
    }
    else if ( depthType == RiaDefines::DepthTypeEnum::TRUE_VERTICAL_DEPTH )
    {
        double waterStartTVD = 0.0;
        double rockEndTVD    = extractor->cellIntersectionTVDs().back();
        return { { waterStartTVD, waterEndTVD }, { waterEndTVD, rockEndTVD } };
    }
    else if ( depthType == RiaDefines::DepthTypeEnum::TRUE_VERTICAL_DEPTH_RKB )
    {
        double waterStartTVDRKB = extractor->wellPathGeometry()->rkbDiff();
        double waterEndTVDRKB   = waterEndTVD + extractor->wellPathGeometry()->rkbDiff();
        double rockEndTVDRKB    = extractor->cellIntersectionTVDs().back() + extractor->wellPathGeometry()->rkbDiff();
        return { { waterStartTVDRKB, waterEndTVDRKB }, { waterEndTVDRKB, rockEndTVDRKB } };
    }
    return {};
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellLogTrackRegionAnnotations::updateFormationNamesOnPlot()
{
    RimDepthTrackPlot* plot = m_track->firstAncestorOrThisOfTypeAsserted<RimDepthTrackPlot>();

    RiaDefines::DepthUnitType fromDepthUnit = plot->caseDepthUnit();
    RiaDefines::DepthUnitType toDepthUnit   = plot->depthUnit();

    auto orientation = plot->depthOrientation();

    RiuQwtPlotWidget* plotWidget = m_track->viewer();
    if ( !plotWidget ) return;

    RimWellPath*                    formationWellPathForSourceCase     = m_track->formationWellPath();
    RimWellPath*                    formationWellPathForSourceWellPath = nullptr;
    RimCase*                        formationCase                      = m_track->formationNamesCase();
    QString                         formationSimWellName               = m_track->formationSimWellName();
    int                             formationBranchIndex               = m_track->formationBranchIndex();
    bool                            formationBranchDetection           = m_track->formationBranchDetection();
    RimWellLogTrack::TrajectoryType formationTrajectoryType            = m_track->formationTrajectoryType();

    // Get formation source settings from track fields through public methods
    // Note: Some private members need accessor methods added to RimWellLogTrack

    if ( m_track->m_formationSource() == RimWellLogTrack::FormationSource::WELL_PICK_FILTER )
    {
        formationWellPathForSourceWellPath = m_track->m_formationWellPathForSourceWellPath;
        if ( formationWellPathForSourceWellPath == nullptr ) return;

        if ( plot->depthType() != RiaDefines::DepthTypeEnum::MEASURED_DEPTH &&
             plot->depthType() != RiaDefines::DepthTypeEnum::TRUE_VERTICAL_DEPTH &&
             plot->depthType() != RiaDefines::DepthTypeEnum::TRUE_VERTICAL_DEPTH_RKB )
        {
            return;
        }

        std::vector<double> yValues;

        const RigWellPathFormations* formations = formationWellPathForSourceWellPath->formationsGeometry();
        if ( !formations ) return;

        std::vector<QString> formationNamesToPlot;
        formations->depthAndFormationNamesUpToLevel( m_track->m_formationLevel(),
                                                     &formationNamesToPlot,
                                                     &yValues,
                                                     m_track->m_showformationFluids,
                                                     plot->depthType() );

        if ( plot->depthType() == RiaDefines::DepthTypeEnum::TRUE_VERTICAL_DEPTH_RKB )
        {
            for ( double& depthValue : yValues )
            {
                depthValue += formationWellPathForSourceWellPath->wellPathGeometry()->rkbDiff();
            }
        }

        std::vector<double> convertedYValues = RiaWellLogUnitTools<double>::convertDepths( yValues, fromDepthUnit, toDepthUnit );

        m_annotationTool->attachWellPicks( plotWidget->qwtPlot(), formationNamesToPlot, convertedYValues );
    }
    else
    {
        RimMainPlotCollection* mainPlotCollection = m_track->firstAncestorOrThisOfTypeAsserted<RimMainPlotCollection>();

        RimWellLogPlotCollection* wellLogCollection = mainPlotCollection->wellLogPlotCollection();

        CurveSamplingPointData curveData;

        RigEclipseWellLogExtractor* eclWellLogExtractor     = nullptr;
        RigGeoMechWellLogExtractor* geoMechWellLogExtractor = nullptr;

        if ( formationTrajectoryType == RimWellLogTrack::SIMULATION_WELL )
        {
            eclWellLogExtractor = RimWellLogTrackTools::createSimWellExtractor( wellLogCollection,
                                                                                formationCase,
                                                                                formationSimWellName,
                                                                                formationBranchIndex,
                                                                                formationBranchDetection );
        }
        else
        {
            eclWellLogExtractor = RiaExtractionTools::findOrCreateWellLogExtractor( formationWellPathForSourceCase,
                                                                                    dynamic_cast<RimEclipseCase*>( formationCase ) );
        }

        if ( eclWellLogExtractor )
        {
            RimEclipseCase*             eclipseCase = dynamic_cast<RimEclipseCase*>( formationCase );
            cvf::ref<RigResultAccessor> resultAccessor =
                RigResultAccessorFactory::createFromResultAddress( eclipseCase->eclipseCaseData(),
                                                                   0,
                                                                   RiaDefines::PorosityModelType::MATRIX_MODEL,
                                                                   0,
                                                                   RigEclipseResultAddress( RiaDefines::ResultCatType::FORMATION_NAMES,
                                                                                            RiaResultNames::activeFormationNamesResultName() ) );

            if ( resultAccessor.notNull() )
            {
                curveData = RimWellLogTrackTools::curveSamplingPointData( eclWellLogExtractor, resultAccessor.p() );
            }
        }
        else
        {
            geoMechWellLogExtractor = RiaExtractionTools::findOrCreateWellLogExtractor( formationWellPathForSourceCase,
                                                                                        dynamic_cast<RimGeoMechCase*>( formationCase ) );
            if ( !geoMechWellLogExtractor ) return;

            std::string activeFormationNamesResultName = RiaResultNames::activeFormationNamesResultName().toStdString();
            curveData =
                RimWellLogTrackTools::curveSamplingPointData( geoMechWellLogExtractor,
                                                              RigFemResultAddress( RIG_FORMATION_NAMES, activeFormationNamesResultName, "" ) );
        }

        if ( geoMechWellLogExtractor )
        {
            // Attach water and rock base formations

            const caf::ColorTable                        waterAndRockColors = RiaColorTables::waterAndRockPaletteColors();
            const std::vector<std::pair<double, double>> waterAndRockIntervals =
                waterAndRockRegions( plot->depthType(), geoMechWellLogExtractor );

            const std::vector<std::pair<double, double>> convertedYValues =
                RiaWellLogUnitTools<double>::convertDepths( waterAndRockIntervals, fromDepthUnit, toDepthUnit );

            m_annotationTool->attachNamedRegions( plotWidget->qwtPlot(),
                                                  { "Sea Level", "" },
                                                  orientation,
                                                  convertedYValues,
                                                  m_track->annotationDisplay(),
                                                  waterAndRockColors,
                                                  ( ( 100 - m_track->m_colorShadingTransparency ) * 255 ) / 100,
                                                  m_track->m_showRegionLabels,
                                                  RiaDefines::TrackSpan::LEFT_COLUMN,
                                                  { Qt::SolidPattern, Qt::Dense6Pattern } );
        }

        if ( m_track->m_formationSource() == RimWellLogTrack::FormationSource::CASE && plotWidget )
        {
            if ( ( formationSimWellName == QString( "None" ) && formationWellPathForSourceCase == nullptr ) || formationCase == nullptr )
                return;

            std::vector<QString> formationNamesVector = RimWellLogTrackTools::formationNamesVector( formationCase );

            if ( m_track->overburdenHeight() > 0.0 )
            {
                RimWellLogTrackTools::addOverburden( formationNamesVector, curveData, m_track->overburdenHeight() );
            }

            if ( m_track->underburdenHeight() > 0.0 )
            {
                RimWellLogTrackTools::addUnderburden( formationNamesVector, curveData, m_track->underburdenHeight() );
            }

            std::vector<std::pair<double, double>> yValues;

            std::vector<QString> formationNamesToPlot;
            RimWellLogTrackTools::findRegionNamesToPlot( curveData, formationNamesVector, plot->depthType(), &formationNamesToPlot, &yValues );

            std::vector<std::pair<double, double>> convertedYValues =
                RiaWellLogUnitTools<double>::convertDepths( yValues, fromDepthUnit, toDepthUnit );

            caf::ColorTable colorTable( m_track->m_colorShadingLegend->colorArray() );
            m_annotationTool->attachNamedRegions( plotWidget->qwtPlot(),
                                                  formationNamesToPlot,
                                                  orientation,
                                                  convertedYValues,
                                                  m_track->annotationDisplay(),
                                                  colorTable,
                                                  ( ( 100 - m_track->m_colorShadingTransparency.value() ) * 255 ) / 100,
                                                  m_track->m_showRegionLabels.value() );
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellLogTrackRegionAnnotations::updateResultPropertyNamesOnPlot()
{
    RimDepthTrackPlot* plot = m_track->firstAncestorOrThisOfTypeAsserted<RimDepthTrackPlot>();

    RiaDefines::DepthUnitType fromDepthUnit = plot->caseDepthUnit();
    RiaDefines::DepthUnitType toDepthUnit   = plot->depthUnit();

    auto orientation = plot->depthOrientation();

    RiuQwtPlotWidget* plotWidget = m_track->viewer();
    if ( !plotWidget ) return;

    RimWellPath* formationWellPathForSourceCase = m_track->formationWellPath();
    RimCase*     formationCase                  = m_track->formationNamesCase();
    QString      formationSimWellName           = m_track->formationSimWellName();

    RigEclipseWellLogExtractor* eclWellLogExtractor =
        RiaExtractionTools::findOrCreateWellLogExtractor( formationWellPathForSourceCase, dynamic_cast<RimEclipseCase*>( formationCase ) );

    if ( !eclWellLogExtractor )
    {
        RiaLogging::error( "No well log extractor found for case." );
        return;
    }

    RimEclipseCase* eclipseCase = dynamic_cast<RimEclipseCase*>( formationCase );

    RimEclipseResultDefinition* resultDefinition = m_track->m_resultDefinition;
    resultDefinition->loadResult();

    size_t                      timeStep = 0;
    cvf::ref<RigResultAccessor> resultAccessor =
        RigResultAccessorFactory::createFromResultDefinition( eclipseCase->eclipseCaseData(), 0, timeStep, resultDefinition );
    if ( !resultAccessor.notNull() )
    {
        QString resultTypeStr = caf::AppEnum<RiaDefines::ResultCatType>( resultDefinition->resultType() ).uiText();
        RiaLogging::error( QString( "Unable to find result for region annotation for '%1' track. Tried '%2' (%3) on case: '%4'" )
                               .arg( m_track->description() )
                               .arg( resultDefinition->resultVariable() )
                               .arg( resultTypeStr )
                               .arg( eclipseCase->caseUserDescription() ) );
        return;
    }

    CurveSamplingPointData curveData = RimWellLogTrackTools::curveSamplingPointData( eclWellLogExtractor, resultAccessor.p() );

    // Attach water and rock base formations

    if ( m_track->m_formationSource() == RimWellLogTrack::FormationSource::CASE )
    {
        if ( ( formationSimWellName == QString( "None" ) && formationWellPathForSourceCase == nullptr ) || formationCase == nullptr )
            return;

        std::vector<cvf::Color3ub> colors;

        RimColorLegend* colorShadingLegend = m_track->m_colorShadingLegend;

        // Find the largest category number.
        int maxCategoryValue = std::numeric_limits<int>::min();
        for ( RimColorLegendItem* legendItem : colorShadingLegend->colorLegendItems() )
        {
            maxCategoryValue = std::max( maxCategoryValue, legendItem->categoryValue() );
        }

        // Insert each name at index matching the category number.
        std::vector<QString> namesVector( maxCategoryValue + 1 );
        for ( RimColorLegendItem* legendItem : colorShadingLegend->colorLegendItems() )
        {
            namesVector[legendItem->categoryValue()] = legendItem->categoryName();
        }

        if ( m_track->overburdenHeight() > 0.0 )
        {
            RimWellLogTrackTools::addOverburden( namesVector, curveData, m_track->overburdenHeight() );
        }

        if ( m_track->underburdenHeight() > 0.0 )
        {
            RimWellLogTrackTools::addUnderburden( namesVector, curveData, m_track->underburdenHeight() );
        }

        std::vector<QString>                   namesToPlot;
        std::vector<std::pair<double, double>> yValues;
        RimWellLogTrackTools::findRegionNamesToPlot( curveData, namesVector, plot->depthType(), &namesToPlot, &yValues );

        // convert to plot depth unit
        std::vector<std::pair<double, double>> convertedYValues =
            RiaWellLogUnitTools<double>::convertDepths( yValues, fromDepthUnit, toDepthUnit );

        // TODO: unnecessarily messy!
        // Need to map colors to names (since a category can be used several times)
        for ( QString nameToPlot : namesToPlot )
        {
            bool isFound = false;
            for ( RimColorLegendItem* legendItem : colorShadingLegend->colorLegendItems() )
            {
                if ( legendItem->categoryName() == nameToPlot )
                {
                    colors.push_back( cvf::Color3ub( legendItem->color() ) );
                    isFound = true;
                }
            }

            if ( !isFound )
            {
                colors.push_back( cvf::Color3ub( RiaColorTables::undefinedCellColor() ) );
            }
        }

        if ( colors.empty() )
        {
            RiaLogging::error( "No colors found." );
            return;
        }

        caf::ColorTable colorTable( colors );

        int fontSize = caf::FontTools::absolutePointSize( RiaPreferences::current()->defaultPlotFontSize(), m_track->m_regionLabelFontSize() );

        m_annotationTool->attachNamedRegions( plotWidget->qwtPlot(),
                                              namesToPlot,
                                              orientation,
                                              convertedYValues,
                                              m_track->annotationDisplay(),
                                              colorTable,
                                              ( ( 100 - m_track->m_colorShadingTransparency.value() ) * 255 ) / 100,
                                              m_track->m_showRegionLabels.value(),
                                              RiaDefines::TrackSpan::FULL_WIDTH,
                                              {},
                                              fontSize );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellLogTrackRegionAnnotations::updateCurveDataRegionsOnPlot()
{
    RimWellBoreStabilityPlot* wellBoreStabilityPlot = m_track->firstAncestorOrThisOfType<RimWellBoreStabilityPlot>();
    if ( wellBoreStabilityPlot )
    {
        RiaDefines::DepthUnitType fromDepthUnit = wellBoreStabilityPlot->caseDepthUnit();
        RiaDefines::DepthUnitType toDepthUnit   = wellBoreStabilityPlot->depthUnit();

        auto orientation = wellBoreStabilityPlot->depthOrientation();

        wellBoreStabilityPlot->updateCommonDataSource();
        RimGeoMechCase* geoMechCase = dynamic_cast<RimGeoMechCase*>( wellBoreStabilityPlot->commonDataSource()->caseToApply() );
        RimWellPath*    wellPath    = wellBoreStabilityPlot->commonDataSource()->wellPathToApply();
        int             timeStep    = wellBoreStabilityPlot->commonDataSource()->timeStepToApply();
        if ( geoMechCase && wellPath && timeStep >= 0 )
        {
            auto [stepIdx, frameIdx] = geoMechCase->geoMechData()->femPartResults()->stepListIndexToTimeStepAndDataFrameIndex( timeStep );

            RigGeoMechWellLogExtractor* geoMechWellLogExtractor = nullptr;
            geoMechWellLogExtractor =
                RiaExtractionTools::findOrCreateWellLogExtractor( wellPath, dynamic_cast<RimGeoMechCase*>( geoMechCase ) );
            if ( !geoMechWellLogExtractor ) return;

            CurveSamplingPointData curveData;
            curveData.md  = geoMechWellLogExtractor->cellIntersectionMDs();
            curveData.tvd = geoMechWellLogExtractor->cellIntersectionTVDs();

            RimWellLogExtractionCurve::findAndLoadWbsParametersFromFiles( wellPath, geoMechWellLogExtractor );
            RimWellBoreStabilityPlot* wbsPlot = m_track->firstAncestorOrThisOfType<RimWellBoreStabilityPlot>();
            if ( wbsPlot )
            {
                wbsPlot->applyWbsParametersToExtractor( geoMechWellLogExtractor );
            }

            std::vector<double> ppSourceRegions      = geoMechWellLogExtractor->porePressureSourceRegions( stepIdx, frameIdx );
            std::vector<double> poissonSourceRegions = geoMechWellLogExtractor->poissonSourceRegions( stepIdx, frameIdx );
            std::vector<double> ucsSourceRegions     = geoMechWellLogExtractor->ucsSourceRegions( stepIdx, frameIdx );

            RiuQwtPlotWidget* plotWidget         = m_track->viewer();
            RimColorLegend*   colorShadingLegend = m_track->m_colorShadingLegend;

            {
                caf::ColorTable colorTable( colorShadingLegend->colorArray() );

                std::vector<QString> sourceNames =
                    RigWbsParameter::PP_Reservoir().allSourceUiLabels( "\n", wbsPlot->userDefinedValue( RigWbsParameter::PP_NonReservoir() ) );
                curveData.data = ppSourceRegions;

                std::vector<QString>                   sourceNamesToPlot;
                std::vector<std::pair<double, double>> yValues;
                RimWellLogTrackTools::findRegionNamesToPlot( curveData, sourceNames, wellBoreStabilityPlot->depthType(), &sourceNamesToPlot, &yValues );

                // convert to plot depth unit
                std::vector<std::pair<double, double>> convertedYValues =
                    RiaWellLogUnitTools<double>::convertDepths( yValues, fromDepthUnit, toDepthUnit );

                m_annotationTool->attachNamedRegions( plotWidget->qwtPlot(),
                                                      sourceNamesToPlot,
                                                      orientation,
                                                      convertedYValues,
                                                      m_track->annotationDisplay(),
                                                      colorTable,
                                                      ( ( ( 100 - m_track->m_colorShadingTransparency.value() ) * 255 ) / 100 ) / 3,
                                                      m_track->m_showRegionLabels.value(),
                                                      RiaDefines::TrackSpan::LEFT_COLUMN );
            }
            {
                caf::ColorTable colorTable( colorShadingLegend->colorArray() );

                std::vector<QString> sourceNames =
                    RigWbsParameter::poissonRatio().allSourceUiLabels( "\n", wbsPlot->userDefinedValue( RigWbsParameter::poissonRatio() ) );
                curveData.data = poissonSourceRegions;

                std::vector<QString>                   sourceNamesToPlot;
                std::vector<std::pair<double, double>> yValues;
                RimWellLogTrackTools::findRegionNamesToPlot( curveData, sourceNames, wellBoreStabilityPlot->depthType(), &sourceNamesToPlot, &yValues );

                // convert to plot depth unit
                std::vector<std::pair<double, double>> convertedYValues =
                    RiaWellLogUnitTools<double>::convertDepths( yValues, fromDepthUnit, toDepthUnit );

                m_annotationTool->attachNamedRegions( plotWidget->qwtPlot(),
                                                      sourceNamesToPlot,
                                                      orientation,
                                                      convertedYValues,
                                                      m_track->annotationDisplay(),
                                                      colorTable,
                                                      ( ( ( 100 - m_track->m_colorShadingTransparency.value() ) * 255 ) / 100 ) / 3,
                                                      m_track->m_showRegionLabels.value(),
                                                      RiaDefines::TrackSpan::CENTRE_COLUMN );
            }
            {
                caf::ColorTable colorTable( colorShadingLegend->colorArray() );

                std::vector<QString> sourceNames =
                    RigWbsParameter::UCS().allSourceUiLabels( "\n", wbsPlot->userDefinedValue( RigWbsParameter::UCS() ) );

                curveData.data = ucsSourceRegions;

                std::vector<QString>                   sourceNamesToPlot;
                std::vector<std::pair<double, double>> yValues;
                RimWellLogTrackTools::findRegionNamesToPlot( curveData, sourceNames, wellBoreStabilityPlot->depthType(), &sourceNamesToPlot, &yValues );

                // convert to plot depth unit
                std::vector<std::pair<double, double>> convertedYValues =
                    RiaWellLogUnitTools<double>::convertDepths( yValues, fromDepthUnit, toDepthUnit );

                m_annotationTool->attachNamedRegions( plotWidget->qwtPlot(),
                                                      sourceNamesToPlot,
                                                      orientation,
                                                      convertedYValues,
                                                      m_track->annotationDisplay(),
                                                      colorTable,
                                                      ( ( ( 100 - m_track->m_colorShadingTransparency.value() ) * 255 ) / 100 ) / 3,
                                                      m_track->m_showRegionLabels.value(),
                                                      RiaDefines::TrackSpan::RIGHT_COLUMN );
            }
        }
    }
}
