/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2023 Equinor ASA
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

#include "RimSeismicDataInterface.h"

#include "RimRegularLegendConfig.h"
#include "RimSeismicAlphaMapper.h"

#include "cvfBoundingBox.h"

CAF_PDM_XML_ABSTRACT_SOURCE_INIT( RimSeismicDataInterface, "SeismicDataInterface" ); // Abstract class.

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSeismicDataInterface::RimSeismicDataInterface()
{
    CAF_PDM_InitObject( "SeismicDataBase" );

    CAF_PDM_InitFieldNoDefault( &m_legendConfig, "LegendDefinition", "Color Legend" );
    m_legendConfig = new RimRegularLegendConfig();
    m_legendConfig.uiCapability()->setUiTreeHidden( true );

    CAF_PDM_InitField( &m_userClipValue, "userClipValue", std::make_pair( false, 0.0 ), "Clip Value" );
    CAF_PDM_InitField( &m_userMuteThreshold,
                       "userMuteThreshold",
                       std::make_pair( false, 0.0 ),
                       "Mute Threshold",
                       "",
                       "Samples with an absolute value below the threshold will be replaced with 0." );

    m_alphaValueMapper = std::make_shared<RimSeismicAlphaMapper>();
    m_boundingBox      = std::make_shared<cvf::BoundingBox>();

    initColorLegend();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSeismicDataInterface::~RimSeismicDataInterface()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimSeismicDataInterface::gridIsEqual( RimSeismicDataInterface* other )
{
    if ( other == nullptr ) return false;

    bool equal = ( inlineMin() == other->inlineMin() ) && ( inlineMax() == other->inlineMax() ) && ( inlineStep() == other->inlineStep() );
    equal = equal && ( xlineMin() == other->xlineMin() ) && ( xlineMax() == other->xlineMax() ) && ( xlineStep() == other->xlineStep() );
    equal = equal && ( zMin() == other->zMin() ) && ( zMax() == other->zMax() ) && ( zStep() == other->zStep() );

    return equal;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimRegularLegendConfig* RimSeismicDataInterface::legendConfig() const
{
    return m_legendConfig();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimSeismicDataInterface::hasValidData() const
{
    return true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimSeismicDataInterface::initColorLegend()
{
    m_legendConfig->setColorLegend( RimRegularLegendConfig::mapToColorLegend( RimRegularLegendConfig::ColorRangesType::BLUE_WHITE_RED ) );
    m_legendConfig->setMappingMode( RimRegularLegendConfig::MappingType::LINEAR_CONTINUOUS );
    m_legendConfig->setRangeMode( RimLegendConfig::RangeModeType::USER_DEFINED );
    m_legendConfig->setCenterLegendAroundZero( true );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimSeismicAlphaMapper* RimSeismicDataInterface::alphaValueMapper() const
{
    return m_alphaValueMapper.get();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
cvf::BoundingBox* RimSeismicDataInterface::boundingBox() const
{
    return m_boundingBox.get();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<double> RimSeismicDataInterface::histogramXvalues() const
{
    return m_clippedHistogramXvalues;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<double> RimSeismicDataInterface::histogramYvalues() const
{
    return m_clippedHistogramYvalues;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<double> RimSeismicDataInterface::alphaValues() const
{
    return m_clippedAlphaValues;
}