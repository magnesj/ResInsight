//##################################################################################################
//
//   Custom Visualization Core library
//   Copyright (C) 2011-2013 Ceetron AS
//
//   This library may be used under the terms of either the GNU General Public License or
//   the GNU Lesser General Public License as follows:
//
//   GNU General Public License Usage
//   This library is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//   See the GNU General Public License at <<http://www.gnu.org/licenses/gpl.html>>
//   for more details.
//
//   GNU Lesser General Public License Usage
//   This library is free software; you can redistribute it and/or modify
//   it under the terms of the GNU Lesser General Public License as published by
//   the Free Software Foundation; either version 2.1 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//   See the GNU Lesser General Public License at <<http://www.gnu.org/licenses/lgpl-2.1.html>>
//   for more details.
//
//##################################################################################################

#include "cafPdmValidationCapability.h"

#include "cafPdmObjectHandle.h"

using namespace caf;

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
PdmValidationCapability::PdmValidationCapability( PdmObjectHandle* owner, bool giveOwnership, ValidationCallback callback )
    : PdmObjectCapability()
    , m_owner( owner )
    , m_validationCallback( std::move( callback ) )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
ValidationResult PdmValidationCapability::validate( const QString& configName ) const
{
    if ( m_validationCallback )
    {
        return m_validationCallback( configName );
    }

    // No callback provided - assume valid
    ValidationResult result;
    result.status = ValidationStatus::Valid;
    return result;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
ValidationResult PdmValidationCapability::validateObject( const PdmObjectHandle* object, const QString& configName )
{
    if ( !object )
    {
        ValidationResult result;
        result.status  = ValidationStatus::Error;
        result.message = "Object is null";
        return result;
    }

    auto* validationCap = object->capability<PdmValidationCapability>();
    if ( validationCap )
    {
        return validationCap->validate( configName );
    }

    // No validation capability - assume valid
    ValidationResult result;
    result.status = ValidationStatus::Valid;
    return result;
}