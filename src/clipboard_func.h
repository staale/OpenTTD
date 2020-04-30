/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file clipboard_func.h Functions related to the clipboad. */

#ifndef CLIPBOARD_FUNC_H
#define CLIPBOARD_FUNC_H

#include "clipboard_type.h"


static const uint NUM_CLIPBOARD_BUFFERS = 5; ///< Total amount of clipboard buffers

/** Helper class to build a object list while copying to the clipboard. */
template <typename T>
class ClipboardItemsBuilder {
protected:
	typedef typename T::IdType IdType;

	T *list; ///< the list of objects

	T **FindItem(IdType id);
	T *AddItem(IdType id);

public:
	ClipboardItemsBuilder() : list(NULL)
	{ }

	virtual ~ClipboardItemsBuilder();

	void BuildDone(Map *buffer);
};

/** Helper class to build a station list while copying to the clipboard. */
class ClipboardStationsBuilder : public ClipboardItemsBuilder<ClipboardStation> {
protected:
	void AddSpecToStation(ClipboardStation *st, StationClassID station_class, byte station_type, byte specindex);

public:
	/**
	 * Add a "simple" station part (bus/truck/dock/buoy).
	 * @param sid id of the station
	 */
	inline void AddPart(StationID sid)
	{
		this->AddItem(sid);
	}

	/**
	 * Add a rail station/waypoint part.
	 * @param sid id of the station
	 * @param station_class custom station class
	 * @param station_type type within the custom station class
	 * @param specindex index of the given station spec in the list of specs of this station (aka custom station spec index)
	 */
	inline void AddRailPart(StationID sid, StationClassID station_class, byte station_type, byte specindex)
	{
		this->AddSpecToStation(this->AddItem(sid), station_class, station_type, specindex);
	}

	void AddAirportPart(StationID sid, RawTileIndex tile, AirportTypes type, byte layout);
};

/** Helper class to build a object list while copying to the clipboard. */
class ClipboardObjectsBuilder : public ClipboardItemsBuilder<ClipboardObject> {
public:
	void Add(ObjectID oid, ObjectType type, const RawTileArea &location, byte colour, byte view);
};

/** Helper class to build a industry list while copying to the clipboard. */
class ClipboardIndustriesBuilder : public ClipboardItemsBuilder<ClipboardIndustry> {
public:
	void Add(IndustryID iid, IndustryType type, const RawTileArea &location, byte layout, uint16 random);
};

bool IsClipboardBuffer(const Map *map);
Map *GetClipboardBuffer(uint index);
uint GetClipboardBufferIndex(const Map *buffer);
void AllocateClipboardBuffer(Map *buffer, uint content_size_x, uint content_size_y);
bool IsClipboardBufferEmpty(const Map *buffer);
void EmptyClipboardBuffer(Map *buffer);

#endif /* CLIPBOARD_FUNC_H */
