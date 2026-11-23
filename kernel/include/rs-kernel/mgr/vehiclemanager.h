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

#ifndef VEHICLEMANAGER_H
#define VEHICLEMANAGER_H

#include <string>
#include <vector>

#include "rs-utils/common.h"
#include "rs-kernel/model/vehicle.h"

namespace rs_knl {

struct VehicleStats {
    std::string name;
    uint8_t type = 0;
    int health = 30;
    int speed = 1024;
    int acceleration = 1400;
    int dwell_time = 10000;
    uint16_t base_anim = 446;
};

class VehicleManager {
public:
    VehicleManager();

    //! Loads vehicle attributes from ref/vehicles.dat
    static void loadVehicleStats();

    //! Returns vehicle stats for the specified vehicle type (subtype in mission data)
    static const VehicleStats & getStats(uint8_t type);

    //! Returns all registered vehicle types
    static const std::vector<VehicleStats> & getAllStats();
};

}

#endif // VEHICLEMANAGER_H

