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

#include "RimWellLogTrackWellPathComponents.h"

#include "RimWellLogTrack.h"

#include "RiaDefines.h"

#include "RimDepthTrackPlot.h"
#include "RimWellLogPlot.h"
#include "RimWellPath.h"
#include "RimWellPathAttribute.h"
#include "RimWellPathAttributeCollection.h"
#include "RimWellPathCompletions.h"

#include "RiuQwtPlotWidget.h"
#include "RiuWellPathComponentPlotItem.h"

#include <map>
#include <set>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimWellLogTrackWellPathComponents::RimWellLogTrackWellPathComponents( RimWellLogTrack* track )
    : m_track( track )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RimWellLogTrackWellPathComponents::~RimWellLogTrackWellPathComponents()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellLogTrackWellPathComponents::updateWellPathAttributesOnPlot()
{
    m_wellPathAttributePlotObjects.clear();

    RimWellPath* wellPathAttributeSource = m_track->wellPathAttributeSource();

    if ( wellPathAttributeSource )
    {
        std::vector<const RimWellPathComponentInterface*> allWellPathComponents;

        if ( wellPathAttributeSource->wellPathGeometry() && ( m_track->showWellPathAttributes() || m_track->m_showWellPathCompletions.value() ) )
        {
            m_wellPathAttributePlotObjects.push_back( std::make_unique<RiuWellPathComponentPlotItem>( wellPathAttributeSource ) );
        }

        if ( m_track->showWellPathAttributes() )
        {
            RimWellPathAttributeCollection* attributeCollection = m_track->m_wellPathAttributeCollection();
            if ( attributeCollection )
            {
                std::vector<RimWellPathAttribute*> attributes = attributeCollection->attributes();
                for ( const RimWellPathAttribute* attribute : attributes )
                {
                    if ( attribute->isEnabled() )
                    {
                        allWellPathComponents.push_back( attribute );
                    }
                }
            }
        }
        if ( m_track->m_showWellPathCompletions.value() )
        {
            const RimWellPathCompletions*                     completionsCollection = wellPathAttributeSource->completions();
            std::vector<const RimWellPathComponentInterface*> allCompletions        = completionsCollection->allCompletions();

            for ( const RimWellPathComponentInterface* completion : allCompletions )
            {
                if ( completion->isEnabled() )
                {
                    allWellPathComponents.push_back( completion );
                }
            }
        }

        const std::map<RiaDefines::WellPathComponentType, int> sortIndices = { { RiaDefines::WellPathComponentType::WELL_PATH, 0 },
                                                                               { RiaDefines::WellPathComponentType::CASING, 1 },
                                                                               { RiaDefines::WellPathComponentType::LINER, 2 },
                                                                               { RiaDefines::WellPathComponentType::PERFORATION_INTERVAL, 3 },
                                                                               { RiaDefines::WellPathComponentType::FISHBONES, 4 },
                                                                               { RiaDefines::WellPathComponentType::FRACTURE, 5 },
                                                                               { RiaDefines::WellPathComponentType::PACKER, 6 },
                                                                               { RiaDefines::WellPathComponentType::ICD, 7 },
                                                                               { RiaDefines::WellPathComponentType::AICD, 8 },
                                                                               { RiaDefines::WellPathComponentType::ICV, 9 },
                                                                               { RiaDefines::WellPathComponentType::MSW_SEGMENT, 10 } };

        std::stable_sort( allWellPathComponents.begin(),
                          allWellPathComponents.end(),
                          [&sortIndices]( const RimWellPathComponentInterface* lhs, const RimWellPathComponentInterface* rhs )
                          { return sortIndices.at( lhs->componentType() ) < sortIndices.at( rhs->componentType() ); } );

        std::set<QString> completionsAssignedToLegend;
        for ( const RimWellPathComponentInterface* component : allWellPathComponents )
        {
            std::unique_ptr<RiuWellPathComponentPlotItem> plotItem( new RiuWellPathComponentPlotItem( wellPathAttributeSource, component ) );
            QString legendTitle     = plotItem->legendTitle();
            bool contributeToLegend = m_track->m_wellPathCompletionsInLegend.value() && !completionsAssignedToLegend.count( legendTitle );
            plotItem->setContributeToLegend( contributeToLegend );
            m_wellPathAttributePlotObjects.push_back( std::move( plotItem ) );
            completionsAssignedToLegend.insert( legendTitle );
        }

        RimDepthTrackPlot*            wellLogPlot      = m_track->firstAncestorOrThisOfTypeAsserted<RimDepthTrackPlot>();
        RimWellLogPlot::DepthTypeEnum depthType        = wellLogPlot->depthType();
        auto                          depthOrientation = wellLogPlot->depthOrientation();

        RiuQwtPlotWidget* plotWidget = m_track->viewer();

        for ( auto& attributePlotObject : m_wellPathAttributePlotObjects )
        {
            attributePlotObject->setDepthType( depthType );
            attributePlotObject->setDepthOrientation( depthOrientation );
            attributePlotObject->setShowLabel( m_track->m_showWellPathComponentLabels.value() );
            attributePlotObject->loadDataAndUpdate( false );
            attributePlotObject->setParentPlotNoReplot( plotWidget->qwtPlot() );
        }
    }
    m_track->updatePropertyValueZoom();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellLogTrackWellPathComponents::updateWellPathAttributesCollection()
{
    m_track->m_wellPathAttributeCollection = nullptr;

    RimWellPath* wellPathComponentSource = m_track->wellPathAttributeSource();
    if ( wellPathComponentSource )
    {
        std::vector<RimWellPathAttributeCollection*> attributeCollection =
            wellPathComponentSource->descendantsIncludingThisOfType<RimWellPathAttributeCollection>();
        if ( !attributeCollection.empty() )
        {
            m_track->m_wellPathAttributeCollection = attributeCollection.front();
        }
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellLogTrackWellPathComponents::detachAllPlotItems()
{
    for ( auto& plotObjects : m_wellPathAttributePlotObjects )
    {
        plotObjects->detachFromQwt();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RimWellLogTrackWellPathComponents::reattachAllPlotItems()
{
    for ( auto& plotObjects : m_wellPathAttributePlotObjects )
    {
        plotObjects->reattachToQwt();
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RimWellLogTrackWellPathComponents::depthValueRange( double* minimumDepth, double* maximumDepth ) const
{
    double minDepth = HUGE_VAL;
    double maxDepth = -HUGE_VAL;

    for ( const std::unique_ptr<RiuWellPathComponentPlotItem>& plotObject : m_wellPathAttributePlotObjects )
    {
        double minObjectDepth = HUGE_VAL;
        double maxObjectDepth = -HUGE_VAL;
        if ( plotObject->depthValueRange( &minObjectDepth, &maxObjectDepth ) )
        {
            if ( minObjectDepth < minDepth )
            {
                minDepth = minObjectDepth;
            }

            if ( maxObjectDepth > maxDepth )
            {
                maxDepth = maxObjectDepth;
            }
        }
    }

    if ( minDepth != HUGE_VAL && maxDepth != -HUGE_VAL )
    {
        *minimumDepth = minDepth;
        *maximumDepth = maxDepth;
        return true;
    }

    return false;
}
