/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file clipboard_type.h Types related to the clipboard. */

#ifndef CLIPBOARD_TYPE_H
#define CLIPBOARD_TYPE_H

#include "airport.h"
#include "newgrf_station.h"
#include "station_type.h"
#include "object_type.h"
#include "tilearea_type.h"

template <typename T, typename Tid>
struct ClipboardItem
{
public:
	typedef Tid IdType;

public:
	virtual ~ClipboardItem()
	{ }

	IdType id; ///< ID
	T *next;   ///< "Next" pointer to make a linked list

	static bool HasItems(Map *clipboard);
	static T *Get(IdType id, Map *clipboard);
	static T *GetByTile(GenericTileIndex tile);
};

struct ClipboardStation : ClipboardItem<ClipboardStation, StationID> {
	struct Spec {
		StationClassID stat_class;
		byte stat_type;
	};

	struct AirportPart : RawTileArea {
		AirportTypes type; ///< Airport type
		byte layout;                              ///< Airport layout
	};

	AirportPart airport;   ///< Airport details
	uint8       num_specs; ///< Number of specs in the speclist
	Spec       *speclist;  ///< List of station specs of this station

	ClipboardStation();
	~ClipboardStation();
};

struct ClipboardObject : ClipboardItem<ClipboardObject, ObjectID> {
	ObjectType type;      ///< Type of the object
	RawTileArea location; ///< Location in tile array
	byte colour;          ///< Colour of the object, for display purpose
	byte view;            ///< The view setting for this object

	ClipboardObject();
};

struct ClipboardIndustry : ClipboardItem<ClipboardIndustry, IndustryID> {
	IndustryType type;    ///< Type of the industry
	RawTileArea location; ///< Location in tile array
	byte selected_layout; ///< Layout, see Industry::selected_layout
	uint16 random;        ///< Random bits, see Industry::random

	ClipboardIndustry();
};

#endif /* CLIPBOARD_TYPE_H */
