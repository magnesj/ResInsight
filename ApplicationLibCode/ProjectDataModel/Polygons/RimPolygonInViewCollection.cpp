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

#include "RimPolygonInViewCollection.h"

#include "Rim3dView.h"
#include "RimPolygon.h"
#include "RimPolygonCollection.h"
#include "RimPolygonInView.h"
#include "RimTools.h"

CAF_PDM_SOURCE_INIT( RimPolygonInViewCollection, "RimPolygonInViewCollection" );

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimPolygonInViewCollection::RimPolygonInViewCollection()
{
    CAF_PDM_InitObject( "Polygons (Under construction)", ":/PolylinesFromFile16x16.png" );

    CAF_PDM_InitFieldNoDefault( &m_polygons, "Polygons", "Polygons" );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPolygonInViewCollection::syncPolygonsInView()
{
    std::vector<RimPolygonInView*> existingPolygonsInView = m_polygons.childrenByType();
    m_polygons.clearWithoutDelete();

    auto polygonCollection = RimTools::polygonCollection();
    if ( polygonCollection )
    {
        std::vector<RimPolygonInView*> newPolygonsInView;

        for ( auto polygon : polygonCollection->allPolygons() )
        {
            auto it = std::find_if( existingPolygonsInView.begin(),
                                    existingPolygonsInView.end(),
                                    [polygon]( auto* polygonInView ) { return polygonInView->polygon() == polygon; } );

            if ( it != existingPolygonsInView.end() )
            {
                newPolygonsInView.push_back( *it );
                existingPolygonsInView.erase( it );
            }
            else
            {
                auto polygonInView = new RimPolygonInView();
                polygonInView->setPolygon( polygon );
                newPolygonsInView.push_back( polygonInView );
            }
        }

        m_polygons.setValue( newPolygonsInView );
    }

    for ( auto polyInView : existingPolygonsInView )
    {
        delete polyInView;
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RimPolygonInView*> RimPolygonInViewCollection::polygonsInView() const
{
    return m_polygons.childrenByType();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimPolygonInViewCollection::fieldChangedByUi( const caf::PdmFieldHandle* changedField, const QVariant& oldValue, const QVariant& newValue )
{
    RimCheckableObject::fieldChangedByUi( changedField, oldValue, newValue );

    if ( changedField == &m_isChecked )
    {
        for ( auto poly : polygonsInView() )
        {
            poly->updateConnectedEditors();
        }

        if ( auto view = firstAncestorOfType<Rim3dView>() )
        {
            view->scheduleCreateDisplayModelAndRedraw();
        }
    }
}