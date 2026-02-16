
#include "gtest/gtest.h"

#include "cafPdmUiNumberFormat.h"

using namespace caf;

TEST( PdmUiNumberFormatTest, ValueToText )
{
    struct TestCase
    {
        double           value;
        NumberFormatType format;
        int              precision;
        std::string      expected;
    };

    std::vector<TestCase> testCases = {
        // Fixed format
        { 3.14159, NumberFormatType::FIXED, 2, "3.14" },
        { 0.0, NumberFormatType::FIXED, 3, "0.000" },
        { -1.5, NumberFormatType::FIXED, 1, "-1.5" },
        { 1.0 / 3.0, NumberFormatType::FIXED, 8, "0.33333333" },
        { 3.7, NumberFormatType::FIXED, 0, "4" },
        { -2.3, NumberFormatType::FIXED, 0, "-2" },
        { 1234567.89, NumberFormatType::FIXED, 2, "1234567.89" },

        // Scientific format
        { 1000.0, NumberFormatType::SCIENTIFIC, 2, "1.00e+03" },
        { 0.001, NumberFormatType::SCIENTIFIC, 1, "1.0e-03" },
        { -5.67e8, NumberFormatType::SCIENTIFIC, 3, "-5.670e+08" },
        { 1.23e-10, NumberFormatType::SCIENTIFIC, 2, "1.23e-10" },

        // Auto format
        { 42.0, NumberFormatType::AUTO, 6, "42" },
        { 0.000123, NumberFormatType::AUTO, 3, "0.000123" },
        { 0.0000001, NumberFormatType::AUTO, 3, "1e-07" },
        { 1.23456e8, NumberFormatType::AUTO, 4, "1.235e+08" },
        { 3.14159265, NumberFormatType::AUTO, 2, "3.1" },
        { 3.14159265, NumberFormatType::AUTO, 6, "3.14159" },
    };

    for ( const auto& tc : testCases )
    {
        QString result = PdmUiNumberFormat::valueToText( tc.value, tc.format, tc.precision );
        EXPECT_EQ( result.toStdString(), tc.expected )
            << "Failed for value=" << tc.value << " format=" << static_cast<int>( tc.format )
            << " precision=" << tc.precision;
    }
}
