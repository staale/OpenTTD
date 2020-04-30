/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file clipboard.cpp Implementaion of clipboard related to both copying and pasting. */

#include "stdafx.h"
#include "core/alloc_func.hpp"
#include "core/mem_func.hpp"
#include "clipboard_func.h"
#include "clipboard_gui.h"
#include "tilearea_type.h"
#include "clear_map.h"
#include "industry_map.h"
#include "station_map.h"
#include "object_map.h"
#include "void_map.h"
#include "newgrf_airport.h"

#include "safeguards.h"

static Map _clipboard_buffers[NUM_CLIPBOARD_BUFFERS];

template <typename T>
struct ClipboardItemsList {
private:
	static T *list[NUM_CLIPBOARD_BUFFERS];

public:
	static T* &Get(Map *buffer)
	{
		return ClipboardItemsList<T>::list[GetClipboardBufferIndex(buffer)];
	}

	static void Set(T *list, Map *buffer)
	{
		assert(list == NULL || !IsClipboardBufferEmpty(buffer));
		T **items = &ClipboardItemsList<T>::Get(buffer);
		ClipboardItemsList<T>::Free(items);
		*items = list;
	}

	/**
	* Free a list of clipboard items (e.g. stations).
	* @param list the list
	*/
	static void Free(T **list)
	{
		for (T *item = *list, *next; item != NULL; item = next) {
			next = item->next;
			delete item;
		}
		*list = NULL;
	}
};

template <typename T>
T *ClipboardItemsList<T>::list[NUM_CLIPBOARD_BUFFERS];

/**
 * Test whether a given #Map is a clipboard buffer.
 * @return if the map a clipboard buffer
 */
bool IsClipboardBuffer(const Map *map)
{
	return (size_t)(map - _clipboard_buffers) < NUM_CLIPBOARD_BUFFERS;
}

/**
 * Get a clipboard buffer by it's index.
 * @param index the index
 * @return the buffer
 *
 * @pre index < NUM_CLIPBOARD_BUFFERS
 */
Map *GetClipboardBuffer(uint index)
{
	assert(index < NUM_CLIPBOARD_BUFFERS);
	return &_clipboard_buffers[index];
}

/**
 * Get the index of a clipboard buffer.
 * @param buffer the buffer
 * @return the index
 *
 * @pre IsClipboardBuffer(buffer)
 */
uint GetClipboardBufferIndex(const Map *buffer)
{
	assert(IsClipboardBuffer(buffer));
	return buffer - _clipboard_buffers;
}

/**
 * Test if a clipboard buffer is empty.
 * @param buffer the buffer
 * @return true iff there is no content in the buffer
 *
 * @pre IsClipboardBuffer(buffer)
 */
bool IsClipboardBufferEmpty(const Map *buffer)
{
	assert(IsClipboardBuffer(buffer));
	return buffer->m == NULL;
};

/**
 * Clear content of a clipboard buffer.
 * @param buffer the buffer
 *
 * @pre IsClipboardBuffer(buffer)
 */
void EmptyClipboardBuffer(Map *buffer)
{
	if (IsClipboardBufferEmpty(buffer)) return;

	ClipboardItemsList<ClipboardStation>::Set(NULL, buffer);
	ClipboardItemsList<ClipboardObject>::Set(NULL, buffer);
	ClipboardItemsList<ClipboardIndustry>::Set(NULL, buffer);

	buffer->size_x = 0;
	buffer->size_y = 0;
	buffer->size = 0;

	free(buffer->m);
	buffer->m = NULL;
	free(buffer->me);
	buffer->me = NULL;
}

/**
 * Allocate space in a clipboard buffer.
 * @param buffer the buffer
 * @param content_size_x X size of the content (excluding MP_VOID tiles on southern borders)
 * @param content_size_y Y size of the content (excluding MP_VOID tiles on southern borders)
 *
 * @pre IsClipboardBuffer(buffer)
 */
void AllocateClipboardBuffer(Map *buffer, uint content_size_x, uint content_size_y)
{
	assert(IsInsideMM(content_size_x, 1, INT_MAX - 1));
	assert(IsInsideMM(content_size_y, 1, INT_MAX - 1));

	EmptyClipboardBuffer(buffer);

	buffer->size_x = content_size_x + 1;
	buffer->size_y = content_size_y + 1;
	buffer->size = buffer->size_x * buffer->size_y;
	buffer->m = CallocT<Tile>(buffer->size);
	buffer->me = CallocT<TileExtended>(buffer->size);

	GENERIC_TILE_AREA_LOOP(iter, GenericTileArea(TileXY(0, 0, buffer), buffer->size_x - 1, buffer->size_y - 1)) {
		/* Set ground to something invalid to preven it from being pasted in scenario editor. */
		SetClearGroundDensity(iter, CLEAR_END, 0);
	}

	GENERIC_TILE_AREA_LOOP(iter, GenericTileArea(TileXY(buffer->size_x - 1, 0, buffer), 1, buffer->size_y)) {
		MakeVoid(iter);
	}
	GENERIC_TILE_AREA_LOOP(iter, GenericTileArea(TileXY(0, buffer->size_y - 1, buffer), buffer->size_x, 1)) {
		MakeVoid(iter);
	}
}

void InitializeClipboard()
{
	for (uint i = 0; i < NUM_CLIPBOARD_BUFFERS; i++) {
		EmptyClipboardBuffer(GetClipboardBuffer(i));
	}
	InitializeClipboardGUI();
}

/**
 * Test if there are any items in a clipboard buffer.
 * @param clipboard The buffer.
 */
template <typename T, typename Tid>
/* static */ bool ClipboardItem<T, Tid>::HasItems(Map *clipboard)
{
	return ClipboardItemsList<T>::Get(clipboard) != NULL;
}

/**
 * Get #ClipboardItem by a given ID.
 * @param id the ID of the item
 * @param clipboard clipboard buffer
 */
template <typename T, typename Tid>
/* static */ T *ClipboardItem<T, Tid>::Get(Tid id, Map *clipboard)
{
	for (T *ret = ClipboardItemsList<T>::Get(clipboard); ret != NULL; ret = ret->next) {
		if (ret->id == id) return ret;
	}
	return NULL;
}

/**
 * Get #ClipboardStation by a given tile.
 * @param tile any tile that belongs to the station
 * @return station pointer or NULL if the tile is not a station
 *
 * @pre IsClipboardBuffer(MapOf(tile))
 */
template <>
/* static */ ClipboardStation *ClipboardItem<ClipboardStation, StationID>::GetByTile(GenericTileIndex tile)
{
	return ClipboardStation::Get(GetStationIndex(tile), MapOf(tile));
}

/**
 * Get #ClipboardObject by a given tile.
 * @param tile any tile that belongs to the object
 * @return object pointer or NULL if the tile is not an object
 *
 * @pre IsClipboardBuffer(MapOf(tile))
 */
template <>
/* static */ ClipboardObject *ClipboardItem<ClipboardObject, ObjectID>::GetByTile(GenericTileIndex tile)
{
	return ClipboardObject::Get(GetObjectIndex(tile), MapOf(tile));
}

/**
 * Get #ClipboardIndustry by a given tile.
 * @param tile any tile that belongs to the industry
 * @return industry pointer or NULL if the tile is not an industry
 *
 * @pre IsClipboardBuffer(MapOf(tile))
 */
template <>
/* static */ ClipboardIndustry *ClipboardItem<ClipboardIndustry, IndustryID>::GetByTile(GenericTileIndex tile)
{
	return ClipboardIndustry::Get(GetIndustryIndex(tile), MapOf(tile));
}

ClipboardStation::ClipboardStation()
{
	this->id             = INVALID_STATION;
	this->airport.tile   = INVALID_TILE_INDEX;
	this->airport.w      = 0;
	this->airport.h      = 0;
	this->airport.type   = AT_INVALID;
	this->airport.layout = 0;
	this->num_specs      = 0;
	this->speclist       = NULL;
	this->next           = NULL;
}

ClipboardStation::~ClipboardStation()
{
	free(this->speclist);
}

ClipboardObject::ClipboardObject()
{
	this->id            = INVALID_OBJECT;
	this->type          = INVALID_OBJECT_TYPE;
	this->location.tile = INVALID_TILE_INDEX;
	this->location.w    = 0;
	this->location.h    = 0;
	this->colour        = 0;
	this->view          = 0;
	this->next          = NULL;
}

ClipboardIndustry::ClipboardIndustry()
{
	this->id              = INVALID_INDUSTRY;
	this->type            = INVALID_INDUSTRYTYPE;
	this->location.tile   = INVALID_TILE_INDEX;
	this->location.w      = 0;
	this->location.h      = 0;
	this->selected_layout = 0;
	this->random          = 0;
	this->next            = NULL;
}

template <typename T>
ClipboardItemsBuilder<T>::~ClipboardItemsBuilder()
{
	ClipboardItemsList<T>::Free(&this->list);
}

/**
 * Find position of item of a given id.
 *
 * @param id id of the item
 * @return position of the found item or position of the list NULL terminator if the item can't be found
 */
template <typename T>
T **ClipboardItemsBuilder<T>::FindItem(typename T::IdType id)
{
	T **ret = &this->list;
	while (*ret != NULL) {
		if ((*ret)->id == id) break;
		ret = &((*ret)->next);
	}
	return ret;
}

/**
 * Find item of a given id or add new if not yet exists.
 *
 * @param id id of the item
 * @return the item
 */
template <typename T>
T *ClipboardItemsBuilder<T>::AddItem(typename T::IdType id)
{
	T **pos = this->FindItem(id);
	if (*pos == NULL) {
		*pos = new T;
		(*pos)->id = id;
	}
	return *pos;
}

/**
 * Finish building and store results.
 * @param buffer clipboard buffer to store the list in
 *
 * @pre IsClipboardBuffer(MapOf(tile))
 */
template <typename T>
void ClipboardItemsBuilder<T>::BuildDone(Map *buffer)
{
	ClipboardItemsList<T>::Set(this->list, buffer);
	this->list = NULL;
}

void ClipboardStationsBuilder::AddSpecToStation(ClipboardStation *st, StationClassID station_class, byte station_type, byte specindex)
{
	assert(specindex != 0 || (station_type == 0 && (station_class == STAT_CLASS_DFLT || station_class == STAT_CLASS_WAYP)));

	if (specindex >= st->num_specs) {
		/* Add "empty" placeholders. */
		st->speclist = ReallocT(st->speclist, specindex + 1);
		for (int i = st->num_specs; i < specindex; i++) {
			st->speclist[i].stat_class = STAT_CLASS_DFLT;
			st->speclist[i].stat_type = 0;
		}
		st->num_specs = specindex + 1;
	} else {
		/* We can override an "empty" placeholder, but if the spec was added before, it shouldn't change. */
		assert((st->speclist[specindex].stat_class == station_class && st->speclist[specindex].stat_type == station_type) ||
				(st->speclist[specindex].stat_class == STAT_CLASS_DFLT && st->speclist[specindex].stat_type == 0));
	}
	st->speclist[specindex].stat_class = station_class;
	st->speclist[specindex].stat_type = station_type;
}

/**
 * Add an airport part.
 *
 * @param sid id of the station
 * @param tile northern tile of the airport
 * @param type airport type
 * @param layout airport layout
 */
void ClipboardStationsBuilder::AddAirportPart(StationID sid, RawTileIndex tile, AirportTypes type, byte layout)
{
	ClipboardStation *st = this->AddItem(sid);

	assert(st->airport.type == AT_INVALID); // single airport per station!
	const AirportSpec *spec = AirportSpec::Get(type);
	st->airport.tile = tile;
	if (spec->rotation[layout] != DIR_E && spec->rotation[layout] != DIR_W) {
		st->airport.w = spec->size_x;
		st->airport.h = spec->size_y;
	} else {
		st->airport.w = spec->size_y;
		st->airport.h = spec->size_x;
	}
	st->airport.type = type;
	st->airport.layout = layout;
}

/**
 * Add an object.
 *
 * @param oid object id
 * @param type object type
 * @param colour object colour
 * @param view object view
 */
void ClipboardObjectsBuilder::Add(ObjectID oid, ObjectType type, const RawTileArea &location, byte colour, byte view)
{
	ClipboardObject **obj = this->FindItem(oid);

	if (*obj == NULL) {
		*obj = new ClipboardObject;
		(*obj)->id       = oid;
		(*obj)->type     = type;
		(*obj)->location = location;
		(*obj)->colour   = colour;
		(*obj)->view     = view;
	} else {
		assert((*obj)->type          == type);
		assert((*obj)->location.tile == location.tile);
		assert((*obj)->location.w    == location.w);
		assert((*obj)->location.h    == location.h);
		assert((*obj)->colour        == colour);
		assert((*obj)->view          == view);
	}
}

/**
 * Add an industry.
 *
 * @param iid industry id
 * @param type industry type
 * @param location location of the industry
 * @param random random bits, see Industry::random
 */
void ClipboardIndustriesBuilder::Add(IndustryID iid, IndustryType type, const RawTileArea &location, byte selected_layout, uint16 random)
{
	ClipboardIndustry **ind = this->FindItem(iid);

	if (*ind == NULL) {
		*ind = new ClipboardIndustry;
		(*ind)->id              = iid;
		(*ind)->type            = type;
		(*ind)->location        = location;
		(*ind)->selected_layout = selected_layout;
		(*ind)->random          = random;
	} else {
		assert((*ind)->type            == type);
		assert((*ind)->location.tile   == location.tile);
		assert((*ind)->location.w      == location.w);
		assert((*ind)->location.h      == location.h);
		assert((*ind)->selected_layout == selected_layout);
		assert((*ind)->random          == random);
	}
}

/* instantiate */
template struct ClipboardItem<ClipboardStation, StationID>;
template struct ClipboardItem<ClipboardObject, ObjectID>;
template struct ClipboardItem<ClipboardIndustry, IndustryID>;
template struct ClipboardItemsList<ClipboardStation>;
template struct ClipboardItemsList<ClipboardObject>;
template struct ClipboardItemsList<ClipboardIndustry>;
template class ClipboardItemsBuilder<ClipboardStation>;
template class ClipboardItemsBuilder<ClipboardObject>;
template class ClipboardItemsBuilder<ClipboardIndustry>;
