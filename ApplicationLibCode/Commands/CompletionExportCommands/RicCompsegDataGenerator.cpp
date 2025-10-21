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

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RigCompsegData> RicCompsegDataGenerator::generateCompsegData( const RicMswExportInfo& exportInfo )
{
    std::vector<RigCompsegData> compsegData;
    
    QString wellName = exportInfo.mainBoreBranch()->wellPath()->completionSettings()->wellNameForExport();
    
    processSegmentsRecursively( exportInfo.mainBoreBranch(), wellName, compsegData );
    
    return compsegData;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void RicCompsegDataGenerator::processSegmentsRecursively( const RicMswBranch*          branch,
                                                         const QString&               wellName,
                                                         std::vector<RigCompsegData>& compsegData )
{
    for ( auto segment : branch->segments() )
    {
        for ( auto intersection : segment->intersections() )
        {
            RigCompletionDataGridCell gridCell( intersection->globalCellIndex(), nullptr );
            
            RigCompsegData data( wellName,
                                gridCell,
                                branch->branchNumber(),
                                segment->startMD(),
                                segment->endMD() );
            
            compsegData.push_back( data );
        }
        
        // Process completions on this segment
        for ( auto completion : segment->completions() )
        {
            auto mswCompletion = dynamic_cast<const RicMswCompletion*>( completion );
            if ( mswCompletion && !mswCompletion->segments().empty() )
            {
                processSegmentsRecursively( mswCompletion, wellName, compsegData );
            }
        }
    }
    
    // Process child branches
    for ( auto childBranch : branch->branches() )
    {
        processSegmentsRecursively( childBranch, wellName, compsegData );
    }
}