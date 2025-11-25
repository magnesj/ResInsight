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

#pragma once

#include "cafPdmObjectCapability.h"

#include <QString>
#include <functional>
#include <map>

namespace caf
{

class PdmObjectHandle;

enum class ValidationStatus
{
    Valid,           // All fields and their combinations are valid
    Warning,         // Non-critical issues that should be noted  
    Error,           // Critical validation errors that prevent proper functionality
    Incomplete       // Required fields are missing or incomplete
};

struct ValidationResult
{
    ValidationStatus                status = ValidationStatus::Valid;
    QString                        message;                    // Overall validation message
    std::map<QString, QString>     fieldErrors;               // Map of field keywords to error messages
};

// Type alias for validation callback function
using ValidationCallback = std::function<ValidationResult(const QString& configName)>;

//==================================================================================================
//
// Validation capability that uses callback functions for custom validation logic
//
//==================================================================================================
class PdmValidationCapability : public PdmObjectCapability
{
public:
    PdmValidationCapability( PdmObjectHandle* owner, bool giveOwnership, ValidationCallback callback );
    ~PdmValidationCapability() override = default;

    // Performs validation using the provided callback
    ValidationResult validate( const QString& configName = "" ) const;

    // Helper method to access validation capability from any PdmObject
    static ValidationResult validateObject( const PdmObjectHandle* object, const QString& configName = "" );

private:
    PdmObjectHandle*    m_owner;
    ValidationCallback  m_validationCallback;
};

} // namespace caf

// Macro to add validation capability with a callback function
#define CAF_PDM_InitValidation( validationFunction ) \
    this->addCapability( new caf::PdmValidationCapability( this, true, \
        [this]( const QString& configName ) -> caf::ValidationResult { \
            return this->validationFunction( configName ); \
        } ), true );

// Macro for objects that don't need validation (creates a default "valid" callback)
#define CAF_PDM_InitDefaultValidation() \
    this->addCapability( new caf::PdmValidationCapability( this, true, \
        []( const QString& ) -> caf::ValidationResult { \
            caf::ValidationResult result; \
            result.status = caf::ValidationStatus::Valid; \
            return result; \
        } ), true );