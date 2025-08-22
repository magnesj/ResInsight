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

#include <memory>

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
        , m_baseCapability(
              std::make_unique<PdmFieldScriptingCapability<
                  caf::PdmField<caf::FontTools::RelativeSizeEnum>>>( static_cast<caf::PdmField<caf::FontTools::RelativeSizeEnum>*>( field ),
                                                                     fieldName,
                                                                     false ) )
    {
    }

    void writeToField( QTextStream&          inputStream,
                       PdmObjectFactory*     objectFactory,
                       PdmScriptIOMessages*  errorMessageContainer,
                       bool                  stringsAreQuoted    = true,
                       caf::PdmObjectHandle* existingObjectsRoot = nullptr ) override
    {
        m_baseCapability->writeToField( inputStream, objectFactory, errorMessageContainer, stringsAreQuoted, existingObjectsRoot );
    }

    void readFromField( QTextStream& outputStream, bool quoteStrings = true, bool quoteNonBuiltins = false ) const override
    {
        m_baseCapability->readFromField( outputStream, quoteStrings, quoteNonBuiltins );
    }

    QStringList enumScriptTexts() const override { return m_baseCapability->enumScriptTexts(); }
    QString     dataType() const override { return "str"; }

private:
    std::unique_ptr<PdmFieldScriptingCapability<caf::PdmField<caf::FontTools::RelativeSizeEnum>>> m_baseCapability;
};

}; // namespace caf
