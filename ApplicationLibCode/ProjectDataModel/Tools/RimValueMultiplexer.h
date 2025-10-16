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

#include "cafPdmObject.h"
#include "cafPdmPtrField.h"
#include "cafPdmUiCommandSystemProxy.h"

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
class RimValueMultiplexer : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;

public:
    RimValueMultiplexer();

    caf::PdmObject*     source() const;
    QString             sourceFieldName() const;
    caf::PdmValueField* sourceField() const;

    caf::PdmObject*     destination() const;
    QString             destinationFieldName() const;
    caf::PdmValueField* destinationField() const;

    void setSource( caf::PdmObject* source, const QString& fieldName );
    void setDestination( caf::PdmObject* destination, const QString& fieldName );

private:
    QList<caf::PdmOptionItemInfo> calculateValueOptions( const caf::PdmFieldHandle* fieldNeedingOptions ) override;

private:
    caf::PdmPtrField<caf::PdmObject*> m_source;
    caf::PdmField<QString>            m_sourceFieldName;

    caf::PdmPtrField<caf::PdmObject*> m_destination;
    caf::PdmField<QString>            m_destinationFieldName;
};
