/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file bridge_map.cpp Map accessor functions for bridges. */

#include "stdafx.h"
#include "landscape.h"
#include "tunnelbridge_map.h"

#include "safeguards.h"


/**
 * Finds the end of a bridge in the specified direction starting at a middle tile
 * @param tile the bridge tile to find the bridge ramp for
 * @param dir  the direction to search in
 */
template <typename Tindex>
static Tindex GetBridgeEnd(Tindex tile, DiagDirection dir)
{
	TileIndexDiff delta = TileOffsByDiagDir(dir, MapOf(tile));

	dir = ReverseDiagDir(dir);
	do {
		tile += delta;
	} while (!IsBridgeTile(tile) || GetTunnelBridgeDirection(tile) != dir);

	return tile;
}


/**
 * Finds the northern end of a bridge starting at a middle tile
 * @param t the bridge tile to find the bridge ramp for
 */
TileIndex GetNorthernBridgeEnd(TileIndex t)
{
	return GetBridgeEnd(t, ReverseDiagDir(AxisToDiagDir(GetBridgeAxis(t))));
}


/**
 * Finds the southern end of a bridge starting at a middle tile
 * @param t the bridge tile to find the bridge ramp for
 */
TileIndex GetSouthernBridgeEnd(TileIndex t)
{
	return GetBridgeEnd(t, AxisToDiagDir(GetBridgeAxis(t)));
}


/**
 * Starting at one bridge end finds the other bridge end
 * @param tile the bridge ramp tile to find the other bridge ramp for
 */
template <typename Tindex>
static inline Tindex GetOtherBridgeEnd(const Tindex &tile)
{
	assert(IsBridgeTile(tile));
	return GetBridgeEnd(tile, GetTunnelBridgeDirection(tile));
}

/** @copydoc GetOtherBridgeEnd(const Tindex&) */
TileIndex GetOtherBridgeEnd(TileIndex tile)
{
	return GetOtherBridgeEnd<TileIndex>(tile);
}

/** @copydoc GetOtherBridgeEnd(const Tindex&) */
GenericTileIndex GetOtherBridgeEnd(GenericTileIndex tile)
{
	return GetOtherBridgeEnd<GenericTileIndex>(tile);
}

/**
 * Get the height ('z') of a bridge.
 * @param t the bridge ramp tile to get the bridge height from
 * @return the height of the bridge.
 */
template <typename Tindex>
static inline int GetBridgeHeight(const Tindex &t)
{
	int h;
	Slope tileh = GetTileSlope(t, &h);
	Foundation f = GetBridgeFoundation(tileh, DiagDirToAxis(GetTunnelBridgeDirection(t)));

	/* one height level extra for the ramp */
	return h + 1 + ApplyFoundationToSlope(f, &tileh);
}

/** @copydoc GetBridgeHeight(const Tindex&) */
int GetBridgeHeight(TileIndex t)
{
	return GetBridgeHeight<TileIndex>(t);
}

/** @copydoc GetBridgeHeight(const Tindex&) */
int GetBridgeHeight(GenericTileIndex t)
{
	return GetBridgeHeight<GenericTileIndex>(t);
}
