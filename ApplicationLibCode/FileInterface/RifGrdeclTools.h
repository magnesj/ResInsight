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

#pragma once

#include <QString>

#include "cafVecIjk.h"

#include "cvfArray.h"
#include "cvfVector3.h"

#include <array>
#include <cmath>
#include <expected>
#include <optional>
#include <vector>

class RigEclipseCaseData;
class RigMainGrid;

//==================================================================================================
//
//
//==================================================================================================
class RifGrdeclTools
{
public:
    RifGrdeclTools( RigEclipseCaseData* eclipseCase );

    std::expected<bool, QString> exportCornerPointGrid( const QString&             gridFileName,
                                                        bool                       exportInLocalCoordinates,
                                                        const cvf::UByteArray*     cellVisibilityOverrideForActnum,
                                                        std::optional<caf::VecIjk> min,
                                                        std::optional<caf::VecIjk> max,
                                                        std::optional<caf::VecIjk> refinement );

private:
    std::vector<cvf::Vec3d> createPillars() const;

    std::array<cvf::Vec3d, 8> getCorners( size_t i, size_t j, size_t k ) const;
    std::vector<double>       computeZcorn();

private:
    RigEclipseCaseData* m_eclipseCase;
    RigMainGrid*        m_mainGrid;

    size_t m_iMin;
    size_t m_iMax;
    size_t m_ni;

    size_t m_jMin;
    size_t m_jMax;
    size_t m_nj;

    size_t m_kMin;
    size_t m_kMax;
    size_t m_nk;
};
