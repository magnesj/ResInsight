/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2025     Equinor ASA
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

#include "RifGrdeclTools.h"

#include "RigEclipseCaseData.h"
#include "RigMainGrid.h"

#include <fstream>
#include <iomanip>
#include <iostream>

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
RifGrdeclTools::RifGrdeclTools( RigEclipseCaseData* eclipseCase )
    : m_eclipseCase( eclipseCase )
    , m_iMin( 0 )
    , m_iMax( 0 )
    , m_ni( 0 )
    , m_jMin( 0 )
    , m_jMax( 0 )
    , m_nj( 0 )
    , m_kMin( 0 )
    , m_kMax( 0 )
    , m_nk( 0 )
{
    m_mainGrid = eclipseCase->mainGrid();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::expected<bool, QString> RifGrdeclTools::exportCornerPointGrid( const QString&             gridFileName,
                                                                    bool                       exportInLocalCoordinates,
                                                                    const cvf::UByteArray*     cellVisibilityOverrideForActnum,
                                                                    std::optional<caf::VecIjk> min,
                                                                    std::optional<caf::VecIjk> max,
                                                                    std::optional<caf::VecIjk> refinement )
{
    if ( min.has_value() )
    {
        m_iMin = min->i();
        m_jMin = min->j();
        m_kMin = min->k();
    }

    if ( max.has_value() )
    {
        m_iMax = max->i();
        m_jMax = max->j();
        m_kMax = max->k();
    }
    else
    {
        m_iMax = m_mainGrid->cellCountI();
        m_jMax = m_mainGrid->cellCountJ();
        m_kMax = m_mainGrid->cellCountK();
    }

    m_ni = m_iMax - m_iMin + 1;
    m_nj = m_jMax - m_jMin + 1;
    m_nk = m_kMax - m_kMin + 1;

    if ( m_ni == 0 || m_nj == 0 || m_nk == 0 )
        return std::unexpected( "Detected zero for one of IJK dimensions during export of corner point grid, nothing to do." );

    std::ofstream file( gridFileName.toStdString() );
    if ( !file.is_open() )
    {
        return std::unexpected( "Not able to open file for export : " + gridFileName );
    }

    // Write the header sections
    file << "MAPUNITS\n 'METRES  '\n/\n\n";
    file << "GRIDUNIT\n 'METRES  ' '        '\n/\n\n";

    // Write SPECGRID section
    file << "SPECGRID\n  " << m_ni << "  " << m_nj << "  " << m_nk << "  1  F /\n\n";

    // Write COORD section
    file << "COORD\n";
    std::vector<cvf::Vec3d> pillars = createPillars();
    std::vector<double>     zcorn   = computeZcorn();

    if ( pillars.empty() ) return std::unexpected( "Empty corner point grid pillars detected, aborting export." );
    if ( zcorn.empty() ) return std::unexpected( "Empty corner point grid ZCORN detected, aborting export." );

    int coordValuesPerLine = 4;
    file << std::scientific << std::setprecision( 6 );
    file << std::right;
    size_t lineCount = 1;
    for ( size_t i = 0; i < pillars.size(); i++ )
    {
        file << "   " << pillars[i].x();
        if ( lineCount++ % coordValuesPerLine == 0 ) file << "\n";

        file << "   " << pillars[i].y();
        if ( lineCount++ % coordValuesPerLine == 0 ) file << "\n";

        file << "   " << -pillars[i].z();
        if ( ( lineCount++ % coordValuesPerLine == 0 ) && ( lineCount < zcorn.size() - 1 ) ) file << "\n";
    }
    file << "\n/\n\n";

    // Write ZCORN section
    file << "ZCORN\n";

    int valuesPerLine = 4;
    for ( size_t i = 0; i < zcorn.size(); ++i )
    {
        file << std::scientific << std::setprecision( 8 ) << "   " << -zcorn[i];

        if ( ( i + 1 ) % valuesPerLine == 0 && i < zcorn.size() - 1 )
        {
            file << "\n";
        }
    }
    file << "\n/\n\n";

    return true;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<cvf::Vec3d> RifGrdeclTools::createPillars() const
{
    std::vector<cvf::Vec3d> pillars( ( m_ni + 1 ) * ( m_nj + 1 ) * 2 );

    // For each pillar position
    for ( int j = 0; j <= m_nj; ++j )
    {
        for ( int i = 0; i <= m_ni; ++i )
        {
            int pillarIndex = int( j * ( m_ni + 1 ) + i );

            // Find the top and bottom points for this pillar
            cvf::Vec3d top, bottom;

            // If we're on an edge, we need to find cells adjacent to this pillar
            if ( i < m_ni && j < m_nj )
            {
                // This pillar corresponds to corner 0 of cell (i,j,0)
                auto cornersMin = getCorners( m_iMin + i, m_jMin + j, m_kMin );
                bottom          = cornersMin[0];

                auto cornersMax = getCorners( m_iMin + i, m_jMin + j, m_kMax );
                top             = cornersMax[4];
            }
            else if ( i == m_ni && j < m_nj )
            {
                // This pillar corresponds to corner 1 of cell (i-1,j,0)
                auto cornersMin = getCorners( m_iMin + i - 1, m_jMin + j, m_kMin );
                bottom          = cornersMin[1];

                auto cornersMax = getCorners( m_iMin + i - 1, m_jMin + j, m_kMax );
                top             = cornersMax[5];
            }
            else if ( i < m_ni && j == m_nj )
            {
                // This pillar corresponds to corner 3 of cell (i,j-1,0)
                auto cornersMin = getCorners( m_iMin + i, m_jMin + j - 1, m_kMin );
                bottom          = cornersMin[3];

                auto cornersMax = getCorners( m_iMin + i, m_jMin + j - 1, m_kMax );
                top             = cornersMax[7];
            }
            else if ( i == m_ni && j == m_nj )
            {
                // This pillar corresponds to corner 2 of cell (i-1,j-1,0)
                auto cornersMin = getCorners( m_iMin + i - 1, m_jMin + j - 1, m_kMin );
                bottom          = cornersMin[2];

                auto cornersMax = getCorners( m_iMin + i - 1, m_jMin + j - 1, m_kMax );
                top             = cornersMax[6];
            }

            // Set the bottom point (x, y, z0) and top point (x, y, z1)
            pillars[pillarIndex * 2]     = bottom;
            pillars[pillarIndex * 2 + 1] = top;
        }
    }

    return pillars;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::array<cvf::Vec3d, 8> RifGrdeclTools::getCorners( size_t i, size_t j, size_t k ) const
{
    size_t mainIndex = m_mainGrid->cellIndexFromIJK( i, j, k );

    std::array<cvf::Vec3d, 8> corners;
    m_mainGrid->cellCornerVertices( mainIndex, corners.data() );

    return corners;
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
std::vector<double> RifGrdeclTools::computeZcorn()
{
    std::vector<double> zcorn( 8 * m_ni * m_nj * m_nk );

    int cellIndex = 0;
    for ( int k = 0; k < m_nk; ++k )
    {
        // Compute zcorn for first layer (bottom)
        for ( int j = 0; j < m_nj; ++j )
        {
            for ( int i = 0; i < m_ni; ++i )
            {
                auto corners = getCorners( i + m_iMin, j + m_jMin, k + m_kMin );

                zcorn[cellIndex++] = corners[0].z();
                zcorn[cellIndex++] = corners[1].z();
                zcorn[cellIndex++] = corners[3].z();
                zcorn[cellIndex++] = corners[2].z();
            }
        }

        // Compute zcorn for second layer (top)
        for ( int j = 0; j < m_nj; ++j )
        {
            for ( int i = 0; i < m_ni; ++i )
            {
                auto corners       = getCorners( i + m_iMin, j + m_jMin, k + m_kMin );
                zcorn[cellIndex++] = corners[4].z();
                zcorn[cellIndex++] = corners[5].z();
                zcorn[cellIndex++] = corners[7].z();
                zcorn[cellIndex++] = corners[6].z();
            }
        }
    }

    return zcorn;
}
