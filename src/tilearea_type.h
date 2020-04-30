/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file tilearea_type.h Type for storing the 'area' of something uses on the map. */

/**
 * @defgroup TileIndexTransformations Tile index transformations
 *
 * @image html tile_index_transformations.svg
 */

#ifndef TILEAREA_TYPE_H
#define TILEAREA_TYPE_H

#include "map_func.h"

/**
 * Set of coordinates representing rectangular piece of a tile map.
 *
 * This "raw" area does not point to any map. These are pure coordinates. Unless
 * we bound them to a cartain map we cannot make most of calculations.
 *
 * @see RawTileIndex
 * @see OrthogonalTileAreaT
 */
struct RawTileArea {
	RawTileIndex tile; ///< The base (northern) tile of the area
	uint16 w;          ///< The width of the area
	uint16 h;          ///< The height of the area
};

/**
 * Set of coordinates representing rectangular piece of a tile map e.g. a rail station.
 *
 * This tile area, based on template args, can represent a part of either the main map
 * or any chosen map.
 *
 * @tparam Tgeneric If \c false, this area will represent tiles on the main map.
 *                  If \c true, this area will be able to represent tiles on any map chosen at runtime.
 *
 * @note To use a specific overload there are #TileArea and #GenericTileArea to your
 *       disposition. Use OrthogonalTileAreaT type directly only when working with templates.
 *
 * @see RawOrthogonalTileArea
 * @see TileIndexT
 */
template <bool Tgeneric>
struct OrthogonalTileAreaT {
	typedef typename TileIndexT<Tgeneric>::T TileIndexType; ///< The type of tile indices, TileIndex or GenericTileIndex.

	TileIndexType tile; ///< The base tile of the area
	uint16 w;           ///< The width of the area
	uint16 h;           ///< The height of the area

	/**
	 * Construct this tile area with some set values
	 * @param tile the base tile
	 * @param w the width
	 * @param h the height
	 */
	OrthogonalTileAreaT(TileIndexType tile = TileIndexType(INVALID_TILE), uint8 w = 0, uint8 h = 0) : tile(tile), w(w), h(h)
	{
	}

	/**
	 * Make a copy of a given tile area
	 * @param ta the area to copy
	 */
	template <bool TotherGeneric>
	OrthogonalTileAreaT(const OrthogonalTileAreaT<TotherGeneric> &ta)
		: tile(MakeTileIndex<Tgeneric>(IndexOf(ta.tile), MapOf(ta.tile))), w(ta.w), h(ta.h)
	{
	}

	/**
	 * Construct this tile area from a "raw" tile area and a given tile map
	 * @param ta the "raw" tile area
	 * @param map the map
	 */
	inline OrthogonalTileAreaT(const RawTileArea &ta, Map *map)
		: tile(MakeTileIndex<Tgeneric>(ta.tile, map)), w(ta.w), h(ta.h)
	{
	}

	OrthogonalTileAreaT(TileIndexType start, TileIndexType end);

	void Add(TileIndexType to_add);

	/**
	 * Clears the 'tile area', i.e. make the tile invalid.
	 */
	void Clear()
	{
		IndexOf(this->tile) = INVALID_TILE_INDEX;
		this->w = 0;
		this->h = 0;
	}

	bool Intersects(const OrthogonalTileAreaT<Tgeneric> &ta) const;

	bool Contains(const OrthogonalTileAreaT<Tgeneric> &ta) const;
	bool Contains(TileIndexType tile) const;

	OrthogonalTileAreaT<Tgeneric> &Expand(int rad);

	void ClampToMap();

	/**
	 * Get the center tile.
	 * @return The tile at the center, or just north of it.
	 */
	TileIndexType GetCenterTile() const
	{
		return TILE_ADDXY(this->tile, this->w / 2, this->h / 2);
	}
};

/** Represents a diagonal tile area. */
template <bool Tgeneric>
struct DiagonalTileAreaT {
	typedef typename TileIndexT<Tgeneric>::T TileIndexType; ///< The type of tile indices, TileIndex or GenericTileIndex.

	TileIndexType tile; ///< Base tile of the area
	int16 a;        ///< Extent in diagonal "x" direction (may be negative to signify the area stretches to the left)
	int16 b;        ///< Extent in diagonal "y" direction (may be negative to signify the area stretches upwards)

	/**
	 * Construct this tile area with some set values.
	 * @param tile The base tile.
	 * @param a The "x" extent.
	 * @param b The "y" estent.
	 */
	DiagonalTileAreaT(TileIndexType tile = TileIndexType(INVALID_TILE), int8 a = 0, int8 b = 0) : tile(tile), a(a), b(b)
	{
	}

	DiagonalTileAreaT(TileIndexType start, TileIndexType end);

	/**
	 * Clears the TileArea by making the tile invalid and setting a and b to 0.
	 */
	void Clear()
	{
		IndexOf(this->tile) = INVALID_TILE_INDEX;
		this->a = 0;
		this->b = 0;
	}

	bool Contains(TileIndexType tile) const;
};

/**
 * Set of coordinates representing rectangular piece of the main tile map.
 *
 * This is the most common type of tile area. It represents tiles on the main tile array.
 *
 * @see TileIndex
 * @see GenericTileArea
 */
typedef OrthogonalTileAreaT<false> OrthogonalTileArea;

/** Shorthand for the much more common orthogonal tile area. */
typedef OrthogonalTileArea TileArea;

/**
 * Set of coordinates representing rectangular piece of a tile map.
 *
 * This "generic" tile area is able to represent part of any map chosen at runtime.
 *
 * @see GenericTileIndex
 * @see TileArea
 */
typedef OrthogonalTileAreaT<true> GenericTileArea;

typedef DiagonalTileAreaT<false> DiagonalTileArea;

/** Base class for tile iterators. */
template <bool Tgeneric>
class TileIteratorT {
public:
	typedef typename TileIndexT<Tgeneric>::T TileIndexType; ///< The type of tile indices, TileIndex or GenericTileIndex.

protected:
	TileIndexType tile; ///< The current tile we are at.

	/**
	 * Initialise the iterator starting at this tile.
	 * @param tile The tile we start iterating from.
	 */
	TileIteratorT(TileIndexType tile = TileIndexType(INVALID_TILE)) : tile(tile)
	{
	}

public:
	/** Some compilers really like this. */
	virtual ~TileIteratorT()
	{
	}

	/**
	 * Get the tile we are currently at.
	 * @return The tile we are at, or "invalid" when we're done.
	 */
	inline operator TileIndexType () const
	{
		return this->tile;
	}

	/**
	 * Move ourselves to the next tile in the rectangle on the map.
	 */
	virtual TileIteratorT<Tgeneric>& operator ++() = 0;

	/**
	 * Allocate a new iterator that is a copy of this one.
	 */
	virtual TileIteratorT<Tgeneric> *Clone() const = 0;
};

/** Base class for tile iterators of the main map. */
typedef TileIteratorT<false> TileIterator;

/** Iterator to iterate over a tile area (rectangle) of a map. */
template <bool Tgeneric>
class OrthogonalTileIteratorT : public TileIteratorT<Tgeneric> {
public:
	typedef typename TileIteratorT<Tgeneric>::TileIndexType TileIndexType;

	int w; ///< The width of the iterated area.
	int x; ///< The current 'x' position in the rectangle.
	int y; ///< The current 'y' position in the rectangle.

	/**
	 * Construct the iterator.
	 * @param ta Area, i.e. begin point and width/height of to-be-iterated area.
	 */
	OrthogonalTileIteratorT(const OrthogonalTileAreaT<Tgeneric> &ta)
		: TileIteratorT<Tgeneric>(ta.tile), w(ta.w), x(ta.w), y(ta.h)
	{
		if (ta.w == 0 || ta.h == 0) IndexOf(this->tile) = INVALID_TILE_INDEX;
	}

	/**
	 * Construct the iterator.
	 * @param corner1 Tile from where to begin iterating.
	 * @param corner2 Tile where to end the iterating.
	 */
	OrthogonalTileIteratorT(TileIndexType corner1, TileIndexType corner2)
	{
		*this = OrthogonalTileIteratorT<Tgeneric>(OrthogonalTileAreaT<Tgeneric>(corner1, corner2));
	}

	/**
	 * Move ourselves to the next tile in the rectangle on the map.
	 */
	inline TileIteratorT<Tgeneric>& operator ++()
	{
		assert(IndexOf(this->tile) != INVALID_TILE_INDEX);

		if (--this->x > 0) {
			++this->tile;
		} else if (--this->y > 0) {
			this->x = this->w;
			this->tile += TileDiffXY(1, 1, MapOf(this->tile)) - this->w;
		} else {
			IndexOf(this->tile) = INVALID_TILE_INDEX;
		}

		return *this;
	}

	virtual TileIteratorT<Tgeneric> *Clone() const
	{
		return new OrthogonalTileIteratorT<Tgeneric>(*this);
	}
};

/** Iterator to iterate over a tile area (rectangle) of the main map. */
typedef OrthogonalTileIteratorT<false> OrthogonalTileIterator;

/** Iterator to iterate over a diagonal area of a map. */
template <bool Tgeneric>
class DiagonalTileIteratorT : public TileIteratorT<Tgeneric> {
public:
	typedef typename TileIteratorT<Tgeneric>::TileIndexType TileIndexType;

	uint base_x; ///< The base tile x coordinate from where the iterating happens.
	uint base_y; ///< The base tile y coordinate from where the iterating happens.
	int a_cur;   ///< The current (rotated) x coordinate of the iteration.
	int b_cur;   ///< The current (rotated) y coordinate of the iteration.
	int a_max;   ///< The (rotated) x coordinate of the end of the iteration.
	int b_max;   ///< The (rotated) y coordinate of the end of the iteration.

	/**
	 * Construct the iterator.
	 * @param ta Area, i.e. begin point and (diagonal) width/height of to-be-iterated area.
	 */
	DiagonalTileIteratorT(const DiagonalTileAreaT<Tgeneric> &ta) : TileIteratorT<Tgeneric>(ta.tile)
	{
		this->base_x = TileX(ta.tile);
		this->base_y = TileY(ta.tile);
		this->a_cur = 0;
		this->b_cur = 0;
		this->a_max = ta.a;
		this->b_max = ta.b;
	}

	/**
	 * Construct the iterator.
	 * @param corner1 Tile from where to begin iterating.
	 * @param corner2 Tile where to end the iterating.
	 */
	DiagonalTileIteratorT(TileIndexType corner1, TileIndexType corner2)
	{
		*this = DiagonalTileIteratorT<Tgeneric>(DiagonalTileAreaT<Tgeneric>(corner1, corner2));
	}

	virtual TileIteratorT<Tgeneric>& operator ++();

	virtual TileIteratorT<Tgeneric> *Clone() const
	{
		return new DiagonalTileIteratorT<Tgeneric>(*this);
	}
};

/** Iterator to iterate over a diagonal area of the main map. */
typedef DiagonalTileIteratorT<false> DiagonalTileIterator;

/**
 * Iterator to iterate over a diagonal area of a map performing transformation on tile indices.
 *
 * Iterator will iterate over source area in the same way OrthogonalTileIteratorT do, additionally
 * performing transformation on tile indices. You can call SrcTile or DstTile to get the tile before
 * and after transformation.
 *
 * The tile of this iterator (it's base) is the transformed one.
 */
template <bool Tgeneric>
class TransformationTileIteratorT : public TileIteratorT<Tgeneric> {
public:
	typedef typename TileIteratorT<Tgeneric>::TileIndexType TileIndexType;

protected:
	OrthogonalTileIteratorT<true> src_iter; ///< Source tile iterator.
	TileTransformation transformation; ///< Transformation.

private:
	inline void UpdateDstTile()
	{
		if (IndexOf(this->src_iter) == INVALID_TILE_INDEX) {
			IndexOf(this->tile) = INVALID_TILE_INDEX;
		} else {
			IndexOf(this->tile) = IndexOf(TransformTile(this->src_iter, this->transformation, MapOf(this->tile)));
		}
	}

public:
	/**
	 * Create a TransformationTileIteratorT that transforms tile indices
	 * from one tile area to another.
	 *
	 * @param src_area Source area to iterate over.
	 * @param transformation Transformation to perform on tile indices.
	 * @param dst_map Map of the destination iterator.
	 */
	TransformationTileIteratorT(const GenericTileArea &src_area, TileTransformation transformation, Map *dst_map = _main_map)
		: TileIteratorT<Tgeneric>(MakeTileIndex<Tgeneric>(INVALID_TILE_INDEX, dst_map))
		, src_iter(src_area)
		, transformation(transformation)
	{
		this->UpdateDstTile();
	}

	/**
	 * Copy constructor.
	 * @param iter The other iterator.
	 */
	template <bool TotherGeneric>
	TransformationTileIteratorT(const TransformationTileIteratorT<TotherGeneric> &iter)
		: TileIteratorT<Tgeneric>(MakeTileIndex<Tgeneric>(IndexOf(iter.tile), MapOf(iter.tile)))
		, src_iter(iter.src_iter)
		, transformation(iter.transformation)
	{
	}

	/**
	 * The source tile of the transformation.
	 * @return Tile before transformation.
	 */
	inline GenericTileIndex SrcTile() const { return this->src_iter; }

	/**
	 * The destination tile of the transformation (the tile of this iterator).
	 * @return Tile after transformation.
	 */
	inline TileIndexType DstTile() const { return this->tile; }

	virtual TileIteratorT<Tgeneric> &operator ++ ()
	{
		assert(IndexOf(this->tile) != INVALID_TILE_INDEX);

		++this->src_iter;
		this->UpdateDstTile();
		return *this;
	}

	virtual TileIteratorT<Tgeneric> *Clone() const
	{
		return new TransformationTileIteratorT<Tgeneric>(*this);
	}
};

/**
 * A loop which iterates over the tiles of a TileArea.
 * @param var The name of the variable which contains the current tile.
 *            This variable will be allocated in this \c for of this loop.
 * @param ta  The tile area to search over.
 */
#define TILE_AREA_LOOP(var, ta) for (OrthogonalTileIterator var(ta); var != INVALID_TILE; ++var)

/**
 * A loop which iterates over the tiles of a GenericTileArea.
 * @param var The name of the variable which contains the current tile.
 *            This variable will be allocated in this \c for of this loop.
 * @param ta  The tile area to search over.
 */
#define GENERIC_TILE_AREA_LOOP(var, ta) for (OrthogonalTileIteratorT<true> var(ta); IsValidTileIndex(var); ++var)

#endif /* TILEAREA_TYPE_H */
