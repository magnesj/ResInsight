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

//==================================================================================================
//
//  RifVfpInjTable - VFP Injection Table Reader Implementation
//
//  Based on: ThirdParty/custom-opm-common/opm-common/opm/input/eclipse/Schedule/VFPInjTable.cpp
//
//  This implementation preserves the original Eclipse deck keyword values without unit conversions,
//  unlike the OPM version which automatically converts all values to SI units.
//
//==================================================================================================

#include "RifVfpInjTable.h"

#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/V.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <stdexcept>

namespace
{
template <typename T>
inline const Opm::DeckItem& getNonEmptyItem( const Opm::DeckRecord& record )
{
    const auto& retval = record.getItem<T>();
    if ( !retval.hasValue( 0 ) )
    {
        throw std::invalid_argument( "Missing data" );
    }
    return retval;
}

RifVfpInjTable::FLO_TYPE getFloType( const std::string& flo_string )
{
    if ( flo_string == "OIL" ) return RifVfpInjTable::FLO_TYPE::FLO_OIL;
    if ( flo_string == "WAT" ) return RifVfpInjTable::FLO_TYPE::FLO_WAT;
    if ( flo_string == "GAS" ) return RifVfpInjTable::FLO_TYPE::FLO_GAS;

    throw std::invalid_argument( "Invalid RATE_TYPE string" );
}

void check_axis( const std::vector<double>& axis )
{
    if ( axis.size() == 0 ) throw std::invalid_argument( "Empty axis" );

    if ( !std::is_sorted( axis.begin(), axis.end() ) ) throw std::invalid_argument( "Axis is not sorted" );
}

} // namespace

RifVfpInjTable::RifVfpInjTable()
{
    m_table_num   = -1;
    m_datum_depth = 0.0;
    m_flo_type    = FLO_TYPE::FLO_OIL;
}

RifVfpInjTable::RifVfpInjTable( const Opm::DeckKeyword& table )
{
    using Opm::ParserKeywords::VFPINJ;

    if ( table.size() < 4 )
    {
        throw std::invalid_argument( "VFPINJ table does not appear to have enough records to be valid" );
    }

    const auto& header = table.getRecord( 0 );

    m_table_num   = getNonEmptyItem<VFPINJ::TABLE>( header ).get<int>( 0 );
    m_datum_depth = getNonEmptyItem<VFPINJ::DATUM_DEPTH>( header ).get<double>( 0 );

    m_flo_type = ::getFloType( getNonEmptyItem<VFPINJ::RATE_TYPE>( header ).get<std::string>( 0 ) );

    std::string quantity_string = getNonEmptyItem<VFPINJ::PRESSURE_DEF>( header ).get<std::string>( 0 );
    if ( quantity_string != "THP" )
    {
        throw std::invalid_argument( "PRESSURE_DEF is required to be THP" );
    }

    if ( header.getItem<VFPINJ::UNITS>().hasValue( 0 ) )
    {
        m_units = header.getItem<VFPINJ::UNITS>().get<std::string>( 0 );
    }

    m_body_def = getNonEmptyItem<VFPINJ::BODY_DEF>( header ).get<std::string>( 0 );
    if ( m_body_def != "BHP" )
    {
        throw std::invalid_argument( "Invalid BODY_DEF string" );
    }

    m_flo_data = getNonEmptyItem<VFPINJ::FLOW_VALUES>( table.getRecord( 1 ) ).getData<double>();
    m_thp_data = getNonEmptyItem<VFPINJ::THP_VALUES>( table.getRecord( 2 ) ).getData<double>();

    size_t nt = m_thp_data.size();
    size_t nf = m_flo_data.size();
    m_data.resize( nt * nf );
    std::fill_n( m_data.data(), m_data.size(), std::nan( "0" ) );

    if ( table.size() != nt + 3 )
    {
        throw std::invalid_argument( "VFPINJ table does not contain enough records." );
    }

    for ( size_t i = 3; i < table.size(); ++i )
    {
        const auto& record = table.getRecord( i );
        int         t      = getNonEmptyItem<VFPINJ::THP_INDEX>( record ).get<int>( 0 ) - 1;

        const std::vector<double>& bhp_values = getNonEmptyItem<VFPINJ::VALUES>( record ).getData<double>();

        if ( bhp_values.size() != nf )
        {
            throw std::invalid_argument( "VFPINJ table does not contain enough FLO values." );
        }

        for ( size_t f = 0; f < bhp_values.size(); ++f )
        {
            ( *this )( t, f ) = bhp_values[f];
        }
    }

    check();
}

std::array<size_t, 2> RifVfpInjTable::shape() const
{
    return { m_thp_data.size(), m_flo_data.size() };
}

double RifVfpInjTable::operator()( size_t thp_idx, size_t flo_idx ) const
{
    size_t nt = m_thp_data.size();
    size_t nf = m_flo_data.size();

    assert( thp_idx < nt );
    assert( flo_idx < nf );

    size_t index = thp_idx + nt * flo_idx;
    return m_data[index];
}

double& RifVfpInjTable::operator()( size_t thp_idx, size_t flo_idx )
{
    size_t nt = m_thp_data.size();
    size_t nf = m_flo_data.size();

    assert( thp_idx < nt );
    assert( flo_idx < nf );

    size_t index = thp_idx + nt * flo_idx;
    return m_data[index];
}

void RifVfpInjTable::check()
{
    if ( m_table_num <= 0 ) throw std::invalid_argument( "Invalid table number" );

    check_axis( m_flo_data );
    check_axis( m_thp_data );

    if ( m_data.size() != m_thp_data.size() * m_flo_data.size() ) throw std::invalid_argument( "Wrong data size" );

    for ( size_t t = 0; t < m_thp_data.size(); ++t )
    {
        for ( size_t f = 0; f < m_flo_data.size(); ++f )
        {
            if ( std::isnan( ( *this )( t, f ) ) )
            {
                throw std::invalid_argument( "VFPINJ table element not initialized" );
            }
        }
    }
}