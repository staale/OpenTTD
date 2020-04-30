/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map_func.h Functions related to maps. */

#ifndef MAP_FUNC_H
#define MAP_FUNC_H

#include "core/bitmath_func.hpp"
#include "core/math_func.hpp"
#include "core/geometry_type.hpp"
#include "tile_type.h"
#include "map_type.h"
#include "direction_func.h"

extern MainMap _main_map;

/**
 * 'Wraps' the given tile to it is within the map. It does
 * this by masking the 'high' bits of.
 * @param x the tile to 'wrap'
 */
#define TILE_MASK(x) ((x) & _main_map.tile_mask)

void AllocateMap(uint size_x, uint size_y);

/**
 * Get the tile map that is bounded to a given tile index.
 * @param tile tile index of a tile
 * @return always the main map
 */
static inline Map *MapOf(TileIndex tile)
{
	return &_main_map;
}

/**
 * Get the tile map that is bounded to a given tile index.
 * @param tile tile index of a tile
 * @return the map that contains the tile
 */
static inline Map *MapOf(GenericTileIndex tile)
{
	return tile.map;
}

/**
 * Access the "raw" value (offset into map array) of a given tile index.
 * @param tile tile index to query
 * @return reference to the "raw" value of the index
 */
inline RawTileIndex &IndexOf(TileIndex &tile)
{
	return tile;
}

/** @copydoc IndexOf(TileIndex&) */
inline RawTileIndex IndexOf(const TileIndex &tile)
{
	return tile;
}

/** @copydoc IndexOf(TileIndex&) */
inline RawTileIndex &IndexOf(GenericTileIndex &tile)
{
	return tile.index;
}

/** @copydoc IndexOf(TileIndex&) */
inline RawTileIndex IndexOf(const GenericTileIndex &tile)
{
	return tile.index;
}

/**
 * Get the data of a tile.
 * @param tile index of the tile
 * @return the tile data
 */
template <typename Tindex>
static inline Tile *GetTile(const Tindex &tile)
{
	return &(MapOf(tile)->m[IndexOf(tile)]);
}

/**
 * Get the extended data of a tile.
 * @param tile index of the tile
 * @return the extended tile data
 */
template <typename Tindex>
static inline TileExtended *GetTileEx(const Tindex &tile)
{
	return &(MapOf(tile)->me[IndexOf(tile)]);
}

/**
 * Logarithm of the map size along the X side.
 * @note try to avoid using this one
 * @return 2^"return value" == MapSizeX()
 */
static inline uint MapLogX()
{
	return _main_map.log_x;
}

/**
 * Logarithm of the map size along the y side.
 * @note try to avoid using this one
 * @return 2^"return value" == MapSizeY()
 */
static inline uint MapLogY()
{
	return _main_map.log_y;
}

/**
 * Get the size of a map along the X
 * @param map the map
 * @return the number of tiles along the X of the map
 */
static inline uint MapSizeX(Map *map = &_main_map)
{
	return map->size_x;
}

/**
 * Get the size of a map along the Y
 * @param map the map
 * @return the number of tiles along the Y of the map
 */
static inline uint MapSizeY(Map *map = &_main_map)
{
	return map->size_y;
}

/**
 * Get the size of a map
 * @param map the map
 * @return the number of tiles of the map
 */
static inline uint MapSize(Map *map = &_main_map)
{
	return map->size;
}

/**
 * Gets the maximum X coordinate within a map, including MP_VOID
 * @param map the map
 * @return the maximum X coordinate
 */
static inline uint MapMaxX(Map *map = &_main_map)
{
	return MapSizeX(map) - 1;
}

/**
 * Gets the maximum Y coordinate within a map, including MP_VOID
 * @param map the map
 * @return the maximum Y coordinate
 */
static inline uint MapMaxY(Map *map = &_main_map)
{
	return MapSizeY(map) - 1;
}

/**
 * Scales the given value by the map size, where the given value is
 * for a 256 by 256 map.
 * @param n the value to scale
 * @return the scaled size
 */
static inline uint ScaleByMapSize(uint n)
{
	/* Subtract 12 from shift in order to prevent integer overflow
	 * for large values of n. It's safe since the min mapsize is 64x64. */
	return CeilDiv(n << (MapLogX() + MapLogY() - 12), 1 << 4);
}


/**
 * Scales the given value by the maps circumference, where the given
 * value is for a 256 by 256 map
 * @param n the value to scale
 * @return the scaled size
 */
static inline uint ScaleByMapSize1D(uint n)
{
	/* Normal circumference for the X+Y is 256+256 = 1<<9
	 * Note, not actually taking the full circumference into account,
	 * just half of it. */
	return CeilDiv((n << MapLogX()) + (n << MapLogY()), 1 << 9);
}

/**
 * An offset value between to tiles.
 *
 * This value is used for the difference between
 * two tiles. It can be added to a tileindex to get
 * the resulting tileindex of the start tile applied
 * with this saved difference.
 *
 * @see TileDiffXY
 */
typedef int32 TileIndexDiff;

/**
 * Test if a given tile index is a main map tile index.
 * @param tile the tile index to test
 * @return \c true if the index points to the main map, \c false otherwise
 */
template <typename Tindex>
inline bool IsMainMapTile(const Tindex &tile)
{
	return MapOf(tile) == &_main_map;
}

/**
 * Convert a given tile index to a main map tile index.
 *
 * @param tile the index to convert
 * @return the converted index
 *
 * @pre IsMainMapTile(tile)
 */
template <typename Tindex>
static inline TileIndex AsMainMapTile(const Tindex &tile)
{
	assert(IsMainMapTile(tile));
	return (TileIndex)IndexOf(tile);
}

/**
 * Test whether two tile indices point to the same tile map.
 * @param a the first tile
 * @param b the second tile
 * @return MapOf(a) == MapOf(b)
 */
template <typename TindexA, typename TindexB>
static inline bool IsSameMap(const TindexA &a, const TindexB &b)
{
	return MapOf(a) == MapOf(b);
}

/**
 * Test if a given tile index points to an existing tile.
 *
 * @param tile the tile index
 * @return whether we can access content of this tile
 *
 * @note the function returns \c true also for #MP_VOID tiles
 * @see IsValidTile
 * @see IsInnerTile
 */
template <typename Tindex>
static inline bool IsValidTileIndex(const Tindex &tile)
{
	return MapOf(tile) != NULL && IndexOf(tile) < MapSize(MapOf(tile));
}

/**
 * Create a tile index.
 * @param index the index of the tile
 * @param map the map of the tile
 */
template <bool Tgeneric>
static typename TileIndexT<Tgeneric>::T MakeTileIndex(RawTileIndex index, Map *map);

template <>
inline TileIndex MakeTileIndex<false>(RawTileIndex index, Map *map)
{
	assert(map == &_main_map);
	return (TileIndex)index;
}

template <>
inline GenericTileIndex MakeTileIndex<true>(RawTileIndex index, Map *map)
{
	return GenericTileIndex(index, map);
}

/**
 * Returns the tile index of a coordinate.
 *
 * @param x The x coordinate of the tile
 * @param y The y coordinate of the tile
 * @param map The map of the tile
 * @return The tile index calculated by the coordinate
 */
template <bool Tgeneric>
static typename TileIndexT<Tgeneric>::T TileXY(uint x, uint y, Map *map);

template <>
inline TileIndex TileXY<false>(uint x, uint y, Map *map)
{
	assert(map == &_main_map);
	return (y << MapLogX()) + x;
}

template <>
inline GenericTileIndex TileXY<true>(uint x, uint y, Map *map)
{
	return GenericTileIndex(y * MapSizeX(map) + x, map);
}

/**
 * Returns the TileIndex of a coordinate.
 *
 * @param x The x coordinate of the tile
 * @param y The y coordinate of the tile
 * @return The TileIndex calculated by the coordinate
 */
static inline TileIndex TileXY(uint x, uint y)
{
	return TileXY<false>(x, y, &_main_map);
}

/**
 * Returns the tile index of a coordinate.
 *
 * @param x The x coordinate of the tile
 * @param y The y coordinate of the tile
 * @param map The map of the tile
 * @return The tile index calculated by the coordinate
 */
static inline GenericTileIndex TileXY(uint x, uint y, Map *map)
{
	return TileXY<true>(x, y, map);
}

/**
 * Calculates an offset for the given coordinate(-offset).
 *
 * This function calculate an offset value which can be added to an
 * #TileIndex. The coordinates can be negative.
 *
 * @param x The offset in x direction
 * @param y The offset in y direction
 * @return The resulting offset value of the given coordinate
 * @see ToTileIndexDiff(TileIndexDiffC)
 */
static inline TileIndexDiff TileDiffXY(int x, int y, Map *map = &_main_map)
{
	/* Multiplication gives much better optimization on MSVC than shifting.
	 * 0 << shift isn't optimized to 0 properly.
	 * Typically x and y are constants, and then this doesn't result
	 * in any actual multiplication in the assembly code.. */
	return (y * MapSizeX(map)) + x;
}

/**
 * Get a tile from the virtual XY-coordinate.
 * @param x The virtual x coordinate of the tile.
 * @param y The virtual y coordinate of the tile.
 * @return The TileIndex calculated by the coordinate.
 */
static inline TileIndex TileVirtXY(uint x, uint y)
{
	return (y >> 4 << MapLogX()) + (x >> 4);
}


/**
 * Get the X component of a tile
 * @param tile the tile to get the X component of
 * @return the X component
 */
template <typename Tindex>
static inline uint TileX(const Tindex &tile)
{
	return IndexOf(tile) % MapSizeX(MapOf(tile));
}

/**
 * Get the Y component of a tile
 * @param tile the tile to get the Y component of
 * @return the Y component
 */
template <typename Tindex>
static inline uint TileY(const Tindex &tile)
{
	return IndexOf(tile) / MapSizeX(MapOf(tile));
}

/**
 * Return the offset between two tiles from a TileIndexDiffC struct.
 *
 * This function works like #TileDiffXY and returns the
 * difference between two tiles.
 *
 * @param tidc The coordinate of the offset as TileIndexDiffC
 * @param map The map array of the tile
 * @return The difference between two tiles.
 *
 * @see TileDiffXY
 */
template <bool Tgeneric>
static TileIndexDiff ToTileIndexDiff(TileIndexDiffC tidc, Map *map);

template <>
inline TileIndexDiff ToTileIndexDiff<false>(TileIndexDiffC tidc, Map *map)
{
	assert(map == &_main_map);
	return (tidc.y << MapLogX()) + tidc.x;
}

template <>
inline TileIndexDiff ToTileIndexDiff<true>(TileIndexDiffC tidc, Map *map)
{
	return (tidc.y * MapSizeX(map)) + tidc.x;
}

/**
 * Return the offset between to tiles from a TileIndexDiffC struct.
 *
 * This function works like #TileDiffXY and returns the
 * difference between two tiles.
 *
 * @param tidc The coordinate of the offset as TileIndexDiffC
 * @return The difference between two tiles.
 * @see TileDiffXY
 */
static inline TileIndexDiff ToTileIndexDiff(TileIndexDiffC tidc)
{
	return ToTileIndexDiff<false>(tidc, &_main_map);
}

/**
 * Return the offset between two tiles from a TileIndexDiffC struct.
 *
 * This function works like #TileDiffXY and returns the
 * difference between two tiles.
 *
 * @param tidc The coordinate of the offset as TileIndexDiffC
 * @param map The map array of the tile
 * @return The difference between two tiles.
 *
 * @see TileDiffXY
 */
static inline TileIndexDiff ToTileIndexDiff(TileIndexDiffC tidc, Map *map)
{
	return ToTileIndexDiff<true>(tidc, map);
}


#ifndef _DEBUG
	/**
	 * Adds a given offset to a tile.
	 *
	 * @param tile The tile to add to
	 * @param delta The offset to add
	 * @return The resulting tile
	 */
	#define TILE_ADD(tile, delta) ((tile) + (delta))

	/**
	 * Adds a given XY offset to a tile.
	 *
	 * @param tile The tile to add an offset on it
	 * @param x The x offset to add to the tile
	 * @param y The y offset to add to the tile
	 * @return The resulting tile
	 */
	#define TILE_ADDXY(tile, x, y) ((tile) + TileDiffXY(x, y, MapOf(tile)))
#else
	extern TileIndex TileAdd(TileIndex tile, TileIndexDiff add,
		const char *exp, const char *file, int line);

	/**
	 * Adds a given offset to a tile.
	 *
	 * @param tile The tile to add to
	 * @param delta The offset to add
	 * @return The resulting tile
	 */
	#define TILE_ADD(tile, delta) (TileAdd((tile), (delta), #tile " + " #delta, __FILE__, __LINE__))

	GenericTileIndex TileAddXY(GenericTileIndex tile, int x, int y, const char *exp, const char *file, int line);

	static inline TileIndex TileAddXY(TileIndex tile, int x, int y, const char *exp, const char *file, int line)
	{
		return AsMainMapTile(TileAddXY(GenericTileIndex(tile), x, y, exp, file, line));
	}

	#define TILE_ADDXY(tile, ...) TileAddXY((tile), __VA_ARGS__, #tile " + <" #__VA_ARGS__ ">", __FILE__, __LINE__)
#endif

TileIndex TileAddWrap(TileIndex tile, int addx, int addy);

/**
 * Returns the TileIndexDiffC offset from a DiagDirection.
 *
 * @param dir The given direction
 * @return The offset as TileIndexDiffC value
 */
static inline TileIndexDiffC TileIndexDiffCByDiagDir(DiagDirection dir)
{
	extern const TileIndexDiffC _tileoffs_by_diagdir[DIAGDIR_END];

	assert(IsValidDiagDirection(dir));
	return _tileoffs_by_diagdir[dir];
}

/**
 * Returns the TileIndexDiffC offset from a Direction.
 *
 * @param dir The given direction
 * @return The offset as TileIndexDiffC value
 */
static inline TileIndexDiffC TileIndexDiffCByDir(Direction dir)
{
	extern const TileIndexDiffC _tileoffs_by_dir[DIR_END];

	assert(IsValidDirection(dir));
	return _tileoffs_by_dir[dir];
}

/**
 * Add a TileIndexDiffC to a TileIndex and returns the new one.
 *
 * Returns tile + the diff given in diff. If the result tile would end up
 * outside of the map, INVALID_TILE is returned instead.
 *
 * @param tile The base tile to add the offset on
 * @param diff The offset to add on the tile
 * @return The resulting TileIndex
 */
static inline TileIndex AddTileIndexDiffCWrap(TileIndex tile, TileIndexDiffC diff)
{
	int x = TileX(tile) + diff.x;
	int y = TileY(tile) + diff.y;
	/* Negative value will become big positive value after cast */
	if ((uint)x >= MapSizeX() || (uint)y >= MapSizeY()) return INVALID_TILE;
	return TileXY(x, y);
}

/**
 * Returns the diff between two tiles
 *
 * @param tile_a from tile
 * @param tile_b to tile
 * @return the difference between tila_a and tile_b
 */
static inline TileIndexDiffC TileIndexToTileIndexDiffC(TileIndex tile_a, TileIndex tile_b)
{
	TileIndexDiffC difference;

	difference.x = TileX(tile_a) - TileX(tile_b);
	difference.y = TileY(tile_a) - TileY(tile_b);

	return difference;
}

/**
 * Get the offset of transformed northern tile corner.
 *
 * When transforming a tile, it's nothern corner can move to other location.
 * The function retuns difference (TileIndexDiffC) between new and old
 * locations e.g. when rotating 90 degree left, northern corner becomes
 * western and the difference is (1, 0).
 *
 * Scheme of a tile with corners and offsets: <tt><pre>
 *               N  (0, 0)
 *             /   \
 *    (1, 0)  W     E  (0, 1)
 *             \   /
 *               S  (1, 1)
 * </pre></tt>
 *
 * @param transformation The transformation.
 * @return Offset to new location of northern corner.
 *
 * @see TileIndexTransformations
 */
static inline TileIndexDiffC TransformedNorthCornerDiffC(DirTransformation transformation)
{
	/* lookup tables (bit arrays)
	 *                          N                   E                      S                     W                      E                        S                      W                        N   */
	static const uint8 DIFF_X = 0 << DTR_IDENTITY | 0 << DTR_ROTATE_90_R | 1 << DTR_ROTATE_180 | 1 << DTR_ROTATE_90_L | 0 << DTR_REFLECT_NE_SW | 1 << DTR_REFLECT_W_E | 1 << DTR_REFLECT_NW_SE | 0 << DTR_REFLECT_N_S;
	static const uint8 DIFF_Y = 0 << DTR_IDENTITY | 1 << DTR_ROTATE_90_R | 1 << DTR_ROTATE_180 | 0 << DTR_ROTATE_90_L | 1 << DTR_REFLECT_NE_SW | 1 << DTR_REFLECT_W_E | 0 << DTR_REFLECT_NW_SE | 0 << DTR_REFLECT_N_S;

	assert(IsValidDirTransform(transformation));

	TileIndexDiffC ret = { (int16)GB(DIFF_X, transformation, 1), (int16)GB(DIFF_Y, transformation, 1) };
	return ret;
}

TileTransformation TransformationBetweenTiles(int from_x, int from_y, int to_x, int to_y, DirTransformation dtr);
Point TransformTile(int x, int y, TileTransformation transformation);

/**
 * Create an invertion of a #TileTransformation.
 *
 * @param transformation Transformation to invert.
 * @return The inverted transformation.
 */
static inline TileTransformation InvertTileTransformation(TileTransformation transformation)
{
	return TransformationBetweenTiles(transformation.offset.x, transformation.offset.y, 0, 0, InvertDirTransform(transformation.dtr));
}

/**
 * Transform a tile index.
 *
 * @param tile The tile index to transform.
 * @param transformation Transformation to perform.
 * @param dst_map Destination map.
 * @return The transformed tile index.
 */
template <typename Tindex>
static inline GenericTileIndex TransformTile(const Tindex &tile, TileTransformation transformation, Map *dest_map)
{
	Point ret = TransformTile(TileX(tile), TileY(tile), transformation);
	return TileXY(ret.x, ret.y, dest_map);
}

/**
 * Transform a tile index.
 *
 * @param tile The tile index to transform.
 * @param transformation Transformation to perform.
 * @return The transformed tile index.
 */
static inline TileIndex TransformTile(TileIndex tile, TileTransformation transformation)
{
	return AsMainMapTile(TransformTile(tile, transformation, &_main_map));
}

/* Functions to calculate distances */
uint DistanceManhattan(TileIndex, TileIndex); ///< also known as L1-Norm. Is the shortest distance one could go over diagonal tracks (or roads)
uint DistanceSquare(TileIndex, TileIndex); ///< euclidian- or L2-Norm squared
uint DistanceMax(TileIndex, TileIndex); ///< also known as L-Infinity-Norm
uint DistanceMaxPlusManhattan(TileIndex, TileIndex); ///< Max + Manhattan
uint DistanceFromEdge(TileIndex); ///< shortest distance from any edge of the map
uint DistanceFromEdgeDir(TileIndex, DiagDirection); ///< distance from the map edge in given direction

/**
 * Convert a DiagDirection to a TileIndexDiff
 *
 * @param dir The DiagDirection
 * @param map The tile map (result will be valid only there)
 * @return The resulting TileIndexDiff
 *
 * @see TileIndexDiffCByDiagDir
 */
template <bool Tgeneric>
static inline TileIndexDiff TileOffsByDiagDir(DiagDirection dir, Map *map)
{
	extern const TileIndexDiffC _tileoffs_by_diagdir[DIAGDIR_END];

	assert(IsValidDiagDirection(dir));
	return ToTileIndexDiff<Tgeneric>(_tileoffs_by_diagdir[dir], map);
}

/**
 * Convert a DiagDirection to a TileIndexDiff
 *
 * @param dir The DiagDirection
 * @return The resulting TileIndexDiff
 * @see TileIndexDiffCByDiagDir
 */
static inline TileIndexDiff TileOffsByDiagDir(DiagDirection dir)
{
	return TileOffsByDiagDir<false>(dir, &_main_map);
}

/**
 * Convert a DiagDirection to a TileIndexDiff
 *
 * @param dir The DiagDirection
 * @param map The tile map (result will be valid only there)
 * @return The resulting TileIndexDiff
 * @see TileIndexDiffCByDiagDir
 */
static inline TileIndexDiff TileOffsByDiagDir(DiagDirection dir, Map *map)
{
	return TileOffsByDiagDir<true>(dir, map);
}

/**
 * Convert a Direction to a TileIndexDiff.
 *
 * @param dir The direction to convert from
 * @param map The tile map (result will be valid only there)
 * @return The resulting TileIndexDiff
 */
template <bool Tgeneric>
static inline TileIndexDiff TileOffsByDir(Direction dir, Map *map)
{
	extern const TileIndexDiffC _tileoffs_by_dir[DIR_END];

	assert(IsValidDirection(dir));
	return ToTileIndexDiff<Tgeneric>(_tileoffs_by_dir[dir], map);
}

/**
 * Convert a Direction to a TileIndexDiff.
 *
 * @param dir The direction to convert from
 * @return The resulting TileIndexDiff
 */
static inline TileIndexDiff TileOffsByDir(Direction dir)
{
	return TileOffsByDir<false>(dir, &_main_map);
}

/**
 * Convert a Direction to a TileIndexDiff.
 *
 * @param dir The direction to convert from
 * @param map The tile map (result will be valid only there)
 * @return The resulting TileIndexDiff
 */
static inline TileIndexDiff TileOffsByDir(Direction dir, Map *map)
{
	return TileOffsByDir<true>(dir, map);
}

/**
 * Adds a Direction to a tile.
 *
 * @param tile The current tile
 * @param dir The direction in which we want to step
 * @return the moved tile
 */
static inline TileIndex TileAddByDir(TileIndex tile, Direction dir)
{
	return TILE_ADD(tile, TileOffsByDir(dir));
}

/**
 * Adds a DiagDir to a tile.
 *
 * @param tile The current tile
 * @param dir The direction in which we want to step
 * @return the moved tile
 */
template <typename Tindex>
static inline Tindex TileAddByDiagDir(const Tindex &tile, DiagDirection dir)
{
	return TILE_ADDXY(tile, TileIndexDiffCByDiagDir(dir).x, TileIndexDiffCByDiagDir(dir).y);
}

/**
 * Determines the DiagDirection to get from one tile to another.
 * The tiles do not necessarily have to be adjacent.
 * @param tile_from Origin tile
 * @param tile_to Destination tile
 * @return DiagDirection from tile_from towards tile_to, or INVALID_DIAGDIR if the tiles are not on an axis
 */
static inline DiagDirection DiagdirBetweenTiles(TileIndex tile_from, TileIndex tile_to)
{
	int dx = (int)TileX(tile_to) - (int)TileX(tile_from);
	int dy = (int)TileY(tile_to) - (int)TileY(tile_from);
	if (dx == 0) {
		if (dy == 0) return INVALID_DIAGDIR;
		return (dy < 0 ? DIAGDIR_NW : DIAGDIR_SE);
	} else {
		if (dy != 0) return INVALID_DIAGDIR;
		return (dx < 0 ? DIAGDIR_NE : DIAGDIR_SW);
	}
}

/**
 * A callback function type for searching tiles.
 *
 * @param tile The tile to test
 * @param user_data additional data for the callback function to use
 * @return A boolean value, depend on the definition of the function.
 */
typedef bool TestTileOnSearchProc(TileIndex tile, void *user_data);

bool CircularTileSearch(TileIndex *tile, uint size, TestTileOnSearchProc proc, void *user_data);
bool CircularTileSearch(TileIndex *tile, uint radius, uint w, uint h, TestTileOnSearchProc proc, void *user_data);

/**
 * Get a random tile out of a given seed.
 * @param r the random 'seed'
 * @return a valid tile
 */
static inline TileIndex RandomTileSeed(uint32 r)
{
	return TILE_MASK(r);
}

/**
 * Get a valid random tile.
 * @note a define so 'random' gets inserted in the place where it is actually
 *       called, thus making the random traces more explicit.
 * @return a valid tile
 */
#define RandomTile() RandomTileSeed(Random())

uint GetClosestWaterDistance(TileIndex tile, bool water);

#endif /* MAP_FUNC_H */
