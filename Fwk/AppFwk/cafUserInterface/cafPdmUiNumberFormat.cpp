#include "cafPdmUiNumberFormat.h"

#include "cafAppEnum.h"

template <>
void caf::AppEnum<caf::NumberFormatType>::setUp()
{
    addItem( caf::NumberFormatType::AUTO, "AUTO", "Automatic" );
    addItem( caf::NumberFormatType::FIXED, "FIXED", "Fixed, decimal" );
    addItem( caf::NumberFormatType::SCIENTIFIC, "SCIENTIFIC", "Scientific notation" );
    setDefault( caf::NumberFormatType::FIXED );
};

namespace caf
{
//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString PdmUiNumberFormat::valueToText( double value, NumberFormatType numberFormat, int precision )
{
    QString valueString;

    switch ( numberFormat )
    {
        case NumberFormatType::FIXED:
            valueString = QString::number( value, 'f', precision );
            break;
        case NumberFormatType::SCIENTIFIC:
            valueString = QString::number( value, 'e', precision );
            break;
        default:
            valueString = QString::number( value );
            break;
    }

    return valueString;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString PdmUiNumberFormat::sprintfFormat( NumberFormatType numberFormat, int precision )
{
    switch ( numberFormat )
    {
        case NumberFormatType::FIXED:
            return QString( "%.%1f" ).arg( precision );
        case NumberFormatType::SCIENTIFIC:
            return QString( "%.%1e" ).arg( precision );
        default:
            return QString( "%.%1g" ).arg( precision );
    }
}
} // namespace caf
