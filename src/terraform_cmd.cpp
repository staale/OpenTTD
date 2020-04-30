/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file terraform_cmd.cpp Commands related to terraforming. */

#include "stdafx.h"
#include "command_func.h"
#include "copypaste_cmd.h"
#include "tunnel_map.h"
#include "bridge_map.h"
#include "viewport_func.h"
#include "genworld.h"
#include "object_base.h"
#include "company_base.h"
#include "company_func.h"
#include "strings_func.h"
#include "tilearea_func.h"

#include "table/strings.h"

#include <map>
#include <set>

#include "safeguards.h"

/** Set of tiles. */
typedef std::set<TileIndex> TileIndexSet;
/** Mapping of tiles to their height. */
typedef std::map<TileIndex, int> TileIndexToHeightMap;

/** State of the terraforming. */
struct TerraformerState {
	TileIndexSet dirty_tiles;                ///< The tiles that need to be redrawn.
	TileIndexToHeightMap tile_to_new_height; ///< The tiles for which the height has changed.
};

TileIndex _terraform_err_tile; ///< first tile we couldn't terraform

/**
 * Gets the TileHeight (height of north corner) of a tile as of current terraforming progress.
 *
 * @param ts TerraformerState.
 * @param tile Tile.
 * @return TileHeight.
 */
static int TerraformGetHeightOfTile(const TerraformerState *ts, TileIndex tile)
{
	TileIndexToHeightMap::const_iterator it = ts->tile_to_new_height.find(tile);
	return it != ts->tile_to_new_height.end() ? it->second : TileHeight(tile);
}

/**
 * Stores the TileHeight (height of north corner) of a tile in a TerraformerState.
 *
 * @param ts TerraformerState.
 * @param tile Tile.
 * @param height New TileHeight.
 */
static void TerraformSetHeightOfTile(TerraformerState *ts, TileIndex tile, int height)
{
	ts->tile_to_new_height[tile] = height;
}

/**
 * Adds a tile to the "tile_table" in a TerraformerState.
 *
 * @param ts TerraformerState.
 * @param tile Tile.
 * @ingroup dirty
 */
static void TerraformAddDirtyTile(TerraformerState *ts, TileIndex tile)
{
	ts->dirty_tiles.insert(tile);
}

/**
 * Adds all tiles that incident with the north corner of a specific tile to the "tile_table" in a TerraformerState.
 *
 * @param ts TerraformerState.
 * @param tile Tile.
 * @ingroup dirty
 */
static void TerraformAddDirtyTileAround(TerraformerState *ts, TileIndex tile)
{
	/* Make sure all tiles passed to TerraformAddDirtyTile are within [0, MapSize()] */
	if (TileY(tile) >= 1) TerraformAddDirtyTile(ts, tile + TileDiffXY( 0, -1));
	if (TileY(tile) >= 1 && TileX(tile) >= 1) TerraformAddDirtyTile(ts, tile + TileDiffXY(-1, -1));
	if (TileX(tile) >= 1) TerraformAddDirtyTile(ts, tile + TileDiffXY(-1,  0));
	TerraformAddDirtyTile(ts, tile);
}

/**
 * Terraform the north corner of a tile to a specific height.
 *
 * @param ts TerraformerState.
 * @param tile Tile.
 * @param height Aimed height.
 * @return Error code or cost.
 */
static CommandCost TerraformTileHeight(TerraformerState *ts, TileIndex tile, int height)
{
	assert(tile < MapSize());

	/* Check range of destination height */
	if (height < 0) return_cmd_error(STR_ERROR_ALREADY_AT_SEA_LEVEL);
	if (height > _settings_game.construction.max_heightlevel) return_cmd_error(STR_ERROR_TOO_HIGH);

	/*
	 * Check if the terraforming has any effect.
	 * This can only be true, if multiple corners of the start-tile are terraformed (i.e. the terraforming is done by towns/industries etc.).
	 * In this case the terraforming should fail. (Don't know why.)
	 */
	if (height == TerraformGetHeightOfTile(ts, tile)) return CMD_ERROR;

	/* Check "too close to edge of map". Only possible when freeform-edges is off. */
	uint x = TileX(tile);
	uint y = TileY(tile);
	if (!_settings_game.construction.freeform_edges && ((x <= 1) || (y <= 1) || (x >= MapMaxX() - 1) || (y >= MapMaxY() - 1))) {
		/*
		 * Determine a sensible error tile
		 */
		if (x == 1) x = 0;
		if (y == 1) y = 0;
		_terraform_err_tile = TileXY(x, y);
		return_cmd_error(STR_ERROR_TOO_CLOSE_TO_EDGE_OF_MAP);
	}

	/* Mark incident tiles that are involved in the terraforming. */
	TerraformAddDirtyTileAround(ts, tile);

	/* Store the height modification */
	TerraformSetHeightOfTile(ts, tile, height);

	CommandCost total_cost(EXPENSES_CONSTRUCTION);

	/* Increment cost */
	total_cost.AddCost(_price[PR_TERRAFORM]);

	/* Recurse to neighboured corners if height difference is larger than 1 */
	{
		const TileIndexDiffC *ttm;

		TileIndex orig_tile = tile;
		static const TileIndexDiffC _terraform_tilepos[] = {
			{ 1,  0}, // move to tile in SE
			{-2,  0}, // undo last move, and move to tile in NW
			{ 1,  1}, // undo last move, and move to tile in SW
			{ 0, -2}  // undo last move, and move to tile in NE
		};

		for (ttm = _terraform_tilepos; ttm != endof(_terraform_tilepos); ttm++) {
			tile += ToTileIndexDiff(*ttm);

			if (tile >= MapSize()) continue;
			/* Make sure we don't wrap around the map */
			if (Delta(TileX(orig_tile), TileX(tile)) == MapSizeX() - 1) continue;
			if (Delta(TileY(orig_tile), TileY(tile)) == MapSizeY() - 1) continue;

			/* Get TileHeight of neighboured tile as of current terraform progress */
			int r = TerraformGetHeightOfTile(ts, tile);
			int height_diff = height - r;

			/* Is the height difference to the neighboured corner greater than 1? */
			if (abs(height_diff) > 1) {
				/* Terraform the neighboured corner. The resulting height difference should be 1. */
				height_diff += (height_diff < 0 ? 1 : -1);
				CommandCost cost = TerraformTileHeight(ts, tile, r + height_diff);
				if (cost.Failed()) return cost;
				total_cost.AddCost(cost);
			}
		}
	}

	return total_cost;
}

/**
 * Terraform land
 * @param tile tile to terraform
 * @param flags for this command type
 * @param p1 corners to terraform (SLOPE_xxx)
 * @param p2 direction; eg up (non-zero) or down (zero)
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdTerraformLand(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	_terraform_err_tile = INVALID_TILE;

	CommandCost total_cost(EXPENSES_CONSTRUCTION);
	int direction = (p2 != 0 ? 1 : -1);
	TerraformerState ts;

	/* Compute the costs and the terraforming result in a model of the landscape */
	if ((p1 & SLOPE_W) != 0 && tile + TileDiffXY(1, 0) < MapSize()) {
		TileIndex t = tile + TileDiffXY(1, 0);
		CommandCost cost = TerraformTileHeight(&ts, t, TileHeight(t) + direction);
		if (cost.Failed()) return cost;
		total_cost.AddCost(cost);
	}

	if ((p1 & SLOPE_S) != 0 && tile + TileDiffXY(1, 1) < MapSize()) {
		TileIndex t = tile + TileDiffXY(1, 1);
		CommandCost cost = TerraformTileHeight(&ts, t, TileHeight(t) + direction);
		if (cost.Failed()) return cost;
		total_cost.AddCost(cost);
	}

	if ((p1 & SLOPE_E) != 0 && tile + TileDiffXY(0, 1) < MapSize()) {
		TileIndex t = tile + TileDiffXY(0, 1);
		CommandCost cost = TerraformTileHeight(&ts, t, TileHeight(t) + direction);
		if (cost.Failed()) return cost;
		total_cost.AddCost(cost);
	}

	if ((p1 & SLOPE_N) != 0) {
		TileIndex t = tile + TileDiffXY(0, 0);
		CommandCost cost = TerraformTileHeight(&ts, t, TileHeight(t) + direction);
		if (cost.Failed()) return cost;
		total_cost.AddCost(cost);
	}

	/* Check if the terraforming is valid wrt. tunnels, bridges and objects on the surface
	 * Pass == 0: Collect tileareas which are caused to be auto-cleared.
	 * Pass == 1: Collect the actual cost. */
	for (int pass = 0; pass < 2; pass++) {
		for (TileIndexSet::const_iterator it = ts.dirty_tiles.begin(); it != ts.dirty_tiles.end(); it++) {
			TileIndex tile = *it;

			assert(tile < MapSize());
			/* MP_VOID tiles can be terraformed but as tunnels and bridges
			 * cannot go under / over these tiles they don't need checking. */
			if (IsTileType(tile, MP_VOID)) continue;

			/* Find new heights of tile corners */
			int z_N = TerraformGetHeightOfTile(&ts, tile + TileDiffXY(0, 0));
			int z_W = TerraformGetHeightOfTile(&ts, tile + TileDiffXY(1, 0));
			int z_S = TerraformGetHeightOfTile(&ts, tile + TileDiffXY(1, 1));
			int z_E = TerraformGetHeightOfTile(&ts, tile + TileDiffXY(0, 1));

			/* Find min and max height of tile */
			int z_min = min(min(z_N, z_W), min(z_S, z_E));
			int z_max = max(max(z_N, z_W), max(z_S, z_E));

			/* Compute tile slope */
			Slope tileh = (z_max > z_min + 1 ? SLOPE_STEEP : SLOPE_FLAT);
			if (z_W > z_min) tileh |= SLOPE_W;
			if (z_S > z_min) tileh |= SLOPE_S;
			if (z_E > z_min) tileh |= SLOPE_E;
			if (z_N > z_min) tileh |= SLOPE_N;

			if (pass == 0) {
				/* Check if bridge would take damage */
				if (IsBridgeAbove(tile)) {
					int bridge_height = GetBridgeHeight(GetSouthernBridgeEnd(tile));

					/* Check if bridge would take damage. */
					if (direction == 1 && bridge_height <= z_max) {
						_terraform_err_tile = tile; // highlight the tile under the bridge
						return_cmd_error(STR_ERROR_MUST_DEMOLISH_BRIDGE_FIRST);
					}

					/* Is the bridge above not too high afterwards? */
					if (direction == -1 && bridge_height > (z_min + _settings_game.construction.max_bridge_height)) {
						_terraform_err_tile = tile;
						return_cmd_error(STR_ERROR_BRIDGE_TOO_HIGH_AFTER_LOWER_LAND);
					}
				}
				/* Check if tunnel would take damage */
				if (direction == -1 && IsTunnelInWay(tile, z_min)) {
					_terraform_err_tile = tile; // highlight the tile above the tunnel
					return_cmd_error(STR_ERROR_EXCAVATION_WOULD_DAMAGE);
				}
			}

			/* Is the tile already cleared? */
			const ClearedObjectArea *coa = FindClearedObject(tile);
			bool indirectly_cleared = coa != nullptr && coa->first_tile != tile;

			/* Check tiletype-specific things, and add extra-cost */
			const bool curr_gen = _generating_world;
			if (_game_mode == GM_EDITOR) _generating_world = true; // used to create green terraformed land
			DoCommandFlag tile_flags = flags | DC_AUTO | DC_FORCE_CLEAR_TILE;
			if (pass == 0) {
				tile_flags &= ~DC_EXEC;
				tile_flags |= DC_NO_MODIFY_TOWN_RATING;
			}
			CommandCost cost;
			if (indirectly_cleared) {
				cost = DoCommand(tile, 0, 0, tile_flags, CMD_LANDSCAPE_CLEAR);
			} else {
				cost = _tile_type_procs[GetTileType(tile)]->terraform_tile_proc(tile, tile_flags, z_min, tileh);
			}
			_generating_world = curr_gen;
			if (cost.Failed()) {
				_terraform_err_tile = tile;
				return cost;
			}
			if (pass == 1) total_cost.AddCost(cost);
		}
	}

	Company *c = Company::GetIfValid(_current_company);
	if (c != nullptr && GB(c->terraform_limit, 16, 16) < ts.tile_to_new_height.size()) {
		return_cmd_error(STR_ERROR_TERRAFORM_LIMIT_REACHED);
	}

	if (flags & DC_EXEC) {
		/* Mark affected areas dirty. */
		for (TileIndexSet::const_iterator it = ts.dirty_tiles.begin(); it != ts.dirty_tiles.end(); it++) {
			MarkTileDirtyByTile(*it);
			TileIndexToHeightMap::const_iterator new_height = ts.tile_to_new_height.find(tile);
			if (new_height == ts.tile_to_new_height.end()) continue;
			MarkTileDirtyByTile(*it, 0, new_height->second);
		}

		/* change the height */
		for (TileIndexToHeightMap::const_iterator it = ts.tile_to_new_height.begin();
				it != ts.tile_to_new_height.end(); it++) {
			TileIndex tile = it->first;
			int height = it->second;

			SetTileHeight(tile, (uint)height);
		}

		if (c != nullptr) c->terraform_limit -= (uint32)ts.tile_to_new_height.size() << 16;
	}
	return total_cost;
}

class TileHeightSource {
public:
	virtual ~TileHeightSource()
	{ }

	virtual int GetTileTargetHeight(const TileIterator &tile) const = 0;
};

class LevelLandHeightSource : public TileHeightSource {
protected:
	uint min_height;
	uint max_height;

public:
	LevelLandHeightSource(uint min_height, uint max_height)
		: min_height(min_height), max_height(max_height)
	{
		assert(min_height <= max_height);
	}

	virtual int GetTileTargetHeight(const TileIterator &tile) const
	{
		return Clamp(TileHeight(tile), this->min_height, this->max_height);
	}
};

class CopyPasteHeightSource : public TileHeightSource {
protected:
	uint height_delta;

public:
	CopyPasteHeightSource(uint height_delta)
		: height_delta(height_delta)
	{ }

	virtual int GetTileTargetHeight(const TileIterator &tile) const
	{
		GenericTileIndex src_tile = static_cast<const TransformationTileIteratorT<false>&>(tile).SrcTile();
		return TileHeight(src_tile) + this->height_delta;
	}
};

/** Compound result of a terraform process. */
struct TerraformTilesResult {
	Money cost;          ///< Overal cost.
	bool had_success;    ///< Whether any success occured.
	StringID last_error; ///< Last error, STR_NULL if there were no errors.
};

static TerraformTilesResult TerraformTiles(TileIterator *iter, const TileHeightSource *height_source, DoCommandFlag flags, Money available_money = GetAvailableMoneyForCommand());

/**
 * Levels a selected (rectangle) area of land
 * @param tile end tile of area-drag
 * @param flags for this command type
 * @param p1 start tile of area drag
 * @param p2 various bitstuffed data.
 *  bit      0: Whether to use the Orthogonal (0) or Diagonal (1) iterator.
 *  bits 1 - 2: Mode of leveling \c LevelMode.
 * @param text unused
 * @return the cost of this operation or an error
 */
CommandCost CmdLevelLand(TileIndex tile, DoCommandFlag flags, uint32 p1, uint32 p2, const char *text)
{
	if (p1 >= MapSize()) return CMD_ERROR;

	/* compute new height */
	int h = TileHeight(p1);
	switch ((LevelMode)GB(p2, 1, 2)) {
		case LM_LEVEL: break;
		case LM_RAISE: h++; break;
		case LM_LOWER: h--; break;
		default: return CMD_ERROR;
	}

	TerraformTilesResult ret;
	LevelLandHeightSource height_source(h, h);
	if (HasBit(p2, 0)) {
		DiagonalTileIterator iter(DiagonalTileArea(tile, p1));
		ret = TerraformTiles(&iter, &height_source, flags);
	} else {
		OrthogonalTileIterator iter(OrthogonalTileArea(tile, p1));
		ret = TerraformTiles(&iter, &height_source, flags);
	}

	/* If there were only errors then fail with the last one. */
	if (!ret.had_success && ret.last_error != STR_NULL) return_cmd_error(ret.last_error);
	/* Return overal cost. */
	return CommandCost(EXPENSES_CONSTRUCTION, ret.cost);
}

/**
 * Terraform tiles as a part of a pasting process.
 * @param iter Tile iterator that iterates over destination tiles.
 * @param height_source Provides target heights for tiles.
 */
static void TerraformPasteTiles(TileIterator *iter, const TileHeightSource *height_source)
{
	TileIndex start_tile = *iter;

	/* Do actual terraforming. */
	TerraformTilesResult ret = TerraformTiles(iter, height_source, _current_pasting->dc_flags | DC_ALL_TILES, _current_pasting->GetAvailableMoney());

	/* When copy-pasting, we want to higlight error tiles more frequently. TerraformTiles
	 * doesn't always set the _terraform_err_tile (on some errors it's just INVALID_TILE).
	 * We will assume the start tile in these cases. This will give a better overview on
	 * what area failed to paste. */
	if (_terraform_err_tile == INVALID_TILE) _terraform_err_tile = start_tile;

	/* Collect overal cost of the operation. */
	if (ret.had_success) {
		_current_pasting->CollectCost(CommandCost(EXPENSES_CONSTRUCTION, ret.cost), _terraform_err_tile, STR_ERROR_CAN_T_LEVEL_LAND_HERE);
	}

	/* Handle _additional_cash_required */
	if ((_current_pasting->dc_flags & DC_EXEC) && _additional_cash_required > 0) {
		SetDParam(0, _additional_cash_required);
		_current_pasting->CollectError(_terraform_err_tile, STR_ERROR_NOT_ENOUGH_CASH_REQUIRES_CURRENCY, STR_ERROR_CAN_T_LEVEL_LAND_HERE);
	}

	/* Collect last error, if any. */
	if (ret.last_error != STR_NULL) {
		_current_pasting->CollectError(_terraform_err_tile, ret.last_error, STR_ERROR_CAN_T_LEVEL_LAND_HERE);
	}
}

/**
 * Level land (as a part of a pasting process).
 *
 * @param ta Area of tiles corners to level.
 * @param min_height Desired minimal height.
 * @param max_height Desired maximal height.
 */
void LevelPasteLand(const TileArea &ta, uint min_height, uint max_height)
{
	OrthogonalTileIterator iter(ta);
	LevelLandHeightSource height_source(min_height, max_height);
	TerraformPasteTiles(&iter, &height_source);
}

/**
 * Copy and paste heights from one map to another.
 *
 * @param src_area Area to read heights from. It consists of tiles, not of tile corners
 *                  e.g. if you pass a single-tile area then 4 corners will be terraformed.
 * @param copy_paste Parameters of the copy-paste operation.
 */
void CopyPasteHeights(const GenericTileArea &src_area, const CopyPasteParams &copy_paste)
{
	/* include also corners at SW and SE edges */
	GenericTileArea src_corners(src_area.tile, src_area.w + 1, src_area.h + 1);
	TileTransformation to_dst_corner = copy_paste.CornerTransform();

	if (IsMainMapTile(copy_paste.dst_area.tile)) {
		/* paste heights onto the main map */
		TransformationTileIteratorT<false> iter(src_corners, to_dst_corner, &_main_map);
		CopyPasteHeightSource height_source(copy_paste.height_delta);
		TerraformPasteTiles(&iter, &height_source);
	} else {
		/* copy heights into the clipboard */
		TransformationTileIteratorT<true> iter(src_corners, to_dst_corner, MapOf(copy_paste.dst_area.tile));
		for (; IsValidTileIndex(iter); ++iter) {
			SetTileHeight(iter.DstTile(), TileHeight(iter.SrcTile()));
		}
	}
}

/**
 * Terraform multiple tiles.
 *
 * @param iter Tile iterator that iterates over destination tiles.
 * @param height_source Provides target heights for tiles.
 * @param flags Flags for the command.
 * @param available_money Available money to spend.
 * @return The cost of all successfull operations and the last error.
 *
 * @note _terraform_err_tile will be set to the tile where the last error occured
 */
static TerraformTilesResult TerraformTiles(TileIterator *iter, const TileHeightSource *height_source, DoCommandFlag flags, Money available_money)
{
	TerraformTilesResult result = {
		0,       // cost
		false,   // had_success
		STR_NULL // last_error
	};
	TileIndex last_err_tile = INVALID_TILE;

	const Company *c = Company::GetIfValid(_current_company);
	int limit = (c == nullptr ? INT32_MAX : GB(c->terraform_limit, 16, 16));
	if (limit == 0) result.last_error = STR_ERROR_TERRAFORM_LIMIT_REACHED;
	
	for (; *iter != INVALID_TILE && limit > 0; ++(*iter)) {
		int h = height_source->GetTileTargetHeight(*iter);
		TileIndex t = *iter;
		for (int curh = TileHeight(t); curh != h; curh += (curh > h) ? -1 : 1) {
			CommandCost ret = DoCommand(t, SLOPE_N, (curh > h) ? 0 : 1, flags & ~DC_EXEC, CMD_TERRAFORM_LAND);
			if (ret.Failed()) {
				result.last_error = ret.GetErrorMessage();
				last_err_tile = _terraform_err_tile;

				/* Did we reach the limit? */
				if (ret.GetErrorMessage() == STR_ERROR_TERRAFORM_LIMIT_REACHED) limit = 0;
				break;
			}

			if (flags & DC_EXEC) {
				available_money -= ret.GetCost();
				if (available_money < 0) {
					_additional_cash_required = ret.GetCost();
					_terraform_err_tile = t;
					return result;
				}
				DoCommand(t, SLOPE_N, (curh > h) ? 0 : 1, flags, CMD_TERRAFORM_LAND);
			} else {
				/* When we're at the terraform limit we better bail (unneeded) testing as well.
				 * This will probably cause the terraforming cost to be underestimated, but only
				 * when it's near the terraforming limit. Even then, the estimation is
				 * completely off due to it basically counting terraforming double, so it being
				 * cut off earlier might even give a better estimate in some cases. */
				if (--limit <= 0)  {
					result.had_success = true;
					break;
				}
			}

			result.cost += ret.GetCost();
			result.had_success = true;
		}
	}

	if (!result.had_success && result.last_error == STR_NULL) {
		result.last_error = STR_ERROR_ALREADY_LEVELLED;
		last_err_tile = INVALID_TILE;
	}

	_terraform_err_tile = last_err_tile;
	return result;
}
