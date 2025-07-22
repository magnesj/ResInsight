/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2025     Equinor ASA
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

#include "RifOpmSummaryTools.h"

#ifdef _MSC_VER
// Disable warning from external library to make sure treat warnings as error works
#pragma warning( disable : 4267 )
#endif
#include "opm/io/eclipse/ESmry.hpp"
#include "opm/io/eclipse/ExtESmry.hpp"

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::tuple<std::set<RifEclipseSummaryAddress>, std::map<RifEclipseSummaryAddress, size_t>, std::map<RifEclipseSummaryAddress, std::string>>
    RifOpmSummaryTools::buildAddressesSmspecAndKeywordMap( const Opm::EclIO::ESmry* summaryFile )
{
    std::set<RifEclipseSummaryAddress>              addresses;
    std::map<RifEclipseSummaryAddress, size_t>      addressToSmspecIndexMap;
    std::map<RifEclipseSummaryAddress, std::string> addressToKeywordMap;

    if ( summaryFile )
    {
        auto keywords = summaryFile->keywordList();
        for ( const auto& keyword : keywords )
        {
            auto eclAdr = RifEclipseSummaryAddress::fromEclipseTextAddress( keyword );
            if ( !eclAdr.isValid() )
            {
                // If a category is not found, use the MISC category
                eclAdr = RifEclipseSummaryAddress::miscAddress( keyword );
            }

            if ( eclAdr.isValid() )
            {
                addresses.insert( eclAdr );
                size_t smspecIndex              = summaryFile->getSmspecIndexForKeyword( keyword );
                addressToSmspecIndexMap[eclAdr] = smspecIndex;
                addressToKeywordMap[eclAdr]     = keyword;
            }
        }
    }

    return { addresses, addressToSmspecIndexMap, addressToKeywordMap };
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::pair<std::set<RifEclipseSummaryAddress>, std::map<RifEclipseSummaryAddress, std::string>>
    RifOpmSummaryTools::buildAddressesAndKeywordMap( const std::vector<std::string>& keywords )
{
    std::set<RifEclipseSummaryAddress>              addresses;
    std::map<RifEclipseSummaryAddress, std::string> addressToKeywordMap;

    std::vector<std::string> invalidKeywords;

#pragma omp parallel
    {
        std::vector<RifEclipseSummaryAddress>                         threadAddresses;
        std::vector<std::pair<RifEclipseSummaryAddress, std::string>> threadAddressToKeywordMap;
        std::vector<std::string>                                      threadInvalidKeywords;

#pragma omp for
        for ( int index = 0; index < (int)keywords.size(); index++ )
        {
            auto keyword = keywords[index];

            auto eclAdr = RifEclipseSummaryAddress::fromEclipseTextAddress( keyword );
            if ( !eclAdr.isValid() )
            {
                threadInvalidKeywords.push_back( keyword );

                // If a category is not found, use the MISC category
                eclAdr = RifEclipseSummaryAddress::miscAddress( keyword );
            }

            if ( eclAdr.isValid() )
            {
                threadAddresses.emplace_back( eclAdr );
                threadAddressToKeywordMap.emplace_back( std::make_pair( eclAdr, keyword ) );
            }
        }

#pragma omp critical
        {
            addresses.insert( threadAddresses.begin(), threadAddresses.end() );
            addressToKeywordMap.insert( threadAddressToKeywordMap.begin(), threadAddressToKeywordMap.end() );
            invalidKeywords.insert( invalidKeywords.end(), threadInvalidKeywords.begin(), threadInvalidKeywords.end() );
        }

        // DEBUG code
        // Used to print keywords not being categorized correctly
        /*
            for ( const auto& kw : invalidKeywords )
            {
                RiaLogging::warning( QString::fromStdString( kw ) );
            }
        */
    }

    return { addresses, addressToKeywordMap };
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RifEclipseSummaryAddressDefines::SummaryCategory RifOpmSummaryTools::categoryFromKeyword( const std::string& keyword )
{
    auto opmCategory = Opm::EclIO::SummaryNode::category_from_keyword( keyword );
    switch ( opmCategory )
    {
        case Opm::EclIO::SummaryNode::Category::Aquifer:
            return SummaryCategory::SUMMARY_AQUIFER;
        case Opm::EclIO::SummaryNode::Category::Block:
            return SummaryCategory::SUMMARY_BLOCK;
        case Opm::EclIO::SummaryNode::Category::Connection:
            return SummaryCategory::SUMMARY_WELL_CONNECTION;
        case Opm::EclIO::SummaryNode::Category::Completion:
            return SummaryCategory::SUMMARY_WELL_COMPLETION;
        case Opm::EclIO::SummaryNode::Category::Field:
            return SummaryCategory::SUMMARY_FIELD;
        case Opm::EclIO::SummaryNode::Category::Group:
            return SummaryCategory::SUMMARY_GROUP;
    }

    return SummaryCategory::SUMMARY_INVALID;
}
