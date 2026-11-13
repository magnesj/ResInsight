/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) Statoil ASA
//  Copyright (C) Ceetron Solutions AS
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

#include "RigHexIntersectionTools.h"

#include "cvfObject.h"
#include "cvfStructGrid.h"
#include "cvfVector3.h"

#include <functional>
#include <optional>
#include <vector>

//==================================================================================================
///
//==================================================================================================
struct WellPathCellIntersectionInfo
{
    size_t     globCellIndex;
    cvf::Vec3d startPoint;
    cvf::Vec3d endPoint;
    double     startMD;
    double     endMD;
    cvf::Vec3d intersectionLengthsInCellCS;

    cvf::StructGridInterface::FaceType intersectedCellFaceIn;
    cvf::StructGridInterface::FaceType intersectedCellFaceOut;

    double startTVD() const { return -startPoint.z(); }
    double endTVD() const { return -endPoint.z(); }
};

//==================================================================================================
/// A crossing of the well path and a cell surface, with the measured depth of the crossing point
//==================================================================================================
struct WellPathCellCrossing
{
    double              measuredDepth;
    HexIntersectionInfo intersection;
};

class RigWellPath;

//==================================================================================================
///
//==================================================================================================
class RigWellLogExtractor : public cvf::Object
{
public:
    RigWellLogExtractor( const RigWellPath* wellpath, const std::string& wellCaseErrorMsgName );
    ~RigWellLogExtractor() override;

    const std::vector<double>&                             cellIntersectionMDs() const;
    const std::vector<double>&                             cellIntersectionTVDs() const;
    const std::vector<size_t>&                             intersectedCellsGlobIdx() const;
    const std::vector<cvf::Vec3d>&                         intersections() const;
    const std::vector<cvf::StructGridInterface::FaceType>& intersectedCellFaces() const;

    const RigWellPath* wellPathGeometry() const;

    std::vector<WellPathCellIntersectionInfo> cellIntersectionInfosAlongWellPath() const;

    void resampleIntersections( double maxDistanceBetweenIntersections );

    std::optional<double> averageMdForCell( size_t cellIndex ) const;

    using CandidateCellsAtPoint = std::function<std::vector<size_t>( const cvf::Vec3d& point )>;
    using IsPointInCell         = std::function<bool( size_t cellIndex, const cvf::Vec3d& point )>;

    static std::vector<WellPathCellIntersectionInfo> assignCellIntervals( const std::vector<cvf::Vec3d>&    wellPathPoints,
                                                                          const std::vector<double>&        measuredDepths,
                                                                          std::vector<WellPathCellCrossing> crossings,
                                                                          double                            mergeTolerance,
                                                                          const CandidateCellsAtPoint&      candidateCellsAtPoint,
                                                                          const IsPointInCell&              isPointInCell );

protected:
    static void appendSegmentCrossings( const std::vector<HexIntersectionInfo>& intersections,
                                        const cvf::Vec3d&                       p1,
                                        double                                  md1,
                                        const cvf::Vec3d&                       p2,
                                        double                                  md2,
                                        std::vector<WellPathCellCrossing>*      crossings );

    void setCellIntervals( const std::vector<WellPathCellIntersectionInfo>& cellIntervals );

    virtual cvf::Vec3d calculateLengthInCell( size_t cellIndex, const cvf::Vec3d& startPoint, const cvf::Vec3d& endPoint ) const = 0;

protected:
    cvf::cref<RigWellPath> m_wellPathGeometry;
    std::string            m_wellCaseErrorMsgName;

private:
    std::vector<cvf::Vec3d>                         m_intersections;
    std::vector<size_t>                             m_intersectedCellsGlobIdx;
    std::vector<cvf::StructGridInterface::FaceType> m_intersectedCellFaces;
    std::vector<double>                             m_intersectionMeasuredDepths;
    std::vector<double>                             m_intersectionTVDs;
};
