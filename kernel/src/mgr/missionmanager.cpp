/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
 *   Copyright (C) 2024-2025  Benoit Blancard <benblan@users.sourceforge.net>
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

#include "rs-kernel/mgr/missionmanager.h"

#include <format>
#include <fstream>
#include <unordered_map>

#include "rs-utils/io/dernc.h"
#include "rs-engine/appcontext.h"
#include "rs-utils/io/file.h"
#include "rs-utils/log/log.h"
#include "rs-kernel/model/missionbriefing.h"
#include "rs-kernel/model/objectivedesc.h"
#include "rs-kernel/model/vehicle.h"
#include "rs-kernel/model/train.h"
#include "rs-kernel/model/squad.h"
#include "rs-kernel/model/mission.h"
#include "rs-kernel/mgr/pedmanager.h"
#include "rs-kernel/mgr/weaponmanager.h"
#include "rs-kernel/mgr/vehiclemanager.h"
#include "rs-kernel/ia/behaviour.h"

namespace rs_knl {

/*!
 * Offset in the game data from the start to find the scenario section.
 */
const uint32_t kScenarioOffset = 97128;
//! Pattern for filename for mission briefing
//constexpr std::string kFileMissionPattern = "miss{}{:02}.dat";
//! Pattern for filename for mission game
//constexpr std::string kFileGamePattern = "game{:02}.dat";

class LoadMissionException : public std::exception
{
public:
    LoadMissionException( const char * Msg )
    {
        this->msg = Msg;
    }

    virtual ~LoadMissionException() throw()
    {

    }

    virtual const char * what() const throw()
    {
        return this->msg.c_str();
    }

private:
    std::string msg;
};

MissionManager::MissionManager(rs_eng::TileManager *pTileManager) : mapManager_(pTileManager) {
    pMission_ = nullptr;
}

/*!
 * Loads the briefing for the given mission id.
 * \param n Mission id
 * \return NULL if mission could not be loaded
 */
MissionBriefing *MissionManager::loadBriefing(int n) {
    std::string filename;
    // Briefing file depends on the current language
    switch(g_Ctx.currLanguage()) {
        case rs_eng::FRENCH:
            filename = std::format("miss{}{:02}.dat", "1", n);
            break;
        case rs_eng::ITALIAN:
            filename = std::format("miss{}{:02}.dat", "2", n);
            break;
        case rs_eng::GERMAN:
            filename = std::format("miss{}{:02}.dat", "3", n);
            break;
        case rs_eng::ENGLISH:
        default:
            filename = std::format("miss{}{:02}.dat", "", n);
            break;
    }

    if (!mapManager_.customDataDir().empty()) {
        std::filesystem::path customBriefing = std::filesystem::path(mapManager_.customDataDir()) / filename;
        if (!std::filesystem::exists(customBriefing)) {
            std::string defaultFilename = std::format("miss{:02}.dat", n);
            std::filesystem::path fallbackBriefing = std::filesystem::path(mapManager_.customDataDir()) / defaultFilename;
            if (std::filesystem::exists(fallbackBriefing)) {
                customBriefing = fallbackBriefing;
            }
        }
        if (std::filesystem::exists(customBriefing)) {
            MissionBriefing *p_mb = loadBriefingFromPath(customBriefing.string());
            if (!p_mb) {
                return nullptr;
            }
            LevelData::LevelDataAll level_data;
            if (!load_level_data(n, level_data)) {
                delete p_mb;
                return nullptr;
            }
            Map *p_map = mapManager_.getMap(level_data.mapinfos);
            if (!p_map) {
                delete p_mb;
                return nullptr;
            }
            p_mb->init_minimap(p_map, level_data);
            return p_mb;
        }
    }

    size_t size;
    uint8_t *data = rs_utl::File::loadOriginalFile(filename, size);
    if (data == NULL) {
        return NULL;
    }

    // Create Briefing
    MissionBriefing *p_mb = new MissionBriefing();
    // Loads briefing text
    if (!p_mb->loadBriefing(data, size)) {
        delete[] data;
        delete p_mb;
        return NULL;
    }
    delete[] data;

    // Loads the mission to get the minimap
    LevelData::LevelDataAll level_data;
    if (load_level_data(n, level_data)) {
        Map *p_map = mapManager_.getMap(level_data.mapinfos);
        if (p_map == NULL) {
            delete p_mb;
            return NULL;
        }
        p_mb->init_minimap(p_map, level_data);
    } else {
        delete p_mb;
        return NULL;
    }

    return p_mb;
}

MissionBriefing * MissionManager::loadBriefingFromPath(const std::string &briefingPath) {
    std::ifstream in(briefingPath, std::ios::binary | std::ios::ate);
    if (!in.is_open()) return nullptr;
    std::streamsize sz = in.tellg();
    if (sz <= 0) return nullptr;
    in.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<size_t>(sz) + 1, 0);
    if (!in.read(reinterpret_cast<char*>(buffer.data()), sz)) return nullptr;

    std::vector<uint8_t> unpacked;
    uint8_t *pData = buffer.data();
    int dataSize = static_cast<int>(sz) + 1;

    if (rnc::isRncCompressed(buffer.data())) {
        size_t unpLen = 0;
        if (rnc::unpackedLength(buffer.data(), unpLen) != rnc::kOk || unpLen == 0) {
            return nullptr;
        }
        unpacked.assign(unpLen + 1, 0);
        size_t realSize = 0;
        if (rnc::unpack(buffer.data(), unpacked.data(), realSize) < 0 || realSize == 0) {
            return nullptr;
        }
        pData = unpacked.data();
        dataSize = static_cast<int>(realSize) + 1;
    }

    MissionBriefing *p_mb = new MissionBriefing();
    if (!p_mb->loadBriefing(pData, dataSize)) {
        delete p_mb;
        return nullptr;
    }
    return p_mb;
}

#define copydata(x, y) memcpy(&level_data.x, data + y, sizeof(level_data.x))

int MissionManager::getMissionPaletteId(int missionId) {
    // 50 territory missions from original Syndicate campaign table (gamesession.cpp g_Blocks[50])
    static const uint8_t kMissionPalettes[51] = {
        1, // 0 (unused dummy)
        2, //  1: West Eur.
        3, //  2: FAR EAST
        4, //  3: MONGOLIA
        5, //  4: Iran
        1, //  5: California
        2, //  6: Irak
        1, //  7: India
        4, //  8: NORTHEAST TERRITORIES
        5, //  9: KAZAKHSTAN
        1, // 10: Eastern Eur.
        2, // 11: Western Australia
        3, // 12: KAMCHATKA
        4, // 13: Mozambique
        5, // 14: Peru
        1, // 15: Central Eur.
        2, // 16: GREENLAND
        3, // 17: ALASKA
        4, // 18: Ural
        5, // 19: Northern Territories
        1, // 20: Scandinavia
        2, // 21: YUKON
        3, // 22: Siberia
        4, // 23: Rockies
        5, // 24: Southern state
        1, // 25: Indonesia
        2, // 26: Mexico
        3, // 27: Zaire
        4, // 28: Algeria
        5, // 29: New England
        1, // 30: Argentina
        2, // 31: South Africa
        3, // 32: Colorado
        4, // 33: Sudan
        5, // 34: Midwest
        1, // 35: Lybia
        2, // 36: Venezuela
        3, // 37: Atlantic Accelerator
        4, // 38: Arabia
        5, // 39: NORTHWEST TERRITORIES
        1, // 40: Kenya
        2, // 41: Mauritania
        3, // 42: New found land
        4, // 43: New South Wales
        5, // 44: Colombia
        1, // 45: Nigeria
        2, // 46: Brazil
        3, // 47: Uruguay
        4, // 48: Pacific Rim
        5, // 49: Paraguay
        1  // 50: China
    };
    if (missionId >= 1 && missionId <= 50) {
        return kMissionPalettes[missionId];
    }
    return 1; // Default palette for custom / new missions
}

/*!
 * Loads a mission.
 * \param n Mission id.
 * \return NULL if Mission could not be loaded.
 */
Mission *MissionManager::loadMission(int missionId, int paletteId)
{
    LOG(Log::k_FLG_IO, "MissionManager", "loadMission()", ("loading mission %i", missionId));

    if (paletteId <= 0) {
        paletteId = getMissionPaletteId(missionId);
    }

    // Initialize LevelData structure from data read in file
    LevelData::LevelDataAll level_data;
    if (load_level_data(missionId, level_data)) {
        pMission_ = create_mission(level_data);

        if (pMission_) {
            if (pMission_->buildNavigationGraph()) {
                if (mapManager_.loadPalette(paletteId)) {
                    return pMission_;
                }   
            }
        }
    }
    destroyMission();
    return NULL;
}

void MissionManager::destroyMission() {
    if (pMission_) {
        LOG(Log::k_FLG_IO, "MissionManager", "destroyMission()", ("destroy mission"));
        delete pMission_;
        pMission_ = NULL;
    }
}

/*!
 * Creates a Mission object from the LevelDataAll structure and fills the overlay for the
 * briefing minimap.
 */
bool MissionManager::load_level_data(int n, LevelData::LevelDataAll &level_data, const std::string &customPath) {
    size_t size = 0;
    uint8_t *data = nullptr;

    if (!customPath.empty()) {
        std::ifstream in(customPath, std::ios::binary | std::ios::ate);
        if (!in.is_open()) {
            RSERR(Log::k_FLG_IO, "MissionManager", "load_level_data()", ("Failed to open file %s", customPath.c_str()));
            return false;
        }
        std::streamsize fileSize = in.tellg();
        if (fileSize <= 0) return false;
        in.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));
        if (!in.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
            return false;
        }

        if (rnc::isRncCompressed(buffer.data())) {
            size_t unpLen = 0;
            if (rnc::unpackedLength(buffer.data(), unpLen) == rnc::kOk && unpLen > 0) {
                data = new uint8_t[unpLen + 1];
                size_t realSize = 0;
                if (rnc::unpack(buffer.data(), data, realSize) >= 0) {
                    size = realSize;
                } else {
                    delete[] data;
                    return false;
                }
            }
        } else {
            size = buffer.size();
            data = new uint8_t[size + 1];
            memcpy(data, buffer.data(), size);
        }
    } else {
        std::string filename = std::format("game{:02}.dat", n);
        if (!mapManager_.customDataDir().empty()) {
            std::filesystem::path customGame = std::filesystem::path(mapManager_.customDataDir()) / filename;
            if (std::filesystem::exists(customGame)) {
                return load_level_data(n, level_data, customGame.string());
            }
        }
        LOG(Log::k_FLG_IO, "MissionManager", "load_level_data()", ("Loading file %s", filename.c_str()));
        data = rs_utl::File::loadOriginalFile(filename, size);
    }

    if (data == NULL) {
        return false;
    }

#if 1
    hackMissions(n, data);
#endif

    // Initialize LevelData structure from data read in file
    memset(&level_data, 0, sizeof(level_data));
    if (size >= 114060) {
        copydata(u01, 0);
        copydata(map, 6);
        copydata(offset_ref, 32774);
        copydata(people, 32776);
        copydata(cars, 56328);
        copydata(statics, 59016);
        copydata(weapons, 71016);
        copydata(sfx, 89448);
        copydata(scenarios, 97128);
        copydata(u09, 113512);
        copydata(mapinfos, 113960);
        copydata(objectives, 113974);
        copydata(u11, 114058);
    } else if (size >= sizeof(LevelData::LevelDataAll)) {
        memcpy(&level_data, data, sizeof(LevelData::LevelDataAll));
    }

    delete[] data;
    return true;
}

/*!
 *
 */
void MissionManager::hackMissions(int missionId, uint8_t *data) {
    if (missionId == 1) { // Western Europe
        hackScenarioLocation(data, 440, kScenarioAttTileZ, 2);  // Ped 8
    } else if (missionId == 2) { // Far east
        // Change the second destination of the car for ped #40
        // because in original it is not walkable
        uint8_t *scen_start = data + kScenarioOffset + 8 * 57;
        scen_start[5] = 86; // change only the ty coord
        // Same for ped #42
        scen_start = data + kScenarioOffset + 8 * 55;
        scen_start[5] = 86; // change only the ty coord
        // Same for ped #44
        scen_start = data + kScenarioOffset + 8 * 45;
        scen_start[5] = 96; // change only the ty coord
        // Same for ped #45
        scen_start = data + kScenarioOffset + 8 * 59;
        scen_start[4] = 114; // change only the tx coord
        // Same for ped #46
        scen_start = data + kScenarioOffset + 8 * 63;
        scen_start[4] = 110; // change only the tx coord
    } else if (missionId == 6) { // Iraq
        // Change the second destination of the car 14 for ped 146
        hackScenarioLocation(data, 384, kScenarioAttTileX, 90);
        hackScenarioLocation(data, 384, kScenarioAttTileZ, 6);
        // Change the second destination of the car 15 for ped 148
        hackScenarioLocation(data, 400, kScenarioAttTileY, 54);
        hackScenarioLocation(data, 400, kScenarioAttTileZ, 6);
        // Change the second destination of the car 16 for ped 171
        hackScenarioLocation(data, 416, kScenarioAttTileY, 54);
        hackScenarioLocation(data, 416, kScenarioAttTileZ, 6);
        // Change the second destination of the car 17 for ped 166
        hackScenarioLocation(data, 432, kScenarioAttTileX, 94);
        hackScenarioLocation(data, 432, kScenarioAttTileZ, 6);
        // Change the second destination of the car 18 for ped 159
        hackScenarioLocation(data, 448, kScenarioAttTileX, 130);
        hackScenarioLocation(data, 448, kScenarioAttTileZ, 6);
        // Change the second destination of the car 9 for ped 143
        hackScenarioLocation(data, 368, kScenarioAttTileY, 50);
        // Change the second destination of the car 11 for ped 131
        hackScenarioLocation(data, 296, kScenarioAttTileX, 200);
    } else if (missionId == 10) { // Western Europe
        // Change the second destination of the car for ped #168
        // as in original scenario that destination seems non walkable
        uint8_t *scen_start = data + kScenarioOffset + 8 * 8;
        scen_start[4] = 74;
        scen_start[5] = 120;
        scen_start[6] = 2;
    } else if (missionId == 22) { // Siberia
        // Change destination of second action for ped #192 (police)
        // because in original it is not walkable
        uint8_t *scen_start = data + kScenarioOffset + 8 * 7;
        scen_start[5] = 120;
        scen_start[6] = 3;
    } else if (missionId == 40) { // Kenya
        // adding additional walking points to scenarios
        uint8_t *scen_start = data + kScenarioOffset + 8 * 86;
        // coord offsets changing for next point
        scen_start[4] = ((scen_start[4] & 0xFE) | 1);
        scen_start[5] = (scen_start[5] & 0xFE);

        scen_start = data + kScenarioOffset + 8 * 87;
        rs_utl::WRITE_LE_UINT16(scen_start, 90 * 8);
        scen_start = data + kScenarioOffset + 8 * 90;
        rs_utl::WRITE_LE_UINT16(scen_start, 91 *8);
        scen_start += 4;
        // coords x = 72,ox = 128, y = 32, oy = 128, z = 2
        scen_start[0] = (72 << 1) | 1;
        scen_start[1] = (32 << 1) | 1;
        // type 1 = reach location
        scen_start[3] = 1;

        scen_start += 4;
        rs_utl::WRITE_LE_UINT16(scen_start, 92 *8);
        scen_start += 4;
        // coords x = 61,ox = 0, y = 32, oy = 0, z = 2
        scen_start[0] = (62 << 1) | 1;
        scen_start[1] = (57 << 1) | 1;
        // type 1 = reach location
        scen_start[3] = 1;

        scen_start += 4;
        rs_utl::WRITE_LE_UINT16(scen_start, 88 *8);
        scen_start += 4;
        // coords x = 42,ox = 0, y = 59, oy = 0, z = 2
        scen_start[0] = (42 << 1) | 1;
        scen_start[1] = (59 << 1);
        // type 1 = reach location
        scen_start[3] = 1;
    }
}

/*!
 * Change an attribute of a scenario
 * @param data Level data
 * @param offset Offset of the scenario to hack
 * @param coord The type of attribute to change
 * @param newCoord The new value
 */
void MissionManager::hackScenarioLocation(uint8_t *data, int offset, ScenarioAttribute coord, uint8_t newCoord) {
    uint8_t *scen_start = data + kScenarioOffset + offset;
    scen_start[coord] = newCoord;
}

namespace {
uint8_t weaponTypeToSubType(Weapon::WeaponType wType) {
    switch (wType) {
        case Weapon::Persuadatron: return 0x01;
        case Weapon::Pistol:       return 0x02;
        case Weapon::GaussGun:     return 0x03;
        case Weapon::Shotgun:      return 0x04;
        case Weapon::Uzi:          return 0x05;
        case Weapon::Minigun:      return 0x06;
        case Weapon::Laser:        return 0x07;
        case Weapon::Flamer:       return 0x08;
        case Weapon::LongRange:    return 0x09;
        case Weapon::Scanner:      return 0x0A;
        case Weapon::MediKit:      return 0x0B;
        case Weapon::TimeBomb:     return 0x0C;
        case Weapon::AccessCard:   return 0x0D;
        case Weapon::AirStrike:    return 0x0E;
        case Weapon::CloneShield:  return 0x0F;
        case Weapon::EnergyShield: return 0x11;
        default:                   return 0x02;
    }
}

uint8_t *spatialPrevPtrForOffset(LevelData::LevelDataAll &levelData, uint16_t off) {
    if (off >= 0x0002 && off < 0x0002 + 256 * 92 && ((off - 0x0002) % 92) == 0) {
        return levelData.people[(off - 0x0002) / 92].offset_prev;
    }
    if (off >= 0x5C02 && off < 0x5C02 + 64 * 42 && ((off - 0x5C02) % 42) == 0) {
        return levelData.cars[(off - 0x5C02) / 42].offset_prev;
    }
    if (off >= 0x6682 && off < 0x6682 + 400 * 30 && ((off - 0x6682) % 30) == 0) {
        return levelData.statics[(off - 0x6682) / 30].offset_prev;
    }
    if (off >= 0x9562 && off < 0x9562 + 512 * 36 && ((off - 0x9562) % 36) == 0) {
        return levelData.weapons[(off - 0x9562) / 36].offset_prev;
    }
    return nullptr;
}

void linkEntityAtTile(LevelData::LevelDataAll &levelData,
                      uint16_t entityOffset,
                      uint8_t tx,
                      uint8_t ty,
                      uint8_t *offsetNextPtr,
                      uint8_t *offsetPrevPtr) {
    if (tx >= 128 || ty >= 128) {
        return;
    }
    size_t tileIdx = static_cast<size_t>(ty) * 128 + static_cast<size_t>(tx);
    uint8_t *headBytes = levelData.map.objs + tileIdx * 2;
    uint16_t oldHead = rs_utl::READ_LE_UINT16(headBytes);

    rs_utl::WRITE_LE_UINT16(offsetPrevPtr, 0);
    rs_utl::WRITE_LE_UINT16(offsetNextPtr, oldHead);
    if (oldHead != 0) {
        if (uint8_t *oldPrevPtr = spatialPrevPtrForOffset(levelData, oldHead)) {
            rs_utl::WRITE_LE_UINT16(oldPrevPtr, entityOffset);
        }
    }
    rs_utl::WRITE_LE_UINT16(headBytes, entityOffset);
}

/*!
 * Rebuilds the 128x128 tile spatial doubly-linked list (levelData.map.objs and
 * offset_next/offset_prev on all active People, Cars, Statics, and Weapons)
 * required by original DOS Syndicate (SYNDICAT/MAIN.EXE 0x1d450 & 0x29580).
 */
void rebuildSpatialMapLinks(LevelData::LevelDataAll &levelData) {
    std::memset(levelData.map.objs, 0, sizeof(levelData.map.objs));

    for (size_t i = 0; i < 256; ++i) {
        rs_utl::WRITE_LE_UINT16(levelData.people[i].offset_next, 0);
        rs_utl::WRITE_LE_UINT16(levelData.people[i].offset_prev, 0);
    }
    for (size_t i = 0; i < 64; ++i) {
        rs_utl::WRITE_LE_UINT16(levelData.cars[i].offset_next, 0);
        rs_utl::WRITE_LE_UINT16(levelData.cars[i].offset_prev, 0);
    }
    for (size_t i = 0; i < 400; ++i) {
        rs_utl::WRITE_LE_UINT16(levelData.statics[i].offset_next, 0);
        rs_utl::WRITE_LE_UINT16(levelData.statics[i].offset_prev, 0);
    }
    for (size_t i = 0; i < 512; ++i) {
        rs_utl::WRITE_LE_UINT16(levelData.weapons[i].offset_next, 0);
        rs_utl::WRITE_LE_UINT16(levelData.weapons[i].offset_prev, 0);
    }

    for (size_t i = 0; i < 256; ++i) {
        auto &p = levelData.people[i];
        if (p.type != 0x01) continue;
        linkEntityAtTile(levelData, static_cast<uint16_t>(0x0002 + i * 92),
                         p.mapposx[1], p.mapposy[1], p.offset_next, p.offset_prev);
    }
    for (size_t i = 0; i < 64; ++i) {
        auto &c = levelData.cars[i];
        if (c.type != 0x02) continue;
        linkEntityAtTile(levelData, static_cast<uint16_t>(0x5C02 + i * 42),
                         c.mapposx[1], c.mapposy[1], c.offset_next, c.offset_prev);
    }
    for (size_t i = 0; i < 400; ++i) {
        auto &s = levelData.statics[i];
        if (s.type == 0 || s.desc == 0) continue;
        linkEntityAtTile(levelData, static_cast<uint16_t>(0x6682 + i * 30),
                         s.mapposx[1], s.mapposy[1], s.offset_next, s.offset_prev);
    }
    for (size_t i = 0; i < 512; ++i) {
        auto &w = levelData.weapons[i];
        if (w.type != 0x04 || w.desc == 0) continue;
        linkEntityAtTile(levelData, static_cast<uint16_t>(0x9562 + i * 36),
                         w.mapposx[1], w.mapposy[1], w.offset_next, w.offset_prev);
    }
}

std::vector<uint16_t> loadHstaStartFrames() {
    size_t sz = 0;
    uint8_t *raw = rs_utl::File::loadOriginalFile("hsta-0.ani", sz);
    std::vector<uint16_t> table;
    if (!raw) {
        return table;
    }
    size_t count = sz / 2;
    table.resize(count);
    for (size_t i = 0; i < count; ++i) {
        table[i] = rs_utl::READ_LE_UINT16(raw + i * 2);
    }
    delete[] raw;
    return table;
}

uint16_t lookupHstaStartFrame(const std::vector<uint16_t> &hstaTable, uint16_t animIdx) {
    if (animIdx < hstaTable.size()) {
        return hstaTable[animIdx];
    }
    return 0;
}
}

bool MissionManager::missionRequiresAmericanRevolt(const LevelData::LevelDataAll &levelData) {
    for (size_t i = 0; i < 512; ++i) {
        uint8_t desc = levelData.weapons[i].desc;
        if (desc == 0x01 || desc == 0x04 || desc == 0x05) {
            uint8_t st = levelData.weapons[i].sub_type;
            if (st == 0x0E || st == 0x0F) { // AirStrike or CloneShield
                return true;
            }
        }
    }
    return false;
}

bool MissionManager::save_level_data(int n, const LevelData::LevelDataAll &level_data, const std::string &customPath) {
    std::string filepath = customPath;
    if (filepath.empty()) {
        filepath = rs_utl::File::getOriginalDataFullPath(std::format("game{:02}.dat", n));
    }

    std::ofstream out(filepath, std::ios::binary);
    if (!out.is_open()) {
        RSERR(Log::k_FLG_IO, "MissionManager", "save_level_data", ("Failed to open file %s for writing", filepath.c_str()));
        return false;
    }

    std::vector<uint8_t> packed;
    rnc::RncRetCode rc = rnc::pack(reinterpret_cast<const uint8_t*>(&level_data), sizeof(level_data), packed);
    if (rc == rnc::kOk && !packed.empty()) {
        out.write(reinterpret_cast<const char*>(packed.data()), packed.size());
    } else {
        RSERR(Log::k_FLG_IO, "MissionManager", "save_level_data", ("RNC pack failed (%d), falling back to raw save", (int)rc));
        out.write(reinterpret_cast<const char*>(&level_data), sizeof(level_data));
    }
    out.close();
    return true;
}

bool MissionManager::exportMissionToBinary(Mission *pMission, int missionId, const std::string &customPath) {
    if (!pMission) return false;

    if (pMission->numPeds() > 256) {
        RSERR(Log::k_FLG_IO, "MissionManager", "exportMissionToBinary",
            ("Cannot export mission: pedestrian count %zu exceeds 256", pMission->numPeds()));
        return false;
    }
    if (pMission->numVehicles() > 64) {
        RSERR(Log::k_FLG_IO, "MissionManager", "exportMissionToBinary",
            ("Cannot export mission: vehicle count %zu exceeds 64", pMission->numVehicles()));
        return false;
    }
    if (pMission->numStatics() > 400) {
        RSERR(Log::k_FLG_IO, "MissionManager", "exportMissionToBinary",
            ("Cannot export mission: static objects count %zu exceeds 400", pMission->numStatics()));
        return false;
    }
    if (pMission->numObjectives() > 6) {
        RSERR(Log::k_FLG_IO, "MissionManager", "exportMissionToBinary",
            ("Cannot export mission: objectives count %zu exceeds 6", pMission->numObjectives()));
        return false;
    }

    auto pLd = std::make_unique<LevelData::LevelDataAll>(pMission->getRawLevelData());
    LevelData::LevelDataAll &levelData = *pLd;
    levelData.mapinfos.unkn1[0] = static_cast<uint8_t>(pMission->techLevel());
    const std::vector<uint16_t> hstaTable = loadHstaStartFrames();

    // Ensure mapinfos has valid scroll limits if not set or invalid
    if (pMission->map()) {
        int maxTx = pMission->map()->maxTx();
        int maxTy = pMission->map()->maxTy();
        uint16_t curMinX = rs_utl::READ_LE_UINT16(levelData.mapinfos.min_x);
        uint16_t curMaxX = rs_utl::READ_LE_UINT16(levelData.mapinfos.max_x);
        uint16_t curMinY = rs_utl::READ_LE_UINT16(levelData.mapinfos.min_y);
        uint16_t curMaxY = rs_utl::READ_LE_UINT16(levelData.mapinfos.max_y);
        if (curMaxX <= curMinX || curMaxY <= curMinY) {
            rs_utl::WRITE_LE_UINT16(levelData.mapinfos.min_x, 0);
            rs_utl::WRITE_LE_UINT16(levelData.mapinfos.min_y, 0);
            rs_utl::WRITE_LE_UINT16(levelData.mapinfos.max_x, static_cast<uint16_t>((maxTx - 1) * 2));
            rs_utl::WRITE_LE_UINT16(levelData.mapinfos.max_y, static_cast<uint16_t>((maxTy - 1) * 2));
        }
        uint16_t mapId = pMission->map()->id();
        if (mapId > 0) {
            rs_utl::WRITE_LE_UINT16(levelData.mapinfos.map, mapId);
        }
    }

    // 1. Squad spawn points
    for (size_t i = 0; i < 4; ++i) {
        TilePoint pt = pMission->getSquadSpawnPoint(i);
        uint8_t ori = pMission->getSquadSpawnOrientation(i);
        levelData.people[i].type = 0x01;
        levelData.people[i].type_ped = PedInstance::kPedTypeAgent;
        levelData.people[i].location = LevelData::kPeopleLocOnMap;
        // Squad agents must start in standing state (0x00); bit 0x10 puts DOS MAIN.EXE into wandering AI mode 0x1D (0x2d9b7).
        levelData.people[i].state = 0x00;
        levelData.people[i].mapposx[0] = static_cast<uint8_t>(pt.ox);
        levelData.people[i].mapposx[1] = static_cast<uint8_t>(pt.tx);
        levelData.people[i].mapposy[0] = static_cast<uint8_t>(pt.oy);
        levelData.people[i].mapposy[1] = static_cast<uint8_t>(pt.ty);
        rs_utl::WRITE_LE_UINT16(levelData.people[i].mapposz, static_cast<uint16_t>((pt.tz << 7) | (pt.oz & 0x7F)));
        levelData.people[i].orientation = ori;
        rs_utl::WRITE_LE_UINT16(levelData.people[i].index_base_anim, 1);
        uint16_t curAnim = rs_utl::READ_LE_UINT16(levelData.people[i].index_current_anim);
        if (curAnim == 0) {
            uint16_t dirIdx = static_cast<uint16_t>(((ori + 0x10) & 0xE0) >> 5);
            curAnim = static_cast<uint16_t>(1 + dirIdx);
            rs_utl::WRITE_LE_UINT16(levelData.people[i].index_current_anim, curAnim);
        }
        if (rs_utl::READ_LE_UINT16(levelData.people[i].index_current_frame) == 0) {
            rs_utl::WRITE_LE_UINT16(levelData.people[i].index_current_frame,
                                    lookupHstaStartFrame(hstaTable, curAnim));
        }
        rs_utl::WRITE_LE_UINT16(levelData.people[i].health, 16);
        if (levelData.people[i].adrena_amount == 0 &&
            levelData.people[i].inteli_amount == 0 &&
            levelData.people[i].percep_amount == 0) {
            levelData.people[i].adrena_amount = levelData.people[i].adrena_dependency = levelData.people[i].adrena_effect = 0x80;
            levelData.people[i].inteli_amount = levelData.people[i].inteli_dependency = levelData.people[i].inteli_effect = 0x80;
            levelData.people[i].percep_amount = levelData.people[i].percep_dependency = levelData.people[i].percep_effect = 0x80;
        }
    }

    // Collision avoidance for squad spawn points on the same tile:
    // If multiple agents share the exact same tile and have overlapping sub-tile positions (dx < 48 && dy < 48),
    // spread them across distinct quadrants so they don't spawn inside one another.
    static const int kSubTileSpread[4][2] = {
        { 64, 64 },
        { 192, 192 },
        { 192, 64 },
        { 64, 192 }
    };
    for (size_t i = 0; i < 4; ++i) {
        if (levelData.people[i].location == LevelData::kPeopleLocNotVisible) continue;
        int tx_i = levelData.people[i].mapposx[1];
        int ty_i = levelData.people[i].mapposy[1];
        int ox_i = levelData.people[i].mapposx[0];
        int oy_i = levelData.people[i].mapposy[0];

        bool collision = false;
        int sameTileCount = 0;
        for (size_t j = 0; j < i; ++j) {
            if (levelData.people[j].location == LevelData::kPeopleLocNotVisible) continue;
            int tx_j = levelData.people[j].mapposx[1];
            int ty_j = levelData.people[j].mapposy[1];
            if (tx_i == tx_j && ty_i == ty_j) {
                sameTileCount++;
                int ox_j = levelData.people[j].mapposx[0];
                int oy_j = levelData.people[j].mapposy[0];
                if (std::abs(ox_i - ox_j) < 48 && std::abs(oy_i - oy_j) < 48) {
                    collision = true;
                }
            }
        }
        if (collision) {
            int spreadIdx = (sameTileCount < 4) ? sameTileCount : 3;
            levelData.people[i].mapposx[0] = static_cast<uint8_t>(kSubTileSpread[spreadIdx][0]);
            levelData.people[i].mapposy[0] = static_cast<uint8_t>(kSubTileSpread[spreadIdx][1]);
        }
    }

    // 2. Active non-squad peds
    std::unordered_map<PedInstance*, uint16_t> pedToSlot;
    std::vector<bool> slotUsed(256, false);
    for (size_t s = 0; s < 8; ++s) {
        slotUsed[s] = true;
    }

    // First pass: identify existing slots for peds with valid IDs
    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        PedInstance *p = pMission->ped(i);
        if (!p || p->isOurAgent()) continue;
        uint16_t pid = p->id();
        if (p->factionOverride() >= 0 && (pid / 8) != p->factionOverride()) {
            // Faction override changed slot range, re-slot in second pass
            continue;
        }
        if (pid >= 8 && pid < 256 && !slotUsed[pid]) {
            pedToSlot[p] = pid;
            slotUsed[pid] = true;
        }
    }

    // Second pass: assign slots for peds needing new slots
    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        PedInstance *p = pMission->ped(i);
        if (!p || p->isOurAgent()) continue;
        if (pedToSlot.find(p) == pedToSlot.end()) {
            int desiredFaction = p->factionOverride();
            bool assigned = false;
            if (desiredFaction >= 1 && desiredFaction < 32) {
                size_t startSlot = desiredFaction * 8;
                size_t endSlot = std::min(size_t(256), startSlot + 8);
                for (size_t s = startSlot; s < endSlot; ++s) {
                    if (!slotUsed[s]) {
                        pedToSlot[p] = static_cast<uint16_t>(s);
                        slotUsed[s] = true;
                        p->setId(static_cast<uint16_t>(s));
                        assigned = true;
                        break;
                    }
                }
            }
            if (!assigned) {
                for (size_t s = 8; s < 256; ++s) {
                    if (!slotUsed[s]) {
                        pedToSlot[p] = static_cast<uint16_t>(s);
                        slotUsed[s] = true;
                        p->setId(static_cast<uint16_t>(s));
                        break;
                    }
                }
            }
        }
    }

    // Clear only genuinely unused non-squad slots
    for (size_t s = 8; s < 256; ++s) {
        if (!slotUsed[s]) {
            memset(&levelData.people[s], 0, sizeof(LevelData::People));
        }
    }

    // Re-serialize scenarios table for all active peds
    for (size_t i = 0; i < 2048; ++i) {
        memset(&levelData.scenarios[i], 0, sizeof(LevelData::Scenarios));
    }
    size_t scenIdx = 1; // Index 0 (byte offset 0) is reserved as null/none
    for (auto &[p, pid] : pedToSlot) {
        LevelData::People &pedref = levelData.people[pid];
        pedref.type = 0x01;
        pedref.type_ped = static_cast<uint8_t>(p->type());
        if (pedref.location != LevelData::kPeopleLocInVehicle) {
            pedref.location = LevelData::kPeopleLocOnMap;
        }
        pedref.state = p->isDead() ? LevelData::kPeopleStateDead : p->initialLevelState();
        pedref.mapposx[0] = static_cast<uint8_t>(p->position().ox);
        pedref.mapposx[1] = static_cast<uint8_t>(p->position().tx);
        pedref.mapposy[0] = static_cast<uint8_t>(p->position().oy);
        pedref.mapposy[1] = static_cast<uint8_t>(p->position().ty);
        rs_utl::WRITE_LE_UINT16(pedref.mapposz, static_cast<uint16_t>((p->position().tz << 7) | (p->position().oz & 0x7F)));
        pedref.orientation = static_cast<uint8_t>(p->direction());
        rs_utl::WRITE_LE_INT16(pedref.health, static_cast<int16_t>(p->health()));
        uint16_t bAnim = p->baseAnim();
        if (bAnim == 0) {
            bAnim = PedManager::getDefaultBaseAnim(p->type());
        }
        rs_utl::WRITE_LE_UINT16(pedref.index_base_anim, bAnim);
        uint16_t curAnim = rs_utl::READ_LE_UINT16(pedref.index_current_anim);
        if (curAnim == 0) {
            uint16_t dirIdx = static_cast<uint16_t>(((p->direction() + 0x10) & 0xE0) >> 5);
            curAnim = static_cast<uint16_t>(bAnim + dirIdx);
            rs_utl::WRITE_LE_UINT16(pedref.index_current_anim, curAnim);
        }
        if (rs_utl::READ_LE_UINT16(pedref.index_current_frame) == 0) {
            rs_utl::WRITE_LE_UINT16(pedref.index_current_frame,
                                    lookupHstaStartFrame(hstaTable, curAnim));
        }
        rs_utl::WRITE_LE_UINT16(pedref.offset_equipment, 0);
        if (pedref.adrena_amount == 0 && pedref.inteli_amount == 0 && pedref.percep_amount == 0) {
            pedref.adrena_amount = pedref.adrena_dependency = pedref.adrena_effect = 0x80;
            pedref.inteli_amount = pedref.inteli_dependency = pedref.inteli_effect = 0x80;
            pedref.percep_amount = pedref.percep_dependency = pedref.percep_effect = 0x80;
        }

        // Cyberware modifications for enemy agents
        if (p->type() == PedInstance::kPedTypeAgent && !p->isOurAgent()) {
            uint16_t mask = p->getModsInfoBitmask();
            if (p->isCyberwareOverridden()) {
                mask |= 0x8000;
            }
            rs_utl::WRITE_LE_UINT16(pedref.mods_info, mask);
        } else {
            rs_utl::WRITE_LE_UINT16(pedref.mods_info, 0);
        }

        if (p->scriptSteps().empty()) {
            rs_utl::WRITE_LE_UINT16(pedref.offset_scenario_start, 0);
        } else {
            if (scenIdx >= 2048) {
                rs_utl::WRITE_LE_UINT16(pedref.offset_scenario_start, 0);
                continue;
            }
            uint16_t startByteOffset = static_cast<uint16_t>(scenIdx * 8);
            rs_utl::WRITE_LE_UINT16(pedref.offset_scenario_start, startByteOffset);

            const auto &steps = p->scriptSteps();
            for (size_t s = 0; s < steps.size() && scenIdx < 2048; ++s) {
                const auto &step = steps[s];
                LevelData::Scenarios &sc = levelData.scenarios[scenIdx];
                sc.type = step.type;
                sc.tilex = static_cast<uint8_t>((step.targetPos.tx << 1) | ((step.targetPos.ox >= 64) ? 1 : 0));
                sc.tiley = static_cast<uint8_t>((step.targetPos.ty << 1) | ((step.targetPos.oy >= 64) ? 1 : 0));
                sc.tilez = static_cast<uint8_t>(step.targetPos.tz);

                if (step.type == LevelData::kScenarioTypeUseVehicle || step.type == 2) {
                    sc.tilex = 0;
                    sc.tiley = 0;
                    sc.tilez = 0;
                    if (step.targetEntityId >= 0) {
                        uint16_t vOff = static_cast<uint16_t>(0x5C02 + step.targetEntityId * 42);
                        rs_utl::WRITE_LE_UINT16(sc.offset_object, vOff);
                    }
                } else if (step.type == LevelData::kScenarioTypeLeaveVehicle || step.type == 6) {
                    sc.tilex = 0;
                    sc.tiley = 0;
                    sc.tilez = 0;
                    rs_utl::WRITE_LE_UINT16(sc.offset_object, 0);
                } else if (step.type == LevelData::kScenarioTypeWait || step.type == 5) {
                    sc.tilex = 0;
                    sc.tiley = 0;
                    sc.tilez = 0;
                    uint16_t ticks = static_cast<uint16_t>(step.param / 100);
                    rs_utl::WRITE_LE_UINT16(sc.offset_object, ticks);
                } else if (step.type == LevelData::kScenarioTypeReset || step.type == 9) {
                    sc.tilex = 0;
                    sc.tiley = 0;
                    sc.tilez = 0;
                    rs_utl::WRITE_LE_UINT16(sc.offset_object, 0);
                } else if (step.type == LevelData::kScenarioTypeTrainWait || step.type == 10) {
                    sc.tilex = 0;
                    sc.tiley = 0;
                    sc.tilez = 0;
                    rs_utl::WRITE_LE_UINT16(sc.offset_object, 100);
                } else if (step.type == LevelData::kScenarioTypeTrigger || step.type == 8) {
                    rs_utl::WRITE_LE_UINT16(sc.offset_object, static_cast<uint16_t>(step.param > 0 ? step.param : 6 * 256));
                }

                if (s + 1 < steps.size() && (scenIdx + 1) < 2048) {
                    uint16_t nextByteOffset = static_cast<uint16_t>((scenIdx + 1) * 8);
                    rs_utl::WRITE_LE_UINT16(sc.next, nextByteOffset);
                } else {
                    rs_utl::WRITE_LE_UINT16(sc.next, 0);
                }
                scenIdx++;
            }
        }
    }

    // 3. Vehicles
    for (size_t i = 0; i < 64; ++i) {
        std::memset(&levelData.cars[i], 0, sizeof(LevelData::Cars));
    }
    for (size_t i = 0; i < pMission->numVehicles() && i < 64; ++i) {
        Vehicle *v = pMission->vehicle(i);
        if (!v) continue;
        LevelData::Cars &car = levelData.cars[i];
        car.type = 0x02;
        car.desc = 0x04;
        car.sub_type = v->getType();
        car.mapposx[0] = static_cast<uint8_t>(v->position().ox);
        car.mapposx[1] = static_cast<uint8_t>(v->position().tx);
        car.mapposy[0] = static_cast<uint8_t>(v->position().oy);
        car.mapposy[1] = static_cast<uint8_t>(v->position().ty);
        rs_utl::WRITE_LE_UINT16(car.mapposz, static_cast<uint16_t>((v->position().tz << 7) | (v->position().oz & 0x7F)));
        car.orientation = static_cast<uint8_t>(v->direction());
        rs_utl::WRITE_LE_INT16(car.health, static_cast<int16_t>(v->health()));
        rs_utl::WRITE_LE_UINT16(car.offset_of_driver, 0);

        uint16_t baseAnim = 446;
        switch (v->getType()) {
            case 0x01: baseAnim = 400; break; // Large Armored
            case 0x05: baseAnim = 416; break; // Train Head
            case 0x09: baseAnim = (v->baseSpriteAnim() != 0) ? v->baseSpriteAnim() : 428; break; // Train Body (428 or 434)
            case 0x0D: baseAnim = 446; break; // Regular Car
            case 0x11: baseAnim = 518; break; // Firefighter
            case 0x1C: baseAnim = 534; break; // Small Armored
            case 0x24: baseAnim = 470; break; // Police
            case 0x28: baseAnim = 494; break; // Medics / Ambulance
            default:   baseAnim = 446; break;
        }
        int dir = (v->direction() >> 5);
        uint16_t curAnim = static_cast<uint16_t>(baseAnim + dir);
        if (v->getType() == Vehicle::kVehicleTypeTrainBody) {
            uint16_t variant = (baseAnim >= 440) ? 2 : ((baseAnim >= 434) ? 1 : 0);
            rs_utl::WRITE_LE_UINT16(car.index_base_anim, variant);
        } else {
            rs_utl::WRITE_LE_UINT16(car.index_base_anim, baseAnim);
        }
        rs_utl::WRITE_LE_UINT16(car.index_current_anim, curAnim);
        rs_utl::WRITE_LE_UINT16(car.index_current_frame, lookupHstaStartFrame(hstaTable, curAnim));

        if (PedInstance *driver = v->getDriver()) {
            auto it = pedToSlot.find(driver);
            if (it != pedToSlot.end()) {
                uint16_t pid = it->second;
                rs_utl::WRITE_LE_UINT16(car.offset_of_driver, static_cast<uint16_t>(pid * 92 + 2));
                levelData.people[pid].location = LevelData::kPeopleLocInVehicle;
                rs_utl::WRITE_LE_UINT16(levelData.people[pid].offset_of_vehicle, static_cast<uint16_t>(0x5C02 + i * 42));
            }
        }
    }

    // 4. Statics
    for (size_t i = 0; i < 400; ++i) {
        std::memset(&levelData.statics[i], 0, sizeof(LevelData::Statics));
    }
    for (size_t i = 0; i < pMission->numStatics() && i < 400; ++i) {
        Static *s = pMission->statics(i);
        if (!s) continue;
        LevelData::Statics &st = levelData.statics[i];
        st.desc = 0x04;
        st.type = 0x05;
        st.sub_type = s->subType();
        st.mapposx[0] = static_cast<uint8_t>(s->position().ox);
        st.mapposx[1] = static_cast<uint8_t>(s->position().tx);
        st.mapposy[0] = static_cast<uint8_t>(s->position().oy);
        st.mapposy[1] = static_cast<uint8_t>(s->position().ty);
        rs_utl::WRITE_LE_UINT16(st.mapposz, static_cast<uint16_t>((s->position().tz << 7) | (s->position().oz & 0x7F)));
        st.orientation = (s->orientation() == Static::kStaticOrientationEW) ? 0x40 : 0x00;
        uint16_t banim = s->baseAnim();
        if (banim == 0) banim = s->getDrawAnimation();
        if (st.sub_type == 0x16) {
            uint16_t variant = 1;
            if (banim >= 227 && banim < 236) {
                variant = (banim - 227) / 3 + 1;
            } else if (banim >= 1 && banim <= 3) {
                variant = banim;
                banim = 227 + (variant - 1) * 3;
            }
            rs_utl::WRITE_LE_UINT16(st.index_base_anim, variant);
            rs_utl::WRITE_LE_UINT16(st.index_current_anim, banim);
        } else {
            rs_utl::WRITE_LE_UINT16(st.index_base_anim, banim);
            rs_utl::WRITE_LE_UINT16(st.index_current_anim, banim);
        }
        rs_utl::WRITE_LE_UINT16(st.index_current_frame, lookupHstaStartFrame(hstaTable, banim));
    }

    // 5. Weapons
    for (size_t i = 0; i < 512; ++i) {
        std::memset(&levelData.weapons[i], 0, sizeof(LevelData::Weapons));
    }
    size_t wIdx = 0;
    for (size_t i = 0; i < pMission->numWeaponsOnGround() && wIdx < 512; ++i, ++wIdx) {
        WeaponInstance *w = pMission->weaponOnGround(i);
        if (!w) continue;
        LevelData::Weapons &wref = levelData.weapons[wIdx];
        wref.desc = 0x04;
        wref.type = 0x04;
        wref.sub_type = weaponTypeToSubType(w->type());
        wref.mapposx[0] = static_cast<uint8_t>(w->position().ox);
        wref.mapposx[1] = static_cast<uint8_t>(w->position().tx);
        wref.mapposy[0] = static_cast<uint8_t>(w->position().oy);
        wref.mapposy[1] = static_cast<uint8_t>(w->position().ty);
        rs_utl::WRITE_LE_UINT16(wref.mapposz, static_cast<uint16_t>((w->position().tz << 7) | (w->position().oz & 0x7F)));
        uint16_t anim = w->getDrawAnimation();
        rs_utl::WRITE_LE_UINT16(wref.index_current_anim, anim);
        rs_utl::WRITE_LE_UINT16(wref.index_current_frame, lookupHstaStartFrame(hstaTable, anim));
        rs_utl::WRITE_LE_INT16(wref.unkn5, static_cast<int16_t>(w->ammoRemaining()));
    }
    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        PedInstance *p = pMission->ped(i);
        if (!p || p->isOurAgent()) continue;
        auto slotIt = pedToSlot.find(p);
        if (slotIt == pedToSlot.end()) continue;
        uint16_t pid = slotIt->second;
        uint16_t pedOff = static_cast<uint16_t>(pid * 92 + 2);
        uint16_t prevInvOff = pedOff;
        size_t firstPedWIdx = wIdx;
        size_t emittedCount = 0;

        for (size_t w = 0; w < p->numWeapons() && wIdx < 512; ++w) {
            WeaponInstance *pw = p->weapon(w);
            if (!pw) continue;
            LevelData::Weapons &wref = levelData.weapons[wIdx];
            wref.desc = 0x05;
            wref.type = 0x04;
            wref.sub_type = weaponTypeToSubType(pw->type());
            rs_utl::WRITE_LE_UINT16(wref.offset_prev_inventory, prevInvOff);
            rs_utl::WRITE_LE_UINT16(wref.offset_owner, pedOff);
            if (emittedCount > 0) {
                uint16_t curWepOff = static_cast<uint16_t>(0x9562 + wIdx * 36);
                rs_utl::WRITE_LE_UINT16(levelData.weapons[wIdx - 1].offset_next_inventory, curWepOff);
            }
            wref.mapposx[0] = static_cast<uint8_t>(p->position().ox);
            wref.mapposx[1] = static_cast<uint8_t>(p->position().tx);
            wref.mapposy[0] = static_cast<uint8_t>(p->position().oy);
            wref.mapposy[1] = static_cast<uint8_t>(p->position().ty);
            rs_utl::WRITE_LE_UINT16(wref.mapposz, static_cast<uint16_t>((p->position().tz << 7) | (p->position().oz & 0x7F)));
            uint16_t anim = pw->getDrawAnimation();
            rs_utl::WRITE_LE_UINT16(wref.index_current_anim, anim);
            rs_utl::WRITE_LE_UINT16(wref.index_current_frame, lookupHstaStartFrame(hstaTable, anim));
            rs_utl::WRITE_LE_INT16(wref.unkn5, static_cast<int16_t>(pw->ammoRemaining()));

            prevInvOff = static_cast<uint16_t>(0x9562 + wIdx * 36);
            ++emittedCount;
            ++wIdx;
        }

        if (emittedCount > 0) {
            rs_utl::WRITE_LE_UINT16(levelData.people[pid].offset_equipment,
                                    static_cast<uint16_t>(0x9562 + firstPedWIdx * 36));
        }
    }

    // 6. Objectives
    for (size_t i = 0; i < 6; ++i) {
        memset(&levelData.objectives[i], 0, sizeof(LevelData::Objectives));
    }
    auto resolvePedSlotOffset = [&](PedInstance *p) -> uint16_t {
        if (!p) return 0;
        auto it = pedToSlot.find(p);
        uint16_t slot = (it != pedToSlot.end()) ? it->second : p->id();
        return static_cast<uint16_t>(slot * 92 + 2);
    };
    for (size_t i = 0; i < pMission->numObjectives() && i < 6; ++i) {
        ObjectiveDesc *obj = pMission->objective(i);
        if (!obj) continue;
        LevelData::Objectives &o = levelData.objectives[i];
        o.status = 1;
        if (ObjPersuade *op = dynamic_cast<ObjPersuade*>(obj)) {
            rs_utl::WRITE_LE_UINT16(o.type, LevelData::kObjectiveTypePersuade);
            if (PedInstance *p = dynamic_cast<PedInstance*>(op->target())) {
                rs_utl::WRITE_LE_UINT16(o.offset, resolvePedSlotOffset(p));
            }
        } else if (ObjAssassinate *oa = dynamic_cast<ObjAssassinate*>(obj)) {
            rs_utl::WRITE_LE_UINT16(o.type, LevelData::kObjectiveTypeAssassinate);
            if (PedInstance *p = dynamic_cast<PedInstance*>(oa->target())) {
                rs_utl::WRITE_LE_UINT16(o.offset, resolvePedSlotOffset(p));
            }
        } else if (ObjProtect *opr = dynamic_cast<ObjProtect*>(obj)) {
            rs_utl::WRITE_LE_UINT16(o.type, LevelData::kObjectiveTypeProtect);
            if (PedInstance *p = dynamic_cast<PedInstance*>(opr->target())) {
                rs_utl::WRITE_LE_UINT16(o.offset, resolvePedSlotOffset(p));
            }
        } else if (ObjDestroyVehicle *odv = dynamic_cast<ObjDestroyVehicle*>(obj)) {
            rs_utl::WRITE_LE_UINT16(o.type, LevelData::kObjectiveTypeDestroyVehicle);
            for (size_t vIdx = 0; vIdx < pMission->numVehicles(); ++vIdx) {
                if (pMission->vehicle(vIdx) == odv->target()) {
                    rs_utl::WRITE_LE_UINT16(o.offset, static_cast<uint16_t>(0x5C02 + vIdx * 42));
                    break;
                }
            }
        } else if (ObjUseVehicle *ouv = dynamic_cast<ObjUseVehicle*>(obj)) {
            rs_utl::WRITE_LE_UINT16(o.type, LevelData::kObjectiveTypeUseVehicle);
            for (size_t vIdx = 0; vIdx < pMission->numVehicles(); ++vIdx) {
                if (pMission->vehicle(vIdx) == ouv->target()) {
                    rs_utl::WRITE_LE_UINT16(o.offset, static_cast<uint16_t>(0x5C02 + vIdx * 42));
                    break;
                }
            }
        } else if (ObjTakeWeapon *otw = dynamic_cast<ObjTakeWeapon*>(obj)) {
            rs_utl::WRITE_LE_UINT16(o.type, LevelData::kObjectiveTypeEquipmentAcquisition);
            uint16_t targetWIdx = 0;
            bool found = false;
            for (size_t wi = 0; wi < pMission->numWeaponsOnGround() && targetWIdx < 512; ++wi, ++targetWIdx) {
                if (pMission->weaponOnGround(wi) == otw->target()) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (size_t pi = 0; pi < pMission->numPeds() && !found; ++pi) {
                    PedInstance *p = pMission->ped(pi);
                    if (!p || p->isOurAgent()) continue;
                    for (size_t wi = 0; wi < p->numWeapons() && targetWIdx < 512; ++wi, ++targetWIdx) {
                        if (p->weapon(wi) == otw->target()) {
                            found = true;
                            break;
                        }
                    }
                }
            }
            rs_utl::WRITE_LE_UINT16(o.offset, static_cast<uint16_t>(0x9562 + targetWIdx * 36));
        } else if (ObjEliminate *oe = dynamic_cast<ObjEliminate*>(obj)) {
            if (oe->groupDefMask() == PedInstance::og_dmPolice) {
                rs_utl::WRITE_LE_UINT16(o.type, LevelData::kObjectiveTypeCombatSweepPolice);
            } else {
                rs_utl::WRITE_LE_UINT16(o.type, LevelData::kObjectiveTypeCombatSweep);
            }
        } else if (ObjEvacuate *oev = dynamic_cast<ObjEvacuate*>(obj)) {
            rs_utl::WRITE_LE_UINT16(o.type, LevelData::kObjectiveTypeEvacuate);
            rs_utl::WRITE_LE_INT16(o.mapposx, oev->location().x);
            rs_utl::WRITE_LE_INT16(o.mapposy, oev->location().y);
            rs_utl::WRITE_LE_INT16(o.mapposz, oev->location().z);
        }
    }

    // 7. AI Tactics & Squad Header Normalization (DOS compatibility)
    uint8_t maxActiveFaction = 0;
    for (size_t i = 8; i < 64; ++i) {
        const auto &p = levelData.people[i];
        if (p.type == 0x01 && (p.type_ped & PedInstance::kPedTypeAgent) != 0 &&
            p.location != LevelData::kPeopleLocNotVisible) {
            uint8_t fac = static_cast<uint8_t>(i / 8);
            if (fac > maxActiveFaction) {
                maxActiveFaction = fac;
            }
        }
    }
    for (size_t i = 1; i < 128; ++i) {
        auto &e = levelData.tactics.entries[i];
        if (e.mode == LevelData::kAITacticModeNone) {
            continue;
        }
        if ((e.sub_filter & LevelData::kAITacticFlagDisabled) == 0 &&
            e.faction_id >= 1 && e.faction_id <= 7 && e.faction_id > maxActiveFaction) {
            maxActiveFaction = e.faction_id;
        }
    }
    uint8_t minSquads = static_cast<uint8_t>(maxActiveFaction + 1);
    if (levelData.tactics.header.num_squads < minSquads) {
        levelData.tactics.header.num_squads = minSquads;
    }
    if (levelData.tactics.header.peds_per_squad == 0) {
        levelData.tactics.header.peds_per_squad = 8;
    }

    rebuildSpatialMapLinks(levelData);

    return save_level_data(missionId, levelData, customPath);
}

/*!
 * Creates a Mission object from the LevelDataAll structure.
 */
Mission * MissionManager::create_mission(LevelData::LevelDataAll &level_data, const std::string &customMapPath) {
    Map *pMap = nullptr;
    if (!customMapPath.empty()) {
        pMap = mapManager_.getMapFromPath(customMapPath, level_data.mapinfos);
    } else {
        pMap = mapManager_.getMap(level_data.mapinfos);
    }
    if (pMap == NULL) {
        return NULL;
    }

    Mission *p_mission = new Mission();

    // Init indexes
    DataIndex di;
    memset(di.vindx, 0xFF, 2*64);
    memset(di.pindx, 0xFF, 2*256);
    memset(di.driverindx, 0xFF, 2*256);


    try {
        if (missionRequiresAmericanRevolt(level_data) && !rs_utl::File::hasAmericanRevoltData()) {
            throw LoadMissionException("This mission requires Syndicate: American Revolt data assets (Air Strike / Clone Shield), but the active game data folder only contains original Syndicate files.\n\nPlease point your data directory to an American Revolt installation (such as Syndicate Plus DATADISK/DATA) in Settings.");
        }

        if (!p_mission->init(pMap)) {
            RSERR(Log::k_FLG_GAME, "MissionManager", "loadMission", ("Failed to load mission : init failed\n"));
            return nullptr;
        }
        p_mission->setRawLevelData(level_data);
        p_mission->setTechLevel(level_data.mapinfos.unkn1[0]);
        for (size_t i = 0; i < 4; ++i) {
            int z = rs_utl::READ_LE_UINT16(level_data.people[i].mapposz) >> 7;
            int oz = level_data.people[i].mapposz[0] & 0x7F;
            TilePoint pt(level_data.people[i].mapposx[1], level_data.people[i].mapposy[1], z,
                         level_data.people[i].mapposx[0], level_data.people[i].mapposy[0], oz);
            p_mission->setSquadSpawnPoint(i, pt, level_data.people[i].orientation);
        }

        LOG(Log::k_FLG_GAME, "MissionManager", "create_mission", ("Vehicles creation"));
        createVehicles(level_data, di, p_mission);

        LOG(Log::k_FLG_GAME, "MissionManager", "create_mission", ("Peds creation"));
        createPeds(level_data, di, p_mission);

        LOG(Log::k_FLG_GAME, "MissionManager", "create_mission", ("Static creation"));
        for (uint16_t i = 0; i < 400; i++) {
            LevelData::Statics & sref = level_data.statics[i];
            if(sref.desc == 0)
                continue;
            auto s = Static::loadInstance((uint8_t *) & sref, i, p_mission->map());
            if (s) {
                p_mission->addStatic(std::unique_ptr<Static>(s));
            }
        }

        LOG(Log::k_FLG_GAME, "MissionManager", "create_mission", ("Weapons creation"));
        createWeapons(level_data, di, p_mission);

        // Ensure all enemy agents have at least a basic weapon (Pistol) if none was assigned in gameXX.dat
        for (size_t i = 0; i < p_mission->numPeds(); ++i) {
            PedInstance *p = p_mission->ped(i);
            if (p && p->type() == PedInstance::kPedTypeAgent && !p->isOurAgent()) {
                if (p->numWeapons() == 0) {
                    Weapon *wDef = g_weaponMgr.getWeapon(Weapon::Pistol);
                    if (wDef) {
                        WeaponInstance *wi = WeaponInstance::createInstance(wDef);
                        p->addWeapon(wi);
                        wi->setOwner(p);
                    }
                }
            }
        }

        LOG(Log::k_FLG_GAME, "MissionManager", "create_mission", ("Objectives creation"));
        createObjectives(level_data, di, p_mission);

        // adding visual markers(arrow + 1,2,3,4) above our agents
        // availiable/selected on screen
        p_mission->addSfxObject(std::make_unique<SFXObject>(p_mission->map(), SFXObject::sfxt_SelArrow, false));
        p_mission->addSfxObject(std::make_unique<SFXObject>(p_mission->map(), SFXObject::sfxt_SelArrow, false));
        p_mission->addSfxObject(std::make_unique<SFXObject>(p_mission->map(), SFXObject::sfxt_SelArrow, false));
        p_mission->addSfxObject(std::make_unique<SFXObject>(p_mission->map(), SFXObject::sfxt_SelArrow, false));
        p_mission->addSfxObject(std::make_unique<SFXObject>(p_mission->map(), SFXObject::sfxt_AgentFirst));
        p_mission->addSfxObject(std::make_unique<SFXObject>(p_mission->map(), SFXObject::sfxt_AgentSecond));
        p_mission->addSfxObject(std::make_unique<SFXObject>(p_mission->map(), SFXObject::sfxt_AgentThird));
        p_mission->addSfxObject(std::make_unique<SFXObject>(p_mission->map(), SFXObject::sfxt_AgentFourth));

        LOG(Log::k_FLG_GAME, "MissionManager", "create_mission", ("End of Mission creation"));
        return p_mission;

    } catch (const LoadMissionException & ex) {
        RSERR(Log::k_FLG_GAME, "MissionManager", "loadMission", ("Failed to load mission %s\n", ex.what()));
        if (p_mission) {
            delete p_mission;
        }
        return NULL;
    }
}

void MissionManager::createWeapons(const LevelData::LevelDataAll &level_data, DataIndex &di, Mission *pMission) {
    for (uint16_t i = 0; i < 512; i++) {
        const LevelData::Weapons & wref = level_data.weapons[i];
        if(wref.desc == 0)
            continue;
        WeaponInstance *w = create_weapon_instance(wref, pMission->map());
        if (w) {
            if (wref.desc == 0x05) {
                uint16_t raw_offset_owner = rs_utl::READ_LE_UINT16(wref.offset_owner);
                if (raw_offset_owner >= 2) {
                    uint16_t offset_owner = (raw_offset_owner - 2) / 92; // 92 = ped data size
                    if (offset_owner > 7 && offset_owner < 256 && di.pindx[offset_owner] != 0xFFFF && di.pindx[offset_owner] < pMission->numPeds()) {
                        PedInstance *pOwner = pMission->ped(di.pindx[offset_owner]);
                        if (pOwner) {
                            pOwner->addWeapon(w);
                            w->setOwner(pOwner);
                            di.weapons[i] = w;
                        } else {
                            delete w;
                        }
                    } else {
                        delete w;
                    }
                } else {
                    delete w;
                }
            } else {
                w->setDrawable(true);
                di.weapons[i] = w;
                pMission->addWeaponToGround(w);
            }
        }
    }
}

WeaponInstance * MissionManager::create_weapon_instance(const LevelData::Weapons &gamdata, Map *pMap) {
    Weapon::WeaponType wType = Weapon::Unknown;
    WeaponInstance *pNewWeapon = NULL;

    switch (gamdata.sub_type) {
        case 0x01:
            wType = Weapon::Persuadatron;
            break;
        case 0x02:
            wType = Weapon::Pistol;
            break;
        case 0x03:
            wType = Weapon::GaussGun;
            break;
        case 0x04:
            wType = Weapon::Shotgun;
            break;
        case 0x05:
            wType = Weapon::Uzi;
            break;
        case 0x06:
            wType = Weapon::Minigun;
            break;
        case 0x07:
            wType = Weapon::Laser;
            break;
        case 0x08:
            wType = Weapon::Flamer;
            break;
        case 0x09:
            wType = Weapon::LongRange;
            break;
        case 0x0A:
            wType = Weapon::Scanner;
            break;
        case 0x0B:
            wType = Weapon::MediKit;
            break;
        case 0x0C:
            wType = Weapon::TimeBomb;
            break;
        case 0x0D:
            wType = Weapon::AccessCard;
            break;
        case 0x0E:
            wType = Weapon::AirStrike;
            break;
        case 0x0F:
            wType = Weapon::CloneShield;
            break;
        case 0x11:
            wType = Weapon::EnergyShield;
            break;
        default:
            RSERR(Log::k_FLG_GAME, "Mission", "create_weapon_instance", ("unknown weapon type : %d", gamdata.sub_type));
            return NULL;
    }

    Weapon *pWeapon = g_weaponMgr.getWeapon(wType);
    if (pWeapon) {
        pNewWeapon = WeaponInstance::createInstance(pWeapon);
        pNewWeapon->setMap(pMap);
        pNewWeapon->setPosition(gamdata.mapposx[1], gamdata.mapposy[1],
            rs_utl::READ_LE_UINT16(gamdata.mapposz) >> 7, gamdata.mapposx[0],
            gamdata.mapposy[0], gamdata.mapposz[0] & 0x7F);
    }

    return pNewWeapon;
}


void MissionManager::createVehicles(const LevelData::LevelDataAll &level_data, DataIndex &di, Mission *pMission) {
    std::vector<TrainHead *> mainTrainHeads;
    for (uint16_t i = 0; i < 64; i++) {
        const LevelData::Cars & car = level_data.cars[i];
        // car.sub_type 0x09 - train
        if (car.type == 0x0)
            continue;
        Vehicle *v =
            createVehicleInstance(car, i, pMission->map());
        if (v) {
            di.vindx[i] = pMission->numVehicles();
            pMission->addVehicle(v);
            uint16_t drv_off = rs_utl::READ_LE_UINT16(car.offset_of_driver);
            if (drv_off != 0 && ((drv_off - 2) % 92) == 0) {
                uint16_t pedIdx = (drv_off - 2) / 92;
                if (pedIdx < 256) {
                    di.driverindx[pedIdx] = di.vindx[i];
                }
            }

            if (v->getType() == Vehicle::kVehicleTypeTrainHead) {
                TrainHead *pHead = dynamic_cast<TrainHead *>(v);
                
                // Determine if this is a trailing head of an existing train or a new train head.
                TrainHead *pMatchingMainHead = nullptr;
                for (TrainHead *pMainHead : mainTrainHeads) {
                    // Check if they are close (within 7 tiles / 1792 units)
                    if (pMainHead->isCloseTo(pHead, 1792)) {
                        // Ensure this main head doesn't already have a trailing head (or check if link chain is short)
                        // A train has 4 bodies + 1 trailing head, so max chain length is 5 links.
                        int chainLen = 0;
                        TrainBody *pCurr = pMainHead->getNext();
                        while (pCurr != nullptr) {
                            chainLen++;
                            pCurr = pCurr->getNext();
                        }
                        if (chainLen < 5) {
                            pMatchingMainHead = pMainHead;
                            break;
                        }
                    }
                }
                
                if (pMatchingMainHead != nullptr) {
                    pMatchingMainHead->appendTrainBody(pHead);
                } else {
                    mainTrainHeads.push_back(pHead);
                }
            } else if (v->getType() == Vehicle::kVehicleTypeTrainBody) {
                TrainBody *pBody = dynamic_cast<TrainBody *>(v);
                
                // Find the closest main train head to append to
                TrainHead *pClosestMainHead = nullptr;
                double closestDist = 999999.0;
                for (TrainHead *pMainHead : mainTrainHeads) {
                    double d = pMainHead->distanceToPosition(pBody->position());
                    if (d < closestDist) {
                        closestDist = d;
                        pClosestMainHead = pMainHead;
                    }
                }
                
                if (pClosestMainHead != nullptr) {
                    pClosestMainHead->appendTrainBody(pBody);
                }
            }
        }
    }
}
/*!
 *
 */
Vehicle * MissionManager::createVehicleInstance(const LevelData::Cars &gamdata, uint16_t id, Map *pMap) {
    const auto &vStats = VehicleManager::getStats(gamdata.sub_type);
    int rawHp = rs_utl::READ_LE_INT16(gamdata.health);
    bool isDestroyed = (rawHp <= 0 && rawHp != 0) || (gamdata.sub_type == 0x04);
    int hp = isDestroyed ? -1 : ((rawHp > 0) ? rawHp : vStats.health);
    int dir = gamdata.orientation >> 5;

    uint16_t cur_anim = rs_utl::READ_LE_UINT16(gamdata.index_current_anim) - dir;

    Vehicle *pVehicle = NULL;
    bool isMoveOnXAxis = (gamdata.orientation == 192 || gamdata.orientation == 64);
    if (gamdata.sub_type == Vehicle::kVehicleTypeTrainHead) {
        LOG(Log::k_FLG_GAME, "MissionManager","createVehicleInstance", ("Create Train Head %d", id))
        // Read speed from mission data and scale to world units/s.
        // Scale factor x8 derived from measured 14s transit in Mission 22 (speed byte = 0xA0 = 160 -> 1280 wu/s).
        // Fallback to configured speed if speed byte is zero (unset in a modded/future map).
        int trainMaxSpeed = (gamdata.speed != 0) ? (gamdata.speed * 8) : vStats.speed;
        pVehicle = new TrainHead(id, Vehicle::kVehicleTypeTrainHead, pMap, hp, isMoveOnXAxis, trainMaxSpeed);
        cur_anim = vStats.base_anim;
    } else if (gamdata.sub_type == Vehicle::kVehicleTypeTrainBody) {
        LOG(Log::k_FLG_GAME, "MissionManager","createVehicleInstance", ("Create Train Body %d", id))
        pVehicle = new TrainBody(id, Vehicle::kVehicleTypeTrainBody, pMap, hp, isMoveOnXAxis);
        cur_anim = vStats.base_anim + (rs_utl::READ_LE_UINT16(gamdata.index_base_anim) * 6);
    } else {
        // standard car
        LOG(Log::k_FLG_GAME, "MissionManager","createVehicleInstance", ("Create generic car %d", id))
        pVehicle = new GenericCar(id, gamdata.sub_type, pMap, vStats.speed, vStats.acceleration);
        pVehicle->setHealth(hp);
        pVehicle->setStartHealth(isDestroyed ? 0 : hp);

        switch (gamdata.sub_type) {
            case 0x04:
                // large armored damaged
                // it is actually base animation and they have 8 directions
                cur_anim = cur_anim - 12 + (dir >> 1);
                break;
            // remaining cases are just here to detect unknown vehicle types
            case 0x01: // large armored
            case 0x0D: // grey vehicle
            case 0x11: // firefighters vehicle
            case 0x1C: // small armored vehicle
            case 0x24: // police vehicle
            case 0x28: // medical vehicle
                break;
            default:
                RSERR(Log::k_FLG_GAME, "Mission", "createVehicleInstance", ("Unknown Car subtype : %d", gamdata.sub_type));
                break;
        }

        if (cur_anim == 0 || cur_anim > 2000) {
            cur_anim = vStats.base_anim;
        }
    }

    if (pVehicle) {
        int z = rs_utl::READ_LE_UINT16(gamdata.mapposz) >> 7;

        // TODO: the size should be adjusted on orientation/direction change
        // and it should be different per vehicle type
        pVehicle->setSize(256, 256, 192);

        int oz = gamdata.mapposz[0] & 0x7F;
        pVehicle->setPosition(gamdata.mapposx[1], gamdata.mapposy[1],
                                z, gamdata.mapposx[0],
                                gamdata.mapposy[0], oz);
        pVehicle->setDirection(gamdata.orientation);

        LOG(Log::k_FLG_GAME, "MissionManager","createVehicleInstance", (" - position %d, %d, %d, %d, %d, %d", gamdata.mapposx[1], gamdata.mapposy[1], z, gamdata.mapposx[0],gamdata.mapposy[0], oz))
        LOG(Log::k_FLG_GAME, "MissionManager","createVehicleInstance", (" - field unknown 1 %u", gamdata.unkn1))
        LOG(Log::k_FLG_GAME, "MissionManager","createVehicleInstance", (" - field unknown 2 %u", gamdata.unkn2))
        LOG(Log::k_FLG_GAME, "MissionManager","createVehicleInstance", (" - field unknown 3 %u", gamdata.unkn3))
        LOG(Log::k_FLG_GAME, "MissionManager","createVehicleInstance", (" - field unknown 4 %u", gamdata.unkn4))
        LOG(Log::k_FLG_GAME, "MissionManager","createVehicleInstance", (" - field unknown 6 %u", gamdata.unkn6))

        pVehicle->setAnimations(cur_anim);
        if (isDestroyed) {
            pVehicle->setStartHealth(0);
            pVehicle->setHealth(-1);
            pVehicle->playAnimation(pVehicle->burntAnimation());
        } else {
            pVehicle->playAnimation(pVehicle->regularAnimation());
        }
    }

    return pVehicle;
}

void MissionManager::createPeds(const LevelData::LevelDataAll &level_data, DataIndex &di, Mission *pMission) {

#ifdef _DEBUG
    std::map <uint32_t, std::string> obj_ids;
    // NOTE: not very useful way of remembering "Who is who"
    obj_ids[0] = "Undefined";
    obj_ids[PedInstance::kPlayerGroupId] = "Players Agents or Persuaded";
    obj_ids[2] = "Enemy Agents";
    obj_ids[3] = "Enemy Guards";
    obj_ids[4] = "Policemen";
    obj_ids[5] = "Civilians";
#endif

    bool isCombatSweep = pMission->isCombatSweep();
    bool hasTactics = (pMission->numTactics() > 0);
    bool isAR = missionRequiresAmericanRevolt(level_data);

    PedManager peds;
    for (uint16_t i = 0; i < 256; i++) {
        const LevelData::People & pedref = level_data.people[i];
        uint16_t rawMods = rs_utl::READ_LE_UINT16(pedref.mods_info);
        bool isHunter = false;

        if (!hasTactics && isCombatSweep) {
            // When no AI tactics table is defined in the level:
            // American Revolt activates all hostile factions (peds 8..63 = 56 agents)
            // Vanilla Syndicate activates Faction 1 (peds 8..15)
            if (isAR) {
                isHunter = isHunter || (i >= 8 && i < 64);
            } else {
                isHunter = isHunter || (i >= 8 && i <= 15);
            }
        }

        PedInstance *p =
            peds.loadInstance(pedref, i, pMission->map(), PedInstance::kPlayerGroupId, isHunter);
        if (p) {
            di.pindx[i] = pMission->numPeds();
            pMission->addPed(p);

            if (p->type() == PedInstance::kPedTypeAgent && !p->isOurAgent()) {
                p->setHunter(isHunter);
                uint16_t modBits = rawMods & 0x1FFE; // bits 1..12
                if (rawMods & 0x8000) {
                    p->setCyberwareOverridden(true);
                    p->applyCyberwareTier(PedInstance::tierFromModsBitmask(modBits));
                } else if (modBits == 0) {
                    p->setCyberwareOverridden(false);
                    p->applyCyberwareTier(pMission->techLevel());
                } else {
                    if (modBits == PedInstance::modsBitmaskFromTier(pMission->techLevel())) {
                        p->setCyberwareOverridden(false);
                        p->applyCyberwareTier(pMission->techLevel());
                    } else {
                        p->setCyberwareOverridden(true);
                        p->applyCyberwareTier(PedInstance::tierFromModsBitmask(modBits));
                    }
                }

                // AI Tactics FSM initialization
                uint8_t factionId = p->factionId();
                int rootTactic = pMission->getRootTacticIndex(factionId);
                if (rootTactic >= 0) {
                    auto *pHos = p->behaviour().getComponent<PlayerHostileBehaviourComponent>();
                    if (pHos) {
                        pHos->setFactionId(factionId);
                        pHos->initFromTactic(pMission, rootTactic);
                        p->setHunter(pHos->isHunter());
                    }
                } else if (hasTactics && isCombatSweep) {
                    // If mission has tactics but this faction has no specific or wildcard entry:
                    // American Revolt hostile squads default to hunting in combat sweep
                    if (isAR && (i >= 8 && i < 64)) {
                        p->setHunter(true);
                        auto *pHos = p->behaviour().getComponent<PlayerHostileBehaviourComponent>();
                        if (pHos) {
                            pHos->setHunter(true);
                        }
                    }
                }
            }

            if (pedref.location == LevelData::kPeopleLocInVehicle || di.driverindx[i] != 0xFFFF) {
                uint16_t vid = 0xFFFF;  // Id of the vehicle
                bool setDriver = false;  // Tells if ped should be the driver
                if (di.driverindx[i] != 0xFFFF) {
                    // Current ped is the driver
                    vid = di.driverindx[i];
                    setDriver = true;
                } else {
                    // Current ped is just a passenger
                    uint16_t vin = rs_utl::READ_LE_UINT16(pedref.offset_of_vehicle);
                    if (vin != 0) {
                        vin = (vin - 0x5C02) / 42; // 42 vehicle data size
                        vid = di.vindx[vin];
                    }
                }

                if(vid == 0xFFFF) {
                    throw LoadMissionException("Vehicle not found");
                }
                Vehicle *pVehicle = pMission->vehicle(vid);

                pVehicle->addPassenger(p);
                if (setDriver) {
                    pVehicle->setDriver(p);
                    p->setPosition(pVehicle->position());
                }
            }

            if (p->isOurAgent()) {
                // adds the agent to the mission squad
                if (i < Squad::kMaxSlot) {
                    pMission->getSquad()->setMember(i, p);
                }
            } else {
                // Set scenarios for non player ped
                createScriptedActionsForPed(pMission, di, level_data, p);
            }
        }
    }
}

void MissionManager::createScriptedActionsForPed(Mission *pMission, DataIndex &di, const LevelData::LevelDataAll &level_data, PedInstance *pPed) {
    const LevelData::People & peopleData = level_data.people[pPed->id()];
    uint16_t offset_start = rs_utl::READ_LE_UINT16(peopleData.offset_scenario_start);
    uint16_t offset_nxt = offset_start;
    Vehicle *v = pPed->inVehicle();
    bool isInVehicle = v != nullptr;

#ifdef _DEBUG
    if (offset_nxt) {
        LOG(Log::k_FLG_GAME, "MissionManager","createScriptedActionsForPed", ("Scenarios for ped : %d", pPed->id()))
    }
#endif

    pPed->clearScriptSteps();

    while (offset_nxt) {
        LevelData::Scenarios sc = level_data.scenarios[offset_nxt / 8];
        LOG(Log::k_FLG_GAME, "MissionManager","createScriptedActionsForPed", ("At offset %d, type : %d", offset_nxt, sc.type))

        offset_nxt = rs_utl::READ_LE_UINT16(sc.next);
        if (offset_nxt == offset_start) {
            throw LoadMissionException("Bad scenario offset");
        }

        ScriptStep step;
        step.type = sc.type;

        if (sc.type == LevelData::kScenarioTypeUseVehicle || sc.type == 2) {
            uint16_t bindx = rs_utl::READ_LE_UINT16(sc.offset_object);
            if (bindx >= 0x5C02 && bindx < 0x6682) {
                bindx -= 0x5C02;
                bindx /= 42;
                if (di.vindx[bindx] != 0xFFFF && di.vindx[bindx] < pMission->numVehicles()) {
                    step.targetEntityId = di.vindx[bindx];
                    isInVehicle = true;
                }
            }
        } else if (sc.type == LevelData::kScenarioTypeLeaveVehicle || sc.type == 6) {
            isInVehicle = false;
        } else if (sc.type == LevelData::kScenarioTypeWait || sc.type == 5) {
            uint16_t ticks = rs_utl::READ_LE_UINT16(sc.offset_object);
            step.param = ticks * 100;
        } else if (sc.type == LevelData::kScenarioTypeTrainWait || sc.type == 10) {
            step.param = 10000;
        } else if (sc.type == LevelData::kScenarioTypeTrigger || sc.type == 8) {
            TilePoint locT(sc.tilex >> 1, sc.tiley >> 1, sc.tilez,
                (sc.tilex & 0x01) << 7, (sc.tiley & 0x01) << 7);
            step.targetPos = locT;
            uint16_t objOff = rs_utl::READ_LE_UINT16(sc.offset_object);
            step.param = (objOff > 0) ? objOff : (6 * 256);
        } else if (sc.tilex != 0 && sc.tiley != 0) {
            TilePoint locT(sc.tilex >> 1, sc.tiley >> 1, sc.tilez,
                (sc.tilex & 0x01) << 7, (sc.tiley & 0x01) << 7);
            step.targetPos = locT;
        }

        pPed->addScriptStep(step);

        if (isInVehicle && offset_nxt == 0) {
            if (step.type != LevelData::kScenarioTypeReset) {
                ScriptStep loopStep;
                loopStep.type = LevelData::kScenarioTypeReset;
                pPed->addScriptStep(loopStep);
            }
        }
    } // end while

    buildScriptedActionsForPed(pMission, pPed);
}

void MissionManager::buildScriptedActionsForPed(Mission *pMission, PedInstance *pPed) {
    if (!pMission || !pPed) return;

    pPed->destroyAllActions(true);

    if (pPed->scriptSteps().empty()) {
        if (!pPed->isDead() && (pPed->initialLevelState() & LevelData::kPeopleStateWalking)) {
            pPed->addToDefaultActions(new WalkToDirectionAction());
            pPed->addMovementAction(pPed->defaultAction(), false);
        }
        return;
    }

    Vehicle *v = pPed->inVehicle();
    if (!v) {
        for (size_t vi = 0; vi < pMission->numVehicles(); ++vi) {
            Vehicle *cand = pMission->vehicle(vi);
            if (cand && cand->getDriver() == pPed) {
                v = cand;
                break;
            }
        }
    }
    bool isInVehicle = (v != nullptr);

    const auto &steps = pPed->scriptSteps();
    for (size_t i = 0; i < steps.size(); ++i) {
        const auto &step = steps[i];
        if (step.type == LevelData::kScenarioTypeGoto || step.type == 1) {
            if (isInVehicle && v) {
                if (v->isCar()) {
                    GenericCar *pCar = dynamic_cast<GenericCar *>(v);
                    pPed->addToDefaultActions(new DriveVehicleAction(pCar, step.targetPos));
                } else if (v->getType() == Vehicle::kVehicleTypeTrainHead || v->getType() == Vehicle::kVehicleTypeTrainBody) {
                    TrainHead *pTrain = dynamic_cast<TrainHead *>(v);
                    if (!pTrain && v->getType() == Vehicle::kVehicleTypeTrainBody) {
                        for (size_t vi = 0; vi < pMission->numVehicles(); ++vi) {
                            if (auto *head = dynamic_cast<TrainHead*>(pMission->vehicle(vi))) {
                                TrainBody *curr = head->getNext();
                                while (curr != nullptr) {
                                    if (curr == v) {
                                        pTrain = head;
                                        break;
                                    }
                                    curr = curr->getNext();
                                }
                                if (pTrain) break;
                            }
                        }
                    }
                    if (pTrain) {
                        pPed->addToDefaultActions(new DriveTrainAction(pTrain, step.targetPos));
                    }
                }
            } else {
                pPed->addToDefaultActions(new WalkAction(step.targetPos));
            }

            if (isInVehicle && v && v->isCar() && pPed->isScriptLooping()) {
                size_t numWaypoints = 0;
                for (const auto &s : steps) {
                    if (s.type == LevelData::kScenarioTypeGoto || s.type == 1) numWaypoints++;
                }
                if (numWaypoints == 1) {
                    GenericCar *pCar = dynamic_cast<GenericCar *>(v);
                    pPed->addToDefaultActions(new DriveVehicleAction(pCar, v->position()));
                }
            }
        } else if (step.type == LevelData::kScenarioTypeTrigger || step.type == 8) {
            uint32_t r = step.param > 0 ? step.param : 6 * 256;
            pPed->addToDefaultActions(new TriggerAction(r, WorldPoint(step.targetPos)));
        } else if (step.type == LevelData::kScenarioTypeUseVehicle || step.type == 2) {
            if (step.targetEntityId >= 0 && static_cast<size_t>(step.targetEntityId) < pMission->numVehicles()) {
                v = pMission->vehicle(step.targetEntityId);
                isInVehicle = (v != nullptr);
                if (v) {
                    if (pPed->inVehicle() != v) {
                        pPed->addToDefaultActions(pPed->createActionEnterVehicle(v));
                    }
                }
            }
        } else if (step.type == LevelData::kScenarioTypeLeaveVehicle || step.type == 6) {
            pPed->addToDefaultActions(new LeaveVehicleAction());
            isInVehicle = false;
            v = nullptr;
        } else if (step.type == LevelData::kScenarioTypeWait || step.type == 5) {
            uint32_t ms = step.param > 0 ? step.param : 5000;
            pPed->addToDefaultActions(new WaitAction(WaitAction::kWaitTime, ms));
        } else if (step.type == LevelData::kScenarioTypeTrainWait || step.type == 10) {
            uint32_t ms = step.param > 0 ? step.param : 10000;
            pPed->addToDefaultActions(new WaitAction(WaitAction::kWaitTime, ms));
        } else if (step.type == LevelData::kScenarioTypeEscape || step.type == 7) {
            pPed->addToDefaultActions(new EscapeAction());
        } else if (step.type == LevelData::kScenarioTypeReset || step.type == 9) {
            pPed->addToDefaultActions(new ResetScriptedAction(Action::kActionDefault));
        }
    }

    MovementAction *pScriptAction = pPed->defaultAction();
    if (pScriptAction && pScriptAction->next() != nullptr &&
        pScriptAction->next()->type() != Action::kActTypeReset) {
        TilePoint currPos = pPed->position();
        if (isInVehicle && v) {
            currPos = v->position();
        }
        if (!steps.empty() && (steps[0].type == LevelData::kScenarioTypeGoto || steps[0].type == 1)) {
            if (currPos.tx == steps[0].targetPos.tx && currPos.ty == steps[0].targetPos.ty) {
                pScriptAction = pScriptAction->next();
            }
        }
    }
    pPed->addMovementAction(pScriptAction, false);
}

void MissionManager::createObjectives(const LevelData::LevelDataAll &level_data,
                                        DataIndex &di, Mission *pMission) {
    // 0x01 offset of ped
    // 0x02 offset of ped
    // 0x03 offset of ped, next objective 0x00 + coord, nxt 0x00 + offset of ped
    // second objective is where ped should go, third ped that should reach it
    // also can be without those data or can have offset of ped + coord
    // 0x05 offset of weapon
    // 0x0E offset of vehicle
    // 0x0F offset of vehicle
    // 0x10 coordinates
    // looks like that objectives even if they are defined where not fully
    // defined(or are correct), indonesia has one objective but has 2
    // 0x0 objectives with peds offset + coords, rockies mission
    // has 0x0e objective + 0x01 but offsets are wrong as in original
    // gameplay only 1 persuade + evacuate present, in description
    // 0x0e + 2 x 0x01 + 0x0f, because of this careful loading required
    // max 5(6 read) objectives
    std::vector <PedInstance *> peds_evacuate;
    for (uint8_t i = 0; i < 6; i++) {
        ObjectiveDesc *objd = nullptr;

        const LevelData::Objectives & obj = level_data.objectives[i];
        uint16_t bindx = rs_utl::READ_LE_UINT16(obj.offset), cindx = 0;
        // TODO: checking is implemented for correct offset, because
        // in game data objective description is not correctly defined
        // some offsets are wrong, objective type is missing somewhere
        // check this, also 0x03 is not fully implemented
        // Also for some objectives there should be "small" actions defined
        // inside ped data, in 1 lvl when agents are close to target
        // ped goes to car and moves to location, if reached mission is
        // failed, similar actions are in many missions(scenarios defines this)

        switch (rs_utl::READ_LE_UINT16(obj.type)) {
            case LevelData::kObjectiveTypeNonAgentAction:
            {
                int x = rs_utl::READ_LE_INT16(obj.mapposx);
                if (x != 0 || bindx != 0) {
                    LOG(Log::k_FLG_GAME, "MissionManager", "createObjectives", (" - %d : Objective 0x00 not implemented", i));
                }
                if (x != 0) {
/*                    objd = new LocationObjective(objv_ReachLocation,
                       READ_LE_INT16(obj.mapposx),
                       READ_LE_INT16(obj.mapposy),
                       READ_LE_INT16(obj.mapposz));
                    //objd->pos_xyz.x = READ_LE_INT16(obj.mapposx);
                    //objd->pos_xyz.y = READ_LE_INT16(obj.mapposy);
                    //objd->pos_xyz.z = READ_LE_INT16(obj.mapposz);
                    objd->msg = "";
                    objd->condition = 32;
                    assert(objectives_.size() != 0);
                    ObjectiveDesc *pLastObj = objectives_.back();
                    pLastObj->condition |= 1;
                    pLastObj->subobjindx = objectives_.size();
                    objd->indx_grpid.targetindx = pLastObj->indx_grpid.targetindx;
                    objd->targettype = pLastObj->targettype;
                    */
                }
            }
                break;
            case LevelData::kObjectiveTypePersuade:
                if (bindx > 0 && bindx < 0x5C02) {
                    cindx = (bindx - 2) / 92;
                    if ((cindx * 92 + 2) == bindx && di.pindx[cindx] != 0xFFFF) {
                        PedInstance *p = pMission->ped(di.pindx[cindx]);
                        objd = new ObjPersuade(p);
                        // Adds the ped to the list of peds to evacuate
                        peds_evacuate.push_back(p);
                        LOG(Log::k_FLG_GAME, "MissionManager", "createObjectives", (" - %d : Persuad", i));
                    } else {
                        RSERR(Log::k_FLG_GAME, "MissionManager", "createObjectives", ("0x01 incorrect offset"));
                    }
                } else  {
                    RSERR(Log::k_FLG_GAME, "MissionManager", "createObjectives", ("0x01 type not matched"));
                }
                break;
            case LevelData::kObjectiveTypeAssassinate: // Assassinate a civilian
                if (bindx > 0 && bindx < 0x5C02) {
                    cindx = (bindx - 2) / 92;
                    if ((cindx * 92 + 2) == bindx && di.pindx[cindx] != 0xFFFF) {
                        PedInstance *p = pMission->ped(di.pindx[cindx]);
                        objd = new ObjAssassinate(p);
                        LOG(Log::k_FLG_GAME, "MissionManager", "createObjectives", (" - %d : Assassinate", i));
                    } else {
                        RSERR(Log::k_FLG_GAME, "MissionManager", "createObjectives", ("0x02 incorrect offset"));
                    }
                } else  {
                    RSERR(Log::k_FLG_GAME, "MissionManager", "createObjectives", ("0x02 type not matched"));
                }
                break;
            case LevelData::kObjectiveTypeProtect:
                if (bindx > 0 && bindx < 0x5C02) {
                    cindx = (bindx - 2) / 92;
                    if ((cindx * 92 + 2) == bindx && di.pindx[cindx] != 0xFFFF) {
                        PedInstance *p = pMission->ped(di.pindx[cindx]);
                        objd = new ObjProtect(p);
                        LOG(Log::k_FLG_GAME, "MissionManager", "createObjectives", (" - %d : Protect", i));
                    } else {
                        RSERR(Log::k_FLG_GAME, "MissionManager", "createObjectives", ("0x03 incorrect offset"));
                    }
                } else  {
                    RSERR(Log::k_FLG_GAME, "MissionManager", "createObjectives", ("0x03 type not matched"));
                }
                break;
            case LevelData::kObjectiveTypeEquipmentAcquisition:
                if (bindx >= 0x9562 && bindx < 0xDD62) {
                    bindx -= 0x9562;
                    cindx = bindx / 36;
                    if ((cindx * 36) == bindx && di.weapons[cindx] != NULL) {
                        objd = new ObjTakeWeapon(di.weapons[cindx]);
                        LOG(Log::k_FLG_GAME, "MissionManager", "createObjectives", (" - %d : TakeWeapon", i));
                    } else {
                        RSERR(Log::k_FLG_GAME, "MissionManager", "createObjectives", ("Error creating Take Weapon objective(0x05) : incorrect offset %d", cindx));
                    }
                } else {
                    RSERR(Log::k_FLG_GAME, "MissionManager", "createObjectives", ("Error creating Take Weapon objective(0x05) : type not matched %X", bindx));
                }

                break;
            case LevelData::kObjectiveTypeCombatSweepPolice:
                objd = new ObjEliminate(PedInstance::og_dmPolice);
                LOG(Log::k_FLG_GAME, "MissionManager", "createObjectives", (" - %d : Eliminate Police", i));
                break;
            case LevelData::kObjectiveTypeCombatSweep:
                objd = new ObjEliminate(PedInstance::og_dmAgent);
                LOG(Log::k_FLG_GAME, "MissionManager", "createObjectives", (" - %d : Eliminate Agent", i));
                break;
            case LevelData::kObjectiveTypeDestroyVehicle:
                if (bindx >= 0x5C02 && bindx < 0x6682) {
                    bindx -= 0x5C02;
                    cindx = bindx / 42;
                    if ((cindx * 42) == bindx && di.vindx[cindx] != 0xFFFF) {
                        objd = new ObjDestroyVehicle(pMission->vehicle(di.vindx[cindx]));
                        LOG(Log::k_FLG_GAME, "MissionManager", "createObjectives", (" - %d : Destroy Vehicle", i));
                    } else {
                        RSERR(Log::k_FLG_GAME, "MissionManager", "createObjectives", ("0x01 incorrect offset"));
                    }
                } else  {
                    RSERR(Log::k_FLG_GAME, "MissionManager", "createObjectives", ("0x0E type not matched"));
                }

                break;
            case LevelData::kObjectiveTypeUseVehicle:
                if (bindx >= 0x5C02 && bindx < 0x6682) {
                    bindx -= 0x5C02;
                    cindx = bindx / 42;
                    if ((cindx * 42) == bindx && di.vindx[cindx] != 0xFFFF) {
                        objd = new ObjUseVehicle(pMission->vehicle(di.vindx[cindx]));
                        LOG(Log::k_FLG_GAME, "MissionManager", "createObjectives", (" - %d : Use Vehicle", i));
                        // TODO Do we have to add the vehicle to the list of object to evacuate?
                    } else {
                        RSERR(Log::k_FLG_GAME, "MissionManager", "createObjectives", ("0x01 incorrect offset"));
                    }
                } else {
                    RSERR(Log::k_FLG_GAME, "MissionManager", "createObjectives", ("0x0F type not matched"));
                }
                break;
            case LevelData::kObjectiveTypeEvacuate:
                // realworld coordinates, not tile based
                objd = new ObjEvacuate(rs_utl::READ_LE_INT16(obj.mapposx),
                    rs_utl::READ_LE_INT16(obj.mapposy),
                    rs_utl::READ_LE_INT16(obj.mapposz),
                    peds_evacuate);
                LOG(Log::k_FLG_GAME, "MissionManager", "createObjectives", (" - %d : Evacuate ped", i));

                break;
#ifdef _DEBUG
            default:
                RSERR(Log::k_FLG_GAME, "MissionManager", "createObjectives", ("Unknown objective %X", rs_utl::READ_LE_UINT16(obj.type)));
                break;
#endif
        }

        // An objective has been created, adds it to the list
        // of objectives
        if (objd != nullptr) {
            pMission->addObjective(objd);
        } else {
            break;
        }
    }

    // Clear temp list
    peds_evacuate.clear();
}

}
