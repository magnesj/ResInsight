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

#include <QJsonObject>
#include <QString>

namespace caf
{
class PdmObjectHandle;
}

//==================================================================================================
///
/// Serialization helpers shared by the UI automation HTTP server. Converts the PDM object
/// tree to/from the JSON shapes described in docs/automation/openapi.yaml.
///
//==================================================================================================
namespace RiaAutomationJson
{
// Opaque, session-stable handle for a PDM object (its in-memory address as decimal text).
QString addressOf( const caf::PdmObjectHandle* object );

// Resolve an object previously reported by addressOf(). Only objects currently reachable from
// the open project are returned, so a stale address resolves to nullptr.
caf::PdmObjectHandle* findObjectByAddress( const QString& address );

// Serialize an object and, when maxDepth != 0, its children. maxDepth < 0 means unlimited.
QJsonObject pdmObjectToJson( caf::PdmObjectHandle* object, int maxDepth );

} // namespace RiaAutomationJson
