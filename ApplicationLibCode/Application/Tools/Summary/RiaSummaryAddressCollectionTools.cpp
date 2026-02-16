/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2026     Equinor ASA
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

#include "RiaSummaryAddressCollectionTools.h"

#include "RifEclipseSummaryAddress.h"
#include "RifEclipseSummaryAddressDefines.h"

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::string RiaSummaryAddressCollectionTools::objectIdentifierForAddress( const RifEclipseSummaryAddress&                    address,
                                                                          RimSummaryAddressCollection::CollectionContentType contentType )
{
    using Category    = RifEclipseSummaryAddressDefines::SummaryCategory;
    using ContentType = RimSummaryAddressCollection::CollectionContentType;

    if ( ( address.category() == Category::SUMMARY_WELL ) && ( contentType == ContentType::WELL ) )
    {
        return address.wellName();
    }
    else if ( ( address.category() == Category::SUMMARY_GROUP ) && ( contentType == ContentType::GROUP ) )
    {
        return address.groupName();
    }
    else if ( ( address.category() == Category::SUMMARY_NETWORK ) && ( contentType == ContentType::NETWORK ) )
    {
        return address.networkName();
    }
    else if ( ( address.category() == Category::SUMMARY_REGION ) && ( contentType == ContentType::REGION ) )
    {
        return std::to_string( address.regionNumber() );
    }
    else if ( ( address.category() == Category::SUMMARY_WELL_SEGMENT ) && ( contentType == ContentType::WELL_SEGMENT ) )
    {
        return std::to_string( address.wellSegmentNumber() );
    }

    return {};
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RiaSummaryCurveDefinition>
    RiaSummaryAddressCollectionTools::buildCurveDefs( const std::vector<RiaSummaryCurveDefinition>&      sourceCurveDefs,
                                                      const std::string&                                 droppedName,
                                                      RimSummaryAddressCollection::CollectionContentType contentType,
                                                      bool                                               appendHistoryVectors )
{
    std::set<RiaSummaryCurveDefinition> uniqueCurveDefs;

    for ( const auto& curveDef : sourceCurveDefs )
    {
        auto       newCurveDef = curveDef;
        const auto curveAdr    = newCurveDef.summaryAddressY();

        std::string objectIdentifierString = objectIdentifierForAddress( curveAdr, contentType );

        if ( !objectIdentifierString.empty() )
        {
            newCurveDef.setIdentifierText( curveAdr.category(), droppedName );
            uniqueCurveDefs.insert( newCurveDef );

            const auto& addr = curveDef.summaryAddressY();
            if ( !addr.isHistoryVector() && appendHistoryVectors )
            {
                auto historyAddr = addr;
                historyAddr.setVectorName( addr.vectorName() + RifEclipseSummaryAddressDefines::historyIdentifier() );

                auto historyCurveDef = newCurveDef;
                historyCurveDef.setSummaryAddressY( historyAddr );
                uniqueCurveDefs.insert( historyCurveDef );
            }
        }
    }

    return { uniqueCurveDefs.begin(), uniqueCurveDefs.end() };
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<RiaSummaryCurveDefinition> RiaSummaryAddressCollectionTools::removeExistingCurveDefs(
    const std::vector<RiaSummaryCurveDefinition>&                            candidateCurveDefs,
    const std::map<RifEclipseSummaryAddress, std::set<RimSummaryCase*>>&     existingSummaryCurves,
    const std::map<RifEclipseSummaryAddress, std::set<RimSummaryEnsemble*>>& existingEnsembleCurves )
{
    std::vector<RiaSummaryCurveDefinition> filtered;

    for ( const auto& curveDef : candidateCurveDefs )
    {
        const auto& addr = curveDef.summaryAddressY();

        if ( curveDef.ensemble() )
        {
            auto it = existingEnsembleCurves.find( addr );
            if ( it != existingEnsembleCurves.end() && it->second.count( curveDef.ensemble() ) > 0 ) continue;
        }
        else if ( curveDef.summaryCaseY() )
        {
            auto it = existingSummaryCurves.find( addr );
            if ( it != existingSummaryCurves.end() && it->second.count( curveDef.summaryCaseY() ) > 0 ) continue;
        }

        filtered.push_back( curveDef );
    }

    return filtered;
}
