/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tilearea.cpp Handling of tile areas. */

#include "stdafx.h"

#include "core/geometry_func.hpp"
#include "tilearea_type.h"

#include "safeguards.h"

/**
 * Construct this tile area based on two points.
 * @param start the start of the area
 * @param end   the end of the area
 */
template <bool Tgeneric>
OrthogonalTileAreaT<Tgeneric>::OrthogonalTileAreaT(TileIndexType start, TileIndexType end)
{
	assert(IsSameMap(start, end));
	assert(IsValidTileIndex(start));
	assert(IsValidTileIndex(end));

	uint sx = TileX(start);
	uint sy = TileY(start);
	uint ex = TileX(end);
	uint ey = TileY(end);

	if (sx > ex) Swap(sx, ex);
	if (sy > ey) Swap(sy, ey);

	this->tile = TileXY<Tgeneric>(sx, sy, MapOf(start));
	this->w    = ex - sx + 1;
	this->h    = ey - sy + 1;
}

/**
 * Add a single tile to a tile area; enlarge if needed.
 * @param to_add The tile to add
 */
template <bool Tgeneric>
void OrthogonalTileAreaT<Tgeneric>::Add(TileIndexType to_add)
{
	if (!IsValidTileIndex(this->tile)) {
		this->tile = to_add;
		this->w = 1;
		this->h = 1;
		return;
	}

	assert(IsSameMap(this->tile, to_add));

	uint sx = TileX(this->tile);
	uint sy = TileY(this->tile);
	uint ex = sx + this->w - 1;
	uint ey = sy + this->h - 1;

	uint ax = TileX(to_add);
	uint ay = TileY(to_add);

	sx = min(ax, sx);
	sy = min(ay, sy);
	ex = max(ax, ex);
	ey = max(ay, ey);

	this->tile = TileXY<Tgeneric>(sx, sy, MapOf(to_add));
	this->w    = ex - sx + 1;
	this->h    = ey - sy + 1;
}

/**
 * Does this tile area intersect with another?
 * @param ta the other tile area to check against.
 * @return true if they intersect.
 */
template <bool Tgeneric>
bool OrthogonalTileAreaT<Tgeneric>::Intersects(const OrthogonalTileAreaT<Tgeneric> &ta) const
{
	if (ta.w == 0 || this->w == 0) return false;

	assert(ta.w != 0 && ta.h != 0 && this->w != 0 && this->h != 0);
	assert(IsSameMap(this->tile, ta.tile));

	uint left1   = TileX(this->tile);
	uint top1    = TileY(this->tile);
	uint right1  = left1 + this->w - 1;
	uint bottom1 = top1  + this->h - 1;

	uint left2   = TileX(ta.tile);
	uint top2    = TileY(ta.tile);
	uint right2  = left2 + ta.w - 1;
	uint bottom2 = top2  + ta.h - 1;

	return !(
			left2   > right1  ||
			right2  < left1   ||
			top2    > bottom1 ||
			bottom2 < top1
		);
}

/**
 * Does this tile area contains another?
 * @param ta the other tile area to check against.
 * @return true if the other area is fully contained.
 */
template <bool Tgeneric>
bool OrthogonalTileAreaT<Tgeneric>::Contains(const OrthogonalTileAreaT<Tgeneric> &ta) const
{
	if (ta.w == 0 || this->w == 0) return false;

	assert(ta.w != 0 && ta.h != 0 && this->w != 0 && this->h != 0);
	assert(IsSameMap(this->tile, ta.tile));

	uint left1   = TileX(this->tile);
	uint top1    = TileY(this->tile);
	uint right1  = left1 + this->w - 1;
	uint bottom1 = top1  + this->h - 1;

	uint left2   = TileX(ta.tile);
	uint top2    = TileY(ta.tile);
	uint right2  = left2 + ta.w - 1;
	uint bottom2 = top2  + ta.h - 1;

	return
			left2   >= left1  &&
			right2  <= right1 &&
			top2    >= top1   &&
			bottom2 <= bottom1;
}

/**
 * Does this tile area contain a tile?
 * @param tile Tile to test for.
 * @return True if the tile is inside the area.
 */
template <bool Tgeneric>
bool OrthogonalTileAreaT<Tgeneric>::Contains(TileIndexType tile) const
{
	if (this->w == 0) return false;

	assert(this->w != 0 && this->h != 0);
	assert(IsSameMap(this->tile, tile));

	uint left   = TileX(this->tile);
	uint top    = TileY(this->tile);
	uint tile_x = TileX(tile);
	uint tile_y = TileY(tile);

	return IsInsideBS(tile_x, left, this->w) && IsInsideBS(tile_y, top, this->h);
}

/**
 * Expand a tile area by rad tiles in each direction, keeping within map bounds.
 * @param rad Number of tiles to expand
 * @return The OrthogonalTileArea.
 */
template <bool Tgeneric>
OrthogonalTileAreaT<Tgeneric> &OrthogonalTileAreaT<Tgeneric>::Expand(int rad)
{
	int x = TileX(this->tile);
	int y = TileY(this->tile);

	int sx = max(x - rad, 0);
	int sy = max(y - rad, 0);
	int ex = min(x + this->w + rad, MapSizeX());
	int ey = min(y + this->h + rad, MapSizeY());

	this->tile = TileXY<Tgeneric>(sx, sy, MapOf(this->tile));
	this->w    = ex - sx;
	this->h    = ey - sy;
	return *this;
}

/**
 * Clamp the tile area to map borders.
 */
template <bool Tgeneric>
void OrthogonalTileAreaT<Tgeneric>::ClampToMap()
{
	assert(IsValidTileIndex(this->tile));
	this->w = min(this->w, MapSizeX(MapOf(this->tile)) - TileX(this->tile));
	this->h = min(this->h, MapSizeY(MapOf(this->tile)) - TileY(this->tile));
}

/**
 * Create a #TileTransformation based on two tile areas - before and after transformation.
 *
 * @param from Area before transormation.
 * @param to Transformed area.
 * @param dtr Direction transformation.
 * @return Transformation between the two areas.
 */
TileTransformation TransformationBetweenTileAreas(const GenericTileArea &from, const GenericTileArea &to, DirTransformation dtr)
{
	assert((TransformAxis(AXIS_X, dtr) == AXIS_X) ? (from.w == to.w && from.h == to.h) : (from.w == to.h && from.h == to.w));

	TileIndexDiffC dir = TransformedNorthCornerDiffC(dtr);
	return TransformationBetweenTiles(
			TileX(from.tile),
			TileY(from.tile),
			TileX(to.tile) + (to.w - 1) * dir.x,
			TileY(to.tile) + (to.h - 1) * dir.y,
			dtr);
}

/**
 * Create a #TileTransformation between tile corners of two tile areas - before and after transformation.
 *
 * @param from Area before transormation.
 * @param to Transformed area.
 * @param dtr Direction transformation.
 * @return Transformation between tile corners of the two areas.
 */
TileTransformation TransformationBetweenTileAreaCorners(const GenericTileArea &from, const GenericTileArea &to, DirTransformation dtr)
{
	TileTransformation ret = TransformationBetweenTileAreas(from, to, dtr);
	TileIndexDiffC extra = TransformedNorthCornerDiffC(dtr);
	ret.offset.x += extra.x;
	ret.offset.y += extra.y;
	return ret;
}

/**
 * Transform a tile area.
 *
 * @param ta The area to transform.
 * @param transformation Transformation to perform.
 * @param dst_map Destination map.
 * @return The transformed area.
 */
GenericTileArea TransformTileArea(const GenericTileArea &ta, TileTransformation transformation, Map *dst_map)
{
	Dimension size = { ta.w, ta.h };
	size = TransformDimension(size, transformation.dtr);
	Point pt = TransformTile(TileX(ta.tile), TileY(ta.tile), transformation);
	TileIndexDiffC dir = TransformedNorthCornerDiffC(transformation.dtr);
	pt.x -= (size.width - 1) * dir.x;
	pt.y -= (size.height - 1) * dir.y;

	return GenericTileArea(TileXY(pt.x, pt.y, dst_map), size.width, size.height);
}

GenericTileArea TransformTileArea(const GenericTileArea &ta, GenericTileIndex dst_tile, DirTransformation transformation)
{
	Dimension size = { ta.w, ta.h };
	size = TransformDimension(size, transformation);
	return GenericTileArea(dst_tile, size.width, size.height);
}

/**
 * Create a diagonal tile area from two corners.
 * @param start First corner of the area.
 * @param end Second corner of the area.
 */
template <bool Tgeneric>
DiagonalTileAreaT<Tgeneric>::DiagonalTileAreaT(TileIndexType start, TileIndexType end) : tile(start)
{
	assert(IsSameMap(start, end));
	assert(IsValidTileIndex(start));
	assert(IsValidTileIndex(end));

	/* Unfortunately we can't find a new base and make all a and b positive because
	 * the new base might be a "flattened" corner where there actually is no single
	 * tile. If we try anyway the result is either inaccurate ("one off" half of the
	 * time) or the code gets much more complex;
	 *
	 * We also need to increment/decrement a and b here to have one-past-end semantics
	 * for a and b, just the way the orthogonal tile area does it for w and h. */

	this->a = TileY(end) + TileX(end) - TileY(start) - TileX(start);
	this->b = TileY(end) - TileX(end) - TileY(start) + TileX(start);
	if (this->a > 0) {
		this->a++;
	} else {
		this->a--;
	}

	if (this->b > 0) {
		this->b++;
	} else {
		this->b--;
	}
}

/**
 * Does this tile area contain a tile?
 * @param tile Tile to test for.
 * @return True if the tile is inside the area.
 */
template <bool Tgeneric>
bool DiagonalTileAreaT<Tgeneric>::Contains(TileIndexType tile) const
{
	assert(IsSameMap(this->tile, tile));

	int a = TileY(tile) + TileX(tile);
	int b = TileY(tile) - TileX(tile);

	int start_a = TileY(this->tile) + TileX(this->tile);
	int start_b = TileY(this->tile) - TileX(this->tile);

	int end_a = start_a + this->a;
	int end_b = start_b + this->b;

	/* Swap if necessary, preserving the "one past end" semantics. */
	if (start_a > end_a) {
		int tmp = start_a;
		start_a = end_a + 1;
		end_a = tmp + 1;
	}
	if (start_b > end_b) {
		int tmp = start_b;
		start_b = end_b + 1;
		end_b = tmp + 1;
	}

	return (a >= start_a && a < end_a && b >= start_b && b < end_b);
}

/**
 * Perform single iteration step.
 */
template <bool Tgeneric>
TileIteratorT<Tgeneric>& DiagonalTileIteratorT<Tgeneric>::operator ++()
{
	assert(IsValidTileIndex(this->tile));

	/* Determine the next tile, while clipping at map borders */
	bool new_line = false;
	do {
		/* Iterate using the rotated coordinates. */
		if (this->a_max == 1 || this->a_max == -1) {
			/* Special case: Every second column has zero length, skip them completely */
			this->a_cur = 0;
			if (this->b_max > 0) {
				this->b_cur = min(this->b_cur + 2, this->b_max);
			} else {
				this->b_cur = max(this->b_cur - 2, this->b_max);
			}
		} else {
			/* Every column has at least one tile to process */
			if (this->a_max > 0) {
				this->a_cur += 2;
				new_line = this->a_cur >= this->a_max;
			} else {
				this->a_cur -= 2;
				new_line = this->a_cur <= this->a_max;
			}
			if (new_line) {
				/* offset of initial a_cur: one tile in the same direction as a_max
				 * every second line.
				 */
				this->a_cur = abs(this->a_cur) % 2 ? 0 : (this->a_max > 0 ? 1 : -1);

				if (this->b_max > 0) {
					++this->b_cur;
				} else {
					--this->b_cur;
				}
			}
		}

		/* And convert the coordinates back once we've gone to the next tile. */
		uint x = this->base_x + (this->a_cur - this->b_cur) / 2;
		uint y = this->base_y + (this->b_cur + this->a_cur) / 2;
		/* Prevent wrapping around the map's borders. */
		if (x >= MapSizeX(MapOf(this->tile)) || y >= MapSizeY(MapOf(this->tile))) {
			IndexOf(this->tile) = INVALID_TILE_INDEX;
		} else {
			this->tile = TileXY<Tgeneric>(x, y, MapOf(this->tile));
		}
	} while (!IsValidTileIndex(this->tile) && this->b_max != this->b_cur);

	if (this->b_max == this->b_cur) IndexOf(this->tile) = INVALID_TILE_INDEX;

	return *this;
}

/* instantiate */
template struct OrthogonalTileAreaT<false>;
template struct OrthogonalTileAreaT<true>;
template struct DiagonalTileAreaT<false>;
template struct DiagonalTileAreaT<true>;
template class DiagonalTileIteratorT<true>;
template class DiagonalTileIteratorT<false>;
