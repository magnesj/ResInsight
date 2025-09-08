//##################################################################################################
//
//   Custom Visualization Core library
//   Copyright (C) 2011-2013 Ceetron AS
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
//##################################################################################################

#include "../cvfStructGrid.h"
#include "cvfVector3.h"
#include "gtest/gtest.h"

using namespace cvf;

// Test the static utility methods of StructGridInterface
TEST(CvfStructGridTest, StaticFaceUtilities)
{
    // Test cell face vertex indices
    cvf::ubyte vertices[4];
    
    StructGridInterface::cellFaceVertexIndices(StructGridInterface::POS_I, vertices);
    EXPECT_EQ(1, vertices[0]);
    EXPECT_EQ(2, vertices[1]);
    EXPECT_EQ(6, vertices[2]);
    EXPECT_EQ(5, vertices[3]);
    
    StructGridInterface::cellFaceVertexIndices(StructGridInterface::NEG_I, vertices);
    EXPECT_EQ(0, vertices[0]);
    EXPECT_EQ(4, vertices[1]);
    EXPECT_EQ(7, vertices[2]);
    EXPECT_EQ(3, vertices[3]);
}

TEST(CvfStructGridTest, OppositeFaces)
{
    EXPECT_EQ(StructGridInterface::NEG_I, StructGridInterface::oppositeFace(StructGridInterface::POS_I));
    EXPECT_EQ(StructGridInterface::POS_I, StructGridInterface::oppositeFace(StructGridInterface::NEG_I));
    EXPECT_EQ(StructGridInterface::NEG_J, StructGridInterface::oppositeFace(StructGridInterface::POS_J));
    EXPECT_EQ(StructGridInterface::POS_J, StructGridInterface::oppositeFace(StructGridInterface::NEG_J));
    EXPECT_EQ(StructGridInterface::NEG_K, StructGridInterface::oppositeFace(StructGridInterface::POS_K));
    EXPECT_EQ(StructGridInterface::POS_K, StructGridInterface::oppositeFace(StructGridInterface::NEG_K));
    EXPECT_EQ(StructGridInterface::NO_FACE, StructGridInterface::oppositeFace(StructGridInterface::NO_FACE));
}

TEST(CvfStructGridTest, NeighborIJKCalculation)
{
    size_t ni, nj, nk;
    
    // Test neighbor calculation for each face direction
    StructGridInterface::neighborIJKAtCellFace(5, 10, 15, StructGridInterface::POS_I, &ni, &nj, &nk);
    EXPECT_EQ(6u, ni);
    EXPECT_EQ(10u, nj);
    EXPECT_EQ(15u, nk);
    
    StructGridInterface::neighborIJKAtCellFace(5, 10, 15, StructGridInterface::NEG_I, &ni, &nj, &nk);
    EXPECT_EQ(4u, ni);
    EXPECT_EQ(10u, nj);
    EXPECT_EQ(15u, nk);
    
    StructGridInterface::neighborIJKAtCellFace(5, 10, 15, StructGridInterface::POS_J, &ni, &nj, &nk);
    EXPECT_EQ(5u, ni);
    EXPECT_EQ(11u, nj);
    EXPECT_EQ(15u, nk);
    
    StructGridInterface::neighborIJKAtCellFace(5, 10, 15, StructGridInterface::NEG_J, &ni, &nj, &nk);
    EXPECT_EQ(5u, ni);
    EXPECT_EQ(9u, nj);
    EXPECT_EQ(15u, nk);
    
    StructGridInterface::neighborIJKAtCellFace(5, 10, 15, StructGridInterface::POS_K, &ni, &nj, &nk);
    EXPECT_EQ(5u, ni);
    EXPECT_EQ(10u, nj);
    EXPECT_EQ(16u, nk);
    
    StructGridInterface::neighborIJKAtCellFace(5, 10, 15, StructGridInterface::NEG_K, &ni, &nj, &nk);
    EXPECT_EQ(5u, ni);
    EXPECT_EQ(10u, nj);
    EXPECT_EQ(14u, nk);
}

TEST(CvfStructGridTest, GridAxisFromFace)
{
    EXPECT_EQ(StructGridInterface::GridAxisType::AXIS_I, StructGridInterface::gridAxisFromFace(StructGridInterface::POS_I));
    EXPECT_EQ(StructGridInterface::GridAxisType::AXIS_I, StructGridInterface::gridAxisFromFace(StructGridInterface::NEG_I));
    EXPECT_EQ(StructGridInterface::GridAxisType::AXIS_J, StructGridInterface::gridAxisFromFace(StructGridInterface::POS_J));
    EXPECT_EQ(StructGridInterface::GridAxisType::AXIS_J, StructGridInterface::gridAxisFromFace(StructGridInterface::NEG_J));
    EXPECT_EQ(StructGridInterface::GridAxisType::AXIS_K, StructGridInterface::gridAxisFromFace(StructGridInterface::POS_K));
    EXPECT_EQ(StructGridInterface::GridAxisType::AXIS_K, StructGridInterface::gridAxisFromFace(StructGridInterface::NEG_K));
    EXPECT_EQ(StructGridInterface::GridAxisType::NO_AXIS, StructGridInterface::gridAxisFromFace(StructGridInterface::NO_FACE));
}

TEST(CvfStructGridTest, ValidFaceTypes)
{
    std::vector<StructGridInterface::FaceType> validFaces = StructGridInterface::validFaceTypes();
    
    EXPECT_EQ(6u, validFaces.size());
    EXPECT_TRUE(std::find(validFaces.begin(), validFaces.end(), StructGridInterface::POS_I) != validFaces.end());
    EXPECT_TRUE(std::find(validFaces.begin(), validFaces.end(), StructGridInterface::NEG_I) != validFaces.end());
    EXPECT_TRUE(std::find(validFaces.begin(), validFaces.end(), StructGridInterface::POS_J) != validFaces.end());
    EXPECT_TRUE(std::find(validFaces.begin(), validFaces.end(), StructGridInterface::NEG_J) != validFaces.end());
    EXPECT_TRUE(std::find(validFaces.begin(), validFaces.end(), StructGridInterface::POS_K) != validFaces.end());
    EXPECT_TRUE(std::find(validFaces.begin(), validFaces.end(), StructGridInterface::NEG_K) != validFaces.end());
}

TEST(CvfStructGridTest, EdgeVertexIndices)
{
    // Test edge vertex indices between adjacent faces
    auto edge = StructGridInterface::edgeVertexIndices(StructGridInterface::POS_I, StructGridInterface::POS_J);
    
    // Should return the two vertex indices that form the edge between these faces
    EXPECT_TRUE(edge.first < 8);
    EXPECT_TRUE(edge.second < 8);
    EXPECT_NE(edge.first, edge.second);
}

// Mock class for testing interface methods (cannot instantiate abstract class directly)
class MockStructGrid : public StructGridInterface 
{
public:
    MockStructGrid() : m_cellCountI(10), m_cellCountJ(5), m_cellCountK(3) {}
    
    size_t cellCountI() const override { return m_cellCountI; }
    size_t cellCountJ() const override { return m_cellCountJ; }
    size_t cellCountK() const override { return m_cellCountK; }
    
    bool isCellValid(size_t, size_t, size_t) const override { return true; }
    cvf::Vec3d minCoordinate() const override { return cvf::Vec3d(0, 0, 0); }
    cvf::Vec3d maxCoordinate() const override { return cvf::Vec3d(100, 50, 30); }
    
    bool cellIJKNeighbor(size_t, size_t, size_t, FaceType, size_t*) const override { return false; }
    size_t cellIndexFromIJK(size_t i, size_t j, size_t k) const override { return i + j * m_cellCountI + k * m_cellCountI * m_cellCountJ; }
    bool ijkFromCellIndex(size_t cellIndex, size_t* i, size_t* j, size_t* k) const override 
    {
        *k = cellIndex / (m_cellCountI * m_cellCountJ);
        size_t remainder = cellIndex % (m_cellCountI * m_cellCountJ);
        *j = remainder / m_cellCountI;
        *i = remainder % m_cellCountI;
        return true;
    }
    bool cellIJKFromCoordinate(const cvf::Vec3d&, size_t*, size_t*, size_t*) const override { return false; }
    std::array<cvf::Vec3d, 8> cellCornerVertices(size_t) const override { return std::array<cvf::Vec3d, 8>(); }
    cvf::Vec3d cellCentroid(size_t) const override { return cvf::Vec3d::ZERO; }
    void cellMinMaxCordinates(size_t, cvf::Vec3d*, cvf::Vec3d*) const override {}
    size_t gridPointIndexFromIJK(size_t, size_t, size_t) const override { return 0; }
    cvf::Vec3d gridPointCoordinate(size_t, size_t, size_t) const override { return cvf::Vec3d::ZERO; }

private:
    size_t m_cellCountI, m_cellCountJ, m_cellCountK;
};

TEST(CvfStructGridTest, MockGridBasicFunctionality)
{
    MockStructGrid grid;
    
    EXPECT_EQ(10u, grid.cellCountI());
    EXPECT_EQ(5u, grid.cellCountJ());
    EXPECT_EQ(3u, grid.cellCountK());
    
    EXPECT_TRUE(grid.isCellValid(0, 0, 0));
    
    cvf::Vec3d minCoord = grid.minCoordinate();
    cvf::Vec3d maxCoord = grid.maxCoordinate();
    
    EXPECT_EQ(cvf::Vec3d(0, 0, 0), minCoord);
    EXPECT_EQ(cvf::Vec3d(100, 50, 30), maxCoord);
}

TEST(CvfStructGridTest, CellIndexConversion)
{
    MockStructGrid grid;
    
    // Test round-trip conversion: IJK -> Index -> IJK
    size_t originalI = 3, originalJ = 2, originalK = 1;
    size_t cellIndex = grid.cellIndexFromIJK(originalI, originalJ, originalK);
    
    size_t convertedI, convertedJ, convertedK;
    bool success = grid.ijkFromCellIndex(cellIndex, &convertedI, &convertedJ, &convertedK);
    
    EXPECT_TRUE(success);
    EXPECT_EQ(originalI, convertedI);
    EXPECT_EQ(originalJ, convertedJ);
    EXPECT_EQ(originalK, convertedK);
}