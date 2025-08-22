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

#pragma once

#include "cafFontTools.h"
#include "cafPdmField.h"
#include "cafPdmFieldScriptingCapability.h"

//==================================================================================================
///
///
//==================================================================================================
class RimFontSizeField : public caf::PdmField<caf::FontTools::RelativeSizeEnum>
{
public:
    void configureCapabilities() override;

    RimFontSizeField& operator=( const caf::FontTools::RelativeSize& value )
    {
        setValue( value );
        return *this;
    }
};

namespace caf
{
template <>
class PdmFieldScriptingCapability<RimFontSizeField> : public PdmAbstractFieldScriptingCapability
{
public:
    PdmFieldScriptingCapability( RimFontSizeField* field, const QString& fieldName, bool giveOwnership )
        : PdmAbstractFieldScriptingCapability( field, fieldName, giveOwnership )
    {
        m_field = field;
    }

    void writeToField( QTextStream&          inputStream,
                       PdmObjectFactory*     objectFactory,
                       PdmScriptIOMessages*  errorMessageContainer,
                       bool                  stringsAreQuoted    = true,
                       caf::PdmObjectHandle* existingObjectsRoot = nullptr ) override
    {
        if ( this->isIOWriteable() )
        {
            caf::FontTools::RelativeSizeEnum value;

            PdmFieldScriptingCapabilityIOHandler<caf::FontTools::RelativeSizeEnum>::writeToField( value,
                                                                                                  inputStream,
                                                                                                  errorMessageContainer,
                                                                                                  stringsAreQuoted );
            m_field->setValue( value );
        }
    }

    void readFromField( QTextStream& outputStream, bool quoteStrings = true, bool quoteNonBuiltins = false ) const override
    {
        PdmFieldScriptingCapabilityIOHandler<caf::FontTools::RelativeSizeEnum>::readFromField( m_field->value(),
                                                                                               outputStream,
                                                                                               quoteStrings,
                                                                                               quoteNonBuiltins );
    }

    QStringList enumScriptTexts() const override
    {
        QStringList enumTexts;

        for ( size_t i = 0; i < caf::FontTools::RelativeSizeEnum::size(); i++ )
        {
            auto enumText = caf::FontTools::RelativeSizeEnum::text( caf::FontTools::RelativeSizeEnum::fromIndex( i ) );
            enumTexts.push_back( enumText );
        }

        return enumTexts;
    }

    QString dataType() const override { return "str"; }

private:
    RimFontSizeField* m_field;
};

}; // namespace caf
