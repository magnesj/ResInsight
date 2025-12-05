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
//  RifVfpProdTable - VFP Production Table Reader
//
//  This class is based on the OPM VFPProdTable implementation found at:
//  ThirdParty/custom-opm-common/opm-common/opm/input/eclipse/Schedule/VFPProdTable.hpp
//
//  RATIONALE:
//  This ResInsight-specific implementation is required because the OPM VFPProdTable class
//  automatically converts all data values to SI units during import. For ResInsight's use cases,
//  we need access to the original keyword values as they appear in the Eclipse deck file
//  without any unit conversions applied.
//
//  KEY DIFFERENCES FROM OPM VERSION:
//  - Removed all unit conversion functions (convertFloToSI, convertTHPToSI, etc.)
//  - Datum depth stored as raw value (not converted to SI)
//  - All axis data (FLO, THP, WFR, GFR, ALQ) preserved in original units
//  - Table data (BHP values) stored without pressure scaling
//  - Added storage for original UNITS and BODY_DEF strings for reference
//
//==================================================================================================

#pragma once

#include <array>
#include <string>
#include <vector>

namespace Opm
{
class DeckKeyword;
}

class RifVfpProdTable
{
public:
    enum class FLO_TYPE
    {
        FLO_OIL = 1,
        FLO_LIQ,
        FLO_GAS
    };

    enum class WFR_TYPE
    {
        WFR_WOR = 11,
        WFR_WCT,
        WFR_WGR
    };

    enum class GFR_TYPE
    {
        GFR_GOR = 21,
        GFR_GLR,
        GFR_OGR
    };

    enum class ALQ_TYPE
    {
        ALQ_GRAT = 31,
        ALQ_IGLR,
        ALQ_TGLR,
        ALQ_PUMP,
        ALQ_COMP,
        ALQ_BEAN,
        ALQ_UNDEF
    };

    RifVfpProdTable();
    RifVfpProdTable( const Opm::DeckKeyword& table, bool gaslift_opt_active );

    int                getTableNum() const { return m_table_num; }
    double             getDatumDepth() const { return m_datum_depth; }
    FLO_TYPE           getFloType() const { return m_flo_type; }
    WFR_TYPE           getWFRType() const { return m_wfr_type; }
    GFR_TYPE           getGFRType() const { return m_gfr_type; }
    ALQ_TYPE           getALQType() const { return m_alq_type; }
    const std::string& getUnits() const { return m_units; }
    const std::string& getBodyDef() const { return m_body_def; }

    const std::vector<double>& getFloAxis() const { return m_flo_data; }
    const std::vector<double>& getTHPAxis() const { return m_thp_data; }
    const std::vector<double>& getWFRAxis() const { return m_wfr_data; }
    const std::vector<double>& getGFRAxis() const { return m_gfr_data; }
    const std::vector<double>& getALQAxis() const { return m_alq_data; }
    const std::vector<double>& getTable() const { return m_data; }

    std::array<size_t, 5> shape() const;
    double                operator()( size_t thp_idx, size_t wfr_idx, size_t gfr_idx, size_t alq_idx, size_t flo_idx ) const;

private:
    int         m_table_num;
    double      m_datum_depth;
    FLO_TYPE    m_flo_type;
    WFR_TYPE    m_wfr_type;
    GFR_TYPE    m_gfr_type;
    ALQ_TYPE    m_alq_type;
    std::string m_units;
    std::string m_body_def;

    std::vector<double> m_flo_data;
    std::vector<double> m_thp_data;
    std::vector<double> m_wfr_data;
    std::vector<double> m_gfr_data;
    std::vector<double> m_alq_data;
    std::vector<double> m_data;

    void    check();
    double& operator()( size_t thp_idx, size_t wfr_idx, size_t gfr_idx, size_t alq_idx, size_t flo_idx );
};