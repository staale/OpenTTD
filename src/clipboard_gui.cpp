/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file clipboard_gui.cpp GUIs related to the clipboard. */

#include "stdafx.h"
#include "core/geometry_func.hpp"
#include "network/network.h"
#include "widgets/clipboard_widget.h"
#include "clipboard_func.h"
#include "clipboard_gui.h"
#include "command_func.h"
#include "company_func.h"
#include "company_base.h"
#include "copypaste_cmd.h"
#include "direction_func.h"
#include "error.h"
#include "gfx_func.h"
#include "gui.h"
#include "hotkeys.h"
#include "rail.h"
#include "rail_gui.h"
#include "rail_map.h"
#include "road_map.h"
#include "slope_func.h"
#include "sound_func.h"
#include "station_map.h"
#include "strings_func.h"
#include "terraform_gui.h"
#include "tilearea_func.h"
#include "tilehighlight_func.h"
#include "track_func.h"
#include "tunnelbridge_map.h"
#include "viewport_func.h"
#include "window_gui.h"
#include "window_func.h"

#include "table/sprites.h"
#include "table/strings.h"

#include "safeguards.h"

static const int CLIPBOARD_ADDITIONAL_HEIGHT_MAX = 15;
static const int CLIPBOARD_ADDITIONAL_HEIGHT_MIN = -15;
static const uint NUM_USER_CLIPBOARDS = NUM_CLIPBOARD_BUFFERS - 1; ///< Number of clipboards available

/** Clipboard parameters. */
struct ClipboardProps {
	TileArea          copy_area;               ///< Area on the main map selected as a source of copy operation
	TileArea          paste_area;              ///< Area on the main map selected as a destination for paste operation
	CopyPasteMode     mode;                    ///< Copy/paste mode selected in the clipboard toolbar
	RailType          railtype;                ///< #Railtype to convert to
	DirTransformation transformation;          ///< Rotation/reflection to apply when pasting, selected in the clipboard toolbar
	int               additional_height_delta; ///< Additional amount of tile heights to add
};

ClipboardProps _clipboard_props[NUM_USER_CLIPBOARDS]; ///< Clipboard parameters selected via GUI
ClipboardProps *_current_clipboard;                   ///< Currently selected clipboard
static TileArea _clipboard_paste_area;                ///< Area on the main map selected as destination for a paste operation

/** Reset clipboard gui settings. */
void InitializeClipboardGUI()
{
	for (uint i = 0; i < NUM_USER_CLIPBOARDS; i++) {
		_clipboard_props[i].copy_area               = TileArea(INVALID_TILE, 0, 0);
		_clipboard_props[i].mode                    = (_game_mode == GM_EDITOR) ? CPM_DEFAULT_SCENEDIT : CPM_DEFAULT;
		_clipboard_props[i].railtype                = INVALID_RAILTYPE;
		_clipboard_props[i].transformation          = DTR_IDENTITY;
		_clipboard_props[i].additional_height_delta = 0;
	}

	_clipboard_paste_area = TileArea(INVALID_TILE, 0, 0);
	_current_clipboard = &_clipboard_props[0];
}

/**
 * Whether the copy/paste operations are performed with the clipboard buffer, or instantantly.
 *
 * If true, clipboard buffer is on. Each "copy" user action moves selected area to the clipboard
 * (to the buffer) and each "paste" tries to reproduce contents of the clipboard on the main map.
 *
 * If false, clipboard buffer is off. "copy" user action just selects area and
 * "paste" makes an instant copy&paste from the selected area to pointed place.
 *
 * @return whether the clipboard buffer is available for local company
 */
static inline bool IsClipboardBufferOn() { return !_networking; }

static inline Map *GetCurrentClipboardBuffer()
{
	return IsClipboardBufferOn() ? GetClipboardBuffer(_current_clipboard - _clipboard_props) : NULL;
}

static inline bool IsClipboardCopyAreaSelected()
{
	return _current_clipboard->copy_area.tile != INVALID_TILE;
}

static inline bool IsClipboardPasteSourceSet()
{
	return IsClipboardBufferOn() ? !IsClipboardBufferEmpty(GetCurrentClipboardBuffer()) : IsClipboardCopyAreaSelected();
}

static void ClipboardRecalcPasteAreaSize()
{
	assert(IsClipboardPasteSourceSet());

	Dimension size;
	if (IsClipboardBufferOn()) {
		size.width = GetCurrentClipboardBuffer()->size_x - 1;
		size.height = GetCurrentClipboardBuffer()->size_y - 1;
	} else {
		size.width = _current_clipboard->copy_area.w;
		size.height = _current_clipboard->copy_area.h;
	}
	size = TransformDimension(size, _current_clipboard->transformation);

	_clipboard_paste_area.w = size.width;
	_clipboard_paste_area.h = size.height;
}

void CcPaste(const CommandCost &result, TileIndex tile, uint32 p1, uint32 p2, uint32 cmd)
{
	if (_paste_err_tile != INVALID_TILE) SetRedErrorSquare(_paste_err_tile);

	if (result.Succeeded()) {
		if (_settings_client.sound.confirm) SndPlayTileFx(SND_1F_SPLAT_OTHER, tile);
		if (!_settings_client.gui.persistent_buildingtools) ResetObjectToPlace();
	}
}

void GetTilePastePreview(TileIndex tile, TilePastePreview *ret)
{
	_clipboard_paste_area.tile = TileVirtXY(_thd.pos.x, _thd.pos.y);

	extern bool TestRailTileCopyability(GenericTileIndex tile, CopyPasteMode mode, CompanyID company, TileContentPastePreview *preview);
	extern bool TestRoadTileCopyability(GenericTileIndex tile, CopyPasteMode mode, CompanyID company, TileContentPastePreview *preview);
	extern bool TestWaterTileCopyability(GenericTileIndex tile, const GenericTileArea &src_area, CopyPasteMode mode, GenericTileArea *object_rect, CompanyID company, TileContentPastePreview *preview);
	extern bool TestTunnelBridgeTileCopyability(GenericTileIndex tile, const GenericTileArea &src_area, CopyPasteMode mode, GenericTileIndex *other_end, CompanyID company, TileContentPastePreview *preview);
	extern bool TestStationTileCopyability(GenericTileIndex tile, const GenericTileArea &src_area, CopyPasteMode mode, GenericTileArea *station_part_area, CompanyID company, TileContentPastePreview *preview);
	extern bool TestObjectTileCopyability(GenericTileIndex tile, const GenericTileArea &src_area, CopyPasteMode mode, GenericTileArea *object_rect, TileContentPastePreview *preview);
	extern bool TestTownTileCopyability(GenericTileIndex tile, const GenericTileArea &src_area, CopyPasteMode mode, GenericTileArea *house_rect, TileContentPastePreview *preview);
	extern bool TestIndustryTileCopyability(GenericTileIndex tile, const GenericTileArea &src_area, CopyPasteMode mode, GenericTileArea *industry_rect, TileContentPastePreview *preview);

	MemSetT(ret, 0);

	Map *clipboard = GetCurrentClipboardBuffer();

	/* the area we are copying from */
	GenericTileArea src_area = IsClipboardBufferOn() ?
			GenericTileArea(TileXY(0, 0, clipboard), clipboard->size_x - 1, clipboard->size_y - 1) :
			GenericTileArea(_current_clipboard->copy_area);

	/* calculate the height difference between areas */
	int height_delta = CalcCopyPasteHeightDelta(src_area, _clipboard_paste_area, _current_clipboard->additional_height_delta);

	if (_clipboard_paste_area.Contains(tile)) {
		TileTransformation to_src_tile = TransformationBetweenTileAreas(_clipboard_paste_area, src_area, InvertDirTransform(_current_clipboard->transformation));
		/* source tile of the 'tile' */
		GenericTileIndex src_tile = TransformTile(tile, to_src_tile, MapOf(src_area.tile));

		bool has_preview = false;
		switch(GetTileType(src_tile)) {
			case MP_RAILWAY:      has_preview = TestRailTileCopyability(src_tile, _current_clipboard->mode, _local_company, ret); break;
			case MP_ROAD:         has_preview = TestRoadTileCopyability(src_tile, _current_clipboard->mode, _local_company, ret); break;
			case MP_STATION:      has_preview = TestStationTileCopyability(src_tile, src_area, _current_clipboard->mode, NULL, _local_company, ret); break;
			case MP_WATER:        has_preview = TestWaterTileCopyability(src_tile, src_area, _current_clipboard->mode, NULL, _local_company, ret); break;
			case MP_TUNNELBRIDGE: has_preview = TestTunnelBridgeTileCopyability(src_tile, src_area, _current_clipboard->mode, NULL, _local_company, ret); break;
			case MP_OBJECT:       has_preview = TestObjectTileCopyability(src_tile, src_area, _current_clipboard->mode, NULL, ret); break;
			case MP_HOUSE:        has_preview = TestTownTileCopyability(src_tile, src_area, _current_clipboard->mode, NULL, ret); break;
			case MP_INDUSTRY:     has_preview = TestIndustryTileCopyability(src_tile, src_area, _current_clipboard->mode, NULL, ret); break;
			default:              break;
		}

		if (has_preview) ret->highlight_track_bits = TransformTrackBits(ret->highlight_track_bits, _current_clipboard->transformation);
	} else {
		assert(TileArea(_clipboard_paste_area.tile, _clipboard_paste_area.w + 1, _clipboard_paste_area.h + 1).Contains(tile));
	}

	TileTransformation to_src_corner = TransformationBetweenTileAreaCorners(_clipboard_paste_area, src_area, InvertDirTransform(_current_clipboard->transformation));
	ret->tile_height = TileHeight(TransformTile(tile, to_src_corner, MapOf(src_area.tile))) + height_delta;
}

struct ClipboardToolbarWindow : Window {
	static HotkeyList hotkeys;

	static CopyPasteMode FlagButtonToFlagBit(int button)
	{
		switch (button) {
			case WID_CT_WITH_RAIL:        return CPM_WITH_RAIL_TRANSPORT;
			case WID_CT_WITH_ROAD:        return CPM_WITH_ROAD_TRANSPORT;
			case WID_CT_WITH_WATER:       return CPM_WITH_WATER_TRANSPORT;
			case WID_CT_WITH_AIR:         return CPM_WITH_AIR_TRANSPORT;
			case WID_CT_MIRROR_SIGNALS:   return CPM_MIRROR_SIGNALS;
			case WID_CT_UPGRADE_BRIDGES:  return CPM_UPGRADE_BRIDGES;
			case WID_CT_WITH_STATIONS:    return CPM_WITH_STATIONS;
			case WID_CT_WITH_OBJECTS:     return CPM_WITH_OBJECTS;
			case WID_CT_WITH_HOUSES:      return CPM_WITH_HOUSES;
			case WID_CT_WITH_INDUSTRIES:  return CPM_WITH_INDUSTRIES;
			case WID_CT_WITH_RIVERS:      return CPM_WITH_RIVERS;
			case WID_CT_WITH_TREES:       return CPM_WITH_TREES;
			case WID_CT_WITH_GROUND:      return CPM_WITH_GROUND;
			default: NOT_REACHED(); break;
		};
		return CPM_NONE;
	}

	ClipboardToolbarWindow(WindowDesc *desc, WindowNumber window_number) : Window(desc)
	{
		this->InitNested(window_number);

		if (!IsClipboardBufferOn()) {
			NWidgetCore *button = this->GetWidget<NWidgetCore>(WID_CT_COPY);
			button->widget_data = SPR_IMG_CLIPBOARD_SELECT_COPY_AREA; // instead of SPR_IMG_CLIPBOARD_COPY
			button->tool_tip = STR_CLIPBOARD_TOOLTIP_SELECT_COPY_AREA; // instead of STR_CLIPBOARD_TOOLTIP_COPY

			button = this->GetWidget<NWidgetCore>(WID_CT_PASTE);
			button->widget_data = SPR_IMG_CLIPBOARD_INSTANT_COPY_PASTE; // instead of SPR_IMG_CLIPBOARD_PASTE
			button->tool_tip = STR_CLIPBOARD_TOOLTIP_INSTANT_COPY_PASTE; // instead of STR_CLIPBOARD_TOOLTIP_PASTE
		}

		/* select another railtype if the one that was used last time is invalid/unavailable */
		if (_game_mode != GM_EDITOR) {
			for (uint i = 0; i < lengthof(_clipboard_props); i++) {
				if (!IsInsideMM(_clipboard_props[i].railtype, RAILTYPE_BEGIN, RAILTYPE_END)) {
					_clipboard_props[i].railtype = RAILTYPE_BEGIN;
				}
				RailType rt = _clipboard_props[i].railtype;
				while (!HasRailtypeAvail(_local_company, rt)) {
					rt++;
					if (rt >= RAILTYPE_END) rt = RAILTYPE_BEGIN;

					if (rt == _clipboard_props[i].railtype) { // did we get back to the point where we started?
						rt = INVALID_RAILTYPE;
						_clipboard_props[i].mode &= ~CPM_CONVERT_RAILTYPE;
						break;
					}
				}
				_clipboard_props[i].railtype = rt;
			}
		}

		this->UpdateButtons();

		if (_settings_client.gui.link_terraform_toolbar) ShowTerraformToolbar(this);
	}

	~ClipboardToolbarWindow()
	{
		if (_settings_client.gui.link_terraform_toolbar) DeleteWindowById(WC_SCEN_LAND_GEN, 0, false);
	}

	void UpdateButtons()
	{
		/* lower clipboard index indicator */
		this->SetWidgetLoweredState(WID_CT_CLIPBOARD_1, _current_clipboard == &_clipboard_props[0]);
		this->SetWidgetLoweredState(WID_CT_CLIPBOARD_2, _current_clipboard == &_clipboard_props[1]);
		this->SetWidgetLoweredState(WID_CT_CLIPBOARD_3, _current_clipboard == &_clipboard_props[2]);
		this->SetWidgetLoweredState(WID_CT_CLIPBOARD_4, _current_clipboard == &_clipboard_props[3]);
		/* disable the paste button if there is nothing to paste */
		this->SetWidgetDisabledState(WID_CT_PASTE, !IsClipboardPasteSourceSet());
		/* lower on/off buttons */
		for (int widget = WID_CT_PASTE_FLAG_BUTTON_BEGIN; widget < WID_CT_PASTE_FLAG_BUTTON_END; widget++) {
			NWidgetCore *nwid = this->GetWidget<NWidgetCore>(widget);
			if (nwid == NULL) continue;
			nwid->SetLowered((_current_clipboard->mode & ClipboardToolbarWindow::FlagButtonToFlagBit(widget)) != 0);
		}
		this->SetWidgetLoweredState(WID_CT_TERRAFORM, (_current_clipboard->mode & CPM_TERRAFORM_MASK) != CPM_TERRAFORM_NONE);
		/* set the sprite on the railtype button */
		NWidgetCore *railtype_btn = this->GetWidget<NWidgetCore>(WID_CT_CONVERT_RAILTYPE);
		if (railtype_btn != NULL) {
			railtype_btn->widget_data = (_current_clipboard->mode & CPM_CONVERT_RAILTYPE) ?
				GetRailTypeInfo(_current_clipboard->railtype)->gui_sprites.convert_rail :
				SPR_IMG_CLIPBOARD_NO_RAIL_CONVERTION;
		}

		this->SetDirty();
	}

	virtual void DrawWidget(const Rect &r, int widget) const
	{
		int offset = this->IsWidgetLowered(widget) ? 2 : 1;
		switch (widget) {
			case WID_CT_WITH_RAIL:
			case WID_CT_WITH_ROAD:
			case WID_CT_WITH_WATER:
			case WID_CT_WITH_AIR:
			case WID_CT_WITH_STATIONS:
			case WID_CT_WITH_OBJECTS:
			case WID_CT_WITH_HOUSES:
			case WID_CT_WITH_INDUSTRIES:
			case WID_CT_WITH_RIVERS:
			case WID_CT_WITH_TREES:
			case WID_CT_WITH_GROUND: {
				offset++;
				DrawSprite(SPR_BLOT, this->IsWidgetLowered(widget) ? PALETTE_TO_GREEN : PALETTE_TO_RED, r.left + offset, r.top + offset);
				break;
			}

			case WID_CT_TERRAFORM: {
				offset++;
				PaletteID pal;
				switch (_current_clipboard->mode & CPM_TERRAFORM_MASK) {
					case CPM_TERRAFORM_FULL:    pal = PALETTE_TO_GREEN;  break;
					case CPM_TERRAFORM_MINIMAL: pal = PALETTE_TO_YELLOW; break;
					default:                    pal = PALETTE_TO_RED;    break;
				}
				DrawSprite(SPR_BLOT, pal, r.left + offset, r.top + offset);
				break;
			}

			case WID_CT_TRANSFORMATION:
				DrawSprite(SPR_IMG_TRANFORMATION_IDENTITY + _current_clipboard->transformation, PAL_NONE, r.left + offset, r.top + offset);
				break;

			case WID_CT_HEIGHT_DIFF_GLYPH:
				DrawSprite(SPR_IMG_CLIPBOARD_HEIGHT_PANEL, PAL_NONE, r.left, r.top);
				break;

			default:
				break;
		}
	}

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
		Dimension d;

		switch (widget) {
			case WID_CT_CLIPBOARD_1:
			case WID_CT_CLIPBOARD_2:
			case WID_CT_CLIPBOARD_3:
			case WID_CT_CLIPBOARD_4:
				d.width = GetDigitWidth() + 4;
				d.height = FONT_HEIGHT_NORMAL;
				break;

			case WID_CT_HEIGHT_DIFF_GLYPH:
				d = GetSpriteSize(SPR_IMG_CLIPBOARD_HEIGHT_PANEL);
				break;

			case WID_CT_HEIGHT_DIFF: {
				/* Backup the height delta. The variable will be used to calculate the size of the widget. */
				int backup = _current_clipboard->additional_height_delta;
				/* calculate the size */
				d.width = d.height = 0;
				for (_current_clipboard->additional_height_delta = CLIPBOARD_ADDITIONAL_HEIGHT_MIN; _current_clipboard->additional_height_delta <= CLIPBOARD_ADDITIONAL_HEIGHT_MAX; _current_clipboard->additional_height_delta++) {
					this->SetStringParameters(WID_CT_HEIGHT_DIFF); // additional_height_delta will be used there
					d = maxdim(d, GetStringBoundingBox(this->GetWidget<NWidgetCore>(WID_CT_HEIGHT_DIFF)->widget_data));
				}
				d.width += 1;
				/* restore */
				_current_clipboard->additional_height_delta = backup;
				break;
			}

			default:
				return;
		}

		d.width += padding.width;
		d.height += padding.height;
		*size = maxdim(*size, d);
	}

	virtual void SetStringParameters(int widget) const
	{
		switch (widget) {
			case WID_CT_CLIPBOARD_1:
			case WID_CT_CLIPBOARD_2:
			case WID_CT_CLIPBOARD_3:
			case WID_CT_CLIPBOARD_4:
				SetDParam(0, widget - WID_CT_CLIPBOARD_1 + 1);
				break;

			case WID_CT_HEIGHT_DIFF:
				SetDParam(0, (StringID)(STR_CLIPBOARD_HEIGHT_DIFF_NEUTRAL + sgn(_current_clipboard->additional_height_delta)));
				SetDParam(1, abs(_current_clipboard->additional_height_delta));
				break;
		}
	}

	virtual void OnClick(Point pt, int widget, int click_count)
	{
		if (this->IsWidgetDisabled(widget)) return;

		DirTransformation add_clipboard_transformation = DTR_IDENTITY; // additional transformation

		switch (widget) {
			case WID_CT_CLIPBOARD_1:
			case WID_CT_CLIPBOARD_2:
			case WID_CT_CLIPBOARD_3:
			case WID_CT_CLIPBOARD_4:
				/* switch to another clipboard */
				assert(IsInsideMM(widget - WID_CT_CLIPBOARD_1, 0, lengthof(_clipboard_props)));
				_current_clipboard = &_clipboard_props[widget - WID_CT_CLIPBOARD_1];
				this->UpdateButtons();

				if (this->IsWidgetLowered(WID_CT_PASTE)) {
					if (IsClipboardPasteSourceSet()) {
						/* update paste preview */
						ClipboardRecalcPasteAreaSize();
						SetTileSelectSize(_clipboard_paste_area.w + 1, _clipboard_paste_area.h + 1);
						UpdateTileSelection();
						MarkWholeScreenDirty();
					} else {
						ResetObjectToPlace(); // current clipboard is empty!
					}
				}
				break;

			case WID_CT_COPY:
				if (HandlePlacePushButton(this, widget, SPR_CURSOR_COPY, HT_RECT)) {
					this->SetWidgetDirty(widget);
				}
				return;

			case WID_CT_PASTE:
				if (HandlePlacePushButton(this, widget, _ctrl_pressed ? SPR_CURSOR_ADJUST_HEIGHT : SPR_CURSOR_PASTE, HT_POINT | HT_PASTE_PREVIEW)) {
					ClipboardRecalcPasteAreaSize();
					SetTileSelectSize(_clipboard_paste_area.w + 1, _clipboard_paste_area.h + 1);
					this->SetWidgetDirty(widget);
				}
				return;

			case WID_CT_TERRAFORM: {
				switch (_current_clipboard->mode & CPM_TERRAFORM_MASK) {
					case CPM_TERRAFORM_NONE:    (_current_clipboard->mode &= ~CPM_TERRAFORM_MASK) |= CPM_TERRAFORM_FULL;    break;
					case CPM_TERRAFORM_MINIMAL: (_current_clipboard->mode &= ~CPM_TERRAFORM_MASK) |= CPM_TERRAFORM_NONE;    break;
					case CPM_TERRAFORM_FULL:    (_current_clipboard->mode &= ~CPM_TERRAFORM_MASK) |= CPM_TERRAFORM_MINIMAL; break;
					default: NOT_REACHED();
				}
				this->UpdateButtons();
				break;
			}

			case WID_CT_TRANSFORMATION:
				/* reset transformation - combined with its inversion will give identity */
				add_clipboard_transformation = InvertDirTransform(_current_clipboard->transformation);
				break;

			case WID_CT_ROTATE_LEFT:   add_clipboard_transformation = DTR_ROTATE_90_L;   break;
			case WID_CT_ROTATE_RIGHT:  add_clipboard_transformation = DTR_ROTATE_90_R;   break;
			case WID_CT_REFLECT_NE_SW: add_clipboard_transformation = DTR_REFLECT_NE_SW; break;
			case WID_CT_REFLECT_NW_SE: add_clipboard_transformation = DTR_REFLECT_NW_SE; break;

			case WID_CT_WITH_RAIL:
			case WID_CT_WITH_ROAD:
			case WID_CT_WITH_WATER:
			case WID_CT_WITH_AIR:
			case WID_CT_MIRROR_SIGNALS:
			case WID_CT_UPGRADE_BRIDGES:
			case WID_CT_WITH_STATIONS:
			case WID_CT_WITH_OBJECTS:
			case WID_CT_WITH_HOUSES:
			case WID_CT_WITH_INDUSTRIES:
			case WID_CT_WITH_RIVERS:
			case WID_CT_WITH_TREES:
			case WID_CT_WITH_GROUND:
				_current_clipboard->mode ^= ClipboardToolbarWindow::FlagButtonToFlagBit(widget);
				this->UpdateButtons();
				break;

			case WID_CT_CONVERT_RAILTYPE:
				ShowDropDownList(this, GetRailTypeDropDownList(),
						(_current_clipboard->mode & CPM_CONVERT_RAILTYPE) ? INVALID_RAILTYPE : _current_clipboard->railtype,
						WID_CT_CONVERT_RAILTYPE, 140, true, true);
				break;

			case WID_CT_HEIGHT_DIFF_INCREASE: this->ModifyAdditionalHeightDelta(+1); break;
			case WID_CT_HEIGHT_DIFF_DECREASE: this->ModifyAdditionalHeightDelta(-1); break;

			default:
				return;
		}

		if (_settings_client.sound.click_beep) SndPlayFx(SND_15_BEEP);

		if (add_clipboard_transformation != DTR_IDENTITY) {
			_current_clipboard->transformation = CombineDirTransform(_current_clipboard->transformation, add_clipboard_transformation);
			this->SetWidgetDirty(WID_CT_TRANSFORMATION);
			if (this->IsWidgetLowered(WID_CT_PASTE)) {
				ClipboardRecalcPasteAreaSize();
				SetTileSelectSize(_clipboard_paste_area.w + 1, _clipboard_paste_area.h + 1);
			}
		}
	}

	virtual EventState OnHotkey(int hotkey)
	{
		switch (hotkey) {
			case WID_CT_CONVERT_RAILTYPE:
				this->OnDropdownSelect(WID_CT_CONVERT_RAILTYPE, (_current_clipboard->mode & CPM_CONVERT_RAILTYPE) ? INVALID_RAILTYPE : _current_clipboard->railtype);
				this->SetWidgetDirty(WID_CT_CONVERT_RAILTYPE);
				if (_settings_client.sound.click_beep) SndPlayFx(SND_15_BEEP);
				return ES_HANDLED;

			case WID_CT_WITH_RAIL:
			case WID_CT_WITH_ROAD:
			case WID_CT_WITH_WATER:
			case WID_CT_WITH_AIR:
			case WID_CT_TERRAFORM:
			case WID_CT_WITH_STATIONS:
			case WID_CT_WITH_OBJECTS:
			case WID_CT_WITH_HOUSES:
			case WID_CT_WITH_INDUSTRIES:
			case WID_CT_WITH_RIVERS:
			case WID_CT_WITH_TREES:
			case WID_CT_WITH_GROUND:
				if (this->IsWidgetLowered(WID_CT_PASTE)) MarkWholeScreenDirty(); // redraw tile selection
				break;

			default:
				break;
		}

		return this->Window::OnHotkey(hotkey);
	}

	virtual void OnDropdownSelect(int widget, int index)
	{
		assert(widget == WID_CT_CONVERT_RAILTYPE);
		if (index == INVALID_RAILTYPE) {
			_current_clipboard->mode &= ~CPM_CONVERT_RAILTYPE;
		} else {
			_current_clipboard->mode |= CPM_CONVERT_RAILTYPE;
			_current_clipboard->railtype = (RailType)index;
		}
		this->UpdateButtons();
	}

	virtual EventState OnCTRLStateChange()
	{
		if (this->IsWidgetLowered(WID_CT_PASTE)) SetMouseCursor(_ctrl_pressed ? SPR_CURSOR_ADJUST_HEIGHT : SPR_CURSOR_PASTE, PAL_NONE);

		return ES_NOT_HANDLED;
	}

	virtual void OnPlaceObject(Point pt, TileIndex tile)
	{
		if (this->IsWidgetLowered(WID_CT_COPY)) {
			/* start copy area dragging */
			if (IsClipboardBufferOn()) {
				VpStartPlaceSizing(tile, VPM_X_AND_Y, DDSP_COPY_TO_CLIPBOARD);
			} else {
				VpStartPlaceSizing(tile, VPM_X_AND_Y_LIMITED, DDSP_COPY_TO_CLIPBOARD);
				VpSetPlaceSizingLimit(_settings_game.construction.clipboard_capacity);
			}
		} else {
			_clipboard_paste_area.tile = tile;

			/* do paste */
			assert(IsClipboardPasteSourceSet());

			uint32 p1 = 0, p2 = 0;
			SB(p1, 28,  4, _current_clipboard->railtype);
			SB(p2, 12,  5, _current_clipboard->additional_height_delta);
			SB(p2, 17,  3, _current_clipboard->transformation);
			SB(p2, 20, 10, _current_clipboard->mode);
			if (IsClipboardBufferOn()) {
				/* copy/paste clipboard-to-map */
				SB(p1, 0, 2, GetClipboardBufferIndex(GetCurrentClipboardBuffer()));
				SetDParam(COPY_PASTE_ERR_SUMMARY_PARAM, STR_ERROR_CAN_T_PASTE_HERE);
				DoCommandP(tile, p1, p2, CMD_PASTE_FROM_CLIPBOARD | CMD_MSG(STR_COPY_PASTE_ERROR_SUMMARY), CcPaste);
			} else {
				/* copy/paste map-to-map */
				SB(p1, 0, 28, _current_clipboard->copy_area.tile);
				SB(p2, 0,  6, _current_clipboard->copy_area.w);
				SB(p2, 6,  6, _current_clipboard->copy_area.h);
				SetDParam(COPY_PASTE_ERR_SUMMARY_PARAM, STR_ERROR_CAN_T_PASTE_HERE);
				DoCommandP(tile, p1, p2, CMD_INSTANT_COPY_PASTE | CMD_MSG(STR_COPY_PASTE_ERROR_SUMMARY), CcPaste);
			}

			MarkWholeScreenDirty(); // redraw tile selection
		}
	}

	virtual void OnPlaceDrag(ViewportPlaceMethod select_method, ViewportDragDropSelectionProcess select_proc, Point pt)
	{
		VpSelectTilesWithMethod(pt.x, pt.y, select_method);
	}

	virtual void OnPlaceMouseUp(ViewportPlaceMethod select_method, ViewportDragDropSelectionProcess select_proc, Point pt, TileIndex start_tile, TileIndex end_tile)
	{
		if (pt.x != -1) {
			switch (select_proc) {
				case DDSP_COPY_TO_CLIPBOARD: {
					TileArea ta = TileArea(start_tile, end_tile);

					/* do copy */
					if (IsClipboardBufferOn()) {
						/* copy into the buffer */
						uint32 p1 = 0, p2 = 0;
						SB(p1, 0, 2, GetClipboardBufferIndex(GetCurrentClipboardBuffer()));
						SB(p2,  0, 14, ta.w); // source area width
						SB(p2, 14, 14, ta.h); // source area height
						if (!DoCommandP(ta.tile, p1, p2, CMD_COPY_TO_CLIPBOARD) || _shift_pressed) return; // leave copy tool opened
					}
					ResetObjectToPlace();

					/* select copy area */
					_current_clipboard->copy_area = ta;

					/* reset transformation and update buttons */
					_current_clipboard->transformation = DTR_IDENTITY;
					this->ModifyAdditionalHeightDelta(-_current_clipboard->additional_height_delta);
					this->UpdateButtons();
					break;
				}

				default:
					NOT_REACHED();
			}
		}
	}

	virtual void OnPlaceObjectAbort()
	{
		/* Unclick "copy" and "paste" buttons */
		this->RaiseWidget(WID_CT_COPY);
		this->RaiseWidget(WID_CT_PASTE);
		this->SetWidgetDirty(WID_CT_COPY);
		this->SetWidgetDirty(WID_CT_PASTE);
	}

	EventState OnPlaceMouseWheel(Point pt, int mousewheel)
	{
		if (mousewheel == 0 || !_ctrl_pressed || !this->IsWidgetLowered(WID_CT_PASTE)) return ES_NOT_HANDLED;
		this->ModifyAdditionalHeightDelta(-sgn(mousewheel));
		return ES_HANDLED;
	}

	void ModifyAdditionalHeightDelta(int diff)
	{
		_current_clipboard->additional_height_delta = Clamp(_current_clipboard->additional_height_delta + diff, CLIPBOARD_ADDITIONAL_HEIGHT_MIN, CLIPBOARD_ADDITIONAL_HEIGHT_MAX);
		this->SetWidgetDirty(WID_CT_HEIGHT_DIFF);
	}
};

EventState ClipboardGlobalHotkeys(int hotkey)
{
	Window *w = ShowClipboardToolbar();
	if (w == NULL) return ES_NOT_HANDLED;
	return w->OnHotkey(hotkey);
}

static Hotkey _clipboard_hotkeys[] = {
	Hotkey('C' | WKC_CTRL | WKC_GLOBAL_HOTKEY, "copy",  WID_CT_COPY),
	Hotkey('V' | WKC_CTRL | WKC_GLOBAL_HOTKEY, "paste", WID_CT_PASTE),
	Hotkey('1', "clipboard1",      WID_CT_CLIPBOARD_1),
	Hotkey('2', "clipboard2",      WID_CT_CLIPBOARD_2),
	Hotkey('3', "clipboard3",      WID_CT_CLIPBOARD_3),
	Hotkey('4', "clipboard4",      WID_CT_CLIPBOARD_4),
	Hotkey('5', "rail",            WID_CT_WITH_RAIL),
	Hotkey('6', "road",            WID_CT_WITH_ROAD),
	Hotkey('7', "water",           WID_CT_WITH_WATER),
	Hotkey('8', "air",             WID_CT_WITH_AIR),
	Hotkey('9', "terrain",         WID_CT_TERRAFORM),
	Hotkey('0', "rail_conversion", WID_CT_CONVERT_RAILTYPE),
	Hotkey('S', "signal_mirror",   WID_CT_MIRROR_SIGNALS),
	Hotkey('B', "bridge_upgrade",  WID_CT_UPGRADE_BRIDGES),
	Hotkey('N', "with_stations",   WID_CT_WITH_STATIONS),
	HOTKEY_LIST_END
};
HotkeyList ClipboardToolbarWindow::hotkeys("clipboard", _clipboard_hotkeys, ClipboardGlobalHotkeys);

static const NWidgetPart _nested_clipboard_toolbar_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN), SetDataTip(STR_CLIPBOARD_TOOLBAR_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_STICKYBOX, COLOUR_DARK_GREEN),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		/* CLIPBOARD INDEX BUTTONS */
		NWidget(WWT_TEXTBTN, COLOUR_DARK_GREEN, WID_CT_CLIPBOARD_1),
						SetFill(0, 1), SetMinimalSize(8, 22), SetDataTip(STR_BLACK_INT, STR_CLIPBOARD_TOOLTIP_SWITCH_TO_1ST_CLIPBOARD),
		NWidget(WWT_TEXTBTN, COLOUR_DARK_GREEN, WID_CT_CLIPBOARD_2),
						SetFill(0, 1), SetMinimalSize(8, 22), SetDataTip(STR_BLACK_INT, STR_CLIPBOARD_TOOLTIP_SWITCH_TO_2ND_CLIPBOARD),
		NWidget(WWT_TEXTBTN, COLOUR_DARK_GREEN, WID_CT_CLIPBOARD_3),
						SetFill(0, 1), SetMinimalSize(8, 22), SetDataTip(STR_BLACK_INT, STR_CLIPBOARD_TOOLTIP_SWITCH_TO_3RD_CLIPBOARD),
		NWidget(WWT_TEXTBTN, COLOUR_DARK_GREEN, WID_CT_CLIPBOARD_4),
						SetFill(0, 1), SetMinimalSize(8, 22), SetDataTip(STR_BLACK_INT, STR_CLIPBOARD_TOOLTIP_SWITCH_TO_4TH_CLIPBOARD),
		NWidget(WWT_PANEL, COLOUR_DARK_GREEN),
						SetFill(0, 1), SetMinimalSize(4, 22), EndContainer(),

		/* COPY / PASTE BUTTONS */
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_CT_COPY),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_CLIPBOARD_COPY, STR_CLIPBOARD_TOOLTIP_COPY),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_CT_PASTE),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_CLIPBOARD_PASTE, STR_CLIPBOARD_TOOLTIP_PASTE),
		NWidget(WWT_PANEL, COLOUR_DARK_GREEN),
						SetFill(0, 1), SetMinimalSize(4, 22), EndContainer(),

		/* COPY/PASTE MODE SELECTORS */
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_CT_WITH_RAIL),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_BUILDRAIL, STR_CLIPBOARD_TOOLTIP_COPY_WITH_RAIL_TRANSPORT),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_CT_WITH_ROAD),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_BUILDROAD, STR_CLIPBOARD_TOOLTIP_COPY_WITH_ROAD_TRANSPORT),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_CT_WITH_WATER),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_BUILDWATER, STR_CLIPBOARD_TOOLTIP_COPY_WITH_WATER_TRANSPORT),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_CT_WITH_AIR),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_BUILDAIR, STR_CLIPBOARD_TOOLTIP_COPY_WITH_AIR_TRANSPORT),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_CT_TERRAFORM),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_LANDSCAPING, STR_CLIPBOARD_TOOLTIP_TERRAFORM),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_CT_CONVERT_RAILTYPE),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_CLIPBOARD_NO_RAIL_CONVERTION, STR_CLIPBOARD_TOOLTIP_CONVERT_RAIL),
		NWidget(WWT_IMGBTN_2, COLOUR_DARK_GREEN, WID_CT_MIRROR_SIGNALS),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_CLIPBOARD_MIRROR_SIGNALS_OFF, STR_CLIPBOARD_TOOLTIP_MIRROR_SIGNALS),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_CT_UPGRADE_BRIDGES),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_CLIPBOARD_UPGRADE_BRIDGES, STR_CLIPBOARD_TOOLTIP_UPGRADE_BRIDGES),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_CT_WITH_STATIONS),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_COMPANY_LIST, STR_CLIPBOARD_TOOLTIP_WITH_STATIONS),
		NWidget(WWT_PANEL, COLOUR_DARK_GREEN),
						SetFill(0, 1), SetMinimalSize(4, 22), EndContainer(),

		/* TRANSFORMATIONS */
		NWidget(WWT_PUSHBTN, COLOUR_DARK_GREEN, WID_CT_TRANSFORMATION),
						SetFill(0, 1), SetMinimalSize(23, 22), SetDataTip(0, STR_CLIPBOARD_TOOLTIP_TRANSFORMATION),
		NWidget(WWT_PUSHIMGBTN, COLOUR_DARK_GREEN, WID_CT_ROTATE_LEFT),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_CLIPBOARD_ROTATE_LEFT, STR_CLIPBOARD_TOOLTIP_ROTATE_LEFT),
		NWidget(WWT_PUSHIMGBTN, COLOUR_DARK_GREEN, WID_CT_ROTATE_RIGHT),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_CLIPBOARD_ROTATE_RIGHT, STR_CLIPBOARD_TOOLTIP_ROTATE_RIGHT),
		NWidget(WWT_PUSHIMGBTN, COLOUR_DARK_GREEN, WID_CT_REFLECT_NE_SW),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_CLIPBOARD_REFLECT_NE_SW, STR_CLIPBOARD_TOOLTIP_REFLECT_NE_SW),
		NWidget(WWT_PUSHIMGBTN, COLOUR_DARK_GREEN, WID_CT_REFLECT_NW_SE),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_CLIPBOARD_REFLECT_NW_SE, STR_CLIPBOARD_TOOLTIP_REFLECT_NW_SE),
		NWidget(WWT_PANEL, COLOUR_DARK_GREEN),
						SetFill(0, 1), SetMinimalSize(4, 22), EndContainer(),

		/* HEIGHT MANIPULATOR */
		NWidget(WWT_PANEL, COLOUR_DARK_GREEN), SetMinimalSize(0, 22),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_TEXT, COLOUR_DARK_GREEN, WID_CT_HEIGHT_DIFF_GLYPH), SetDataTip(STR_EMPTY, STR_NULL), SetFill(0, 1),
				NWidget(WWT_TEXT, COLOUR_DARK_GREEN, WID_CT_HEIGHT_DIFF), SetDataTip(STR_CLIPBOARD_HEIGHT_DIFF, STR_NULL), SetFill(0, 1),
				NWidget(NWID_VERTICAL), SetPIP(3, 0, 3),
					NWidget(NWID_HORIZONTAL), SetPIP(0, 1, 3),
						NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_CT_HEIGHT_DIFF_INCREASE), SetDataTip(SPR_ARROW_UP, STR_NULL), SetFill(0, 1),
						NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_CT_HEIGHT_DIFF_DECREASE), SetDataTip(SPR_ARROW_DOWN, STR_NULL), SetFill(0, 1),
					EndContainer(),
				EndContainer(),
			EndContainer(),
		EndContainer(),
	EndContainer(),
};

static WindowDesc _clipboard_toolbar_desc(
	WDP_ALIGN_TOOLBAR, "toolbar_clipboard", 0, 0,
	WC_BUILD_TOOLBAR, WC_NONE,
	WDF_CONSTRUCTION,
	_nested_clipboard_toolbar_widgets, lengthof(_nested_clipboard_toolbar_widgets),
	&ClipboardToolbarWindow::hotkeys
);

static const NWidgetPart _nested_scenedit_clipboard_toolbar_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_DARK_GREEN),
		NWidget(WWT_CAPTION, COLOUR_DARK_GREEN), SetDataTip(STR_CLIPBOARD_TOOLBAR_CAPTION, STR_TOOLTIP_WINDOW_TITLE_DRAG_THIS),
		NWidget(WWT_STICKYBOX, COLOUR_DARK_GREEN),
	EndContainer(),
	NWidget(NWID_HORIZONTAL),
		/* CLIPBOARD INDEX BUTTONS */
		NWidget(WWT_TEXTBTN, COLOUR_DARK_GREEN, WID_CT_CLIPBOARD_1),
						SetFill(0, 1), SetMinimalSize(8, 22), SetDataTip(STR_BLACK_INT, STR_CLIPBOARD_TOOLTIP_SWITCH_TO_1ST_CLIPBOARD),
		NWidget(WWT_TEXTBTN, COLOUR_DARK_GREEN, WID_CT_CLIPBOARD_2),
						SetFill(0, 1), SetMinimalSize(8, 22), SetDataTip(STR_BLACK_INT, STR_CLIPBOARD_TOOLTIP_SWITCH_TO_2ND_CLIPBOARD),
		NWidget(WWT_TEXTBTN, COLOUR_DARK_GREEN, WID_CT_CLIPBOARD_3),
						SetFill(0, 1), SetMinimalSize(8, 22), SetDataTip(STR_BLACK_INT, STR_CLIPBOARD_TOOLTIP_SWITCH_TO_3RD_CLIPBOARD),
		NWidget(WWT_TEXTBTN, COLOUR_DARK_GREEN, WID_CT_CLIPBOARD_4),
						SetFill(0, 1), SetMinimalSize(8, 22), SetDataTip(STR_BLACK_INT, STR_CLIPBOARD_TOOLTIP_SWITCH_TO_4TH_CLIPBOARD),
		NWidget(WWT_PANEL, COLOUR_DARK_GREEN),
						SetFill(0, 1), SetMinimalSize(4, 22), EndContainer(),

		/* COPY / PASTE BUTTONS */
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_CT_COPY),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_CLIPBOARD_COPY, STR_CLIPBOARD_TOOLTIP_COPY),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_CT_PASTE),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_CLIPBOARD_PASTE, STR_CLIPBOARD_TOOLTIP_PASTE),
		NWidget(WWT_PANEL, COLOUR_DARK_GREEN),
						SetFill(0, 1), SetMinimalSize(4, 22), EndContainer(),

		/* COPY/PASTE MODE SELECTORS */
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_CT_WITH_ROAD),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_BUILDROAD, STR_CLIPBOARD_TOOLTIP_COPY_WITH_ROAD_TRANSPORT),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_CT_WITH_WATER),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_BUILDWATER, STR_CLIPBOARD_TOOLTIP_COPY_WITH_WATER_TRANSPORT),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_CT_TERRAFORM),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_LANDSCAPING, STR_CLIPBOARD_TOOLTIP_TERRAFORM),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_CT_WITH_OBJECTS),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_TRANSMITTER, STR_CLIPBOARD_TOOLTIP_WITH_OBJECTS),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_CT_WITH_HOUSES),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_TOWN, STR_CLIPBOARD_TOOLTIP_WITH_HOUSES),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_CT_WITH_INDUSTRIES),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_INDUSTRY, STR_CLIPBOARD_TOOLTIP_WITH_INDUSTRIES),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_CT_WITH_RIVERS),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_BUILD_RIVER, STR_CLIPBOARD_TOOLTIP_WITH_RIVERS),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_CT_WITH_TREES),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_PLANTTREES, STR_CLIPBOARD_TOOLTIP_WITH_TREES),
		NWidget(WWT_IMGBTN, COLOUR_DARK_GREEN, WID_CT_WITH_GROUND),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_DESERT, STR_CLIPBOARD_TOOLTIP_WITH_GROUND),
		NWidget(WWT_PANEL, COLOUR_DARK_GREEN),
						SetFill(0, 1), SetMinimalSize(4, 22), EndContainer(),

		/* TRANSFORMATIONS */
		NWidget(WWT_PUSHBTN, COLOUR_DARK_GREEN, WID_CT_TRANSFORMATION),
						SetFill(0, 1), SetMinimalSize(23, 22), SetDataTip(0, STR_CLIPBOARD_TOOLTIP_TRANSFORMATION),
		NWidget(WWT_PUSHIMGBTN, COLOUR_DARK_GREEN, WID_CT_ROTATE_LEFT),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_CLIPBOARD_ROTATE_LEFT, STR_CLIPBOARD_TOOLTIP_ROTATE_LEFT),
		NWidget(WWT_PUSHIMGBTN, COLOUR_DARK_GREEN, WID_CT_ROTATE_RIGHT),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_CLIPBOARD_ROTATE_RIGHT, STR_CLIPBOARD_TOOLTIP_ROTATE_RIGHT),
		NWidget(WWT_PUSHIMGBTN, COLOUR_DARK_GREEN, WID_CT_REFLECT_NE_SW),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_CLIPBOARD_REFLECT_NE_SW, STR_CLIPBOARD_TOOLTIP_REFLECT_NE_SW),
		NWidget(WWT_PUSHIMGBTN, COLOUR_DARK_GREEN, WID_CT_REFLECT_NW_SE),
						SetFill(0, 1), SetMinimalSize(22, 22), SetDataTip(SPR_IMG_CLIPBOARD_REFLECT_NW_SE, STR_CLIPBOARD_TOOLTIP_REFLECT_NW_SE),
		NWidget(WWT_PANEL, COLOUR_DARK_GREEN),
						SetFill(0, 1), SetMinimalSize(4, 22), EndContainer(),

		/* HEIGHT MANIPULATOR */
		NWidget(WWT_PANEL, COLOUR_DARK_GREEN), SetMinimalSize(0, 22),
			NWidget(NWID_HORIZONTAL),
				NWidget(WWT_TEXT, COLOUR_DARK_GREEN, WID_CT_HEIGHT_DIFF_GLYPH), SetDataTip(STR_EMPTY, STR_NULL), SetFill(0, 1),
				NWidget(WWT_TEXT, COLOUR_DARK_GREEN, WID_CT_HEIGHT_DIFF), SetDataTip(STR_CLIPBOARD_HEIGHT_DIFF, STR_NULL), SetFill(0, 1),
				NWidget(NWID_VERTICAL), SetPIP(3, 0, 3),
					NWidget(NWID_HORIZONTAL), SetPIP(0, 1, 3),
						NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_CT_HEIGHT_DIFF_INCREASE), SetDataTip(SPR_ARROW_UP, STR_NULL), SetFill(0, 1),
						NWidget(WWT_PUSHIMGBTN, COLOUR_GREY, WID_CT_HEIGHT_DIFF_DECREASE), SetDataTip(SPR_ARROW_DOWN, STR_NULL), SetFill(0, 1),
					EndContainer(),
				EndContainer(),
			EndContainer(),
		EndContainer(),
	EndContainer(),
};

static WindowDesc _scenedit_clipboard_toolbar_desc(
	WDP_AUTO, "toolbar_clipboard_scenedit", 0, 0,
	WC_SCEN_BUILD_TOOLBAR, WC_NONE,
	WDF_CONSTRUCTION,
	_nested_scenedit_clipboard_toolbar_widgets, lengthof(_nested_scenedit_clipboard_toolbar_widgets),
	&ClipboardToolbarWindow::hotkeys
);

/**
 * Open the clipboard toolbar to copy and paste map pieces.
 * @return newly opened clipboard toolbar, or NULL if the toolbar could not be opened.
 */
Window *ShowClipboardToolbar()
{
	WindowDesc *desc;
	if (_game_mode == GM_EDITOR) {
		desc = &_scenedit_clipboard_toolbar_desc;
	} else {
		if (!IsClipboardBufferOn() && _settings_game.construction.clipboard_capacity == 0) return NULL;
		if (!Company::IsValidID(_local_company)) return NULL;
		DeleteWindowByClass(WC_BUILD_TOOLBAR);
		desc = &_clipboard_toolbar_desc;
	}
	return AllocateWindowDescFront<ClipboardToolbarWindow>(desc, WN_CLIPBOARD_TOOLBAR);
}
