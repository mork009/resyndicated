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

#include "rs-kernel/mgr/vehiclemanager.h"

#include <unordered_map>
#include "rs-utils/log/log.h"
#include "rs-utils/io/file.h"
#include "rs-utils/io/configfile.h"

namespace rs_knl {

static std::unordered_map<uint8_t, VehicleStats> s_statsMap;
static std::vector<VehicleStats> s_allStats;
static VehicleStats s_defaultStats;
static bool s_vehicleStatsLoaded = false;

VehicleManager::VehicleManager() {
    if (!s_vehicleStatsLoaded) {
        loadVehicleStats();
    }
}

void VehicleManager::loadVehicleStats() {
    std::string filename(rs_utl::File::getResyndDataFullPath("ref/vehicles.dat"));
    s_statsMap.clear();
    s_allStats.clear();

    auto addStats = [](uint8_t type, const std::string &name, int hp, int spd, int acc, int dwell, uint16_t anim) {
        VehicleStats st;
        st.type = type;
        st.name = name;
        st.health = hp;
        st.speed = spd;
        st.acceleration = acc;
        st.dwell_time = dwell;
        st.base_anim = anim;
        s_statsMap[type] = st;
        s_allStats.push_back(st);
    };

    // Built-in defaults in case file is missing
    addStats(Vehicle::kVehicleTypeRegularCar, "Regular Car", 30, 1024, 1400, 0, 446);
    addStats(Vehicle::kVehicleTypeSmallArmored, "Armored Car", 10, 768, 1000, 0, 534);
    addStats(Vehicle::kVehicleTypeLargeArmored, "Large Armored", 600, 640, 800, 0, 400);
    addStats(Vehicle::kVehicleTypePolice, "Police Car", 120, 1280, 1800, 0, 470);
    addStats(Vehicle::kVehicleTypeMedics, "Ambulance", 115, 1150, 1400, 0, 494);
    addStats(Vehicle::kVehicleTypeFireFighter, "Firetruck", 40, 1024, 1000, 0, 518);
    addStats(Vehicle::kVehicleTypeTrainHead, "Train Locomotive", 100, 1280, 1600, 10000, 416);
    addStats(Vehicle::kVehicleTypeTrainBody, "Train Carriage", 80, 0, 0, 10000, 428);

    s_defaultStats = s_statsMap[Vehicle::kVehicleTypeRegularCar];

    try {
        ConfigFile conf(filename);

        auto updateFromConfig = [&](const std::string &prefix, uint8_t type) {
            if (s_statsMap.find(type) == s_statsMap.end()) return;
            VehicleStats &st = s_statsMap[type];
            st.name = conf.read<std::string>(prefix + ".name", st.name);
            st.health = conf.read<int>(prefix + ".health", st.health);
            st.speed = conf.read<int>(prefix + ".speed", st.speed);
            st.acceleration = conf.read<int>(prefix + ".acceleration", st.acceleration);
            st.dwell_time = conf.read<int>(prefix + ".dwell_time", st.dwell_time);
            st.base_anim = static_cast<uint16_t>(conf.read<int>(prefix + ".base_anim", st.base_anim));
        };

        updateFromConfig("vehicle.regular_car", Vehicle::kVehicleTypeRegularCar);
        updateFromConfig("vehicle.small_armored", Vehicle::kVehicleTypeSmallArmored);
        updateFromConfig("vehicle.large_armored", Vehicle::kVehicleTypeLargeArmored);
        updateFromConfig("vehicle.police", Vehicle::kVehicleTypePolice);
        updateFromConfig("vehicle.ambulance", Vehicle::kVehicleTypeMedics);
        updateFromConfig("vehicle.firetruck", Vehicle::kVehicleTypeFireFighter);
        updateFromConfig("vehicle.train_head", Vehicle::kVehicleTypeTrainHead);
        updateFromConfig("vehicle.train_carriage", Vehicle::kVehicleTypeTrainBody);

        s_allStats.clear();
        for (const auto &pair : s_statsMap) {
            s_allStats.push_back(pair.second);
        }

        s_vehicleStatsLoaded = true;
    } catch (...) {
        LOG(Log::k_FLG_IO, "VehicleManager", "loadVehicleStats", ("Failed to load %s, using defaults", filename.c_str()));
        s_vehicleStatsLoaded = true;
    }
}

const VehicleStats & VehicleManager::getStats(uint8_t type) {
    if (!s_vehicleStatsLoaded) {
        loadVehicleStats();
    }
    auto it = s_statsMap.find(type);
    if (it != s_statsMap.end()) {
        return it->second;
    }
    return s_defaultStats;
}

const std::vector<VehicleStats> & VehicleManager::getAllStats() {
    if (!s_vehicleStatsLoaded) {
        loadVehicleStats();
    }
    return s_allStats;
}

}

