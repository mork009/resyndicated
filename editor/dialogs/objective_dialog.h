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

#ifndef EDITOR_OBJECTIVE_DIALOG_H_
#define EDITOR_OBJECTIVE_DIALOG_H_

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>
#include "rs-kernel/model/mission.h"
#include "rs-kernel/model/objectivedesc.h"

namespace rs_win_editor {

struct ObjectiveDialogData {
    // 0x01: Persuade
    // 0x02: Assassinate
    // 0x03: Protect
    // 0x0B: Combat Sweep
    // 0x0E: Destroy Vehicle
    // 0x0F: Use Vehicle
    // 0x10: Evacuate
    uint16_t type = 0x01;

    // Target Ped ID or Vehicle index
    uint16_t targetPedId = 0;
    uint16_t targetVehicleIdx = 0;

    // Evacuate coordinates (world coordinates, x, y, z)
    int evacX = 0;
    int evacY = 0;
    int evacZ = 0;

    std::string msg;
};

class WinMapViewport;

class ObjectiveDialog {
public:
    static bool show(HWND hParent, WinMapViewport *pViewport, rs_knl::Mission *pMission, ObjectiveDialogData &data, bool isEdit = false, int editingIndex = -1);
    static rs_knl::ObjectiveDesc* createObjectiveFromData(rs_knl::Mission *pMission, const ObjectiveDialogData &data);
    static ObjectiveDialogData extractDataFromObjective(rs_knl::Mission *pMission, rs_knl::ObjectiveDesc *pObj);
};

} // namespace rs_win_editor

#endif // EDITOR_OBJECTIVE_DIALOG_H_
