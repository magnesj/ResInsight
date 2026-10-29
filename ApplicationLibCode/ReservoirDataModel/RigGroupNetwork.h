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

#include <ctime>
#include <map>
#include <optional>
#include <string>
#include <vector>

//==================================================================================================
/// Production and injection network trees read from the SCHEDULE section of a deck. The standard
/// network is defined by GRUPTREE, the extended network by BRANPROP. Wells are attached as leaf
/// nodes from WELSPECS. Both trees can be redefined at any date, so the result is a list of dated
/// trees per tree type, as the gruptree table of res2df.
//==================================================================================================
class RigGroupNetwork
{
public:
    enum class TreeType
    {
        GRUPTREE,
        BRANPROP
    };

    enum class EdgeKeyword
    {
        GRUPTREE,
        BRANPROP,
        WELSPECS
    };

    struct Edge
    {
        std::string child;
        std::string parent; // Empty for the root node
        EdgeKeyword keyword = EdgeKeyword::GRUPTREE;

        // Node properties from GRUPNET or NODEPROP (TERMINAL_PRESSURE) and edge properties from BRANPROP
        // (VFP_TABLE, ALQ)
        std::map<std::string, double> properties;

        bool isWell() const;
        bool isRoot() const;
    };

    struct DatedTree
    {
        std::time_t       date = 0;
        std::vector<Edge> edges; // Parents are listed before their children
    };

    void addTree( TreeType treeType, const DatedTree& tree );

    bool                          hasTreeType( TreeType treeType ) const;
    const std::vector<DatedTree>& trees( TreeType treeType ) const;
    std::vector<std::time_t>      dates( TreeType treeType ) const;

    // The last tree defined at or before the date. The first tree is used for dates before it.
    const DatedTree* treeAtDate( TreeType treeType, std::time_t date ) const;

    bool isEmpty() const;

    static std::string keywordText( EdgeKeyword keyword );

private:
    std::map<TreeType, std::vector<DatedTree>> m_trees;
};
