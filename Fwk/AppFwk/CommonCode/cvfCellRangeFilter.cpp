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

#include "cvfCellRangeFilter.h"

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
cvf::CellRangeFilter::CellRange::CellRange()
    : m_min( cvf::Vec3st::ZERO )
    , m_max( UNDEFINED_SIZE_T, UNDEFINED_SIZE_T, UNDEFINED_SIZE_T )
    , m_applyToSubGridAreas( true )
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
cvf::CellRangeFilter::CellRange::CellRange( size_t minI,
                                            size_t minJ,
                                            size_t minK,
                                            size_t maxI,
                                            size_t maxJ,
                                            size_t maxK,
                                            bool   applyToSubGridAreas )
    : m_min( minI, minJ, minK )
    , m_max( maxI, maxJ, maxK )
    , m_applyToSubGridAreas( applyToSubGridAreas )

{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool cvf::CellRangeFilter::CellRange::isInRange( size_t i, size_t j, size_t k, bool isInSubGridArea ) const
{
    if ( isInSubGridArea && !m_applyToSubGridAreas ) return false;
    cvf::Vec3st test( i, j, k );

    int idx;
    for ( idx = 0; idx < 3; idx++ )
    {
        if ( test[idx] < m_min[idx] || m_max[idx] <= test[idx] )
        {
            return false;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
cvf::CellRangeFilter::CellRangeFilter()
{
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void cvf::CellRangeFilter::addCellIncludeRange( size_t minI,
                                                size_t minJ,
                                                size_t minK,
                                                size_t maxI,
                                                size_t maxJ,
                                                size_t maxK,
                                                bool   applyToSubGridAreas )
{
    m_includeRanges.push_back( CellRange( minI, minJ, minK, maxI, maxJ, maxK, applyToSubGridAreas ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void cvf::CellRangeFilter::addCellInclude( size_t i, size_t j, size_t k, bool applyToSubGridAreas )
{
    m_includeRanges.push_back( CellRange( i, j, k, i + 1, j + 1, k + 1, applyToSubGridAreas ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void cvf::CellRangeFilter::addCellExcludeRange( size_t minI,
                                                size_t minJ,
                                                size_t minK,
                                                size_t maxI,
                                                size_t maxJ,
                                                size_t maxK,
                                                bool   applyToSubGridAreas )
{
    m_excludeRanges.push_back( CellRange( minI, minJ, minK, maxI, maxJ, maxK, applyToSubGridAreas ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void cvf::CellRangeFilter::addCellExclude( size_t i, size_t j, size_t k, bool applyToSubGridAreas )
{
    m_excludeRanges.push_back( CellRange( i, j, k, i + 1, j + 1, k + 1, applyToSubGridAreas ) );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool cvf::CellRangeFilter::isCellVisible( size_t i, size_t j, size_t k, bool isInSubGridArea ) const
{
    if ( m_includeRanges.empty() )
    {
        return false;
    }

    size_t idx;
    for ( idx = 0; idx < m_excludeRanges.size(); idx++ )
    {
        if ( m_excludeRanges[idx].isInRange( i, j, k, isInSubGridArea ) )
        {
            return false;
        }
    }

    for ( idx = 0; idx < m_includeRanges.size(); idx++ )
    {
        if ( m_includeRanges[idx].isInRange( i, j, k, isInSubGridArea ) )
        {
            return true;
        }
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool cvf::CellRangeFilter::isCellExcluded( size_t i, size_t j, size_t k, bool isInSubGridArea ) const
{
    for ( size_t idx = 0; idx < m_excludeRanges.size(); idx++ )
    {
        if ( m_excludeRanges[idx].isInRange( i, j, k, isInSubGridArea ) )
        {
            return true;
        }
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
bool cvf::CellRangeFilter::hasIncludeRanges() const
{
    if ( !m_includeRanges.empty() )
        return true;
    else
        return false;
}
