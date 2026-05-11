/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2026     Equinor ASA
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

#include "cafPdmField.h"
#include "cafPdmObjectHandle.h"
#include "cafPdmObjectMethod.h"

#include <QString>

namespace caf
{

//==================================================================================================
///
/// Generic "Add Folder" script method registered once against caf::PdmNestedCollectionBase.
/// Every concrete nested collection inherits the method through the CAF method-factory
/// inheritance walk, so derived classes do not need their own AddFolder counterpart.
///
//==================================================================================================
class PdmcAddFolderMethod : public PdmObjectCreationMethod
{
    CAF_PDM_HEADER_INIT;

public:
    PdmcAddFolderMethod( PdmObjectHandle* self );

    std::expected<PdmObjectHandle*, QString> execute() override;
    QString                                  classKeywordReturnedType() const override;

private:
    PdmField<QString> m_folderName;
};

} // namespace caf
