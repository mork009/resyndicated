/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2026  mork009 <mork009resynd@gmail.com>
 *
 *   This program is free software: you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as 
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>. 
 * 
 */

#ifndef EDITOR_RESOURCE_H_
#define EDITOR_RESOURCE_H_

#define IDR_MAIN_MENU           101
#define IDR_ACCELERATOR         102
#define IDI_APP_ICON            1

// File Menu
#define IDM_FILE_NEW            1001
#define IDM_FILE_OPEN           1002
#define IDM_FILE_CLOSE          1007
#define IDM_FILE_SAVE           1003
#define IDM_FILE_SAVEAS         1004
#define IDM_FILE_SETTINGS       1005
#define IDM_FILE_EXIT           1006

// Edit Menu
#define IDM_EDIT_UNDO           1101
#define IDM_EDIT_REDO           1102
#define IDM_EDIT_DELETE         1103
#define IDM_EDIT_DESELECT       1104

// View Menu
#define IDM_VIEW_SLICE          1201
#define IDM_VIEW_ZOOM_IN        1202
#define IDM_VIEW_ZOOM_OUT       1203
#define IDM_VIEW_ZOOM_100       1204
#define IDM_VIEW_ZOOM_150       1205
#define IDM_VIEW_ZOOM_200       1206
#define IDM_VIEW_CENTER_SQUAD   1207
#define IDM_VIEW_SHOW_STATS     1208
#define IDM_VIEW_FILTER_BUILDINGS 1209
#define IDM_VIEW_FILTER_VEHICLES  1210
#define IDM_VIEW_FILTER_CIVILIANS 1211
#define IDM_VIEW_FILTER_POLICE    1212
#define IDM_VIEW_FILTER_GUARDS    1213
#define IDM_VIEW_FILTER_CRIMINALS 1214
#define IDM_VIEW_FILTER_AGENTS    1215
#define IDM_VIEW_FILTER_WEAPONS   1216
#define IDM_VIEW_FILTER_SELECT_ALL 1217
#define IDM_VIEW_FILTER_CLEAR_ALL  1218

// Tools Menu
#define IDM_TOOLS_PENCIL        1301
#define IDM_TOOLS_BOXFILL       1302
#define IDM_TOOLS_SELECT        1304
#define IDM_TOOLS_SEARCH        1307

// Layer Menu
#define IDM_LAYER_UP            1401
#define IDM_LAYER_DOWN          1402
#define IDM_LAYER_AUTO          1403
#define IDM_LAYER_0             1410
#define IDM_LAYER_1             1411
#define IDM_LAYER_2             1412
#define IDM_LAYER_3             1413
#define IDM_LAYER_4             1414
#define IDM_LAYER_5             1415
#define IDM_LAYER_6             1416
#define IDM_LAYER_7             1417
#define IDM_LAYER_8             1418
#define IDM_LAYER_9             1419
#define IDM_LAYER_10            1420
#define IDM_LAYER_11            1421
#define IDM_LAYER_12            1422

// Mission Menu
#define IDM_MISSION_PLAYTEST    1501
#define IDM_MISSION_PROPERTIES  1502
#define IDM_MISSION_LOADOUT     1504
#define IDM_MISSION_VALIDATE    1505
#define IDM_MISSION_AI_TACTICS  1506

// Help Menu
#define IDM_HELP_SHORTCUTS      1601
#define IDM_HELP_ABOUT          1602
#define IDM_HELP_TOOLS          1603

// Toolbar Button IDs
#define IDC_TOOLBAR             2001
#define IDC_STATUSBAR           2002
#define IDC_VIEWPORT            2003
#define IDC_ARTPANEL            2004

// Art Panel Controls
#define IDC_TAB_CONTROL         3001
#define IDC_COMBO_CATEGORY      3002
#define IDC_TILE_GRID           3003
#define IDC_BTN_TOOL_PENCIL     3004
#define IDC_BTN_TOOL_BOX        3005
#define IDC_ENTITY_GRID         3007
#define IDC_COMBO_ENTITY_TYPE   3008
#define IDC_BTN_TOOL_TILE_SELECT 3009
#define IDC_LIST_OBJECTIVES     3010
#define IDC_BTN_ADD_OBJ         3011
#define IDC_BTN_DEL_OBJ         3012
#define IDC_EDIT_PROPERTIES     3013
#define IDC_BTN_EDIT_OBJ        3014
#define IDC_BTN_CLEAR_OBJ       3015

// Inspector controls
#define IDC_INSPECT_HEADER      3020
#define IDC_INSPECT_TYPE        3021
#define IDC_INSPECT_HEALTH      3022
#define IDC_INSPECT_ORIENT      3023
#define IDC_INSPECT_PREVIEW_BOX 3024
#define IDC_INSPECT_WEAPONS     3025
#define IDC_INSPECT_APPLY       3026
#define IDC_INSPECT_DELETE      3027
#define IDC_INSPECT_CANCEL      3028
#define IDC_INSPECT_DRIVER      3029
#define IDC_INSPECT_CYBERWARE   3066
#define IDC_INSPECT_CHECK_DEAD  3067
#define IDC_INSPECT_MOVEMENT_MODE 3068
#define IDC_INSPECT_FACTION       3069
#define IDC_INSPECT_BTN_AI_TACTICS 3072
#define IDC_INSPECT_BTN_PLAYTEST_CONFIG 3073
#define IDC_INSPECT_COMBO_TYPE  3074
#define IDC_INSPECT_POS_LABEL   3050
#define IDC_INSPECT_POS_VALUE   3051
#define IDC_INSPECT_LOC_LABEL   3052
#define IDC_INSPECT_LOC_VALUE   3053
#define IDC_INSPECT_ROT_LEFT    3054
#define IDC_INSPECT_PLAY_ANIM   3055
#define IDC_INSPECT_ROT_RIGHT   3056

// Unified Tab Controls
#define IDC_BTN_ENT_SELECT      3032
#define IDC_BTN_ENT_PLACE       3033
#define IDC_BTN_TOOL_TILE_SEARCH 3036
#define IDC_BTN_ENT_SEARCH       3037

// Tile Inspector Controls
#define IDC_TILE_INSPECT_PREVIEW_BOX 3060
#define IDC_TILE_INSPECT_TYPE        3061
#define IDC_TILE_INSPECT_SURFACE    3062
#define IDC_TILE_INSPECT_POS_LABEL  3063
#define IDC_TILE_INSPECT_POS_VALUE  3064
#define IDC_TILE_INSPECT_PLACEMENTS 3065

// Search Results Table Controls
#define IDC_TILE_SEARCH_RESULTS     3070
#define IDC_ENT_SEARCH_RESULTS      3071

// Inspector Route & Behavior Controls
#define IDC_INSPECT_ROUTE_INFO  3040
#define IDC_INSPECT_BTN_ROUTE   3041
#define IDC_INSPECT_CHECK_LOOP  3042
#define IDC_INSPECT_BTN_CLEAR_ROUTE 3043
#define IDC_INSPECT_LIST_ACTIONS 3044
#define IDC_INSPECT_BTN_ADD_ACTION 3045
#define IDC_INSPECT_BTN_DEL_ACTION 3046
#define IDC_INSPECT_BTN_MOVE_UP  3047
#define IDC_INSPECT_BTN_MOVE_DOWN 3048

// Bitmaps & Binary Resources
#define IDB_TOOLBAR_ICONS       4001
#define IDB_TAB_ICONS           4002
#define IDR_ICONS_MASTER        4003
#define IDB_TOOLBAR_ICONS_2X    4004
#define IDB_TAB_ICONS_2X        4005

#endif // EDITOR_RESOURCE_H_
