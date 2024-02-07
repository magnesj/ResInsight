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

#include "RimPolygonAppearance.h"

#include "RigPolyLinesData.h"

#include "cafPdmUiDoubleSliderEditor.h"

CAF_PDM_SOURCE_INIT( RimPolygonAppearance, "RimPolygonAppearance" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimPolygonAppearance::RimPolygonAppearance()
    : objectChanged( this )

{
    CAF_PDM_InitObject( "Polygon", ":/PolylinesFromFile16x16.png" );

    CAF_PDM_InitField( &m_isClosed, "IsClosed", false, "Closed Polygon" );
    CAF_PDM_InitField( &m_showLines, "ShowLines", true, "Show Lines" );
    CAF_PDM_InitField( &m_showSpheres, "ShowSpheres", false, "Show Spheres" );

    CAF_PDM_InitField( &m_lineThickness, "LineThickness", 3, "Line Thickness" );
    CAF_PDM_InitField( &m_sphereRadiusFactor, "SphereRadiusFactor", 0.15, "Sphere Radius Factor" );

    CAF_PDM_InitField( &m_lineColor, "LineColor", cvf::Color3f( cvf::Color3f::WHITE ), "Line Color" );
    CAF_PDM_InitField( &m_sphereColor, "SphereColor", cvf::Color3f( cvf::Color3f::WHITE ), "Sphere Color" );

    CAF_PDM_InitField( &m_polygonPlaneDepth, "PolygonPlaneDepth", 0.0, "Polygon Plane Depth" );
    CAF_PDM_InitField( &m_lockPolygonToPlane, "LockPolygon", false, "Lock Polygon to Plane" );

    m_polygonPlaneDepth.uiCapability()->setUiEditorTypeName( caf::PdmUiDoubleSliderEditor::uiEditorTypeName() );
    m_polygonPlaneDepth.uiCapability()->setUiLabelPosition( caf::PdmUiItemInfo::LabelPosType::TOP );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimPolygonAppearance::~RimPolygonAppearance()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPolygonAppearance::applyAppearanceSettings( RigPolyLinesData* polyLinesData )
{
    polyLinesData->setLineAppearance( m_lineThickness, m_lineColor, m_isClosed );
    polyLinesData->setSphereAppearance( m_sphereRadiusFactor, m_sphereColor );
    polyLinesData->setZPlaneLock( m_lockPolygonToPlane, -m_polygonPlaneDepth );
    polyLinesData->setVisibility( m_showLines, m_showSpheres );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPolygonAppearance::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    uiOrdering.add( &m_showLines );
    if ( m_showLines )
    {
        uiOrdering.add( &m_lineThickness );
        uiOrdering.add( &m_lineColor );
    }

    uiOrdering.add( &m_showSpheres );
    if ( m_showSpheres )
    {
        uiOrdering.add( &m_sphereRadiusFactor );
        uiOrdering.add( &m_sphereColor );
    }

    uiOrdering.add( &m_lockPolygonToPlane );
    if ( m_lockPolygonToPlane )
    {
        uiOrdering.add( &m_polygonPlaneDepth );
    }
    uiOrdering.skipRemainingFields();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPolygonAppearance::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    objectChanged.send();
}
