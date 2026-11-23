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

#ifndef EDITOR_ICON_MANAGER_H_
#define EDITOR_ICON_MANAGER_H_

#include <windows.h>
#include <commctrl.h>
#include <vector>

namespace rs_win_editor {

enum class IconId {
    New = 0,
    Open = 1,
    Save = 2,
    Undo = 3,
    Redo = 4,
    Pencil = 5,
    BoxFill = 6,
    Picker = 7, // Unused slot in icons_master_48.dat
    Select = 8,
    Slice = 9,
    LayerUp = 10,
    LayerDown = 11,
    Spawns = 12,
    Playtest = 13,
    ZoomIn = 14,
    ZoomOut = 15,
    Zoom100 = 16,
    TabTiles = 17,
    TabEntities = 18,
    TabObjectives = 19,
    Settings = 20,
    TabProperties = Settings,
    Search = 21,
    Trash = 22, // Unused slot in icons_master_48.dat
    Computer = 23, // Unused slot in icons_master_48.dat
    Count = 24
};

class IconManager {
public:
    static IconManager& instance();

    void initialize(int iconSize = 24);
    ~IconManager();

    HIMAGELIST toolbarImageList() const { return hToolbarImageList_; }
    HIMAGELIST tabsImageList() const { return hTabsImageList_; }

    HBITMAP createIconBitmap(IconId id, int size, uint32_t bgPixel = 0x00000000);

private:
    IconManager() = default;
    bool loadMasterResource();
    bool renderMasterIcon(uint32_t *dst, int size, IconId id, uint32_t bgPixel = 0x00000000);
    void drawIcon(uint32_t *pixels, int size, IconId id);

private:
    HIMAGELIST hToolbarImageList_ = nullptr;
    HIMAGELIST hTabsImageList_ = nullptr;
    int iconSize_ = 24;
    const uint32_t *pMasterIcons_ = nullptr;
    int masterIconCount_ = 0;
    static constexpr int kMasterIconSize = 48;
    static constexpr int kIconCount = static_cast<int>(IconId::Count);
};

} // namespace rs_win_editor

#endif // EDITOR_ICON_MANAGER_H_
