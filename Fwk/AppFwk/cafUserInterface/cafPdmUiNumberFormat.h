#pragma once

#include <QString>

namespace caf
{
class PdmUiNumberFormat
{
public:
    enum class NumberFormatType
    {
        AUTO,
        SCIENTIFIC,
        FIXED
    };

    static QString valueToText( double value, PdmUiNumberFormat::NumberFormatType numberFormat, int precision );
    static QString sprintfFormat( PdmUiNumberFormat::NumberFormatType numberFormat, int precision );
};
} // namespace caf
