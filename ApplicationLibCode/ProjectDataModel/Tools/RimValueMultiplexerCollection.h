/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2024- Equinor ASA
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

#include "cafPdmChildArrayField.h"
#include "cafPdmObject.h"
#include "cafPdmPtrField.h"

class RimValueMultiplexer;

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
class RimValueMultiplexerCollection : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    RimValueMultiplexerCollection();

    void            setRoot( caf::PdmObject* root );
    caf::PdmObject* root() const;

    void addMultiplexer( caf::PdmObject* source, const QString& fieldName, caf::PdmObject* destination, const QString& destinationFieldName );
    void removeMultiplexer( caf::PdmObject* source, const QString& fieldName, caf::PdmObject* destination, const QString& destinationFieldName );

    void notifyFieldChanged( caf::PdmObject* source, const QString& fieldName, QVariant newValue );

private:
    void appendMenuItems( caf::CmdFeatureMenuBuilder& menuBuilder ) const override;

private:
    caf::PdmPtrField<caf::PdmObject*>             m_root;
    caf::PdmChildArrayField<RimValueMultiplexer*> m_valueMultiplexers;
};
