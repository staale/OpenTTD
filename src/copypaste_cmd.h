/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file copypaste_cmd.h Helper types and functions for copy/paste commands. */

#ifndef COPYPASTE_CMD_H
#define COPYPASTE_CMD_H

#include "bridge.h"
#include "command_type.h"
#include "company_type.h"
#include "newgrf_station.h"
#include "clear_map.h"
#include "rail_type.h"
#include "road_map.h"
#include "station_map.h"
#include "tree_map.h"
#include "water_map.h"
#include "tilearea_func.h"
#include "track_type.h"

#include "table/strings.h"

/** Pasting modifiers. */
enum CopyPasteMode {
	CPM_WITH_RAIL_TRANSPORT  =   1 << 0, ///< copy-paste rail transport infrastructure (newgame only)
	CPM_WITH_ROAD_TRANSPORT  =   1 << 1, ///< copy-paste road transport infrastructure
	CPM_WITH_WATER_TRANSPORT =   1 << 2, ///< copy-paste water transport infrastructure
	CPM_WITH_AIR_TRANSPORT   =   1 << 3, ///< copy-paste air transport infrastructure (newgame only)
	CPM_ALL_TRANSPORT_MASK   = 0xF << 0, ///< bitmask with all transport types
	CPM_SCENEDIT_TRANSPORT_MASK = CPM_WITH_ROAD_TRANSPORT | CPM_WITH_WATER_TRANSPORT, ///< bitmask with transport types available in the scenario editor

	CPM_TERRAFORM_NONE       =   0 << 4, ///< do not alter tile heights
	CPM_TERRAFORM_MINIMAL    =   1 << 4, ///< terraform as less as possible to paste all objects at right heights
	CPM_TERRAFORM_FULL       =   2 << 4, ///< copy-paste all tile heights
	CPM_TERRAFORM_MASK       = 0x3 << 4, ///< bitmask to extract terraforming modes

	CPM_CONVERT_RAILTYPE     =   1 << 6, ///< convert rails to a given rail type (newgame only)
	CPM_MIRROR_SIGNALS       =   1 << 7, ///< mirror signals direction (newgame only)
	CPM_UPGRADE_BRIDGES      =   1 << 8, ///< upgrade bridge types to fastes possible (newgame only)
	CPM_WITH_STATIONS        =   1 << 9, ///< also copy and pase stations and waypoints (newgame only)
	CPM_FLAGS_MASK           = 0xF << 6, ///< bitmask to mask all 'newgame' flags

	CPM_WITH_OBJECTS         =   1 << 0, ///< copy-paste objects e.g. transmitters (scenario editor only)
	CPM_WITH_HOUSES          =   1 << 3, ///< copy-paste town houses (scenario editor only)
	CPM_WITH_INDUSTRIES      =   1 << 6, ///< copy-paste industries (scenario editor only)
	CPM_WITH_RIVERS          =   1 << 7, ///< copy-paste rivers (scenario editor only)
	CPM_WITH_TREES           =   1 << 8, ///< copy-paste trees (scenario editor only)
	CPM_WITH_GROUND          =   1 << 9, ///< copy-paste tile ground (grass, rough grass, desert, farms) (scenario editor only)
	CPM_SCENEDIT_FLAGS_MASK  = 1 << 0 | 1 << 3 | 0xF << 6, ///< bitmask to mask all scenario editor flags

	CPM_NONE             = 0, ///< empty set of modes
	CPM_MASK             = CPM_ALL_TRANSPORT_MASK      | CPM_TERRAFORM_MASK    | CPM_FLAGS_MASK,          ///< all possible bits (newgame)
	CPM_MASK_SCENEDIT    = CPM_SCENEDIT_TRANSPORT_MASK | CPM_TERRAFORM_MASK    | CPM_SCENEDIT_FLAGS_MASK, ///< all possible bits (scenario editor)
	CPM_DEFAULT          = CPM_ALL_TRANSPORT_MASK      | CPM_TERRAFORM_MINIMAL | CPM_WITH_STATIONS,       ///< defult paste mode (newgame)
	CPM_DEFAULT_SCENEDIT = CPM_SCENEDIT_TRANSPORT_MASK | CPM_TERRAFORM_FULL    | CPM_SCENEDIT_FLAGS_MASK, ///< defult paste mode (scenario editor)
	CPM_COPY             = CPM_ALL_TRANSPORT_MASK      | CPM_TERRAFORM_FULL    | CPM_WITH_STATIONS,       ///< mode used when copying to the clipboard (newgame)
	CPM_COPY_SCENEDIT    = CPM_SCENEDIT_TRANSPORT_MASK | CPM_TERRAFORM_FULL    | CPM_SCENEDIT_FLAGS_MASK, ///< mode used when copying to the clipboard (scenario editor)
};
DECLARE_ENUM_AS_BIT_SET(CopyPasteMode)

/** Parameters of a copy/paste command. */
struct CopyPasteParams {
	GenericTileArea src_area;         ///< The area we are copying from
	GenericTileArea dst_area;         ///< The area we are pasting at
	CopyPasteMode mode;               ///< Various flags telling what to copy and how to paste
	RailType railtype;                ///< Convert all rails to a given rail type (only in #CPM_CONVERT_RAILTYPE mode)
	DirTransformation transformation; ///< Transformation to perform on the content while copy-pasting
	int height_delta;                 ///< Amount of units to add to the height of each tile (appropriate terraforming mode must be set e.g. #CPM_TERRAFORM_FULL)

	/**
	 * Transformation between corresponding tiles of the source and the destination area.
	 * @return The transformation.
	 */
	TileTransformation TileTransform() const
	{
		return TransformationBetweenTileAreas(this->src_area, this->dst_area, this->transformation);
	}

	/**
	 * Transformation between corresponding tiles of the source and the destination area.
	 *
	 * This transformation in meant for tile corners e.g. when working with tile heights.
	 *
	 * @return The transformation.
	 */
	TileTransformation CornerTransform() const
	{
		return TransformationBetweenTileAreaCorners(this->src_area, this->dst_area, this->transformation);
	}
};

/**
 * Summary error message for copy/paste command may vary depending on encountered errors.
 * While firing copy/paste command the summary messsage given with CMD_MSG is expected to
 * be STR_COPY_PASTE_ERROR_SUMMARY e.g.
 *
 * <tt>DoCommandP(tile, p1, p2, cmd | CMD_MSG(STR_COPY_PASTE_ERROR_SUMMARY), CcPaste);</tt>
 *
 * STR_COPY_PASTE_ERROR_SUMMARY is "{8:STRING}", real message will be set
 * later through param #8. This constant is the index of the param - 8. */
static const int COPY_PASTE_ERR_SUMMARY_PARAM = 8;

/** Executes commands and gathers results of a paste process. */
struct PasteCmdHelper {
	DoCommandFlag dc_flags;        ///< Flags to use when executing commands
	Money         overal_cost;     ///< Overal cost of currently executed paste command.
	CommandCost   last_result;     ///< Result of the most recent PasteCmdHelper::DoCommand / PasteCmdHelper::CollectCost / PasteCmdHelper::CollectError.
	bool          had_success;     ///< If currently executed paste command had a successful action (at least once).
	StringID      err_summary;     ///< Summary message of the paste error.
	StringID      err_message;     ///< Detailed message of the paste error.
	TileIndex     err_tile;        ///< Tile where the last paste error occured.
	uint64        err_params[COPY_PASTE_ERR_SUMMARY_PARAM]; ///< Parameters for the paste error

	void DoCommand(TileIndex tile, uint32 p1, uint32 p2, uint32 cmd);
	void CollectCost(const CommandCost &cost, TileIndex tile, StringID error_summary = STR_ERROR_CAN_T_PASTE_HERE);
	void CollectError(TileIndex tile, StringID error_message, StringID error_summary = STR_ERROR_CAN_T_PASTE_HERE);
	Money GetAvailableMoney() const;
};

extern PasteCmdHelper *_current_pasting;
extern TileIndex _paste_err_tile;

int CalcCopyPasteHeightDelta(const GenericTileArea &src_area, const GenericTileArea &dst_area, int additional_height);
bool CopyPasteCheckOwnership(GenericTileIndex tile, CompanyID company = INVALID_COMPANY);
WaterClass GetCopyableWater(GenericTileIndex src_tile, CopyPasteMode mode, CompanyID company = INVALID_COMPANY);

void LevelPasteLand(const TileArea &ta, uint min_height, uint max_height);
void CopyPasteHeights(const GenericTileArea &src_area, const CopyPasteParams &copy_paste);

void CopyPastePlaceTracks(GenericTileIndex tile, RailType railtype, TrackBits tracks);
void CopyPastePlaceRoad(GenericTileIndex tile, RoadBits roadbits_road, DisallowedRoadDirections drd, RoadBits roadbits_tram);
void CopyPastePlaceCanal(GenericTileIndex tile);
void CopyPastePlaceRiver(GenericTileIndex tile);
void CopyPastePlaceTunnel(GenericTileIndex tile, DiagDirection dir, uint mid_len, TransportType transport_type, uint rail_road_types);
void CopyPastePlaceBridge(GenericTileIndex tile, DiagDirection dir, uint mid_len, BridgeType bridgetype, TransportType transport_type, uint rail_road_types);
void CopyPastePlaceRailWaypoint(GenericTileIndex tile, StationID sid, Axis axis, RailType rt, StationGfx gfx, StationClassID stat_class, byte stat_type, int specindex, bool adjacent);
void CopyPastePlaceBuoy(GenericTileIndex tile, StationID sid, WaterClass wc);
void CopyPastePlaceClear(GenericTileIndex tile, ClearGround ground, ClearGround raw_ground, uint density);
void CopyPastePlaceTrees(GenericTileIndex tile, TreeType type, uint count, uint growth, TreeGround ground, uint density);
void CopyPastePlaceTropicZone(GenericTileIndex tile, TropicZone tropic_zone);

void AfterCopyingStations(const CopyPasteParams &copy_paste);
void AfterPastingStations(const CopyPasteParams &copy_paste);
void AfterCopyingObjects(const CopyPasteParams &copy_paste);
void AfterCopyingIndustries(const CopyPasteParams &copy_paste);
void AfterPastingIndustries();

#endif /* COPYPASTE_CMD_H */
