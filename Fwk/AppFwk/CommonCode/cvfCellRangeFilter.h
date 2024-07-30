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

#include "cvfVector3.h"
#include <vector>

namespace cvf
{
//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
class CellRangeFilter
{
public:
    CellRangeFilter();

    void addCellIncludeRange( size_t minI, size_t minJ, size_t minK, size_t maxI, size_t maxJ, size_t maxK, bool applyToSubGridAreas );
    void addCellInclude( size_t i, size_t j, size_t k, bool applyToSubGridAreas );

    void addCellExcludeRange( size_t minI, size_t minJ, size_t minK, size_t maxI, size_t maxJ, size_t maxK, bool applyToSubGridAreas );
    void addCellExclude( size_t i, size_t j, size_t k, bool applyToSubGridAreas );

    bool isCellVisible( size_t i, size_t j, size_t k, bool isInSubGridArea ) const;
    bool isCellExcluded( size_t i, size_t j, size_t k, bool isInSubGridArea ) const;

    bool hasIncludeRanges() const;

private:
    class CellRange
    {
    public:
        CellRange();
        CellRange( size_t minI, size_t minJ, size_t minK, size_t maxI, size_t maxJ, size_t maxK, bool applyToSubGridAreas );
        bool isInRange( size_t i, size_t j, size_t k, bool isInSubGridArea ) const;

    public:
        cvf::Vec3st m_min;
        cvf::Vec3st m_max;
        bool        m_applyToSubGridAreas;
    };

private:
    std::vector<CellRange> m_includeRanges;
    std::vector<CellRange> m_excludeRanges;
};

} // namespace cvf
