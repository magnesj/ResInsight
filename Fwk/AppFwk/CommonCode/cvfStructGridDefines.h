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

#include "cafAppEnum.h"

namespace cvf
{
namespace StructGridDefines
{
    enum FaceType
    {
        POS_I,
        NEG_I,
        POS_J,
        NEG_J,
        POS_K,
        NEG_K,
        NO_FACE
    };

    typedef caf::AppEnum<StructGridDefines::FaceType> FaceEnum;

    enum class GridAxisType
    {
        AXIS_I,
        AXIS_J,
        AXIS_K,
        NO_AXIS
    };

    void                        cellFaceVertexIndices( StructGridDefines::FaceType face, cvf::ubyte vertexIndices[4] );
    StructGridDefines::FaceType oppositeFace( StructGridDefines::FaceType face );
    void neighborIJKAtCellFace( size_t i, size_t j, size_t k, StructGridDefines::FaceType face, size_t* ni, size_t* nj, size_t* nk );

    StructGridDefines::GridAxisType gridAxisFromFace( StructGridDefines::FaceType face );

    std::pair<ubyte, ubyte>                  edgeVertexIndices( cvf::StructGridDefines::FaceType face1,
                                                                cvf::StructGridDefines::FaceType face2 );
    std::vector<StructGridDefines::FaceType> validFaceTypes();

} //namespace StructGridDefines
} //namespace cvf
