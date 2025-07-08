#include "gtest/gtest.h"

#include "RifInpExportTools.h"

#include <sstream>
#include <string>

//--------------------------------------------------------------------------------------------------
/// Tests the printLine function which outputs a simple text line to a stream. This basic
/// functionality is used throughout the INP export process to write various data lines
/// to the output file. Verifies that the line content is correctly written to the stream.
//--------------------------------------------------------------------------------------------------
TEST( RifInpExportTools, PrintLine )
{
    std::string line = "this is the line";

    std::stringstream stream;
    // Test that the function succeeds
    ASSERT_TRUE( RifInpExportTools::printLine( stream, line ) );

    // Verify that the line content appears in the output stream
    std::string res = stream.str();
    ASSERT_TRUE( res.find( line ) != std::string::npos );
}

//--------------------------------------------------------------------------------------------------
/// Tests the printHeading function which outputs a formatted heading line for INP files.
/// In INP format, headings are typically prefixed with asterisks (*) to mark them as
/// section headers. This test verifies that the heading is properly formatted and written.
//--------------------------------------------------------------------------------------------------
TEST( RifInpExportTools, PrintHeading )
{
    std::string line = "this is the heading";

    std::stringstream stream;
    // Test that the function succeeds
    ASSERT_TRUE( RifInpExportTools::printHeading( stream, line ) );

    // Verify that the heading appears with proper INP formatting (asterisk prefix)
    std::string res = stream.str();
    ASSERT_TRUE( res.find( '*' + line ) != std::string::npos );
}

//--------------------------------------------------------------------------------------------------
/// Tests the printComment function which outputs comment lines for INP files. In INP format,
/// comments are typically prefixed with double asterisks (**) to distinguish them from
/// headings and data. This test verifies proper comment formatting and output.
//--------------------------------------------------------------------------------------------------
TEST( RifInpExportTools, PrintComment )
{
    std::string line = "this is the comment";

    std::stringstream stream;
    // Test that the function succeeds
    ASSERT_TRUE( RifInpExportTools::printComment( stream, line ) );

    // Verify that the comment appears with proper INP comment formatting ("** " prefix)
    std::string expectedString = std::string( "** " ).append( line );
    std::string res            = stream.str();
    ASSERT_TRUE( res.find( expectedString ) != std::string::npos );
}

//--------------------------------------------------------------------------------------------------
/// Tests the printNodes function which exports mesh node coordinates to INP format. This function
/// is critical for finite element mesh export, as it writes the geometric node definitions that
/// define the mesh vertices. Tests both the formatting and line count to ensure proper INP structure.
//--------------------------------------------------------------------------------------------------
TEST( RifInpExportTools, PrintNodes )
{
    // Create test node coordinates (4 nodes with x,y,z coordinates)
    std::vector<cvf::Vec3d> nodes = {
        cvf::Vec3d( 1.0, 1.1, 1.2 ),
        cvf::Vec3d( 2.0, 2.1, 2.2 ),
        cvf::Vec3d( 3.0, 3.1, 3.2 ),
        cvf::Vec3d( 4.0, 4.1, 4.2 ),
    };

    std::stringstream stream;
    // Test that the function succeeds
    ASSERT_TRUE( RifInpExportTools::printNodes( stream, nodes ) );

    // Helper lambda to split output into lines for analysis
    auto splitLines = []( const std::string& input )
    {
        std::istringstream       stream( input );
        std::string              line;
        std::vector<std::string> lines;

        while ( std::getline( stream, line ) )
        {
            lines.push_back( line );
        }
        return lines;
    };

    auto lines = splitLines( stream.str() );
    // Expect 5 lines: 1 header line (*Node) + 4 node definition lines
    ASSERT_EQ( 5u, lines.size() );

    // Verify that the INP node section header is present
    std::string res = stream.str();
    ASSERT_TRUE( res.find( std::string( "*Node" ) ) != std::string::npos );
}

//--------------------------------------------------------------------------------------------------
/// Tests the printElements function which exports finite element connectivity data to INP format.
/// This function writes element definitions that specify how nodes are connected to form elements
/// in the finite element mesh. Tests both the formatting and line count for proper INP structure.
//--------------------------------------------------------------------------------------------------
TEST( RifInpExportTools, Elements )
{
    // Create test element connectivity data (5 elements with node connectivity)
    std::vector<std::vector<unsigned int>> elements = {
        { 1, 1, 1, 1 },     // Element with 4 node IDs
        { 2, 2, 2, 2 },
        { 3, 3, 3, 3 },
        { 4, 4, 4, 4 },
        { 5, 5, 5, 5 },
    };

    std::stringstream stream;
    // Test that the function succeeds
    ASSERT_TRUE( RifInpExportTools::printElements( stream, elements ) );

    // Helper lambda to split output into lines for analysis
    auto splitLines = []( const std::string& input )
    {
        std::istringstream       stream( input );
        std::string              line;
        std::vector<std::string> lines;

        while ( std::getline( stream, line ) )
        {
            lines.push_back( line );
        }
        return lines;
    };

    auto lines = splitLines( stream.str() );
    // Expect 6 lines: 1 header line (*Element) + 5 element definition lines
    ASSERT_EQ( 6u, lines.size() );

    // Verify that the INP element section header is present
    std::string res = stream.str();
    ASSERT_TRUE( res.find( std::string( "*Element" ) ) != std::string::npos );
}
