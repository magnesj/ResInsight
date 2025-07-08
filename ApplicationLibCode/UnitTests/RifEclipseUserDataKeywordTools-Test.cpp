#include "gtest/gtest.h"

#include "RifEclipseUserDataKeywordTools.h"

//--------------------------------------------------------------------------------------------------
/// Tests the requiredItemsPerLineForKeyword function which determines how many data items
/// should appear on each line for different Eclipse user data keywords. This is essential for
/// proper parsing and formatting of Eclipse summary data files.
//--------------------------------------------------------------------------------------------------
TEST( RifEclipseUserDataKeywordToolsTest, TestIdentifierItemsPerLine )
{
    {
        // Test keyword "AA" - should return empty vector (no specific line format required)
        std::string s = "AA";
        EXPECT_EQ( size_t( 0 ), RifEclipseUserDataKeywordTools::requiredItemsPerLineForKeyword( s ).size() );
    }
    {
        // Test keyword "BB" - should require 3 items per line  
        std::string s = "BB";
        EXPECT_EQ( size_t( 3 ), RifEclipseUserDataKeywordTools::requiredItemsPerLineForKeyword( s )[0] );
    }
    {
        // Test keyword "CC" - should have two line formats: 1 item, then 3 items
        std::string s = "CC";
        EXPECT_EQ( size_t( 1 ), RifEclipseUserDataKeywordTools::requiredItemsPerLineForKeyword( s )[0] );
        EXPECT_EQ( size_t( 3 ), RifEclipseUserDataKeywordTools::requiredItemsPerLineForKeyword( s )[1] );
    }
    {
        // Test keyword "FF" - should return empty vector (no specific line format required)
        std::string s = "FF";
        EXPECT_EQ( size_t( 0 ), RifEclipseUserDataKeywordTools::requiredItemsPerLineForKeyword( s ).size() );
    }
    {
        // Test keyword "GG" - should require 1 item per line
        std::string s = "GG";
        EXPECT_EQ( size_t( 1 ), RifEclipseUserDataKeywordTools::requiredItemsPerLineForKeyword( s )[0] );
    }
    {
        // Test keyword "NN" - should return empty vector (no specific line format required)
        std::string s = "NN";
        EXPECT_EQ( size_t( 0 ), RifEclipseUserDataKeywordTools::requiredItemsPerLineForKeyword( s ).size() );
    }
    {
        // Test keyword "RR" - should require 1 item per line
        std::string s = "RR";
        EXPECT_EQ( size_t( 1 ), RifEclipseUserDataKeywordTools::requiredItemsPerLineForKeyword( s )[0] );
    }
    {
        // Test keyword "SS" - should have two line formats: 1 item, then 1 item
        std::string s = "SS";
        EXPECT_EQ( size_t( 1 ), RifEclipseUserDataKeywordTools::requiredItemsPerLineForKeyword( s )[0] );
        EXPECT_EQ( size_t( 1 ), RifEclipseUserDataKeywordTools::requiredItemsPerLineForKeyword( s )[1] );
    }
    {
        // Test keyword "WW" - should require 1 item per line
        std::string s = "WW";
        EXPECT_EQ( size_t( 1 ), RifEclipseUserDataKeywordTools::requiredItemsPerLineForKeyword( s )[0] );
    }
    {
        // Test keyword "LB" - should have two line formats: 1 item, then 3 items (LGR block data)
        std::string s = "LB";
        EXPECT_EQ( size_t( 1 ), RifEclipseUserDataKeywordTools::requiredItemsPerLineForKeyword( s )[0] );
        EXPECT_EQ( size_t( 3 ), RifEclipseUserDataKeywordTools::requiredItemsPerLineForKeyword( s )[1] );
    }
    {
        // Test keyword "LC" - should have three line formats: 1, 1, 3 items (LGR completion data)
        std::string s = "LC";
        EXPECT_EQ( size_t( 1 ), RifEclipseUserDataKeywordTools::requiredItemsPerLineForKeyword( s )[0] );
        EXPECT_EQ( size_t( 1 ), RifEclipseUserDataKeywordTools::requiredItemsPerLineForKeyword( s )[1] );
        EXPECT_EQ( size_t( 3 ), RifEclipseUserDataKeywordTools::requiredItemsPerLineForKeyword( s )[2] );
    }
    {
        // Test keyword "LW" - should have two line formats: 1 item, then 1 item (LGR well data)
        std::string s = "LW";
        EXPECT_EQ( size_t( 1 ), RifEclipseUserDataKeywordTools::requiredItemsPerLineForKeyword( s )[0] );
        EXPECT_EQ( size_t( 1 ), RifEclipseUserDataKeywordTools::requiredItemsPerLineForKeyword( s )[1] );
    }
}

//--------------------------------------------------------------------------------------------------
/// Tests the buildColumnHeaderText function which constructs table headers from Eclipse keywords
/// and header data lines. This is a basic test case with standard TIME, YEARX, and well-related
/// keywords (WGT1, WR42) to verify proper header construction.
//--------------------------------------------------------------------------------------------------
TEST( RifEclipseUserDataKeywordToolsTest, BuildTableHeaderText )
{
    // Set up test data with 4 keywords and corresponding header data
    std::vector<std::string>              keywordNames = { "TIME", "YEARX", "WGT1", "WR42" };
    std::vector<std::string>              firstheader  = { "OP-1", "OP-1" }; // Well names for WGT1 and WR42
    std::vector<std::vector<std::string>> headerLines  = { firstheader };

    auto tableHeaderData = RifEclipseUserDataKeywordTools::buildColumnHeaderText( keywordNames, headerLines );
    
    // Verify we get the expected number of columns (4) 
    EXPECT_EQ( size_t( 4 ), tableHeaderData.size() );
    
    // TIME and YEARX keywords don't require header data (size 1 each)
    EXPECT_EQ( size_t( 1 ), tableHeaderData[0].size() );
    EXPECT_EQ( size_t( 1 ), tableHeaderData[1].size() );
    
    // WGT1 and WR42 keywords use the well names from header lines (size 2 each)
    EXPECT_EQ( size_t( 2 ), tableHeaderData[2].size() );
    EXPECT_EQ( size_t( 2 ), tableHeaderData[3].size() );
}

//--------------------------------------------------------------------------------------------------
/// Tests buildColumnHeaderText with a more complex scenario involving multiple header lines
/// and various Eclipse keywords including field variables (FVIR), region data (RPR), well data (GOPR),
/// completion data (CWIR), and field totals (FTPTS36). This tests the function's ability to handle
/// multiple header data lines and complex keyword combinations.
//--------------------------------------------------------------------------------------------------
TEST( RifEclipseUserDataKeywordToolsTest, BuildTableHeaderTextComplex )
{
    // Set up complex test data with 8 keywords and two header lines
    std::vector<std::string>              keywordNames = { "TIME", "WGT1", "FVIR", "RPR", "GOPR", "CWIR", "FTPTS36", "CWIR" };
    std::vector<std::string>              firstheader  = { "OP-1", "8", "MANI-D2", "F-2H", "2H" };        // Well names and region numbers
    std::vector<std::string>              secondHeader = { "18", "83", "3", "9", "8", "7" };             // Additional identifiers for completions
    std::vector<std::vector<std::string>> headerLines  = { firstheader, secondHeader };

    auto tableHeaderData = RifEclipseUserDataKeywordTools::buildColumnHeaderText( keywordNames, headerLines );
    
    // Verify we get the expected number of columns (8)
    EXPECT_EQ( size_t( 8 ), tableHeaderData.size() );

    // TIME keyword doesn't require header data
    EXPECT_EQ( size_t( 1 ), tableHeaderData[0].size() );
    
    // WGT1 uses well name from first header line
    EXPECT_EQ( size_t( 2 ), tableHeaderData[1].size() );
    
    // FVIR uses region number from first header line
    EXPECT_EQ( size_t( 1 ), tableHeaderData[2].size() );
    
    // RPR uses well name from first header line plus completion data from second header line
    EXPECT_EQ( size_t( 2 ), tableHeaderData[3].size() );
    
    // GOPR uses well name from first header line plus completion data from second header line
    EXPECT_EQ( size_t( 2 ), tableHeaderData[4].size() );
    
    // CWIR uses well name from first header and completion triplet from second header (well + i,j,k)
    EXPECT_EQ( size_t( 5 ), tableHeaderData[5].size() );
    
    // FTPTS36 uses field total identifier
    EXPECT_EQ( size_t( 1 ), tableHeaderData[6].size() );
    
    // Second CWIR instance uses the same pattern as first CWIR
    EXPECT_EQ( size_t( 5 ), tableHeaderData[7].size() );
}

//--------------------------------------------------------------------------------------------------
/// Tests error handling in buildColumnHeaderText when header data is missing or incomplete.
/// This ensures the function properly validates input data and returns empty results when
/// required header information is not provided, preventing crashes during Eclipse data parsing.
//--------------------------------------------------------------------------------------------------
TEST( RifEclipseUserDataKeywordToolsTest, MissingHeaderData )
{
    {
        // Test case 1: Missing well name for WGT1 keyword
        std::vector<std::string>              keywordNames = { "TIME", "WGT1" };
        std::vector<std::string>              firstheader  = {}; // Missing well name - should cause failure
        std::vector<std::vector<std::string>> headerLines  = { firstheader };

        auto tableHeaderData = RifEclipseUserDataKeywordTools::buildColumnHeaderText( keywordNames, headerLines );
        // Should return empty result when required header data is missing
        EXPECT_EQ( size_t( 0 ), tableHeaderData.size() );
    }

    {
        // Test case 2: Incomplete triplet data in second header line 
        std::vector<std::string>              keywordNames = { "TIME", "WGT1", "FVIR", "RPR", "GOPR", "CWIR", "FTPTS36", "CWIR" };
        std::vector<std::string>              firstheader  = { "OP-1", "8", "MANI-D2", "F-2H", "2H" };
        std::vector<std::string>              secondHeader = { "18", "83", "3", "9", "8" }; // Missing value from last completion triplet
        std::vector<std::vector<std::string>> headerLines  = { firstheader, secondHeader };

        auto tableHeaderData = RifEclipseUserDataKeywordTools::buildColumnHeaderText( keywordNames, headerLines );
        // Should return empty result when header data is incomplete
        EXPECT_EQ( size_t( 0 ), tableHeaderData.size() );
    }
}

//--------------------------------------------------------------------------------------------------
/// Tests the creation of different types of Eclipse summary addresses from user data keywords.
/// This comprehensive test covers all major Eclipse summary address categories including region,
/// well group, well, completion, LGR (Local Grid Refinement), segment, and block data.
/// Verifies that the makeAndFillAddress function correctly interprets quantity names and 
/// column data to create properly categorized summary addresses.
//--------------------------------------------------------------------------------------------------
TEST( RifEclipseUserDataKeywordToolsTest, CreationOfSummaryAddresses )
{
    // Test Region summary address creation
    {
        std::string              quantity   = "RGT1";        // Region keyword
        std::vector<std::string> columnData = { "1" };       // Region number

        auto address = RifEclipseUserDataKeywordTools::makeAndFillAddress( quantity, columnData );

        EXPECT_EQ( address.category(), RifEclipseSummaryAddress::SUMMARY_REGION );
        EXPECT_STREQ( quantity.data(), address.quantityName().data() );
        EXPECT_EQ( 1, address.regionNumber() );
    }

    // Test Well Group summary address creation
    {
        std::string              quantity   = "GT1";         // Group keyword
        std::vector<std::string> columnData = { "OP-1" };    // Group name

        auto address = RifEclipseUserDataKeywordTools::makeAndFillAddress( quantity, columnData );

        EXPECT_EQ( address.category(), RifEclipseSummaryAddress::SUMMARY_WELL_GROUP );
        EXPECT_STREQ( quantity.data(), address.quantityName().data() );
        EXPECT_STREQ( columnData[0].data(), address.groupName().data() );
    }

    // Test Well summary address creation
    {
        std::string              quantity   = "WGT1";        // Well keyword
        std::vector<std::string> columnData = { "OP-1" };    // Well name

        auto address = RifEclipseUserDataKeywordTools::makeAndFillAddress( quantity, columnData );

        EXPECT_EQ( address.category(), RifEclipseSummaryAddress::SUMMARY_WELL );
        EXPECT_STREQ( quantity.data(), address.quantityName().data() );
        EXPECT_STREQ( columnData[0].data(), address.wellName().data() );
    }

    // Test Well Completion summary address creation
    {
        std::string              quantity   = "CWIT";                    // Completion keyword
        std::vector<std::string> columnData = { "F-3H", "1", "2", "3" }; // Well name + grid coordinates (i,j,k)

        auto address = RifEclipseUserDataKeywordTools::makeAndFillAddress( quantity, columnData );

        EXPECT_EQ( address.category(), RifEclipseSummaryAddress::SUMMARY_WELL_COMPLETION );
        EXPECT_STREQ( quantity.data(), address.quantityName().data() );
        EXPECT_STREQ( columnData[0].data(), address.wellName().data() );
        EXPECT_EQ( 1, address.cellI() );
        EXPECT_EQ( 2, address.cellJ() );
        EXPECT_EQ( 3, address.cellK() );
    }

    // Test Well LGR (Local Grid Refinement) summary address creation
    {
        std::string              quantity   = "LWGT1";                   // LGR well keyword
        std::vector<std::string> columnData = { "OP-1", "LGR-NAME" };   // Well name + LGR name

        auto address = RifEclipseUserDataKeywordTools::makeAndFillAddress( quantity, columnData );

        EXPECT_EQ( address.category(), RifEclipseSummaryAddress::SUMMARY_WELL_LGR );
        EXPECT_STREQ( quantity.data(), address.quantityName().data() );
        EXPECT_STREQ( columnData[0].data(), address.wellName().data() );
        EXPECT_STREQ( columnData[1].data(), address.lgrName().data() );
    }

    // Test Well Completion LGR summary address creation
    {
        std::string              quantity   = "LC";                                       // LGR completion keyword
        std::vector<std::string> columnData = { "F-3H", "LGR-NAME", "1", "2", "3" };     // Well + LGR + coordinates

        auto address = RifEclipseUserDataKeywordTools::makeAndFillAddress( quantity, columnData );

        EXPECT_EQ( address.category(), RifEclipseSummaryAddress::SUMMARY_WELL_COMPLETION_LGR );
        EXPECT_STREQ( quantity.data(), address.quantityName().data() );
        EXPECT_STREQ( columnData[0].data(), address.wellName().data() );
        EXPECT_STREQ( columnData[1].data(), address.lgrName().data() );
        EXPECT_EQ( 1, address.cellI() );
        EXPECT_EQ( 2, address.cellJ() );
        EXPECT_EQ( 3, address.cellK() );
    }

    // Test Well Segment summary address creation
    {
        std::string              quantity   = "SCWIT";               // Segment keyword
        std::vector<std::string> columnData = { "F-3H", "1" };      // Well name + segment number

        auto address = RifEclipseUserDataKeywordTools::makeAndFillAddress( quantity, columnData );

        EXPECT_EQ( address.category(), RifEclipseSummaryAddress::SUMMARY_WELL_SEGMENT );
        EXPECT_STREQ( quantity.data(), address.quantityName().data() );
        EXPECT_STREQ( columnData[0].data(), address.wellName().data() );
        EXPECT_EQ( 1, address.wellSegmentNumber() );
    }

    // Test Block summary address creation
    {
        std::string              quantity   = "BWIT";                // Block keyword
        std::vector<std::string> columnData = { "1", "2", "3" };    // Grid coordinates (i,j,k)

        auto address = RifEclipseUserDataKeywordTools::makeAndFillAddress( quantity, columnData );

        EXPECT_EQ( address.category(), RifEclipseSummaryAddress::SUMMARY_BLOCK );
        EXPECT_STREQ( quantity.data(), address.quantityName().data() );
        EXPECT_EQ( 1, address.cellI() );
        EXPECT_EQ( 2, address.cellJ() );
        EXPECT_EQ( 3, address.cellK() );
    }

    // Test Block LGR summary address creation
    {
        std::string              quantity   = "LBWIT";                          // LGR block keyword
        std::vector<std::string> columnData = { "LGR-name", "1", "2", "3" };   // LGR name + coordinates

        auto address = RifEclipseUserDataKeywordTools::makeAndFillAddress( quantity, columnData );

        EXPECT_EQ( address.category(), RifEclipseSummaryAddress::SUMMARY_BLOCK_LGR );
        EXPECT_STREQ( quantity.data(), address.quantityName().data() );
        EXPECT_STREQ( columnData[0].data(), address.lgrName().data() );
        EXPECT_EQ( 1, address.cellI() );
        EXPECT_EQ( 2, address.cellJ() );
        EXPECT_EQ( 3, address.cellK() );
    }
}

//--------------------------------------------------------------------------------------------------
/// Tests the fallback behavior for unrecognized Eclipse keywords. When a quantity name is not
/// recognized as belonging to any specific category (region, well, completion, etc.), it should
/// be classified as SUMMARY_MISC (miscellaneous). This ensures robust handling of custom or
/// non-standard Eclipse summary keywords.
//--------------------------------------------------------------------------------------------------
TEST( RifEclipseUserDataKeywordToolsTest, CreationOfMisc )
{
    // Test unrecognized keyword classification as MISC
    {
        std::string              quantity   = "JI-NOT-REQOGNIZED";  // Custom/unrecognized keyword
        std::vector<std::string> columnData = {};                   // No column data needed for misc
        auto                     address    = RifEclipseUserDataKeywordTools::makeAndFillAddress( quantity, columnData );

        EXPECT_EQ( address.category(), RifEclipseSummaryAddress::SUMMARY_MISC );
        EXPECT_STREQ( quantity.data(), address.quantityName().data() );
    }
}
