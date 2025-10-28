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

#include "RicWellSegmentDataTools.h"

#include "RicMswBranch.h"
#include "RicMswSegment.h"
#include "RicWellSegmentData.h"

#include "RifTextDataTableFormatter.h"

#include "cvfMath.h"

#include <cmath>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicWellSegmentDataTools::writeWelsegsTable( RifTextDataTableFormatter&             formatter,
                                                 const std::vector<RicWellSegmentData>& segmentData,
                                                 const QString&                         wellName,
                                                 double                                 startTVD,
                                                 double                                 startMD,
                                                 double                                 topWellBoreVolume,
                                                 const QString&                         lengthAndDepthText,
                                                 const QString&                         pressureDropText )
{
    formatter.keyword( "WELSEGS" );

    // Write well header
    {
        std::vector<RifTextDataTableColumn> header = {
            RifTextDataTableColumn( "Name" ),
            RifTextDataTableColumn( "Dep 1" ),
            RifTextDataTableColumn( "Tlen 1" ),
            RifTextDataTableColumn( "Vol 1" ),
            RifTextDataTableColumn( "Len&Dep" ),
            RifTextDataTableColumn( "PresDrop" ),
        };
        formatter.header( header );

        formatter.add( wellName );
        formatter.add( startTVD );
        formatter.add( startMD );
        formatter.addValueOrDefaultMarker( topWellBoreVolume, RicWellSegmentData::defaultValue() );
        formatter.add( lengthAndDepthText );
        formatter.add( QString( "'%1'" ).arg( pressureDropText ) );

        formatter.rowCompleted();
    }

    // Write segment data header
    {
        std::vector<RifTextDataTableColumn> header = { RifTextDataTableColumn( "First Seg" ),
                                                       RifTextDataTableColumn( "Last Seg" ),
                                                       RifTextDataTableColumn( "Branch Num" ),
                                                       RifTextDataTableColumn( "Outlet Seg" ),
                                                       RifTextDataTableColumn( "Length" ),
                                                       RifTextDataTableColumn( "Depth Change" ),
                                                       RifTextDataTableColumn( "Diam" ),
                                                       RifTextDataTableColumn( "Rough", RifTextDataTableDoubleFormatting( RIF_FLOAT, 7 ) ) };
        formatter.header( header );
    }

    // Write segment data
    for ( const auto& segment : segmentData )
    {
        if ( !segment.comment().isEmpty() )
        {
            formatter.addOptionalComment( segment.comment() );
        }

        formatter.add( segment.segmentNumber() );
        formatter.add( segment.segmentNumber() );
        formatter.add( segment.branch() );
        formatter.add( segment.joinSegment() );

        if ( !RicWellSegmentData::isDefaultValue( segment.length() ) )
            formatter.add( segment.length() );
        else
            formatter.addValueOrDefaultMarker( segment.length(), RicWellSegmentData::defaultValue() );

        if ( !RicWellSegmentData::isDefaultValue( segment.depth() ) )
            formatter.add( segment.depth() );
        else
            formatter.addValueOrDefaultMarker( segment.depth(), RicWellSegmentData::defaultValue() );

        if ( !RicWellSegmentData::isDefaultValue( segment.diameter() ) )
            formatter.add( segment.diameter() );
        else
            formatter.addValueOrDefaultMarker( segment.diameter(), RicWellSegmentData::defaultValue() );

        if ( !RicWellSegmentData::isDefaultValue( segment.roughness() ) )
            formatter.add( segment.roughness() );
        else
            formatter.addValueOrDefaultMarker( segment.roughness(), RicWellSegmentData::defaultValue() );

        formatter.rowCompleted();
    }

    formatter.tableCompleted();
}
