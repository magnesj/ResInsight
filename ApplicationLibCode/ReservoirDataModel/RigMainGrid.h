/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2011-     Statoil ASA
//  Copyright (C) 2013-     Ceetron Solutions AS
//  Copyright (C) 2011-2012 Ceetron AS
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

#include "RigCell.h"
#include "RigGridBase.h"
#include "RigLocalGrid.h"
#include "RigNestedHybridGridLayout.h"

#include "cvfBoundingBox.h"
#include "cvfCollection.h"

#include <QString>

#include <algorithm>
#include <array>
#include <cstdint>
#include <future>
#include <map>
#include <mutex>
#include <optional>
#include <vector>

class RigActiveCellInfo;
class RigNNCData;

namespace cvf
{
class BoundingBoxTree;
}

class RigMainGrid : public RigGridBase
{
public:
    RigMainGrid();
    ~RigMainGrid() override;

    virtual size_t totalCellCount() const;

    // Global node table. Cells refer to nodes by index using RigCell::cornerIndices().
    size_t            nodeCount() const { return m_nodes.size(); }
    const cvf::Vec3d& node( size_t nodeIndex ) const { return m_nodes[nodeIndex]; }

    void setNodes( std::vector<cvf::Vec3d> nodes );
    void appendNodes( const std::vector<cvf::Vec3d>& nodes );
    void setNode( size_t nodeIndex, const cvf::Vec3d& coordinate );
    void transformNodes( const cvf::Mat4d& transform );

    // Shared node table construction, used when reading grids. A node with exactly the same coordinates as an
    // existing node is not added again, the existing node is used instead. Nodes appended this way are shared
    // between cells, so setNode() moves the corner of every cell using the node.
    // Returns the node table index of the node. Not thread safe.
    size_t appendUniqueNode( const cvf::Vec3d& node );
    // Adds the corner nodes of cellCount cells using appendUniqueNode(). readCell( cellIndex, corners ) is called in
    // parallel. It must fill corners in RigCell::cornerIndices() order and return the corner indices to set, or nullptr
    // to skip the cell. Cells are processed in chunks, so the temporary memory does not depend on the grid size, and the
    // nodes are added in cell order independent of the number of threads.
    template <typename ReadCellFunc>
    void appendUniqueCellNodes( int cellCount, ReadCellFunc&& readCell );
    // Frees the lookup table used to find existing nodes. Call when the grid geometry is complete.
    void releaseNodeLookup();

    virtual RigGridBase*       gridAndGridLocalIdxFromGlobalCellIdx( size_t globalCellIdx, size_t* gridLocalCellIdx );
    virtual const RigGridBase* gridAndGridLocalIdxFromGlobalCellIdx( size_t globalCellIdx, size_t* gridLocalCellIdx ) const;

    virtual const RigCell& cellByGridAndGridLocalCellIdx( size_t gridIdx, size_t gridLocalCellIdx ) const;
    virtual size_t         reservoirCellIndexByGridAndGridLocalCellIndex( size_t gridIdx, size_t gridLocalCellIdx ) const;
    virtual size_t         findReservoirCellIndexFromPoint( const cvf::Vec3d& point ) const;

    void addLocalGrid( RigLocalGrid* localGrid );

    size_t             gridCountOnFile() const;
    size_t             gridCount() const;
    RigGridBase*       gridByIndex( size_t localGridIndex );
    const RigGridBase* gridByIndex( size_t localGridIndex ) const;
    RigGridBase*       gridById( int localGridId );
    RigGridBase*       gridByName( const std::string& name );

    size_t totalTemporaryGridCellCount() const;

    // Nested hybrid grid: maps a reconstructed LGR cell (global reservoir index) to the original
    // flat refined cell it was built from. Used to copy result values onto the LGR cells.
    void                            setNestedHybridLgrSourceCells( const std::map<size_t, size_t>& lgrToFlatCell );
    const std::map<size_t, size_t>& nestedHybridLgrSourceCells() const;

    // Nested hybrid grid: maps a flat cell (global reservoir index) to the natural index of its
    // parent COARSE cell. Used for the volume-weighted QC aggregate onto coarse parents.
    void                            setNestedHybridCoarseParents( const std::map<size_t, size_t>& cellToCoarseParent );
    const std::map<size_t, size_t>& nestedHybridCoarseParents() const;

    // Nested hybrid grid: index layout of the original (coarse) grid embedded in the flat main grid.
    // Not set for other grids.
    void                                            setNestedHybridLayout( const RigNestedHybridGridLayout& layout );
    const std::optional<RigNestedHybridGridLayout>& nestedHybridLayout() const;

    // Nested hybrid grid: the original (coarse) IJK of a cell in any grid of this main grid. LGR cells
    // are followed through their parent cells (recursively) up to the main grid. Returns nullopt if
    // this is not a nested hybrid grid, or the cell has no original IJK (refined flat cells, padding).
    std::optional<caf::VecIjk0> nestedHybridOriginalIjk( const RigGridBase* grid, size_t gridLocalCellIndex ) const;

    RigNNCData* nncData();

    void                             setFaults( const cvf::Collection<RigFault>& faults );
    const cvf::Collection<RigFault>& faults() const;
    cvf::Collection<RigFault>&       faults();
    void                             calculateFaults( const RigActiveCellInfo* activeCellInfo );

    void addUnNamedFaultFaces( int                               gcIdx,
                               const RigActiveCellInfo*          activeCellInfo,
                               const std::vector<cvf::Vec3d>&    vxs,
                               int                               unNamedFaultIdx,
                               int                               unNamedFaultWithInactiveIdx,
                               std::vector<RigFault::FaultFace>& unNamedFaultFaces,
                               std::vector<RigFault::FaultFace>& unNamedFaultFacesInactive,
                               RigFaultsPrCellAccumulator*       faultsPrCellAcc ) const;

    void distributeNNCsToFaults();

    const RigFault* findFaultFromCellIndexAndCellFace( size_t reservoirCellIndex, cvf::StructGridInterface::FaceType face ) const;
    bool            isFaceNormalsOutwards() const;
    void            computeFaceNormalsDirection( const std::vector<size_t>& reservoirCellIndices ) const;

    void computeCachedData( std::string* aabbTreeInfo = nullptr );
    void initAllSubGridsParentGridPointer();

    cvf::Vec3d displayModelOffset() const override;
    void       setDisplayModelOffset( cvf::Vec3d offset );

    void                setFlipAxis( bool flipXAxis, bool flipYAxis );
    std::vector<size_t> findIntersectingCells( const cvf::BoundingBox& inputBB ) const;

    cvf::BoundingBox boundingBox() const;

    bool               isTempGrid() const override;
    const std::string& associatedWellPathName() const override;

    void                         setUseMapAxes( bool useMapAxes );
    bool                         useMapAxes() const;
    void                         setMapAxes( const std::array<double, 6>& mapAxes );
    const std::array<double, 6>& mapAxes() const;
    std::array<float, 6>         mapAxesF() const;

    cvf::Mat4d mapAxisTransform() const;

    bool isDualPorosity() const;
    void setDualPorosity( bool enable );

    // invalidate all cells with I > iLimit (0 based index)
    void invalidateCellsAboveI( size_t iLimit );

    std::tuple<QString, double, cvf::StructGridInterface::FaceType> minimumDistanceFaultToPoint( const cvf::Vec3d& point ) const;

protected: // only for use by file readers and internal services. TODO: replace with a better API
    friend class RigGridBase;
    friend class RigReservoirBuilder;
    friend class RifRoffFileTools;
    friend class RifEclipseOutputFileTools;
    friend class RifReaderEclipseOutput;
    friend class RifReaderOpmCommon;
    friend class RiaGrpcCaseService;
    friend class RiaGrpcActiveCellInfoStateHandler;
    friend class RicCreateTemporaryLgrFeature;
    friend class RimCornerPointCase;
    friend class RigNestedHybridGridReconstructor;
    std::vector<RigCell>&       reservoirCells();
    const std::vector<RigCell>& reservoirCells() const;

private:
    struct CellNodeChunk
    {
        std::vector<std::array<cvf::Vec3d, 8>> corners;
        std::vector<std::array<size_t, 8>*>    cornerIndices;
        std::vector<std::array<uint64_t, 8>>   hashes;
    };
    static uint64_t nodeHash( const cvf::Vec3d& node );
    void            appendUniqueCellNodes( const CellNodeChunk& chunk );

    void   syncNodeLookup();
    size_t appendUniqueNode( const cvf::Vec3d& node, uint64_t hash );
    size_t findOrAddToNodeLookup( const cvf::Vec3d& node, uint64_t hash, size_t nodeIndex );
    void   prefetchNodeLookup( uint64_t hash ) const;
    void   growNodeLookup();
    void   clearNodeLookup();

    void initAllSubCellsMainGridCellIndex();
    bool hasFaultWithName( const QString& name ) const;
    void computeBoundingBox();

    static std::array<double, 6> defaultMapAxes();

    void             doBuildCellSearchTree( std::string* aabbTreeInfo = nullptr ) const;
    void             buildCellSearchTree( size_t cellsPerBoundingBox ) const;
    cvf::BoundingBox cellBoundingBox( size_t reservoirCellIndex ) const;

private:
    std::vector<cvf::Vec3d>       m_nodes; ///< Global vertex table
    std::vector<uint64_t>         m_nodeLookup; ///< Open addressing hash table of node indices, used to share nodes
    size_t                        m_nodeLookupSizeLog    = 0; ///< m_nodeLookup has 2^m_nodeLookupSizeLog slots
    size_t                        m_nodeLookupEntryCount = 0;
    size_t                        m_nodeLookupNodeCount  = 0; ///< Nodes [0, m_nodeLookupNodeCount) are in the lookup
    std::vector<RigCell>          m_cells; ///< Global array of all cells in the reservoir (including the ones in LGR's)
    cvf::Collection<RigLocalGrid> m_localGrids; ///< List of all the LGR's in this reservoir
    std::vector<size_t>           m_gridIdToIndexMapping; ///< Mapping from LGR Id to index.

    std::map<size_t, size_t>                 m_nestedHybridLgrSourceCells; ///< LGR cell global index -> source flat cell global index
    std::map<size_t, size_t>                 m_nestedHybridCoarseParents; ///< flat cell global index -> parent coarse cell natural index
    std::optional<RigNestedHybridGridLayout> m_nestedHybridLayout;

    cvf::Collection<RigFault>            m_faults;
    cvf::ref<RigNNCData>                 m_nncData;
    cvf::ref<RigFaultsPrCellAccumulator> m_faultsPrCellAcc;

    cvf::Vec3d       m_displayModelOffset;
    cvf::BoundingBox m_boundingBox;

    bool m_flipXAxis;
    bool m_flipYAxis;

    bool                  m_useMapAxes;
    std::array<double, 6> m_mapAxes;

    bool m_dualPorosity;

    mutable bool                           m_isFaceNormalsOutwards;
    mutable bool                           m_isFaceNormalsOutwardsComputed;
    mutable cvf::ref<cvf::BoundingBoxTree> m_cellSearchTree;
    mutable size_t                         m_cellSearchTreeCellsPerBoundingBox = 1;
    mutable std::mutex                     m_cellSearchTreeMutex;
};

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
template <typename ReadCellFunc>
void RigMainGrid::appendUniqueCellNodes( int cellCount, ReadCellFunc&& readCell )
{
    constexpr int chunkCellCount = 1 << 15;

    // Two chunk buffers, so the nodes of one chunk are added while the next chunk is read
    std::array<CellNodeChunk, 2> chunks;
    std::future<void>            addingNodes;

    for ( int chunkStart = 0, chunkNumber = 0; chunkStart < cellCount; chunkStart += chunkCellCount, chunkNumber++ )
    {
        const int chunkSize = std::min( chunkCellCount, cellCount - chunkStart );

        CellNodeChunk& chunk = chunks[chunkNumber % 2];
        chunk.corners.resize( chunkSize );
        chunk.cornerIndices.resize( chunkSize );
        chunk.hashes.resize( chunkSize );

#pragma omp parallel for
        for ( int i = 0; i < chunkSize; i++ )
        {
            chunk.cornerIndices[i] = readCell( chunkStart + i, chunk.corners[i] );
            if ( chunk.cornerIndices[i] )
            {
                for ( size_t corner = 0; corner < 8; corner++ )
                {
                    chunk.hashes[i][corner] = nodeHash( chunk.corners[i][corner] );
                }
            }
        }

        if ( addingNodes.valid() ) addingNodes.get();

        if ( chunkStart + chunkSize < cellCount )
        {
            addingNodes = std::async( std::launch::async, [this, &chunk]() { appendUniqueCellNodes( chunk ); } );
        }
        else
        {
            appendUniqueCellNodes( chunk );
        }
    }
}

//==================================================================================================
/// Read-only indexed view of the node table of a main grid, usable with cvf::ArrayWrapperConst
//==================================================================================================
class RigMainGridNodes
{
public:
    explicit RigMainGridNodes( const RigMainGrid& mainGrid )
        : m_mainGrid( mainGrid )
    {
    }

    size_t            size() const { return m_mainGrid.nodeCount(); }
    const cvf::Vec3d& operator[]( size_t nodeIndex ) const { return m_mainGrid.node( nodeIndex ); }

private:
    const RigMainGrid& m_mainGrid;
};
