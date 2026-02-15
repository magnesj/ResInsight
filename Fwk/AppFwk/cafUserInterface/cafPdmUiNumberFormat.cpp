#include "cafPdmUiNumberFormat.h"

#include "cafAppEnum.h"

template <>
void caf::AppEnum<caf::PdmUiNumberFormat::NumberFormatType>::setUp()
{
    addItem( caf::PdmUiNumberFormat::NumberFormatType::AUTO, "AUTO", "Automatic" );
    addItem( caf::PdmUiNumberFormat::NumberFormatType::FIXED, "FIXED", "Fixed, decimal" );
    addItem( caf::PdmUiNumberFormat::NumberFormatType::SCIENTIFIC, "SCIENTIFIC", "Scientific notation" );
    setDefault( caf::PdmUiNumberFormat::NumberFormatType::FIXED );
};

namespace caf
{
//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
QString PdmUiNumberFormat::valueToText( double value, PdmUiNumberFormat::NumberFormatType numberFormat, int precision )
{
    QString valueString;

    switch ( numberFormat )
    {
        case PdmUiNumberFormat::NumberFormatType::FIXED:
            valueString = QString::number( value, 'f', precision );
            break;
        case PdmUiNumberFormat::NumberFormatType::SCIENTIFIC:
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
QString PdmUiNumberFormat::sprintfFormat( PdmUiNumberFormat::NumberFormatType numberFormat, int precision )
{
    switch ( numberFormat )
    {
        case PdmUiNumberFormat::NumberFormatType::FIXED:
            return QString( "%.%1f" ).arg( precision );
        case PdmUiNumberFormat::NumberFormatType::SCIENTIFIC:
            return QString( "%.%1e" ).arg( precision );
        default:
            return QString( "%.%1g" ).arg( precision );
    }
}
} // namespace caf
