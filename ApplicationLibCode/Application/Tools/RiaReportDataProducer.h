/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2026-     Equinor ASA
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

#include "RiaReportTable.h"

#include <QString>

#include <vector>

//==================================================================================================
/// Capability mixin implemented by plots / plot windows that can provide structured report data
/// for the report window ("Show Plot Data")
//==================================================================================================
class RiaReportDataProducer
{
public:
    virtual ~RiaReportDataProducer() = default;

    virtual QString                       reportTitle() const    = 0;
    virtual std::vector<RiaReportSection> reportSections() const = 0;

    // If true, an open report window showing this producer is refreshed when the producer is updated
    virtual bool supportsLiveUpdate() const { return false; }
};
