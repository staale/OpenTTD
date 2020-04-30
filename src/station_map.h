/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file station_map.h Maps accessors for stations. */

#ifndef STATION_MAP_H
#define STATION_MAP_H

#include "rail_map.h"
#include "road_map.h"
#include "water_map.h"
#include "station_func.h"
#include "rail.h"
#include "road.h"

typedef byte StationGfx; ///< Index of station graphics. @see _station_display_datas

/**
 * Get StationID from a tile
 * @param t Tile to query station ID from
 * @pre IsTileType(t, MP_STATION)
 * @return Station ID of the station at \a t
 */
template <typename Tindex>
static inline StationID GetStationIndex(const Tindex &t)
{
	assert(IsTileType(t, MP_STATION));
	return (StationID)GetTile(t)->m2;
}


static const int GFX_DOCK_BASE_WATER_PART          =  4; ///< The offset for the water parts.
static const int GFX_TRUCK_BUS_DRIVETHROUGH_OFFSET =  4; ///< The offset for the drive through parts.

/**
 * Get the station type of this tile
 * @param t the tile to query
 * @pre IsTileType(t, MP_STATION)
 * @return the station type
 */
template <typename Tindex>
static inline StationType GetStationType(const Tindex &t)
{
	assert(IsTileType(t, MP_STATION));
	return (StationType)GB(GetTileEx(t)->m6, 3, 3);
}

/**
 * Get the road stop type of this tile
 * @param t the tile to query
 * @pre GetStationType(t) == STATION_TRUCK || GetStationType(t) == STATION_BUS
 * @return the road stop type
 */
template <typename Tindex>
static inline RoadStopType GetRoadStopType(const Tindex &t)
{
	assert(GetStationType(t) == STATION_TRUCK || GetStationType(t) == STATION_BUS);
	return GetStationType(t) == STATION_TRUCK ? ROADSTOP_TRUCK : ROADSTOP_BUS;
}

/**
 * Get the station graphics of this tile
 * @param t the tile to query
 * @pre IsTileType(t, MP_STATION)
 * @return the station graphics
 */
template <typename Tindex>
static inline StationGfx GetStationGfx(const Tindex &t)
{
	assert(IsTileType(t, MP_STATION));
	return GetTile(t)->m5;
}

/**
 * Set the station graphics of this tile
 * @param t the tile to update
 * @param gfx the new graphics
 * @pre IsTileType(t, MP_STATION)
 */
template <typename Tindex>
static inline void SetStationGfx(const Tindex &t, StationGfx gfx)
{
	assert(IsTileType(t, MP_STATION));
	GetTile(t)->m5 = gfx;
}

/**
 * Is this station tile a rail station?
 * @param t the tile to get the information from
 * @pre IsTileType(t, MP_STATION)
 * @return true if and only if the tile is a rail station
 */
template <typename Tindex>
static inline bool IsRailStation(const Tindex &t)
{
	return GetStationType(t) == STATION_RAIL;
}

/**
 * Is this tile a station tile and a rail station?
 * @param t the tile to get the information from
 * @return true if and only if the tile is a rail station
 */
template <typename Tindex>
static inline bool IsRailStationTile(const Tindex &t)
{
	return IsTileType(t, MP_STATION) && IsRailStation(t);
}

/**
 * Is this station tile a rail waypoint?
 * @param t the tile to get the information from
 * @pre IsTileType(t, MP_STATION)
 * @return true if and only if the tile is a rail waypoint
 */
template <typename Tindex>
static inline bool IsRailWaypoint(const Tindex &t)
{
	return GetStationType(t) == STATION_WAYPOINT;
}

/**
 * Is this tile a station tile and a rail waypoint?
 * @param t the tile to get the information from
 * @return true if and only if the tile is a rail waypoint
 */
template <typename Tindex>
static inline bool IsRailWaypointTile(const Tindex &t)
{
	return IsTileType(t, MP_STATION) && IsRailWaypoint(t);
}

/**
 * Has this station tile a rail? In other words, is this station
 * tile a rail station or rail waypoint?
 * @param t the tile to check
 * @pre IsTileType(t, MP_STATION)
 * @return true if and only if the tile has rail
 */
template <typename Tindex>
static inline bool HasStationRail(const Tindex &t)
{
	return IsRailStation(t) || IsRailWaypoint(t);
}

/**
 * Has this station tile a rail? In other words, is this station
 * tile a rail station or rail waypoint?
 * @param t the tile to check
 * @return true if and only if the tile is a station tile and has rail
 */
template <typename Tindex>
static inline bool HasStationTileRail(const Tindex &t)
{
	return IsTileType(t, MP_STATION) && HasStationRail(t);
}

/**
 * Is this station tile an airport?
 * @param t the tile to get the information from
 * @pre IsTileType(t, MP_STATION)
 * @return true if and only if the tile is an airport
 */
template <typename Tindex>
static inline bool IsAirport(const Tindex &t)
{
	return GetStationType(t) == STATION_AIRPORT;
}

/**
 * Is this tile a station tile and an airport tile?
 * @param t the tile to get the information from
 * @return true if and only if the tile is an airport
 */
template <typename Tindex>
static inline bool IsAirportTile(const Tindex &t)
{
	return IsTileType(t, MP_STATION) && IsAirport(t);
}

bool IsHangar(TileIndex t);

/**
 * Is the station at \a t a truck stop?
 * @param t Tile to check
 * @pre IsTileType(t, MP_STATION)
 * @return \c true if station is a truck stop, \c false otherwise
 */
template <typename Tindex>
static inline bool IsTruckStop(const Tindex &t)
{
	return GetStationType(t) == STATION_TRUCK;
}

/**
 * Is the station at \a t a bus stop?
 * @param t Tile to check
 * @pre IsTileType(t, MP_STATION)
 * @return \c true if station is a bus stop, \c false otherwise
 */
template <typename Tindex>
static inline bool IsBusStop(const Tindex &t)
{
	return GetStationType(t) == STATION_BUS;
}

/**
 * Is the station at \a t a road station?
 * @param t Tile to check
 * @pre IsTileType(t, MP_STATION)
 * @return \c true if station at the tile is a bus top or a truck stop, \c false otherwise
 */
template <typename Tindex>
static inline bool IsRoadStop(const Tindex &t)
{
	assert(IsTileType(t, MP_STATION));
	return IsTruckStop(t) || IsBusStop(t);
}

/**
 * Is tile \a t a road stop station?
 * @param t Tile to check
 * @return \c true if the tile is a station tile and a road stop
 */
template <typename Tindex>
static inline bool IsRoadStopTile(const Tindex &t)
{
	return IsTileType(t, MP_STATION) && IsRoadStop(t);
}

/**
 * Is tile \a t a standard (non-drive through) road stop station?
 * @param t Tile to check
 * @return \c true if the tile is a station tile and a standard road stop
 */
template <typename Tindex>
static inline bool IsStandardRoadStopTile(const Tindex &t)
{
	return IsRoadStopTile(t) && GetStationGfx(t) < GFX_TRUCK_BUS_DRIVETHROUGH_OFFSET;
}

/**
 * Is tile \a t a drive through road stop station?
 * @param t Tile to check
 * @return \c true if the tile is a station tile and a drive through road stop
 */
template <typename Tindex>
static inline bool IsDriveThroughStopTile(const Tindex &t)
{
	return IsRoadStopTile(t) && GetStationGfx(t) >= GFX_TRUCK_BUS_DRIVETHROUGH_OFFSET;
}

/**
 * Get the station graphics of this airport tile
 * @param t the tile to query
 * @pre IsAirport(t)
 * @return the station graphics
 */
static inline StationGfx GetAirportGfx(TileIndex t)
{
	assert(IsAirport(t));
	extern StationGfx GetTranslatedAirportTileID(StationGfx gfx);
	return GetTranslatedAirportTileID(GetStationGfx(t));
}

/**
 * Gets the direction the road stop entrance points towards.
 * @param t the tile of the road stop
 * @pre IsRoadStopTile(t)
 * @return the direction of the entrance
 */
template <typename Tindex>
static inline DiagDirection GetRoadStopDir(const Tindex &t)
{
	StationGfx gfx = GetStationGfx(t);
	assert(IsRoadStopTile(t));
	if (gfx < GFX_TRUCK_BUS_DRIVETHROUGH_OFFSET) {
		return (DiagDirection)(gfx);
	} else {
		return (DiagDirection)(gfx - GFX_TRUCK_BUS_DRIVETHROUGH_OFFSET);
	}
}

/**
 * Is tile \a t part of an oilrig?
 * @param t Tile to check
 * @pre IsTileType(t, MP_STATION)
 * @return \c true if the tile is an oilrig tile
 */
static inline bool IsOilRig(TileIndex t)
{
	return GetStationType(t) == STATION_OILRIG;
}

/**
 * Is tile \a t a dock tile?
 * @param t Tile to check
 * @pre IsTileType(t, MP_STATION)
 * @return \c true if the tile is a dock
 */
template <typename Tindex>
static inline bool IsDock(const Tindex &t)
{
	return GetStationType(t) == STATION_DOCK;
}

/**
 * Is tile \a t a dock tile?
 * @param t Tile to check
 * @return \c true if the tile is a dock
 */
template <typename Tindex>
static inline bool IsDockTile(const Tindex &t)
{
	return IsTileType(t, MP_STATION) && GetStationType(t) == STATION_DOCK;
}

/**
 * Is tile \a t a buoy tile?
 * @param t Tile to check
 * @pre IsTileType(t, MP_STATION)
 * @return \c true if the tile is a buoy
 */
template <typename Tindex>
static inline bool IsBuoy(const Tindex &t)
{
	return GetStationType(t) == STATION_BUOY;
}

/**
 * Is tile \a t a buoy tile?
 * @param t Tile to check
 * @return \c true if the tile is a buoy
 */
template <typename Tindex>
static inline bool IsBuoyTile(const Tindex &t)
{
	return IsTileType(t, MP_STATION) && IsBuoy(t);
}

/**
 * Is tile \a t an hangar tile?
 * @param t Tile to check
 * @return \c true if the tile is an hangar
 */
static inline bool IsHangarTile(TileIndex t)
{
	return IsTileType(t, MP_STATION) && IsHangar(t);
}

/**
 * Get the rail direction of a rail station.
 * @param t Tile to query
 * @pre HasStationRail(t)
 * @return The direction of the rails on tile \a t.
 */
template <typename Tindex>
static inline Axis GetRailStationAxis(const Tindex &t)
{
	assert(HasStationRail(t));
	return HasBit(GetStationGfx(t), 0) ? AXIS_Y : AXIS_X;
}

/**
 * Get the rail track of a rail station tile.
 * @param t Tile to query
 * @pre HasStationRail(t)
 * @return The rail track of the rails on tile \a t.
 */
template <typename Tindex>
static inline Track GetRailStationTrack(const Tindex &t)
{
	return AxisToTrack(GetRailStationAxis(t));
}

/**
 * Get the trackbits of a rail station tile.
 * @param t Tile to query
 * @pre HasStationRail(t)
 * @return The trackbits of the rails on tile \a t.
 */
template <typename Tindex>
static inline TrackBits GetRailStationTrackBits(const Tindex &t)
{
	return AxisToTrackBits(GetRailStationAxis(t));
}

/**
 * Check if a tile is a valid continuation to a railstation tile.
 * The tile \a test_tile is a valid continuation to \a station_tile, if all of the following are true:
 * \li \a test_tile is a rail station tile
 * \li the railtype of \a test_tile is compatible with the railtype of \a station_tile
 * \li the tracks on \a test_tile and \a station_tile are in the same direction
 * \li both tiles belong to the same station
 * \li \a test_tile is not blocked (@see IsStationTileBlocked)
 * @param test_tile Tile to test
 * @param station_tile Station tile to compare with
 * @pre IsRailStationTile(station_tile)
 * @return true if the two tiles are compatible
 */
static inline bool IsCompatibleTrainStationTile(TileIndex test_tile, TileIndex station_tile)
{
	assert(IsRailStationTile(station_tile));
	return IsRailStationTile(test_tile) && IsCompatibleRail(GetRailType(test_tile), GetRailType(station_tile)) &&
			GetRailStationAxis(test_tile) == GetRailStationAxis(station_tile) &&
			GetStationIndex(test_tile) == GetStationIndex(station_tile) &&
			!IsStationTileBlocked(test_tile);
}

/**
 * Get the reservation state of the rail station
 * @pre HasStationRail(t)
 * @param t the station tile
 * @return reservation state
 */
static inline bool HasStationReservation(TileIndex t)
{
	assert(HasStationRail(t));
	return HasBit(GetTileEx(t)->m6, 2);
}

/**
 * Set the reservation state of the rail station
 * @pre HasStationRail(t)
 * @param t the station tile
 * @param b the reservation state
 */
static inline void SetRailStationReservation(TileIndex t, bool b)
{
	assert(HasStationRail(t));
	SB(GetTileEx(t)->m6, 2, 1, b ? 1 : 0);
}

/**
 * Get the reserved track bits for a waypoint
 * @pre HasStationRail(t)
 * @param t the tile
 * @return reserved track bits
 */
static inline TrackBits GetStationReservationTrackBits(TileIndex t)
{
	return HasStationReservation(t) ? GetRailStationTrackBits(t) : TRACK_BIT_NONE;
}

/**
 * Test whether a given water dock tile is the land part of the dock
 * @param t the water dock tile
 * @return if the given tile is the land part of a dock
 * @pre IsDockTile(t)
 */
template <typename Tindex>
static inline bool IsLandDockSection(const Tindex &t)
{
	assert(IsDockTile(t));
	return GetStationGfx(t) < GFX_DOCK_BASE_WATER_PART;
}

/**
 * Get the direction of a dock.
 * @param t Tile to query
 * @pre IsLandDockSection(t)
 * @pre \a t is the land part of the dock
 * @return The direction of the dock on tile \a t.
 */
template <typename Tindex>
static inline DiagDirection GetDockDirection(const Tindex &t)
{
	assert(IsLandDockSection(t));
	return (DiagDirection)GetStationGfx(t);
}

/**
 * Get the other tile of a dock.
 * @param t Tile to query
 * @pre IsDockTile(t)
 * @return The other tile of the dock.
 */
template <typename Tindex>
static inline Tindex GetOtherDockTile(const Tindex &t)
{
	TileIndexDiff delta = ToTileIndexDiff(TileIndexDiffCByDiagDir(AxisToDiagDir((Axis)(GetStationGfx(t) & 0x1))), MapOf(t));
	return IsDockTile(t + delta) ? t + delta : t - delta;
}

/**
 * Get the tileoffset from this tile a ship should target to get to this dock.
 * @param t Tile to query
 * @pre IsTileType(t, MP_STATION)
 * @pre IsBuoy(t) || IsOilRig(t) || IsDock(t)
 * @return The offset from this tile that should be used as destination for ships.
 */
static inline TileIndexDiffC GetDockOffset(TileIndex t)
{
	static const TileIndexDiffC buoy_offset = {0, 0};
	static const TileIndexDiffC oilrig_offset = {2, 0};
	static const TileIndexDiffC dock_offset[DIAGDIR_END] = {
		{-2,  0},
		{ 0,  2},
		{ 2,  0},
		{ 0, -2},
	};
	assert(IsTileType(t, MP_STATION));

	if (IsBuoy(t)) return buoy_offset;
	if (IsOilRig(t)) return oilrig_offset;

	assert(IsDock(t));

	return dock_offset[GetDockDirection(t)];
}

/**
 * Is there a custom rail station spec on this tile?
 * @param t Tile to query
 * @pre HasStationTileRail(t)
 * @return True if this station is part of a newgrf station.
 */
template <typename Tindex>
static inline bool IsCustomStationSpecIndex(const Tindex &t)
{
	assert(HasStationTileRail(t));
	return GetTile(t)->m4 != 0;
}

/**
 * Set the custom station spec for this tile.
 * @param t Tile to set the stationspec of.
 * @param specindex The new spec.
 * @pre HasStationTileRail(t)
 */
template <typename Tindex>
static inline void SetCustomStationSpecIndex(const Tindex &t, byte specindex)
{
	assert(HasStationTileRail(t));
	GetTile(t)->m4 = specindex;
}

/**
 * Get the custom station spec for this tile.
 * @param t Tile to query
 * @pre HasStationTileRail(t)
 * @return The custom station spec of this tile.
 */
template <typename Tindex>
static inline uint GetCustomStationSpecIndex(const Tindex &t)
{
	assert(HasStationTileRail(t));
	return GetTile(t)->m4;
}

/**
 * Set the random bits for a station tile.
 * @param t Tile to set random bits for.
 * @param random_bits The random bits.
 * @pre IsTileType(t, MP_STATION)
 */
static inline void SetStationTileRandomBits(TileIndex t, byte random_bits)
{
	assert(IsTileType(t, MP_STATION));
	SB(GetTile(t)->m3, 4, 4, random_bits);
}

/**
 * Get the random bits of a station tile.
 * @param t Tile to query
 * @pre IsTileType(t, MP_STATION)
 * @return The random bits for this station tile.
 */
static inline byte GetStationTileRandomBits(TileIndex t)
{
	assert(IsTileType(t, MP_STATION));
	return GB(GetTile(t)->m3, 4, 4);
}

/**
 * Make the given tile a station tile.
 * @param t the tile to make a station tile
 * @param o the owner of the station
 * @param sid the station to which this tile belongs
 * @param st the type this station tile
 * @param section the StationGfx to be used for this tile
 * @param wc The water class of the station
 */
template <typename Tindex>
static inline void MakeStation(const Tindex &t, Owner o, StationID sid, StationType st, byte section, WaterClass wc = WATER_CLASS_INVALID)
{
	SetTileType(t, MP_STATION);
	SetTileOwner(t, o);
	SetWaterClass(t, wc);
	SetDockingTile(t, false);
	GetTile(t)->m2 = sid;
	GetTile(t)->m3 = 0;
	GetTile(t)->m4 = 0;
	GetTile(t)->m5 = section;
	SB(GetTileEx(t)->m6, 2, 1, 0);
	SB(GetTileEx(t)->m6, 3, 3, st);
	GetTileEx(t)->m7 = 0;
	GetTileEx(t)->m8 = 0;
}

/**
 * Make the given tile a rail station tile.
 * @param t the tile to make a rail station tile
 * @param o the owner of the station
 * @param sid the station to which this tile belongs
 * @param a the axis of this tile
 * @param section the StationGfx to be used for this tile
 * @param rt the railtype of this tile
 */
template <typename Tindex>
static inline void MakeRailStation(const Tindex &t, Owner o, StationID sid, Axis a, byte section, RailType rt)
{
	MakeStation(t, o, sid, STATION_RAIL, section + a);
	SetRailType(t, rt);
	if (IsMainMapTile(t)) SetRailStationReservation(AsMainMapTile(t), false);
}

/**
 * Make the given tile a rail waypoint tile.
 * @param t the tile to make a rail waypoint
 * @param o the owner of the waypoint
 * @param sid the waypoint to which this tile belongs
 * @param a the axis of this tile
 * @param section the StationGfx to be used for this tile
 * @param rt the railtype of this tile
 */
template <typename Tindex>
static inline void MakeRailWaypoint(const Tindex &t, Owner o, StationID sid, Axis a, byte section, RailType rt)
{
	MakeStation(t, o, sid, STATION_WAYPOINT, section + a);
	SetRailType(t, rt);
	if (IsMainMapTile(t)) SetRailStationReservation(AsMainMapTile(t), false);
}

/**
 * Make the given tile a roadstop tile.
 * @param t the tile to make a roadstop
 * @param o the owner of the roadstop
 * @param sid the station to which this tile belongs
 * @param rst the type of roadstop to make this tile
 * @param road_rt the road roadtype on this tile
 * @param tram_rt the tram roadtype on this tile
 * @param d the direction of the roadstop
 */
template <typename Tindex>
static inline void MakeRoadStop(const Tindex &t, Owner o, StationID sid, RoadStopType rst, RoadType road_rt, RoadType tram_rt, DiagDirection d)
{
	MakeStation(t, o, sid, (rst == ROADSTOP_BUS ? STATION_BUS : STATION_TRUCK), d);
	SetRoadTypes(t, road_rt, tram_rt);
	SetRoadOwner(t, RTT_ROAD, o);
	SetRoadOwner(t, RTT_TRAM, o);
}

/**
 * Make the given tile a drivethrough roadstop tile.
 * @param t the tile to make a roadstop
 * @param station the owner of the roadstop
 * @param road the owner of the road
 * @param tram the owner of the tram
 * @param sid the station to which this tile belongs
 * @param rst the type of roadstop to make this tile
 * @param road_rt the road roadtype on this tile
 * @param tram_rt the tram roadtype on this tile
 * @param a the direction of the roadstop
 */
template <typename Tindex>
static inline void MakeDriveThroughRoadStop(const Tindex &t, Owner station, Owner road, Owner tram, StationID sid, RoadStopType rst, RoadType road_rt, RoadType tram_rt, Axis a)
{
	MakeStation(t, station, sid, (rst == ROADSTOP_BUS ? STATION_BUS : STATION_TRUCK), GFX_TRUCK_BUS_DRIVETHROUGH_OFFSET + a);
	SetRoadTypes(t, road_rt, tram_rt);
	SetRoadOwner(t, RTT_ROAD, road);
	SetRoadOwner(t, RTT_TRAM, tram);
}

/**
 * Make the given tile an airport tile.
 * @param t the tile to make a airport
 * @param o the owner of the airport
 * @param sid the station to which this tile belongs
 * @param section the StationGfx to be used for this tile
 * @param wc the type of water on this tile
 */
template <typename Tindex>
static inline void MakeAirport(const Tindex &t, Owner o, StationID sid, byte section, WaterClass wc)
{
	MakeStation(t, o, sid, STATION_AIRPORT, section, wc);
}

/**
 * Make the given tile a buoy tile.
 * @param t the tile to make a buoy
 * @param sid the station to which this tile belongs
 * @param wc the type of water on this tile
 */
template <typename Tindex>
static inline void MakeBuoy(const Tindex &t, StationID sid, WaterClass wc)
{
	/* Make the owner of the buoy tile the same as the current owner of the
	 * water tile. In this way, we can reset the owner of the water to its
	 * original state when the buoy gets removed. */
	MakeStation(t, GetTileOwner(t), sid, STATION_BUOY, 0, wc);
}

/**
 * Make the given tile a dock tile.
 * @param t the tile to make a dock
 * @param o the owner of the dock
 * @param sid the station to which this tile belongs
 * @param d the direction of the dock
 * @param wc the type of water on this tile
 */
template <typename Tindex>
static inline void MakeDock(const Tindex &t, Owner o, StationID sid, DiagDirection d, WaterClass wc)
{
	MakeStation(t, o, sid, STATION_DOCK, d);
	MakeStation(TileAddByDiagDir(t, d), o, sid, STATION_DOCK, GFX_DOCK_BASE_WATER_PART + DiagDirToAxis(d), wc);
}

/**
 * Make the given tile an oilrig tile.
 * @param t the tile to make an oilrig
 * @param sid the station to which this tile belongs
 * @param wc the type of water on this tile
 */
static inline void MakeOilrig(TileIndex t, StationID sid, WaterClass wc)
{
	MakeStation(t, OWNER_NONE, sid, STATION_OILRIG, 0, wc);
}

#endif /* STATION_MAP_H */
