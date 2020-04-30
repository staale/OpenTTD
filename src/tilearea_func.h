/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tilearea_type.h Functions related to tile areas. */

#ifndef TILEAREA_FUNC_H
#define TILEAREA_FUNC_H

#include "direction_type.h"
#include "map_type.h"
#include "tilearea_type.h"

TileTransformation TransformationBetweenTileAreas(const GenericTileArea &from, const GenericTileArea &to, DirTransformation dtr);
TileTransformation TransformationBetweenTileAreaCorners(const GenericTileArea &from, const GenericTileArea &to, DirTransformation dtr);
GenericTileArea TransformTileArea(const GenericTileArea &ta, TileTransformation transformation, Map *dst_map);
GenericTileArea TransformTileArea(const GenericTileArea &ta, GenericTileIndex dst_tile, DirTransformation transformation);

#endif /* TILEAREA_FUNC_H */
