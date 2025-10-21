/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2025-     Equinor ASA
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

#include "RicCompsegDataGenerator.h"

#include "RicMswBranch.h"
#include "RicMswCompletions.h"
#include "RicMswExportInfo.h"
#include "RicMswSegment.h"
#include "RicMswSegmentCellIntersection.h"

#include "RimWellPath.h"
#include "RimWellPathCompletionSettings.h"

#include <algorithm>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RigCompsegData> RicCompsegDataGenerator::generateCompsegData( const RicMswExportInfo& exportInfo, const RigMainGrid* mainGrid )
{
    std::vector<RigCompsegData> compsegData;

    processSegmentsRecursively( exportInfo.mainBoreBranch(), mainGrid, compsegData );

    return compsegData;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicCompsegDataGenerator::processSegmentsRecursively( const RicMswBranch*          branch,
                                                          const RigMainGrid*           mainGrid,
                                                          std::vector<RigCompsegData>& compsegData )
{
    if ( !branch ) return;

    for ( auto segment : branch->segments() )
    {
        if ( !segment ) continue;

        for ( auto intersection : segment->intersections() )
        {
            if ( !intersection ) continue;

            RigCompletionDataGridCell gridCell( intersection->globalCellIndex(), mainGrid );

            // Determine completion type from branch context
            RigCompletionData::CompletionType completionType = RigCompletionData::CompletionType::CT_UNDEFINED;
            auto                              completion     = dynamic_cast<const RicMswCompletion*>( branch );
            if ( completion )
            {
                completionType = completion->completionType();
            }

            RigCompsegData data( gridCell, branch->branchNumber(), segment->startMD(), segment->endMD(), completionType );

            compsegData.push_back( data );
        }

        // Process completions on this segment
        for ( auto completion : segment->completions() )
        {
            auto mswCompletion = dynamic_cast<const RicMswCompletion*>( completion );
            if ( mswCompletion && !mswCompletion->segments().empty() )
            {
                processSegmentsRecursively( mswCompletion, mainGrid, compsegData );
            }
        }
    }

    // Process child branches
    for ( auto childBranch : branch->branches() )
    {
        processSegmentsRecursively( childBranch, mainGrid, compsegData );
    }
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RigCompsegData> RicCompsegDataGenerator::mainGridData( const std::vector<RigCompsegData>& data )
{
    std::vector<RigCompsegData> filtered;
    std::copy_if( data.begin(), data.end(), std::back_inserter( filtered ), []( const RigCompsegData& item ) { return item.isMainGrid(); } );
    return filtered;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RigCompsegData> RicCompsegDataGenerator::lgrData( const std::vector<RigCompsegData>& data )
{
    std::vector<RigCompsegData> filtered;
    std::copy_if( data.begin(), data.end(), std::back_inserter( filtered ), []( const RigCompsegData& item ) { return !item.isMainGrid(); } );
    return filtered;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RigCompsegData> RicCompsegDataGenerator::sortedData( const std::vector<RigCompsegData>& data )
{
    std::vector<RigCompsegData> sorted = data;
    std::sort( sorted.begin(), sorted.end() );
    return sorted;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool RicCompsegDataGenerator::isValidData( const RigCompsegData& data )
{
    return data.branchNumber() > 0 && data.startLength() < data.endLength();
}
