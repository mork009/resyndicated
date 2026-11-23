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

#include "art_panel.h"
#include "main_window.h"
#include "icon_manager.h"
#include "resource.h"
#include "dialog_theme.h"
#include "ai_tactics_dialog.h"
#include "sprite_rasterizer.h"
#include "core/editor_commands.h"
#include "core/editor_config.h"

#include <windowsx.h>
#include <uxtheme.h>
#include <format>
#include <algorithm>

#include "objective_dialog.h"
#include "rs-kernel/model/objectivedesc.h"
#include "rs-kernel/mgr/pedmanager.h"
#include "rs-kernel/mgr/weaponmanager.h"
#include "rs-kernel/mgr/missionmanager.h"
#include "rs-kernel/mgr/vehiclemanager.h"
#include "rs-kernel/model/vehicle.h"
#include "rs-kernel/model/train.h"
#include "rs-kernel/model/weapon.h"
#include "rs-kernel/model/static.h"
#include "rs-kernel/model/leveldata.h"
#include "rs-engine/gfx/animationmanager.h"
#include "rs-utils/io/file.h"

namespace rs_win_editor {

static bool isAmericanRevoltAvailable() {
    EditorPathsConfig pathsCfg;
    if (EditorConfig::loadPathsConfig(pathsCfg) && !pathsCfg.dataDir.empty()) {
        return rs_utl::File::hasAmericanRevoltData(pathsCfg.dataDir);
    }
    return rs_utl::File::hasAmericanRevoltData();
}

struct WeaponItemDef {
    rs_knl::Weapon::WeaponType type;
    const wchar_t *name;
    const wchar_t *abbr;
};

static const WeaponItemDef kWeaponsList[] = {
    { rs_knl::Weapon::Pistol,       L"Pistol",            L"PST" },
    { rs_knl::Weapon::Shotgun,      L"Shotgun",           L"SHT" },
    { rs_knl::Weapon::Uzi,          L"Uzi",               L"UZI" },
    { rs_knl::Weapon::Minigun,      L"Minigun",           L"MIN" },
    { rs_knl::Weapon::Laser,        L"Laser Gun",         L"LSR" },
    { rs_knl::Weapon::Flamer,       L"Flamer",            L"FLM" },
    { rs_knl::Weapon::LongRange,    L"Long Range Sniper", L"LRG" },
    { rs_knl::Weapon::GaussGun,     L"Gauss Gun",         L"GAU" },
    { rs_knl::Weapon::Scanner,      L"Scanner",           L"SCN" },
    { rs_knl::Weapon::MediKit,      L"Medikit",           L"MED" },
    { rs_knl::Weapon::TimeBomb,     L"Time Bomb",         L"BMB" },
    { rs_knl::Weapon::AccessCard,   L"Access Card",       L"CRD" },
    { rs_knl::Weapon::EnergyShield, L"Energy Shield",     L"SHL" },
    { rs_knl::Weapon::Persuadatron, L"Persuadertron",     L"PRS" },
    { rs_knl::Weapon::AirStrike,    L"Air Raid Com (AR)", L"AIR" },
    { rs_knl::Weapon::CloneShield,  L"Clone Shield (AR)", L"CLN" }
};

static const wchar_t* kArtPanelClassName = L"ReSyndicatedArtPanelClass";
static const wchar_t* kTileGridClassName = L"ReSyndicatedTileGridClass";
static const wchar_t* kEntityGridClassName = L"ReSyndicatedEntityGridClass";
static const wchar_t* kSearchResultsClassName = L"ReSyndicatedSearchResultsClass";
static const wchar_t* kInspectPaneClassName = L"ReSyndicatedInspectPaneClass";
static const wchar_t* kEntityPreviewClassName = L"ReSyndicatedEntityPreviewClass";

using EntityKind = WinArtPanel::EntityKind;
using EntityItem = WinArtPanel::EntityItem;

static std::vector<EntityItem> s_actorItems = {
    { "Civilian (Male)",   L"Civ (M)",     rs_knl::PedInstance::kPedTypeCivilian, 1330, 1330, 2,  EntityKind::Actor },
    { "Civilian (Female)", L"Civ (F)",     rs_knl::PedInstance::kPedTypeCivilian, 626,  626,  2,  EntityKind::Actor },
    { "Civilian (Punk)",   L"Civ (Punk)",  rs_knl::PedInstance::kPedTypeCivilian, 1123, 1123, 2,  EntityKind::Actor },
    { "Civilian (Suit)",   L"Civ (Suit)",  rs_knl::PedInstance::kPedTypeCivilian, 1537, 1537, 2,  EntityKind::Actor },
    { "Criminal (Punk)",   L"Cri (Punk)",  rs_knl::PedInstance::kPedTypeCriminal, 1123, 1123, 4,  EntityKind::Actor },
    { "Criminal (Merc)",   L"Cri (Merc)",  rs_knl::PedInstance::kPedTypeCriminal, 1330, 1330, 4,  EntityKind::Actor },
    { "Platform Guard",    L"Guard",       rs_knl::PedInstance::kPedTypeGuard,    1744, 1744, 8,  EntityKind::Actor },
    { "Police Officer",    L"Police",      rs_knl::PedInstance::kPedTypePolice,   833,  833,  6,  EntityKind::Actor },
    { "Agent",             L"Agent",       rs_knl::PedInstance::kPedTypeAgent,    1,    1,    16, EntityKind::Actor }
};

static std::vector<EntityItem> s_vehicleItems = {
    { "Regular Car",               L"Car",        rs_knl::Vehicle::kVehicleTypeRegularCar,   446, 446, 30,  EntityKind::Vehicle },
    { "Armored Car",               L"Armored",    rs_knl::Vehicle::kVehicleTypeSmallArmored, 534, 534, 10,  EntityKind::Vehicle },
    { "Large Armored",             L"Lg Armor",   rs_knl::Vehicle::kVehicleTypeLargeArmored, 400, 400, 600, EntityKind::Vehicle },
    { "Police Car",                L"Police",     rs_knl::Vehicle::kVehicleTypePolice,       470, 470, 120, EntityKind::Vehicle },
    { "Firetruck",                 L"Fire",       rs_knl::Vehicle::kVehicleTypeFireFighter,  518, 518, 40,  EntityKind::Vehicle },
    { "Ambulance",                 L"Ambulance",  rs_knl::Vehicle::kVehicleTypeMedics,       494, 494, 115, EntityKind::Vehicle },
    { "Train Engine",              L"Train H",    rs_knl::Vehicle::kVehicleTypeTrainHead,    416, 416, 100, EntityKind::Vehicle },
    { "Train Carriage (Windows)",  L"Train B",    rs_knl::Vehicle::kVehicleTypeTrainBody,    428, 428, 80,  EntityKind::Vehicle },
    { "Train Carriage (Door)",     L"Train Door", rs_knl::Vehicle::kVehicleTypeTrainBody,    434, 434, 80,  EntityKind::Vehicle },
    { "Train Carriage (Raised Roof)", L"Train Vent", rs_knl::Vehicle::kVehicleTypeTrainBody, 440, 440, 80, EntityKind::Vehicle },
    { "Train (Full Group)",        L"Train Grp",  0xFE,                                      416, 416, 100, EntityKind::Vehicle }
};

static std::vector<EntityItem> s_staticItems = {
    { "Sliding Door (Metal)",      L"Door Met", 0x0C, 1077, 1077, 500, EntityKind::Static },
    { "Sliding Door (Glass)",      L"Door Gls", 0x0C, 1085, 1085, 200, EntityKind::Static },
    { "Sliding Door (Security)",   L"Door Sec", 0x0C, 1093, 1093, 800, EntityKind::Static },
    { "Sliding Door (Reinforced)", L"Door Rnf", 0x0C, 1101, 1101, 1000, EntityKind::Static },
    { "Sliding Door (Hazard)",     L"Door Haz", 0x0C, 1109, 1109, 1200, EntityKind::Static },
    { "Large Gate Door",           L"Gate",     0x26, 395,  395,  2000, EntityKind::Static },
    { "Window (Closed)",           L"Win Cls",  0x13, 550,  550,  50, EntityKind::Static },
    { "Window (Open)",             L"Win Opn",  0x12, 552,  552,  50, EntityKind::Static },
    { "Window (Damaged)",          L"Win Dmg",  0x15, 556,  556,  50, EntityKind::Static },
    { "Window (Animated)",         L"Win Anim", 0x20, 1959, 1959, 50, EntityKind::Static },
    { "Tree (Conifer)",            L"Tree Con", 0x16, 233,  233,  100, EntityKind::Static },
    { "Tree (Deciduous)",          L"Tree Dec", 0x16, 230,  230,  100, EntityKind::Static },
    { "Tree (Palm)",               L"Tree Plm", 0x16, 558,  558,  100, EntityKind::Static },
    { "Plant / Bush",              L"Plant",    0x16, 227,  227,  50, EntityKind::Static },
    { "Phone Booth",               L"Booth",    0x01, 618,  618,  100, EntityKind::Static },
    { "Traffic Light",             L"Light",    0x05, 1040, 1040, 100, EntityKind::Static },
    { "Mailbox",                   L"Mailbox",  0x19, 561,  561,  50, EntityKind::Static },
    { "Trash Bin",                 L"Trash",    0x1A, 562,  562,  50, EntityKind::Static },
    // Wall Advertisements (0x1F)
    { "Billboard (Bullfrog)",      L"Ad Frog",  0x1F, 572,  572,  50, EntityKind::Static },
    { "Billboard (Corporate)",     L"Ad Corp",  0x1F, 574,  574,  50, EntityKind::Static },
    { "Poster (Woman A)",          L"Post A",   0x1F, 570,  570,  50, EntityKind::Static },
    { "Poster (Woman B)",          L"Post B",   0x1F, 571,  571,  50, EntityKind::Static },
    // Neon Signs (0x0A) - 16 varieties
    { "Neon Sign 1",               L"Neon 1",   0x0A, 1045, 1045, 50, EntityKind::Static },
    { "Neon Sign 2",               L"Neon 2",   0x0A, 1047, 1047, 50, EntityKind::Static },
    { "Neon Sign 3",               L"Neon 3",   0x0A, 1049, 1049, 50, EntityKind::Static },
    { "Neon Sign 4",               L"Neon 4",   0x0A, 1051, 1051, 50, EntityKind::Static },
    { "Neon Sign 5",               L"Neon 5",   0x0A, 1053, 1053, 50, EntityKind::Static },
    { "Neon Sign 6",               L"Neon 6",   0x0A, 1055, 1055, 50, EntityKind::Static },
    { "Neon Sign 7",               L"Neon 7",   0x0A, 1057, 1057, 50, EntityKind::Static },
    { "Neon Sign 8",               L"Neon 8",   0x0A, 1059, 1059, 50, EntityKind::Static },
    { "Neon Sign 9",               L"Neon 9",   0x0A, 1061, 1061, 50, EntityKind::Static },
    { "Neon Sign 10",              L"Neon 10",  0x0A, 1063, 1063, 50, EntityKind::Static },
    { "Neon Sign 11",              L"Neon 11",  0x0A, 1065, 1065, 50, EntityKind::Static },
    { "Neon Sign 12",              L"Neon 12",  0x0A, 1067, 1067, 50, EntityKind::Static },
    { "Neon Sign 13",              L"Neon 13",  0x0A, 1069, 1069, 50, EntityKind::Static },
    { "Neon Sign 14",              L"Neon 14",  0x0A, 1071, 1071, 50, EntityKind::Static },
    { "Neon Sign 15",              L"Neon 15",  0x0A, 1073, 1073, 50, EntityKind::Static },
    { "Neon Sign 16",              L"Neon 16",  0x0A, 1075, 1075, 50, EntityKind::Static }
};

static std::vector<EntityItem> s_weaponItems = {
    { "Pistol",             L"Pistol",   rs_knl::Weapon::Pistol,       368, 368, 1, EntityKind::Weapon },
    { "Shotgun",            L"Shotgun",  rs_knl::Weapon::Shotgun,      370, 370, 1, EntityKind::Weapon },
    { "Uzi",                L"Uzi",      rs_knl::Weapon::Uzi,          371, 371, 1, EntityKind::Weapon },
    { "Minigun",            L"Minigun",  rs_knl::Weapon::Minigun,      372, 372, 1, EntityKind::Weapon },
    { "Laser Gun",          L"Laser",    rs_knl::Weapon::Laser,        373, 373, 1, EntityKind::Weapon },
    { "Flamer",             L"Flamer",   rs_knl::Weapon::Flamer,       374, 374, 1, EntityKind::Weapon },
    { "Long Range Sniper",  L"Sniper",   rs_knl::Weapon::LongRange,    375, 375, 1, EntityKind::Weapon },
    { "Gauss Gun",          L"Gauss",    rs_knl::Weapon::GaussGun,     369, 369, 1, EntityKind::Weapon },
    { "Scanner",            L"Scanner",  rs_knl::Weapon::Scanner,      376, 376, 1, EntityKind::Weapon },
    { "Medikit",            L"Medikit",  rs_knl::Weapon::MediKit,      377, 377, 1, EntityKind::Weapon },
    { "Time Bomb",          L"Bomb",     rs_knl::Weapon::TimeBomb,     378, 378, 1, EntityKind::Weapon },
    { "Access Card",        L"Card",     rs_knl::Weapon::AccessCard,   379, 379, 1, EntityKind::Weapon },
    { "Energy Shield",      L"Shield",   rs_knl::Weapon::EnergyShield, 381, 381, 1, EntityKind::Weapon },
    { "Persuadertron",      L"Persuader",rs_knl::Weapon::Persuadatron, 367, 367, 1, EntityKind::Weapon },
    { "Air Raid Com (AR)",  L"Air Raid", rs_knl::Weapon::AirStrike,    378, 378, 1, EntityKind::Weapon },
    { "Clone Shield (AR)",  L"Clone Sh", rs_knl::Weapon::CloneShield,  381, 381, 1, EntityKind::Weapon }
};

static HBITMAP createEntityThumbnail(uint16_t animId, int frameId, int w, int h) {
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdcScreen = GetDC(nullptr);
    uint32_t *pBits = nullptr;
    HBITMAP hBmp = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&pBits), nullptr, 0);
    ReleaseDC(nullptr, hdcScreen);

    if (hBmp && pBits) {
        for (int i = 0; i < w * h; ++i) {
            int x = i % w;
            int y = i / w;
            bool check = ((x / 8) + (y / 8)) % 2 == 0;
            pBits[i] = check ? 0xFF181C18 : 0xFF202620;
        }

        int minX = 0, minY = 0, maxX = 0, maxY = 0;
        if (SpriteRasterizer::instance().getAnimFrameBounds(animId, frameId, minX, minY, maxX, maxY)) {
            float sprW = static_cast<float>(maxX - minX);
            float sprH = static_cast<float>(maxY - minY);
            float scale = 1.0f;
            float maxW = static_cast<float>(w - 8);
            float maxH = static_cast<float>(h - 8);
            if (sprW > maxW || sprH > maxH) {
                scale = std::min(maxW / sprW, maxH / sprH);
            }
            int anchorX = static_cast<int>(w / 2.0f - ((minX + maxX) / 2.0f) * scale);
            int anchorY = static_cast<int>(h / 2.0f - ((minY + maxY) / 2.0f) * scale);
            SpriteRasterizer::instance().drawAnimFrame(pBits, w, h, animId, frameId, anchorX, anchorY, scale);
        } else {
            SpriteRasterizer::instance().drawAnimFrame(pBits, w, h, animId, frameId, w / 2, h - 8, 1.0f);
        }
    }
    return hBmp;
}

WinArtPanel::WinArtPanel() {
    hBgBrush_ = CreateSolidBrush(RGB(28, 30, 34));
    hInputBgBrush_ = CreateSolidBrush(RGB(20, 22, 26));
    hBtnBrush_ = CreateSolidBrush(RGB(38, 42, 48));

    NONCLIENTMETRICSW ncm = { sizeof(NONCLIENTMETRICSW) };
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &ncm, 0);
    hFont_ = CreateFontIndirectW(&ncm.lfMessageFont);

    LOGFONTW lfBold = ncm.lfMessageFont;
    lfBold.lfWeight = FW_BOLD;
    hBoldFont_ = CreateFontIndirectW(&lfBold);

    LOGFONTW lfHeader = ncm.lfMessageFont;
    lfHeader.lfHeight = -15; // Prominent header size
    lfHeader.lfWeight = FW_BOLD;
    hHeaderFont_ = CreateFontIndirectW(&lfHeader);
}

WinArtPanel::~WinArtPanel() {
    cleanupEntityThumbnails();
    cleanupWeaponIcons();
    if (hFont_) { DeleteObject(hFont_); hFont_ = nullptr; }
    if (hBoldFont_) { DeleteObject(hBoldFont_); hBoldFont_ = nullptr; }
    if (hHeaderFont_) { DeleteObject(hHeaderFont_); hHeaderFont_ = nullptr; }
    if (hBgBrush_) DeleteObject(hBgBrush_);
    if (hInputBgBrush_) DeleteObject(hInputBgBrush_);
    if (hBtnBrush_) DeleteObject(hBtnBrush_);
    if (hBmpPencil_) DeleteObject(hBmpPencil_);
    if (hBmpBox_) DeleteObject(hBmpBox_);
    if (hBmpSelect_) DeleteObject(hBmpSelect_);
    if (hBmpSearch_) DeleteObject(hBmpSearch_);
    if (hBmpTarget_) DeleteObject(hBmpTarget_);
}

const std::vector<const WinArtPanel::EntityItem*>& WinArtPanel::getEntityItemsForCategory(int category) const {
    static std::vector<const EntityItem*> allItems;
    static std::vector<const EntityItem*> actorItems;
    static std::vector<const EntityItem*> vehicleItems;
    static std::vector<const EntityItem*> staticItems;
    static std::vector<const EntityItem*> weaponItems;
    static bool initialized = false;
    static int lastHasAR = -1;

    bool hasAR = isAmericanRevoltAvailable();
    int currentAR = hasAR ? 1 : 0;

    // Dynamically synchronize actor template health values with characters.dat
    for (auto &item : s_actorItems) {
        auto pType = static_cast<rs_knl::PedInstance::PedType>(item.typeId);
        bool isEnemy = (pType == rs_knl::PedInstance::kPedTypeAgent);
        item.defaultHealth = rs_knl::PedManager::getStats(pType, isEnemy).health;
    }

    // Dynamically synchronize vehicle template health values with vehicles.dat
    for (auto &item : s_vehicleItems) {
        if (item.typeId == 0xFE) {
            int headHp = rs_knl::VehicleManager::getStats(rs_knl::Vehicle::kVehicleTypeTrainHead).health;
            int bodyHp = rs_knl::VehicleManager::getStats(rs_knl::Vehicle::kVehicleTypeTrainBody).health;
            item.defaultHealth = headHp * 2 + bodyHp * 4;
        } else {
            const auto &vStats = rs_knl::VehicleManager::getStats(static_cast<uint8_t>(item.typeId));
            item.defaultHealth = vStats.health;
        }
    }

    if (!initialized || lastHasAR != currentAR) {
        allItems.clear();
        actorItems.clear();
        vehicleItems.clear();
        staticItems.clear();
        weaponItems.clear();

        size_t numWpns = hasAR ? s_weaponItems.size() : 14;
        allItems.reserve(s_actorItems.size() + s_vehicleItems.size() + s_staticItems.size() + numWpns);
        actorItems.reserve(s_actorItems.size());
        vehicleItems.reserve(s_vehicleItems.size());
        staticItems.reserve(s_staticItems.size());
        weaponItems.reserve(numWpns);

        for (const auto &item : s_actorItems) {
            actorItems.push_back(&item);
            allItems.push_back(&item);
        }
        for (const auto &item : s_vehicleItems) {
            vehicleItems.push_back(&item);
            allItems.push_back(&item);
        }
        for (const auto &item : s_staticItems) {
            staticItems.push_back(&item);
            allItems.push_back(&item);
        }
        for (const auto &item : s_weaponItems) {
            if (!hasAR && rs_knl::Weapon::isAmericanRevoltOnly(static_cast<rs_knl::Weapon::WeaponType>(item.typeId))) {
                continue;
            }
            weaponItems.push_back(&item);
            allItems.push_back(&item);
        }
        initialized = true;
        lastHasAR = currentAR;
    }

    if (category == 0) return allItems;
    if (category == 1) return actorItems;
    if (category == 2) return vehicleItems;
    if (category == 3) return staticItems;
    if (category == 4) return weaponItems;
    return allItems;
}

const WinArtPanel::EntityItem* WinArtPanel::selectedEntityTemplateItem() const {
    const auto &items = getEntityItemsForCategory(entityCategory_);
    if (selectedEntityGridIdx_ >= 0 && selectedEntityGridIdx_ < static_cast<int>(items.size())) {
        return items[selectedEntityGridIdx_];
    }
    return nullptr;
}

void WinArtPanel::refreshEntityThumbnails() {
    cleanupEntityThumbnails();
    int thumbW = 64;
    int thumbH = 46;
    for (auto &item : s_actorItems) {
        item.hThumb = createEntityThumbnail(item.drawAnim, 0, thumbW, thumbH);
    }
    for (auto &item : s_vehicleItems) {
        item.hThumb = createEntityThumbnail(item.drawAnim, 0, thumbW, thumbH);
    }
    for (auto &item : s_staticItems) {
        item.hThumb = createEntityThumbnail(item.drawAnim, 0, thumbW, thumbH);
    }
    bool hasAR = isAmericanRevoltAvailable();
    for (auto &item : s_weaponItems) {
        if (!hasAR && rs_knl::Weapon::isAmericanRevoltOnly(static_cast<rs_knl::Weapon::WeaponType>(item.typeId))) {
            continue;
        }
        item.hThumb = createEntityThumbnail(item.drawAnim, 0, thumbW, thumbH);
    }
}

void WinArtPanel::cleanupEntityThumbnails() {
    for (auto &item : s_actorItems) {
        if (item.hThumb) { DeleteObject(item.hThumb); item.hThumb = nullptr; }
    }
    for (auto &item : s_vehicleItems) {
        if (item.hThumb) { DeleteObject(item.hThumb); item.hThumb = nullptr; }
    }
    for (auto &item : s_staticItems) {
        if (item.hThumb) { DeleteObject(item.hThumb); item.hThumb = nullptr; }
    }
    for (auto &item : s_weaponItems) {
        if (item.hThumb) { DeleteObject(item.hThumb); item.hThumb = nullptr; }
    }
}

void WinArtPanel::refreshWeaponIcons() {
    cleanupWeaponIcons();
    bool hasAR = isAmericanRevoltAvailable();
    int numWeps = hasAR ? 16 : 14;
    for (int i = 0; i < numWeps && i < 16; ++i) {
        hWeaponIcons_[i] = SpriteRasterizer::instance().createWeaponIconBitmap(kWeaponsList[i].type, 0, 0);
    }
}

void WinArtPanel::cleanupWeaponIcons() {
    for (int i = 0; i < 16; ++i) {
        if (hWeaponIcons_[i]) {
            DeleteObject(hWeaponIcons_[i]);
            hWeaponIcons_[i] = nullptr;
        }
    }
}

void WinArtPanel::updateEntityTypeComboLabels() {
    if (!hComboEntityType_) return;
    int curSel = ComboBox_GetCurSel(hComboEntityType_);
    if (curSel < 0) curSel = 0;

    const auto &allEnts = getEntityItemsForCategory(0);
    const auto &actorEnts = getEntityItemsForCategory(1);
    const auto &vehEnts = getEntityItemsForCategory(2);
    const auto &staticEnts = getEntityItemsForCategory(3);
    const auto &weaponEnts = getEntityItemsForCategory(4);

    std::wstring allEntLabel = L"All Entities (" + std::to_wstring(allEnts.size()) + L")";
    std::wstring actorLabel = L"Characters (" + std::to_wstring(actorEnts.size()) + L")";
    std::wstring vehLabel = L"Vehicles & Trains (" + std::to_wstring(vehEnts.size()) + L")";
    std::wstring staticLabel = L"Static Objects (" + std::to_wstring(staticEnts.size()) + L")";
    std::wstring weaponLabel = L"Weapon Drops (" + std::to_wstring(weaponEnts.size()) + L")";

    SendMessageW(hComboEntityType_, CB_RESETCONTENT, 0, 0);
    SendMessageW(hComboEntityType_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(allEntLabel.c_str()));
    SendMessageW(hComboEntityType_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(actorLabel.c_str()));
    SendMessageW(hComboEntityType_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(vehLabel.c_str()));
    SendMessageW(hComboEntityType_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(staticLabel.c_str()));
    SendMessageW(hComboEntityType_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(weaponLabel.c_str()));
    SendMessageW(hComboEntityType_, CB_SETDROPPEDWIDTH, 170, 0);
    SendMessageW(hComboEntityType_, CB_SETCURSEL, curSel, 0);
}

HBITMAP WinArtPanel::getWeaponIcon(int index) const {
    if (index < 0 || index >= 16) return nullptr;
    if (!hWeaponIcons_[index]) {
        const_cast<WinArtPanel*>(this)->hWeaponIcons_[index] =
            SpriteRasterizer::instance().createWeaponIconBitmap(kWeaponsList[index].type, 0, 0);
    }
    return hWeaponIcons_[index];
}

bool WinArtPanel::create(HWND hWndParent, int x, int y, int w, int h, int id) {
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = hBgBrush_;
    wc.lpszClassName = kArtPanelClassName;
    RegisterClassExW(&wc);

    WNDCLASSEXW wcg = {};
    wcg.cbSize = sizeof(WNDCLASSEXW);
    wcg.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcg.lpfnWndProc = TileGridProc;
    wcg.hInstance = hInstance;
    wcg.hCursor = LoadCursor(nullptr, IDC_HAND);
    wcg.hbrBackground = CreateSolidBrush(RGB(20, 24, 20));
    wcg.lpszClassName = kTileGridClassName;
    RegisterClassExW(&wcg);

    WNDCLASSEXW wce = {};
    wce.cbSize = sizeof(WNDCLASSEXW);
    wce.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wce.lpfnWndProc = EntityGridProc;
    wce.hInstance = hInstance;
    wce.hCursor = LoadCursor(nullptr, IDC_HAND);
    wce.hbrBackground = CreateSolidBrush(RGB(20, 24, 20));
    wce.lpszClassName = kEntityGridClassName;
    RegisterClassExW(&wce);

    WNDCLASSEXW wcs = {};
    wcs.cbSize = sizeof(WNDCLASSEXW);
    wcs.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcs.lpfnWndProc = SearchResultsProc;
    wcs.hInstance = hInstance;
    wcs.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcs.hbrBackground = CreateSolidBrush(RGB(22, 25, 30));
    wcs.lpszClassName = kSearchResultsClassName;
    RegisterClassExW(&wcs);

    WNDCLASSEXW wcp = {};
    wcp.cbSize = sizeof(WNDCLASSEXW);
    wcp.style = CS_HREDRAW | CS_VREDRAW;
    wcp.lpfnWndProc = InspectPaneProc;
    wcp.hInstance = hInstance;
    wcp.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcp.hbrBackground = CreateSolidBrush(RGB(20, 22, 26));
    wcp.lpszClassName = kInspectPaneClassName;
    RegisterClassExW(&wcp);

    WNDCLASSEXW wcprev = {};
    wcprev.cbSize = sizeof(WNDCLASSEXW);
    wcprev.style = CS_HREDRAW | CS_VREDRAW;
    wcprev.lpfnWndProc = EntityPreviewProc;
    wcprev.hInstance = hInstance;
    wcprev.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcprev.hbrBackground = CreateSolidBrush(RGB(14, 16, 20));
    wcprev.lpszClassName = kEntityPreviewClassName;
    RegisterClassExW(&wcprev);

    hWnd_ = CreateWindowExW(
        0, kArtPanelClassName, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        x, y, w, h,
        hWndParent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        hInstance, this
    );

    createChildControls();
    refreshPalette();
    return (hWnd_ != nullptr);
}

static LRESULT CALLBACK DarkTabControlSubclass(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/) {
    if (msg == WM_LBUTTONDOWN) {
        TCHITTESTINFO hit = {};
        hit.pt.x = GET_X_LPARAM(lParam);
        hit.pt.y = GET_Y_LPARAM(lParam);
        int item = TabCtrl_HitTest(hWnd, &hit);
        if (item >= 0) {
            TabCtrl_SetCurSel(hWnd, item);
            HWND hParent = GetParent(hWnd);
            NMHDR nm = {};
            nm.hwndFrom = hWnd;
            nm.idFrom = static_cast<UINT_PTR>(GetDlgCtrlID(hWnd));
            nm.code = TCN_SELCHANGE;
            SendMessageW(hParent, WM_NOTIFY, nm.idFrom, reinterpret_cast<LPARAM>(&nm));
            InvalidateRect(hWnd, nullptr, TRUE);
        }
        return 0;
    }
    if (msg == WM_ERASEBKGND) {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        RECT rc;
        GetClientRect(hWnd, &rc);
        HBRUSH hBr = CreateSolidBrush(RGB(24, 26, 30));
        FillRect(hdc, &rc, hBr);
        DeleteObject(hBr);
        return 1;
    }
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);

        // 1. Fill entire tab control background with dark palette
        HBRUSH hBg = CreateSolidBrush(RGB(24, 26, 30));
        FillRect(hdc, &rc, hBg);
        DeleteObject(hBg);

        // 2. Iterate each tab item and send WM_DRAWITEM to parent
        int count = TabCtrl_GetItemCount(hWnd);
        HWND hParent = GetParent(hWnd);
        int curSel = TabCtrl_GetCurSel(hWnd);

        for (int i = 0; i < count; ++i) {
            RECT rcItem = {};
            TabCtrl_GetItemRect(hWnd, i, &rcItem);

            DRAWITEMSTRUCT dis = {};
            dis.CtlType = ODT_TAB;
            dis.CtlID = static_cast<UINT>(GetDlgCtrlID(hWnd));
            dis.itemID = static_cast<UINT>(i);
            dis.itemAction = ODA_DRAWENTIRE;
            dis.itemState = (i == curSel) ? ODS_SELECTED : 0;
            dis.hwndItem = hWnd;
            dis.hDC = hdc;
            dis.rcItem = rcItem;
            dis.itemData = 0;

            SendMessageW(hParent, WM_DRAWITEM, dis.CtlID, reinterpret_cast<LPARAM>(&dis));
        }

        // 3. Draw clean dark border and dividing line below tabs
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(45, 52, 60));
        HPEN hOld = static_cast<HPEN>(SelectObject(hdc, hPen));
        MoveToEx(hdc, 0, rc.bottom - 1, nullptr);
        LineTo(hdc, rc.right, rc.bottom - 1);
        SelectObject(hdc, hOld);
        DeleteObject(hPen);

        EndPaint(hWnd, &ps);
        return 0;
    }
    if (msg == WM_NCDESTROY) {
        RemoveWindowSubclass(hWnd, DarkTabControlSubclass, uIdSubclass);
    }
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

static void drawDarkButton(DRAWITEMSTRUCT *pDIS, const wchar_t *text, bool isDanger = false, bool isPrimary = false) {
    HDC hdc = pDIS->hDC;
    RECT rc = pDIS->rcItem;

    bool isPressed = (pDIS->itemState & ODS_SELECTED) != 0;
    bool isDisabled = (pDIS->itemState & ODS_DISABLED) != 0;
    bool isFocused = (pDIS->itemState & ODS_FOCUS) != 0;

    COLORREF bgCol;
    COLORREF borderCol;
    COLORREF textCol;

    if (isDisabled) {
        bgCol = RGB(30, 33, 38);
        borderCol = RGB(45, 50, 56);
        textCol = RGB(100, 108, 116);
    } else if (isPressed) {
        bgCol = isDanger ? RGB(80, 20, 24) : (isPrimary ? RGB(10, 60, 80) : RGB(22, 25, 30));
        borderCol = isDanger ? RGB(220, 50, 50) : RGB(0, 180, 216);
        textCol = RGB(255, 255, 255);
    } else {
        if (isPrimary) {
            bgCol = RGB(28, 52, 68);
            borderCol = RGB(0, 160, 200);
            textCol = RGB(220, 245, 255);
        } else if (isDanger) {
            bgCol = RGB(48, 30, 32);
            borderCol = RGB(140, 50, 55);
            textCol = RGB(240, 200, 205);
        } else {
            bgCol = RGB(38, 42, 48);
            borderCol = isFocused ? RGB(0, 180, 216) : RGB(58, 65, 74);
            textCol = RGB(225, 230, 235);
        }
    }

    HBRUSH hBr = CreateSolidBrush(bgCol);
    FillRect(hdc, &rc, hBr);
    DeleteObject(hBr);

    HPEN hPen = CreatePen(PS_SOLID, 1, borderCol);
    HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
    HBRUSH hOldBr = static_cast<HBRUSH>(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(hdc, hOldBr);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);

    if (isFocused && !isDisabled && !isPressed) {
        HPEN hFocusPen = CreatePen(PS_SOLID, 1, isDanger ? RGB(255, 100, 100) : RGB(0, 229, 255));
        HPEN hPrev = static_cast<HPEN>(SelectObject(hdc, hFocusPen));
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rc.left + 1, rc.top + 1, rc.right - 1, rc.bottom - 1);
        SelectObject(hdc, hPrev);
        DeleteObject(hFocusPen);
    }

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textCol);
    HFONT hFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));

    RECT rcText = rc;
    if (isPressed) {
        OffsetRect(&rcText, 1, 1);
    }
    DrawTextW(hdc, text, -1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    SelectObject(hdc, hOldFont);
}

static LRESULT CALLBACK WeaponsGridSubclass(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    auto *pPanel = reinterpret_cast<WinArtPanel*>(dwRefData);
    if (!pPanel) return DefSubclassProc(hWnd, msg, wParam, lParam);

    switch (msg) {
        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);

            HBRUSH hBg = CreateSolidBrush(RGB(24, 26, 30));
            FillRect(hdc, &rc, hBg);
            DeleteObject(hBg);

            bool hasAR = isAmericanRevoltAvailable();
            int numWeps = hasAR ? 16 : 14;
            int cols = hasAR ? 8 : 7;
            int rows = 2;
            int gap = 3;
            int totalW = rc.right - rc.left;
            int totalH = rc.bottom - rc.top;
            int cellW = (totalW - (cols - 1) * gap) / cols;
            int cellH = (totalH - (rows - 1) * gap) / rows;

            HFONT hFont = CreateFontW(
                -11, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
            );
            HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));
            SetBkMode(hdc, TRANSPARENT);

            for (int i = 0; i < numWeps; ++i) {
                int col = i % cols;
                int row = i / cols;
                int cx = col * (cellW + gap);
                int cy = row * (cellH + gap);
                RECT cellRc = { cx, cy, cx + cellW, cy + cellH };

                bool isEquipped = (pPanel->selectedWeapons().count(i) > 0);
                bool isHovered = (pPanel->hoveredWeaponIndex() == i);

                COLORREF bgCol = isEquipped ? (isHovered ? RGB(28, 68, 100) : RGB(22, 54, 82))
                                            : (isHovered ? RGB(38, 44, 52) : RGB(28, 32, 38));
                COLORREF borderCol = isHovered ? RGB(0, 229, 255)
                                               : (isEquipped ? RGB(0, 168, 204) : RGB(48, 54, 64));
                COLORREF textCol = isEquipped ? RGB(255, 255, 255)
                                              : (isHovered ? RGB(220, 230, 242) : RGB(140, 150, 165));

                HBRUSH hCellBr = CreateSolidBrush(bgCol);
                FillRect(hdc, &cellRc, hCellBr);
                DeleteObject(hCellBr);

                HPEN hPen = CreatePen(PS_SOLID, 1, borderCol);
                HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
                HBRUSH hOldNull = static_cast<HBRUSH>(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
                Rectangle(hdc, cellRc.left, cellRc.top, cellRc.right, cellRc.bottom);
                SelectObject(hdc, hOldNull);
                SelectObject(hdc, hOldPen);
                DeleteObject(hPen);

                HBITMAP hIcon = pPanel->getWeaponIcon(i);
                if (hIcon) {
                    BITMAP bm;
                    GetObject(hIcon, sizeof(bm), &bm);
                    int bmpW = bm.bmWidth;
                    int bmpH = bm.bmHeight;
                    int maxDrawW = cellW - 4;
                    int maxDrawH = cellH - 4;
                    float s = std::min(static_cast<float>(maxDrawW) / bmpW,
                                       static_cast<float>(maxDrawH) / bmpH);
                    int drawW = std::max(1, static_cast<int>(bmpW * s));
                    int drawH = std::max(1, static_cast<int>(bmpH * s));
                    int dstX = cellRc.left + (cellW - drawW) / 2;
                    int dstY = cellRc.top + (cellH - drawH) / 2;
                    HDC hdcMem = CreateCompatibleDC(hdc);
                    HBITMAP hOldBmp = static_cast<HBITMAP>(SelectObject(hdcMem, hIcon));
                    BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
                    AlphaBlend(hdc, dstX, dstY, drawW, drawH, hdcMem, 0, 0, bmpW, bmpH, bf);
                    SelectObject(hdcMem, hOldBmp);
                    DeleteDC(hdcMem);
                } else {
                    SetTextColor(hdc, textCol);
                    DrawTextW(hdc, kWeaponsList[i].abbr, -1, &cellRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
                }
            }

            SelectObject(hdc, hOldFont);
            DeleteObject(hFont);
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_MOUSEMOVE: {
            RECT rc;
            GetClientRect(hWnd, &rc);
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            bool hasAR = isAmericanRevoltAvailable();
            int numWeps = hasAR ? 16 : 14;
            int cols = hasAR ? 8 : 7;
            int rows = 2;
            int gap = 3;
            int totalW = rc.right - rc.left;
            int totalH = rc.bottom - rc.top;
            int cellW = (totalW - (cols - 1) * gap) / cols;
            int cellH = (totalH - (rows - 1) * gap) / rows;

            int hovered = -1;
            int col = x / (cellW + gap);
            int row = y / (cellH + gap);
            if (col >= 0 && col < cols && row >= 0 && row < rows) {
                int idx = row * cols + col;
                if (idx < numWeps) {
                    hovered = idx;
                }
            }

            if (hovered != pPanel->hoveredWeaponIndex()) {
                pPanel->setHoveredWeaponIndex(hovered);
                InvalidateRect(hWnd, nullptr, FALSE);

                if (hovered >= 0) {
                    std::wstring label = std::format(L"Weapons: [{}] (Click to toggle)", kWeaponsList[hovered].name);
                    pPanel->setWeaponsLabelText(label);
                } else {
                    pPanel->setWeaponsLabelText(L"Weapons Inventory (Equipped):");
                }

                TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hWnd, 0 };
                TrackMouseEvent(&tme);
            }
            return 0;
        }

        case WM_MOUSELEAVE: {
            pPanel->setHoveredWeaponIndex(-1);
            pPanel->setWeaponsLabelText(L"Weapons Inventory (Equipped):");
            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            RECT rc;
            GetClientRect(hWnd, &rc);
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            bool hasAR = isAmericanRevoltAvailable();
            int numWeps = hasAR ? 16 : 14;
            int cols = hasAR ? 8 : 7;
            int rows = 2;
            int gap = 3;
            int totalW = rc.right - rc.left;
            int totalH = rc.bottom - rc.top;
            int cellW = (totalW - (cols - 1) * gap) / cols;
            int cellH = (totalH - (rows - 1) * gap) / rows;

            int col = x / (cellW + gap);
            int row = y / (cellH + gap);
            if (col >= 0 && col < cols && row >= 0 && row < rows) {
                int idx = row * cols + col;
                if (idx < numWeps) {
                    pPanel->toggleWeapon(idx);
                }
            }
            return 0;
        }

        case WM_MOUSEWHEEL:
            return SendMessageW(GetParent(hWnd), msg, wParam, lParam);

        case WM_NCDESTROY:
            RemoveWindowSubclass(hWnd, WeaponsGridSubclass, uIdSubclass);
            break;
    }
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

static void populateOrient8(HWND cb);
static void populateOrientVehicle(HWND cb);
static void populateOrientStatic(HWND cb);

void WinArtPanel::createChildControls() {
    HINSTANCE hInst = GetModuleHandle(nullptr);

    // 1. Tab Control (Only 3 tabs: Tiles, Entities, Objectives)
    hTabControl_ = CreateWindowExW(
        0, WC_TABCONTROLW, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_FOCUSNEVER | TCS_OWNERDRAWFIXED | TCS_FIXEDWIDTH,
        2, 2, 316, 34,
        hWnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_TAB_CONTROL)),
        hInst, nullptr
    );
    SetWindowSubclass(hTabControl_, DarkTabControlSubclass, 1002, 0);
    TabCtrl_SetImageList(hTabControl_, IconManager::instance().tabsImageList());

    TCITEMW tie = {};
    tie.mask = TCIF_TEXT | TCIF_IMAGE;

    tie.pszText = const_cast<LPWSTR>(L"Tiles");
    tie.iImage = 0;
    TabCtrl_InsertItem(hTabControl_, 0, &tie);

    tie.pszText = const_cast<LPWSTR>(L"Entities");
    tie.iImage = 1;
    TabCtrl_InsertItem(hTabControl_, 1, &tie);

    tie.pszText = const_cast<LPWSTR>(L"Objectives");
    tie.iImage = 2;
    TabCtrl_InsertItem(hTabControl_, 2, &tie);

    // Bitmaps for tool buttons
    hBmpPencil_ = IconManager::instance().createIconBitmap(IconId::Pencil, 20);
    hBmpBox_ = IconManager::instance().createIconBitmap(IconId::BoxFill, 20);
    hBmpSelect_ = IconManager::instance().createIconBitmap(IconId::Select, 20);
    hBmpSearch_ = IconManager::instance().createIconBitmap(IconId::Search, 20);
    hBmpTarget_ = IconManager::instance().createIconBitmap(IconId::Spawns, 16);

    // --- Tab 0: Tiles ---
    hComboCategory_ = CreateWindowExW(
        0, WC_COMBOBOXW, L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        6, 32, 140, 200,
        hWnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_CATEGORY)),
        hInst, nullptr
    );
    applyComboBoxTheme(hComboCategory_);
    struct TileCatEntry {
        TileCategory cat;
        const wchar_t *baseLabel;
    };
    static const TileCatEntry kTileCats[] = {
        { TileCategory::All,      L"All Tiles" },
        { TileCategory::Ground,   L"Ground (Floors)" },
        { TileCategory::Roads,    L"Roads & Crossings" },
        { TileCategory::Walls,    L"Walls & Barriers" },
        { TileCategory::Roofs,    L"Roofs" },
        { TileCategory::Stairs,   L"Stairs & Slopes" },
        { TileCategory::Specials, L"Specials (Rails/Empty)" }
    };
    for (const auto &tc : kTileCats) {
        size_t count = TileRasterizer::instance().getTilesForCategory(tc.cat).size();
        std::wstring label = std::wstring(tc.baseLabel) + L" (" + std::to_wstring(count) + L")";
        SendMessageW(hComboCategory_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }
    SendMessageW(hComboCategory_, CB_SETDROPPEDWIDTH, 190, 0);
    SendMessageW(hComboCategory_, CB_SETCURSEL, 0, 0);

    // Tile tool buttons: Pencil, BoxFill, Tile Select, Tile Search
    hBtnPencil_ = CreateWindowExW(
        0, WC_BUTTONW, L"Pencil",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        152, 30, 32, 26,
        hWnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_TOOL_PENCIL)),
        hInst, nullptr
    );

    hBtnBoxFill_ = CreateWindowExW(
        0, WC_BUTTONW, L"Box",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        188, 30, 32, 26,
        hWnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_TOOL_BOX)),
        hInst, nullptr
    );

    hBtnTileSelect_ = CreateWindowExW(
        0, WC_BUTTONW, L"Select",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        260, 30, 32, 26,
        hWnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_TOOL_TILE_SELECT)),
        hInst, nullptr
    );

    hBtnTileSearch_ = CreateWindowExW(
        0, WC_BUTTONW, L"Search",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        296, 30, 32, 26,
        hWnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_TOOL_TILE_SEARCH)),
        hInst, nullptr
    );

    // Visual Tile Grid
    hTileGrid_ = CreateWindowExW(
        0, kTileGridClassName, L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL,
        6, 62, 304, 224,
        hWnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_TILE_GRID)),
        hInst, this
    );
    enableDarkModeForControl(hTileGrid_);

    // Active Tile Inspector Scrollable Container Pane
    hTileInspectPane_ = CreateWindowExW(
        0, kInspectPaneClassName, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_VSCROLL,
        6, 294, 298, 200,
        hWnd_, nullptr, hInst, this
    );
    enableDarkModeForControl(hTileInspectPane_);

    // Active Tile Inspector Controls
    hTilePreviewBox_ = CreateWindowExW(
        0, kEntityPreviewClassName, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        0, 0, 76, 56,
        hTileInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_TILE_INSPECT_PREVIEW_BOX)),
        hInst, this
    );

    hTileInspectStaticType_ = CreateWindowExW(
        0, WC_STATICW, L"Type:",
        WS_CHILD | WS_VISIBLE,
        0, 4, 66, 20,
        hTileInspectPane_, nullptr, hInst, nullptr
    );

    hTileInspectValueType_ = CreateWindowExW(
        0, WC_STATICW, L"",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
        70, 4, 140, 20,
        hTileInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_TILE_INSPECT_TYPE)),
        hInst, nullptr
    );

    hTileInspectStaticSurface_ = CreateWindowExW(
        0, WC_STATICW, L"Surface:",
        WS_CHILD | WS_VISIBLE,
        0, 28, 66, 20,
        hTileInspectPane_, nullptr, hInst, nullptr
    );

    hTileInspectValueSurface_ = CreateWindowExW(
        0, WC_STATICW, L"",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        70, 28, 140, 32,
        hTileInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_TILE_INSPECT_SURFACE)),
        hInst, nullptr
    );

    hTileInspectStaticPos_ = CreateWindowExW(
        0, WC_STATICW, L"Position:",
        WS_CHILD,
        0, 72, 70, 20,
        hTileInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_TILE_INSPECT_POS_LABEL)),
        hInst, nullptr
    );

    hTileInspectValuePos_ = CreateWindowExW(
        0, WC_STATICW, L"",
        WS_CHILD | SS_LEFT | SS_CENTERIMAGE,
        74, 72, 224, 20,
        hTileInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_TILE_INSPECT_POS_VALUE)),
        hInst, nullptr
    );

    hTileInspectStaticPlacements_ = CreateWindowExW(
        0, WC_STATICW, L"Placements:",
        WS_CHILD | WS_VISIBLE,
        0, 96, 70, 20,
        hTileInspectPane_, nullptr, hInst, nullptr
    );

    hTileInspectValuePlacements_ = CreateWindowExW(
        0, WC_STATICW, L"",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
        74, 96, 224, 20,
        hTileInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_TILE_INSPECT_PLACEMENTS)),
        hInst, nullptr
    );

    // Search Result Table Windows
    hTileSearchResults_ = CreateWindowExW(
        0, kSearchResultsClassName, L"",
        WS_CHILD | WS_VSCROLL,
        6, 294, 298, 200,
        hWnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_TILE_SEARCH_RESULTS)),
        hInst, this
    );
    enableDarkModeForControl(hTileSearchResults_);

    hEntSearchResults_ = CreateWindowExW(
        0, kSearchResultsClassName, L"",
        WS_CHILD | WS_VSCROLL,
        6, 294, 298, 200,
        hWnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_ENT_SEARCH_RESULTS)),
        hInst, this
    );
    enableDarkModeForControl(hEntSearchResults_);

    // --- Tab 1: Entities ---
    hComboEntityType_ = CreateWindowExW(
        0, WC_COMBOBOXW, L"",
        WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        6, 32, 140, 200,
        hWnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_ENTITY_TYPE)),
        hInst, nullptr
    );
    applyComboBoxTheme(hComboEntityType_);
    updateEntityTypeComboLabels();
    SendMessageW(hComboEntityType_, CB_SETDROPPEDWIDTH, 170, 0);
    SendMessageW(hComboEntityType_, CB_SETCURSEL, 0, 0);

    // Entity tool buttons: Pencil (P), Select/Move (V), Search Entity (S)
    hBtnEntPlace_ = CreateWindowExW(
        0, WC_BUTTONW, L"Place",
        WS_CHILD | BS_OWNERDRAW,
        188, 30, 32, 26,
        hWnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_ENT_PLACE)),
        hInst, nullptr
    );

    hBtnEntSelect_ = CreateWindowExW(
        0, WC_BUTTONW, L"Select",
        WS_CHILD | BS_OWNERDRAW,
        260, 30, 32, 26,
        hWnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_ENT_SELECT)),
        hInst, nullptr
    );

    hBtnEntSearch_ = CreateWindowExW(
        0, WC_BUTTONW, L"Search",
        WS_CHILD | BS_OWNERDRAW,
        296, 30, 32, 26,
        hWnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_ENT_SEARCH)),
        hInst, nullptr
    );

    // Visual Entity Grid (4 rows high: 4 * 54 + 8 = 224)
    hEntityGrid_ = CreateWindowExW(
        0, kEntityGridClassName, L"",
        WS_CHILD | WS_VSCROLL,
        6, 62, 298, 224,
        hWnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_ENTITY_GRID)),
        hInst, this
    );
    enableDarkModeForControl(hEntityGrid_);

    // Tab 1 Lower Pane: Integrated Entity Inspector Scrollable Container Pane
    hEntInspectPane_ = CreateWindowExW(
        0, kInspectPaneClassName, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_VSCROLL,
        6, 210, 298, 340,
        hWnd_, nullptr, hInst, this
    );
    enableDarkModeForControl(hEntInspectPane_);

    hEditProperties_ = CreateWindowExW(
        0, WC_EDITW, L"No entity selected.\r\n\r\n\u2022 Click any ped, vehicle, or static object on the map using Select / Move (V) to view and edit its properties here.\r\n\r\n\u2022 Or choose an item from the catalog above and use Pencil (P) to spawn it on the map.",
        WS_CHILD | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
        0, 0, 298, 340,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_EDIT_PROPERTIES)),
        hInst, nullptr
    );
    enableDarkModeForControl(hEditProperties_);

    hInspectHeader_ = CreateWindowExW(
        0, WC_STATICW, L"Selected Entity",
        WS_CHILD | SS_LEFT,
        0, 0, 298, 20,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_HEADER)),
        hInst, nullptr
    );

    hInspectStaticType_ = CreateWindowExW(
        0, WC_STATICW, L"Subtype:",
        WS_CHILD,
        0, 24, 70, 20,
        hEntInspectPane_, nullptr, hInst, nullptr
    );

    hInspectValueType_ = CreateWindowExW(
        0, WC_STATICW, L"",
        WS_CHILD | SS_LEFT | SS_CENTERIMAGE,
        74, 24, 224, 20,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_TYPE)),
        hInst, nullptr
    );

    hInspectComboType_ = CreateWindowExW(
        0, WC_COMBOBOXW, L"",
        WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        74, 24, 224, 140,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_COMBO_TYPE)),
        hInst, nullptr
    );
    applyComboBoxTheme(hInspectComboType_);
    ComboBox_AddString(hInspectComboType_, L"Player's Agent");
    ComboBox_AddString(hInspectComboType_, L"Enemy Agent");
    ComboBox_SetCurSel(hInspectComboType_, 0);

    hInspectStaticHealth_ = CreateWindowExW(
        0, WC_STATICW, L"Health:",
        WS_CHILD,
        0, 48, 70, 20,
        hEntInspectPane_, nullptr, hInst, nullptr
    );

    hInspectEditHealth_ = CreateWindowExW(
        0, WC_EDITW, L"100",
        WS_CHILD | ES_AUTOHSCROLL,
        74, 48, 56, 22,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_HEALTH)),
        hInst, nullptr
    );
    enableDarkModeForControl(hInspectEditHealth_);
    SendMessageW(hInspectEditHealth_, WM_SETFONT, reinterpret_cast<WPARAM>(hFont_), TRUE);

    hInspectCheckDead_ = CreateWindowExW(
        0, WC_BUTTONW, L"Dead",
        WS_CHILD | BS_AUTOCHECKBOX,
        136, 48, 80, 20,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_CHECK_DEAD)),
        hInst, nullptr
    );
    enableDarkModeForControl(hInspectCheckDead_);
    SendMessageW(hInspectCheckDead_, WM_SETFONT, reinterpret_cast<WPARAM>(hFont_), TRUE);

    hInspectStaticOrient_ = CreateWindowExW(
        0, WC_STATICW, L"Direction:",
        WS_CHILD,
        0, 74, 70, 20,
        hEntInspectPane_, nullptr, hInst, nullptr
    );

    hInspectComboOrient_ = CreateWindowExW(
        0, WC_COMBOBOXW, L"",
        WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        74, 74, 224, 180,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_ORIENT)),
        hInst, nullptr
    );
    applyComboBoxTheme(hInspectComboOrient_);
    populateOrient8(hInspectComboOrient_);

    hInspectStaticPos_ = CreateWindowExW(
        0, WC_STATICW, L"Position:",
        WS_CHILD,
        0, 100, 70, 20,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_POS_LABEL)),
        hInst, nullptr
    );

    hInspectValuePos_ = CreateWindowExW(
        0, WC_STATICW, L"",
        WS_CHILD | SS_LEFT | SS_CENTERIMAGE,
        74, 100, 224, 20,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_POS_VALUE)),
        hInst, nullptr
    );

    hInspectStaticLoc_ = CreateWindowExW(
        0, WC_STATICW, L"Location:",
        WS_CHILD,
        0, 122, 70, 20,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_LOC_LABEL)),
        hInst, nullptr
    );

    hInspectValueLoc_ = CreateWindowExW(
        0, WC_STATICW, L"",
        WS_CHILD | SS_LEFT | SS_CENTERIMAGE,
        74, 122, 224, 20,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_LOC_VALUE)),
        hInst, nullptr
    );

    hEntPreviewBox_ = CreateWindowExW(
        0, kEntityPreviewClassName, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        0, 0, 80, 76,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_PREVIEW_BOX)),
        hInst, this
    );

    hInspectBtnRotLeft_ = CreateWindowExW(
        0, WC_BUTTONW, L"\u21BA",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_PUSHBUTTON,
        0, 0, 25, 20,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_ROT_LEFT)),
        hInst, nullptr
    );
    enableDarkModeForControl(hInspectBtnRotLeft_);
    SendMessageW(hInspectBtnRotLeft_, WM_SETFONT, reinterpret_cast<WPARAM>(hFont_), TRUE);

    hInspectBtnPlayAnim_ = CreateWindowExW(
        0, WC_BUTTONW, L"\u25B6",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_PUSHBUTTON,
        0, 0, 26, 20,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_PLAY_ANIM)),
        hInst, nullptr
    );
    enableDarkModeForControl(hInspectBtnPlayAnim_);
    SendMessageW(hInspectBtnPlayAnim_, WM_SETFONT, reinterpret_cast<WPARAM>(hFont_), TRUE);

    hInspectBtnRotRight_ = CreateWindowExW(
        0, WC_BUTTONW, L"\u21BB",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_PUSHBUTTON,
        0, 0, 25, 20,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_ROT_RIGHT)),
        hInst, nullptr
    );
    enableDarkModeForControl(hInspectBtnRotRight_);
    SendMessageW(hInspectBtnRotRight_, WM_SETFONT, reinterpret_cast<WPARAM>(hFont_), TRUE);

    hInspectStaticDriver_ = CreateWindowExW(
        0, WC_STATICW, L"Driver:",
        WS_CHILD,
        0, 146, 70, 20,
        hEntInspectPane_, nullptr, hInst, nullptr
    );

    hInspectComboDriver_ = CreateWindowExW(
        0, WC_COMBOBOXW, L"",
        WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        74, 146, 224, 180,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_DRIVER)),
        hInst, nullptr
    );
    applyComboBoxTheme(hInspectComboDriver_);

    hInspectStaticCyberware_ = CreateWindowExW(
        0, WC_STATICW, L"Cyberware:",
        WS_CHILD,
        0, 146, 70, 20,
        hEntInspectPane_, nullptr, hInst, nullptr
    );

    hInspectComboCyberware_ = CreateWindowExW(
        0, WC_COMBOBOXW, L"",
        WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        74, 146, 224, 180,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_CYBERWARE)),
        hInst, nullptr
    );
    applyComboBoxTheme(hInspectComboCyberware_);

    hInspectStaticMoveMode_ = CreateWindowExW(
        0, WC_STATICW, L"Movement:",
        WS_CHILD,
        0, 146, 70, 20,
        hEntInspectPane_, nullptr, hInst, nullptr
    );

    hInspectComboMoveMode_ = CreateWindowExW(
        0, WC_COMBOBOXW, L"",
        WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        74, 146, 224, 180,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_MOVEMENT_MODE)),
        hInst, nullptr
    );
    applyComboBoxTheme(hInspectComboMoveMode_);
    ComboBox_AddString(hInspectComboMoveMode_, L"Wandering Patrol");
    ComboBox_AddString(hInspectComboMoveMode_, L"Stationary Sentry");
    ComboBox_SetCurSel(hInspectComboMoveMode_, 0);

    hInspectStaticFaction_ = CreateWindowExW(
        0, WC_STATICW, L"Faction:",
        WS_CHILD,
        0, 146, 70, 20,
        hEntInspectPane_, nullptr, hInst, nullptr
    );

    hInspectComboFaction_ = CreateWindowExW(
        0, WC_COMBOBOXW, L"",
        WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        74, 146, 224, 180,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_FACTION)),
        hInst, nullptr
    );
    applyComboBoxTheme(hInspectComboFaction_);
    ComboBox_AddString(hInspectComboFaction_, L"Faction 1 (Blue)");
    ComboBox_AddString(hInspectComboFaction_, L"Faction 2 (Orange)");
    ComboBox_AddString(hInspectComboFaction_, L"Faction 3 (Yellow A)");
    ComboBox_AddString(hInspectComboFaction_, L"Faction 4 (Yellow B)");
    ComboBox_AddString(hInspectComboFaction_, L"Faction 5 (Purple)");
    ComboBox_AddString(hInspectComboFaction_, L"Faction 6 (Grey)");
    ComboBox_AddString(hInspectComboFaction_, L"Faction 7 (Cyan)");
    ComboBox_SetCurSel(hInspectComboFaction_, 0);



    hInspectBtnAiTactics_ = CreateWindowExW(
        0, WC_BUTTONW, L"Edit Faction AI Tactics...",
        WS_CHILD | BS_OWNERDRAW | WS_TABSTOP,
        0, 172, 298, 24,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_BTN_AI_TACTICS)),
        hInst, nullptr
    );

    hInspectBtnPlaytestConfig_ = CreateWindowExW(
        0, WC_BUTTONW, L"Playtest Configuration...",
        WS_CHILD | BS_OWNERDRAW | WS_TABSTOP,
        0, 172, 298, 26,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_BTN_PLAYTEST_CONFIG)),
        hInst, nullptr
    );

    hInspectStaticWeapons_ = CreateWindowExW(
        0, WC_STATICW, L"Weapons Inventory (Equipped):",
        WS_CHILD,
        0, 172, 298, 18,
        hEntInspectPane_, nullptr, hInst, nullptr
    );

    hInspectWeaponsGrid_ = CreateWindowExW(
        0, WC_STATICW, L"",
        WS_CHILD | SS_NOTIFY,
        0, 192, 298, 72,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_WEAPONS)),
        hInst, nullptr
    );
    SetWindowSubclass(hInspectWeaponsGrid_, WeaponsGridSubclass, 2001, reinterpret_cast<DWORD_PTR>(this));

    // Inspector Route Controls (Waypoints, Loops, and Script Actions)
    hInspectStaticRoute_ = CreateWindowExW(
        0, WC_STATICW, L"Route: None (Stationary)",
        WS_CHILD | SS_LEFT,
        0, 268, 298, 18,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_ROUTE_INFO)),
        hInst, nullptr
    );

    hInspectBtnEditRoute_ = CreateWindowExW(
        0, WC_BUTTONW, L"Edit Route (R)",
        WS_CHILD | BS_OWNERDRAW,
        0, 288, 105, 24,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_BTN_ROUTE)),
        hInst, nullptr
    );

    hInspectCheckLoop_ = CreateWindowExW(
        0, WC_BUTTONW, L"Loop Route",
        WS_CHILD | BS_AUTOCHECKBOX,
        106, 289, 110, 22,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_CHECK_LOOP)),
        hInst, nullptr
    );
    applyCheckBoxTheme(hInspectCheckLoop_);

    hInspectBtnClearRoute_ = CreateWindowExW(
        0, WC_BUTTONW, L"Clear",
        WS_CHILD | BS_OWNERDRAW,
        225, 288, 55, 24,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_BTN_CLEAR_ROUTE)),
        hInst, nullptr
    );

    // Ordered Scripted Actions List
    hInspectListActions_ = CreateWindowExW(
        0, WC_LISTBOXW, L"",
        WS_CHILD | LBS_NOTIFY | WS_VSCROLL,
        0, 316, 298, 65,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_LIST_ACTIONS)),
        hInst, nullptr
    );
    enableDarkModeForControl(hInspectListActions_);

    // Scripted Actions Toolbar
    hInspectBtnAddAction_ = CreateWindowExW(
        0, WC_BUTTONW, L"+ Action",
        WS_CHILD | BS_OWNERDRAW,
        0, 386, 72, 24,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_BTN_ADD_ACTION)),
        hInst, nullptr
    );

    hInspectBtnDelAction_ = CreateWindowExW(
        0, WC_BUTTONW, L"Delete",
        WS_CHILD | BS_OWNERDRAW,
        76, 386, 68, 24,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_BTN_DEL_ACTION)),
        hInst, nullptr
    );

    hInspectBtnMoveUp_ = CreateWindowExW(
        0, WC_BUTTONW, L"▲ Up",
        WS_CHILD | BS_OWNERDRAW,
        148, 386, 68, 24,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_BTN_MOVE_UP)),
        hInst, nullptr
    );

    hInspectBtnMoveDown_ = CreateWindowExW(
        0, WC_BUTTONW, L"▼ Down",
        WS_CHILD | BS_OWNERDRAW,
        220, 386, 68, 24,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_BTN_MOVE_DOWN)),
        hInst, nullptr
    );

    hInspectBtnApply_ = CreateWindowExW(
        0, WC_BUTTONW, L"Apply Changes",
        WS_CHILD | BS_OWNERDRAW,
        0, 416, 94, 28,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_APPLY)),
        hInst, nullptr
    );

    hInspectBtnCancel_ = CreateWindowExW(
        0, WC_BUTTONW, L"Cancel",
        WS_CHILD | BS_OWNERDRAW,
        98, 416, 94, 28,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_CANCEL)),
        hInst, nullptr
    );
    EnableWindow(hInspectBtnCancel_, FALSE);

    hInspectBtnDelete_ = CreateWindowExW(
        0, WC_BUTTONW, L"Delete Entity",
        WS_CHILD | BS_OWNERDRAW,
        196, 416, 98, 28,
        hEntInspectPane_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INSPECT_DELETE)),
        hInst, nullptr
    );

    // --- Tab 2: Objectives ---
    hListObjectives_ = CreateWindowExW(
        0, WC_LISTBOXW, L"",
        WS_CHILD | LBS_NOTIFY | WS_VSCROLL,
        6, 32, 298, 250,
        hWnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_LIST_OBJECTIVES)),
        hInst, nullptr
    );
    enableDarkModeForControl(hListObjectives_);

    hBtnAddObj_ = CreateWindowExW(
        0, WC_BUTTONW, L"Add...",
        WS_CHILD | BS_OWNERDRAW,
        6, 288, 94, 26,
        hWnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_ADD_OBJ)),
        hInst, nullptr
    );

    hBtnEditObj_ = CreateWindowExW(
        0, WC_BUTTONW, L"Edit...",
        WS_CHILD | BS_OWNERDRAW,
        106, 288, 94, 26,
        hWnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_EDIT_OBJ)),
        hInst, nullptr
    );

    hBtnDelObj_ = CreateWindowExW(
        0, WC_BUTTONW, L"Delete",
        WS_CHILD | BS_OWNERDRAW,
        206, 288, 98, 26,
        hWnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_DEL_OBJ)),
        hInst, nullptr
    );

    hBtnClearObj_ = CreateWindowExW(
        0, WC_BUTTONW, L"Clear All Objectives",
        WS_CHILD | BS_OWNERDRAW,
        6, 318, 298, 26,
        hWnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_CLEAR_OBJ)),
        hInst, nullptr
    );

    // Tooltips
    hToolTip_ = CreateWindowExW(
        WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
        WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        hWnd_, nullptr, hInst, nullptr
    );
    if (hToolTip_) {
        auto addToolTip = [this](HWND hCtrl, const wchar_t *text) {
            if (!hToolTip_ || !hCtrl) return;
            TTTOOLINFOW ti = { sizeof(TTTOOLINFOW) };
            ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
            ti.hwnd = hWnd_;
            ti.uId = reinterpret_cast<UINT_PTR>(hCtrl);
            ti.lpszText = const_cast<LPWSTR>(text);
            SendMessageW(hToolTip_, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&ti));
        };
        addToolTip(hBtnPencil_, L"Pencil Tool (P) \u2014 Paint tiles on current layer");
        addToolTip(hBtnBoxFill_, L"Box Fill Tool (B) \u2014 Fill rectangular region with active tile");
        addToolTip(hBtnTileSelect_, L"Select / Move (V) \u2014 Select, inspect, and drag tile on map");
        addToolTip(hBtnTileSearch_, L"Search Tile (S) \u2014 Find all occurrences of active tile on map");
        addToolTip(hBtnEntPlace_, L"Pencil Tool (P) \u2014 Spawn selected entity on map");
        addToolTip(hBtnEntSelect_, L"Select / Move (V) \u2014 Select, inspect, and drag entity on map");
        addToolTip(hBtnEntSearch_, L"Search Entity (S) \u2014 Find all occurrences of active entity on map");
        addToolTip(hInspectBtnRotLeft_, L"Rotate Left 45\u00B0 (\u21BA)");
        addToolTip(hInspectBtnPlayAnim_, L"Play / Pause Walk Animation (\u25B6/||)");
        addToolTip(hInspectBtnRotRight_, L"Rotate Right 45\u00B0 (\u21BB)");
    }

    // Apply consistent font across all controls in Tiles, Entities, and Objectives panels
    EnumChildWindows(hWnd_, [](HWND hChild, LPARAM lp) -> BOOL {
        HFONT font = reinterpret_cast<HFONT>(lp);
        SendMessageW(hChild, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        return TRUE;
    }, reinterpret_cast<LPARAM>(hFont_));

    if (hInspectHeader_) {
        SendMessageW(hInspectHeader_, WM_SETFONT, reinterpret_cast<WPARAM>(hHeaderFont_), TRUE);
    }
}

void WinArtPanel::updateTabVisibility(int tab) {
    if (tab < 0) tab = 0;
    if (tab > 2) tab = 2;
    int oldTab = currentTab_;
    currentTab_ = tab;

    // Synchronize tools and deselect tiles or entities when switching tabs
    if (tab != oldTab && pViewport_) {
        ViewportTool curTool = pViewport_->tool();
        if (tab == 1) { // Switching to Entities tab
            if (curTool == ViewportTool::Pencil || curTool == ViewportTool::BoxFill) {
                const auto *pItem = selectedEntityTemplateItem();
                if (pItem) {
                    if (pItem->kind == EntityKind::Actor) pViewport_->setTool(ViewportTool::PlaceActor);
                    else if (pItem->kind == EntityKind::Vehicle) pViewport_->setTool(ViewportTool::PlaceVehicle);
                    else if (pItem->kind == EntityKind::Static) pViewport_->setTool(ViewportTool::PlaceStatic);
                    else if (pItem->kind == EntityKind::Weapon) pViewport_->setTool(ViewportTool::PlaceWeapon);
                } else {
                    pViewport_->setTool(ViewportTool::PlaceActor);
                }
            } else if (curTool == ViewportTool::SelectTile) {
                pViewport_->setTool(ViewportTool::SelectEntity);
            }
            if (pSelectedEntity_) {
                pViewport_->setSelectedEntity(pSelectedEntity_);
            }
        } else if (tab == 0) { // Switching to Tiles tab
            if (curTool == ViewportTool::PlaceActor ||
                curTool == ViewportTool::PlaceVehicle ||
                curTool == ViewportTool::PlaceStatic ||
                curTool == ViewportTool::PlaceWeapon) {
                pViewport_->setTool(ViewportTool::Pencil);
            } else if (curTool == ViewportTool::SelectEntity) {
                pViewport_->setTool(ViewportTool::SelectTile);
            }
        }
        updateToolSelection();
    }

    auto show = [](HWND hw, bool s) {
        if (hw) ShowWindow(hw, s ? SW_SHOW : SW_HIDE);
    };

    // Tab 0: Tiles & Tile Inspector / Search Results
    show(hComboCategory_, tab == 0);
    show(hBtnPencil_, tab == 0);
    show(hBtnBoxFill_, tab == 0);
    show(hBtnTileSelect_, tab == 0);
    show(hBtnTileSearch_, tab == 0);
    show(hTileGrid_, tab == 0);

    bool showTileSearch = (tab == 0 && isTileSearchActive_);
    show(hTileSearchResults_, showTileSearch);
    show(hTileInspectPane_, tab == 0 && !isTileSearchActive_);

    bool hasSelTile = (pViewport_ && pViewport_->hasSelectedTile());
    show(hTilePreviewBox_, true);
    show(hTileInspectStaticType_, true);
    show(hTileInspectValueType_, true);
    show(hTileInspectStaticSurface_, true);
    show(hTileInspectValueSurface_, true);
    show(hTileInspectStaticPos_, hasSelTile);
    show(hTileInspectValuePos_, hasSelTile);
    show(hTileInspectStaticPlacements_, true);
    show(hTileInspectValuePlacements_, true);

    // Tab 1: Entities & Integrated Entity Inspector / Search Results
    show(hComboEntityType_, tab == 1);
    show(hBtnEntPlace_, tab == 1);
    show(hBtnEntSelect_, tab == 1);
    show(hBtnEntSearch_, tab == 1);
    show(hEntityGrid_, tab == 1);

    bool showEntSearch = (tab == 1 && isEntitySearchActive_);
    show(hEntSearchResults_, showEntSearch);
    show(hEntInspectPane_, tab == 1 && !isEntitySearchActive_);

    bool hasEntity = (pSelectedEntity_ != nullptr);
    const auto *pTemplateItem = (!pSelectedEntity_) ? selectedEntityTemplateItem() : nullptr;
    bool isVeh = (pSelectedEntity_ && pSelectedEntity_->is(rs_knl::MapObject::kNatureVehicle)) ||
                 (!pSelectedEntity_ && pTemplateItem && pTemplateItem->kind == EntityKind::Vehicle);
    auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_);
    bool isTrainBody = (pVeh && pVeh->getType() == rs_knl::Vehicle::kVehicleTypeTrainBody) ||
                       (!pSelectedEntity_ && pTemplateItem && pTemplateItem->kind == EntityKind::Vehicle &&
                        pTemplateItem->typeId == rs_knl::Vehicle::kVehicleTypeTrainBody);
    auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_);
    bool isActor = (pPed != nullptr) ||
                   (!pSelectedEntity_ && pTemplateItem && pTemplateItem->kind == EntityKind::Actor);
    bool isOurAgent = (pPed && pPed->isOurAgent()) ||
                      (!pSelectedEntity_ && pTemplateItem && pTemplateItem->kind == EntityKind::Actor &&
                       pTemplateItem->typeId == rs_knl::PedInstance::kPedTypeAgent && templateAgentType_ == 0);
    bool isEnemyAgent = (pPed && pPed->type() == rs_knl::PedInstance::kPedTypeAgent && !pPed->isOurAgent()) ||
                        (!pSelectedEntity_ && pTemplateItem && pTemplateItem->kind == EntityKind::Actor &&
                         pTemplateItem->typeId == rs_knl::PedInstance::kPedTypeAgent && templateAgentType_ == 1);
    bool isAgent = isOurAgent || isEnemyAgent;
    bool showWeapons = !isOurAgent && (isActor || (isVeh && hasEntity && pVeh && pVeh->getDriver()));
    bool showRoute = hasEntity && !isAgent && (isActor || (isVeh && !isTrainBody));

    show(hEditProperties_, false);
    show(hInspectHeader_, hasEntity);
    show(hInspectStaticType_, true);
    show(hInspectValueType_, !isAgent);
    show(hInspectComboType_, isAgent);
    show(hInspectStaticHealth_, !isOurAgent);
    show(hInspectEditHealth_, !isOurAgent);
    show(hInspectCheckDead_, !isOurAgent && (!pPed || !pPed->isOurAgent()));
    show(hInspectStaticOrient_, true);
    show(hInspectComboOrient_, true);
    show(hInspectStaticPos_, hasEntity);
    show(hInspectValuePos_, hasEntity);
    show(hInspectStaticLoc_, hasEntity);
    show(hInspectValueLoc_, hasEntity);
    show(hEntPreviewBox_, tab == 1 && !isEntitySearchActive_);
    show(hInspectBtnRotLeft_, tab == 1 && !isEntitySearchActive_);
    show(hInspectBtnPlayAnim_, tab == 1 && !isEntitySearchActive_);
    show(hInspectBtnRotRight_, tab == 1 && !isEntitySearchActive_);
    show(hInspectStaticDriver_, isVeh && !isTrainBody);
    show(hInspectComboDriver_, isVeh && !isTrainBody);
    show(hInspectStaticCyberware_, isEnemyAgent);
    show(hInspectComboCyberware_, isEnemyAgent);
    show(hInspectStaticMoveMode_, isActor && !isOurAgent);
    show(hInspectComboMoveMode_, isActor && !isOurAgent);
    show(hInspectStaticFaction_, isAgent);
    show(hInspectComboFaction_, isAgent);

    show(hInspectBtnAiTactics_, isEnemyAgent);
    show(hInspectBtnPlaytestConfig_, isOurAgent && hasEntity);
    show(hInspectStaticWeapons_, showWeapons);
    show(hInspectWeaponsGrid_, showWeapons);
    show(hInspectStaticRoute_, showRoute);
    show(hInspectBtnEditRoute_, showRoute);
    show(hInspectCheckLoop_, showRoute);
    show(hInspectBtnClearRoute_, showRoute);
    show(hInspectListActions_, showRoute);
    show(hInspectBtnAddAction_, showRoute);
    show(hInspectBtnDelAction_, showRoute);
    show(hInspectBtnMoveUp_, showRoute);
    show(hInspectBtnMoveDown_, showRoute);
    show(hInspectBtnApply_, hasEntity);
    show(hInspectBtnCancel_, hasEntity);
    show(hInspectBtnDelete_, hasEntity);

    // Tab 2: Objectives
    show(hListObjectives_, tab == 2);
    show(hBtnAddObj_, tab == 2);
    show(hBtnEditObj_, tab == 2);
    show(hBtnDelObj_, tab == 2);
    show(hBtnClearObj_, tab == 2);
    if (tab == 2) {
        refreshObjectivesList();
    }

    bool hasMission = (pMission_ != nullptr);
    EnableWindow(hBtnPencil_, hasMission ? TRUE : FALSE);
    EnableWindow(hBtnBoxFill_, hasMission ? TRUE : FALSE);
    EnableWindow(hBtnTileSelect_, hasMission ? TRUE : FALSE);
    EnableWindow(hBtnTileSearch_, hasMission ? TRUE : FALSE);
    EnableWindow(hBtnEntPlace_, hasMission ? TRUE : FALSE);
    EnableWindow(hBtnEntSelect_, hasMission ? TRUE : FALSE);
    EnableWindow(hBtnEntSearch_, hasMission ? TRUE : FALSE);
    EnableWindow(hBtnAddObj_, hasMission ? TRUE : FALSE);
    EnableWindow(hBtnEditObj_, hasMission ? TRUE : FALSE);
    EnableWindow(hBtnDelObj_, hasMission ? TRUE : FALSE);
    EnableWindow(hBtnClearObj_, hasMission ? TRUE : FALSE);

    if (tab == 0) {
        updateTileGridScroll();
        refreshTileInspector();
        if (hTileGrid_) InvalidateRect(hTileGrid_, nullptr, TRUE);
    } else if (tab == 1) {
        updateEntityGridScroll();
        if (!hasEntity) {
            setSelectedEntityGridIndex(selectedEntityGridIdx_);
        } else {
            refreshInspector();
        }
        if (hEntityGrid_) InvalidateRect(hEntityGrid_, nullptr, TRUE);
        if (hEntInspectPane_) {
            RedrawWindow(hEntInspectPane_, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE | RDW_UPDATENOW);
        }
    }

    if (tabChangedCallback_) {
        tabChangedCallback_(currentTab_);
    }

    if (hWnd_) {
        layoutControls();
        InvalidateRect(hWnd_, nullptr, TRUE);
    }

    HWND hFocus = GetFocus();
    if (hFocus && hWnd_ && IsChild(hWnd_, hFocus) && !IsWindowVisible(hFocus)) {
        SetFocus(hWnd_);
    }
}

int WinArtPanel::getClampedEntityGridHeight(int clientHeight) const {
    int minH = 60;
    int maxH = std::max(minH, clientHeight - 120);
    int h = entityGridHeight_;
    if (h < minH) h = minH;
    if (h > maxH) h = maxH;
    return h;
}

int WinArtPanel::getClampedTileGridHeight(int clientHeight) const {
    int minH = 60;
    int maxH = std::max(minH, clientHeight - 120);
    int h = tileGridHeight_;
    if (h < minH) h = minH;
    if (h > maxH) h = maxH;
    return h;
}

void WinArtPanel::setTileGridHeight(int h) {
    tileGridHeight_ = h;
    layoutControls();
    if (hWnd_) InvalidateRect(hWnd_, nullptr, TRUE);
}

void WinArtPanel::onResize(int x, int y, int w, int h) {
    SetWindowPos(hWnd_, nullptr, x, y, w, h, SWP_NOZORDER);
    layoutControls();
    InvalidateRect(hWnd_, nullptr, TRUE);
}

void WinArtPanel::layoutControls() {
    if (!hWnd_) return;
    RECT rc;
    GetClientRect(hWnd_, &rc);
    int w = rc.right;
    int h = rc.bottom;
    if (w <= 0 || h <= 0) return;

    SetWindowPos(hTabControl_, nullptr, 2, 2, w - 4, 34, SWP_NOZORDER);
    int tabW = (w - 24) / 3;
    if (tabW > 0) {
        SendMessageW(hTabControl_, TCM_SETITEMSIZE, 0, MAKELPARAM(tabW, 32));
    }

    int btnW = 32;
    int btnGap = 3;

    // Tab 0: Tiles & Tile Inspector / Search Results
    int tileBtnsW = 4 * btnW + 3 * btnGap; // 137px
    int tileComboW = (w - 12) - tileBtnsW - 6;
    if (tileComboW < 80) tileComboW = 80;
    int tileBtnsStartX = 6 + tileComboW + 6;

    SetWindowPos(hComboCategory_, nullptr, 6, 40, tileComboW, 200, SWP_NOZORDER);
    SetWindowPos(hBtnPencil_, nullptr, tileBtnsStartX, 39, btnW, 26, SWP_NOZORDER);
    SetWindowPos(hBtnBoxFill_, nullptr, tileBtnsStartX + (btnW + btnGap), 39, btnW, 26, SWP_NOZORDER);
    SetWindowPos(hBtnTileSelect_, nullptr, tileBtnsStartX + (btnW + btnGap) * 2, 39, btnW, 26, SWP_NOZORDER);
    SetWindowPos(hBtnTileSearch_, nullptr, tileBtnsStartX + (btnW + btnGap) * 3, 39, btnW, 26, SWP_NOZORDER);

    int tileGridH = getClampedTileGridHeight(h);
    SetWindowPos(hTileGrid_, nullptr, 6, 72, w - 12, tileGridH, SWP_NOZORDER);

    int tileInspectTop = 72 + tileGridH + 8;
    int tileInspectH = std::max(20, h - tileInspectTop - 8);
    if (isTileSearchActive_) {
        SetWindowPos(hTileSearchResults_, nullptr, 6, tileInspectTop, w - 12, tileInspectH, SWP_NOZORDER);
    } else {
        SetWindowPos(hTileInspectPane_, nullptr, 6, tileInspectTop, w - 12, tileInspectH, SWP_NOZORDER);
        layoutTileInspectPane();
    }

    // Tab 1: Entities & Integrated Entity Inspector / Search Results
    int entBtnsW = 3 * btnW + 2 * btnGap; // 102px
    int entComboW = (w - 12) - entBtnsW - 6;
    if (entComboW < 80) entComboW = 80;
    int entBtnsStartX = 6 + entComboW + 6;

    SetWindowPos(hComboEntityType_, nullptr, 6, 40, entComboW, 200, SWP_NOZORDER);
    SetWindowPos(hBtnEntPlace_, nullptr, entBtnsStartX, 39, btnW, 26, SWP_NOZORDER);
    SetWindowPos(hBtnEntSelect_, nullptr, entBtnsStartX + (btnW + btnGap), 39, btnW, 26, SWP_NOZORDER);
    SetWindowPos(hBtnEntSearch_, nullptr, entBtnsStartX + (btnW + btnGap) * 2, 39, btnW, 26, SWP_NOZORDER);

    int listH = getClampedEntityGridHeight(h);
    SetWindowPos(hEntityGrid_, nullptr, 6, 72, w - 12, listH, SWP_NOZORDER);

    int inspectTop = 72 + listH + 8; // dynamic splitter positioning
    int inspectH = std::max(20, h - inspectTop - 8);
    if (isEntitySearchActive_) {
        SetWindowPos(hEntSearchResults_, nullptr, 6, inspectTop, w - 12, inspectH, SWP_NOZORDER);
    } else {
        SetWindowPos(hEntInspectPane_, nullptr, 6, inspectTop, w - 12, inspectH, SWP_NOZORDER);
        layoutEntInspectPane();
    }

    // Tab 2: Objectives
    SetWindowPos(hListObjectives_, nullptr, 6, 40, w - 12, 238, SWP_NOZORDER);
    int objBtnW = (w - 18) / 3;
    SetWindowPos(hBtnAddObj_, nullptr, 6, 286, objBtnW, 26, SWP_NOZORDER);
    SetWindowPos(hBtnEditObj_, nullptr, 6 + objBtnW + 3, 286, objBtnW, 26, SWP_NOZORDER);
    SetWindowPos(hBtnDelObj_, nullptr, 6 + (objBtnW + 3) * 2, 286, w - 12 - (objBtnW + 3) * 2, 26, SWP_NOZORDER);
    SetWindowPos(hBtnClearObj_, nullptr, 6, 318, w - 12, 26, SWP_NOZORDER);

    updateTileGridScroll();
    updateEntityGridScroll();
    InvalidateRect(hTileGrid_, nullptr, FALSE);
    InvalidateRect(hEntityGrid_, nullptr, FALSE);
}

void WinArtPanel::layoutTileInspectPane() {
    if (!hTileInspectPane_) return;
    RECT rc;
    GetClientRect(hTileInspectPane_, &rc);
    int paneW = rc.right;
    int paneH = rc.bottom;
    if (paneW <= 0 || paneH <= 0) return;

    const UINT kSetPosFlags = SWP_NOZORDER | SWP_NOACTIVATE;

    bool hasSelTile = (pViewport_ && pViewport_->hasSelectedTile());
    bool isBesidePreview = (paneW >= 250);
    int boxW = 76;
    int boxH = 56;
    int curY = 4;
    int boxX = 0;
    int previewBottomY = 0;

    if (isBesidePreview) {
        boxX = paneW - boxW - 8;
        previewBottomY = curY + boxH;
        SetWindowPos(hTilePreviewBox_, nullptr, boxX, curY - tileInspectScrollY_, boxW, boxH, kSetPosFlags);
    } else {
        boxX = std::max(6, (paneW - boxW) / 2);
        previewBottomY = curY + boxH;
        SetWindowPos(hTilePreviewBox_, nullptr, boxX, curY - tileInspectScrollY_, boxW, boxH, kSetPosFlags);
        curY = previewBottomY + 8;
    }

    const int curLblW = 66;
    const int curFldX = 70;
    auto getFldW = [&](int rowY) {
        return (isBesidePreview && rowY < previewBottomY)
            ? std::max(20, boxX - curFldX - 8)
            : std::max(20, paneW - curFldX - 8);
    };

    SetWindowPos(hTileInspectStaticType_, nullptr, 0, curY + 2 - tileInspectScrollY_, curLblW, 20, kSetPosFlags);
    SetWindowPos(hTileInspectValueType_, nullptr, curFldX, curY + 2 - tileInspectScrollY_, getFldW(curY), 20, kSetPosFlags);
    curY += 24;

    int surfH = (isBesidePreview && curY < previewBottomY) ? 32 : 20;
    SetWindowPos(hTileInspectStaticSurface_, nullptr, 0, curY + 2 - tileInspectScrollY_, curLblW, 20, kSetPosFlags);
    SetWindowPos(hTileInspectValueSurface_, nullptr, curFldX, curY + 2 - tileInspectScrollY_, getFldW(curY), surfH, kSetPosFlags);
    curY += surfH + 4;

    if (isBesidePreview) {
        curY = std::max(curY, previewBottomY + 6);
    }

    if (hasSelTile) {
        SetWindowPos(hTileInspectStaticPos_, nullptr, 0, curY + 2 - tileInspectScrollY_, curLblW, 20, kSetPosFlags);
        SetWindowPos(hTileInspectValuePos_, nullptr, curFldX, curY + 2 - tileInspectScrollY_, std::max(20, paneW - curFldX - 8), 20, kSetPosFlags);
        curY += 24;
    }

    SetWindowPos(hTileInspectStaticPlacements_, nullptr, 0, curY + 2 - tileInspectScrollY_, curLblW, 20, kSetPosFlags);
    SetWindowPos(hTileInspectValuePlacements_, nullptr, curFldX, curY + 2 - tileInspectScrollY_, std::max(20, paneW - curFldX - 8), 20, kSetPosFlags);
    curY += 24;

    int totalContentH = curY + 4;
    int maxScroll = std::max(0, totalContentH - paneH);
    if (tileInspectScrollY_ > maxScroll) tileInspectScrollY_ = maxScroll;

    SCROLLINFO si = {};
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = totalContentH;
    si.nPage = std::max(1, paneH);
    si.nPos = tileInspectScrollY_;
    SetScrollInfo(hTileInspectPane_, SB_VERT, &si, TRUE);
    ShowScrollBar(hTileInspectPane_, SB_VERT, (totalContentH > paneH) ? TRUE : FALSE);
    RedrawWindow(hTileInspectPane_, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE);
}

void WinArtPanel::layoutEntInspectPane() {
    if (!hEntInspectPane_) return;
    RECT rc;
    GetClientRect(hEntInspectPane_, &rc);
    int paneW = rc.right;
    int paneH = rc.bottom;
    if (paneW <= 0 || paneH <= 0) return;

    const UINT kSetPosFlags = SWP_NOZORDER | SWP_NOACTIVATE;

    bool hasEntity = (pSelectedEntity_ != nullptr);
    const auto *pTemplateItem = (!pSelectedEntity_) ? selectedEntityTemplateItem() : nullptr;
    bool isVeh = (pSelectedEntity_ && pSelectedEntity_->is(rs_knl::MapObject::kNatureVehicle)) ||
                 (!pSelectedEntity_ && pTemplateItem && pTemplateItem->kind == EntityKind::Vehicle);
    auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_);
    bool isTrainBody = (pVeh && pVeh->getType() == rs_knl::Vehicle::kVehicleTypeTrainBody) ||
                       (!pSelectedEntity_ && pTemplateItem && pTemplateItem->kind == EntityKind::Vehicle &&
                        pTemplateItem->typeId == rs_knl::Vehicle::kVehicleTypeTrainBody);
    auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_);
    bool isActor = (pPed != nullptr) ||
                   (!pSelectedEntity_ && pTemplateItem && pTemplateItem->kind == EntityKind::Actor);
    bool isOurAgent = (pPed && pPed->isOurAgent()) ||
                      (!pSelectedEntity_ && pTemplateItem && pTemplateItem->kind == EntityKind::Actor &&
                       pTemplateItem->typeId == rs_knl::PedInstance::kPedTypeAgent && templateAgentType_ == 0);
    bool isEnemyAgent = (pPed && pPed->type() == rs_knl::PedInstance::kPedTypeAgent && !pPed->isOurAgent()) ||
                        (!pSelectedEntity_ && pTemplateItem && pTemplateItem->kind == EntityKind::Actor &&
                         pTemplateItem->typeId == rs_knl::PedInstance::kPedTypeAgent && templateAgentType_ == 1);
    bool isAgent = isOurAgent || isEnemyAgent;
    bool isWeapon = (pSelectedEntity_ && dynamic_cast<rs_knl::WeaponInstance*>(pSelectedEntity_)) ||
                    (!pSelectedEntity_ && pTemplateItem && pTemplateItem->kind == EntityKind::Weapon);
    bool showWeapons = !isOurAgent && !isWeapon && (isActor || (isVeh && hasEntity && pVeh && pVeh->getDriver()));
    bool showRoute = hasEntity && !isAgent && !isWeapon && (isActor || (isVeh && !isTrainBody));

    int curY = 4;
    if (hasEntity) {
        SetWindowPos(hInspectHeader_, nullptr, 0, curY - entInspectScrollY_, paneW, 20, kSetPosFlags);
        curY += 24;
    }

    bool isBesidePreview = (paneW >= 270);
    int boxW = 76;
    int boxH = 56;
    int btnH = 18;

    int boxX = 0;
    int boxY = 0;
    int btnY = 0;
    int previewBottomY = 0;

    if (isBesidePreview) {
        boxX = paneW - boxW - 8;
        boxY = curY;
        btnY = boxY + boxH + 2;
        previewBottomY = btnY + btnH;
        SetWindowPos(hEntPreviewBox_, nullptr, boxX, boxY - entInspectScrollY_, boxW, boxH, kSetPosFlags);
        int btnW = (boxW - 4) / 3;
        SetWindowPos(hInspectBtnRotLeft_, nullptr, boxX, btnY - entInspectScrollY_, btnW, btnH, kSetPosFlags);
        SetWindowPos(hInspectBtnPlayAnim_, nullptr, boxX + btnW + 2, btnY - entInspectScrollY_, btnW, btnH, kSetPosFlags);
        SetWindowPos(hInspectBtnRotRight_, nullptr, boxX + (btnW + 2) * 2, btnY - entInspectScrollY_, boxW - (btnW + 2) * 2, btnH, kSetPosFlags);
    } else {
        // Narrow pane: preview box at the top centered, then fields below!
        boxX = std::max(6, (paneW - boxW) / 2);
        boxY = curY;
        btnY = boxY + boxH + 2;
        previewBottomY = btnY + btnH;
        SetWindowPos(hEntPreviewBox_, nullptr, boxX, boxY - entInspectScrollY_, boxW, boxH, kSetPosFlags);
        int btnW = (boxW - 4) / 3;
        SetWindowPos(hInspectBtnRotLeft_, nullptr, boxX, btnY - entInspectScrollY_, btnW, btnH, kSetPosFlags);
        SetWindowPos(hInspectBtnPlayAnim_, nullptr, boxX + btnW + 2, btnY - entInspectScrollY_, btnW, btnH, kSetPosFlags);
        SetWindowPos(hInspectBtnRotRight_, nullptr, boxX + (btnW + 2) * 2, btnY - entInspectScrollY_, boxW - (btnW + 2) * 2, btnH, kSetPosFlags);
        curY = previewBottomY + 8;
    }

    if (isWeapon) {
        EnableWindow(hInspectBtnRotLeft_, FALSE);
        EnableWindow(hInspectBtnPlayAnim_, FALSE);
        EnableWindow(hInspectBtnRotRight_, FALSE);
    } else {
        EnableWindow(hInspectBtnRotLeft_, TRUE);
        EnableWindow(hInspectBtnPlayAnim_, TRUE);
        EnableWindow(hInspectBtnRotRight_, TRUE);
    }

    const int curLblW = 64;
    const int curFldX = 68;

    auto placeRow = [&](HWND hLabel, HWND hCtrl, int rowH = 26, int ctrlDropH = 180) {
        if (!hLabel && !hCtrl) return;
        int rowFldW = (isBesidePreview && curY < previewBottomY) ? (boxX - curFldX - 8) : std::max(20, paneW - curFldX - 8);

        if (hLabel) {
            SetWindowPos(hLabel, nullptr, 0, curY + 2 - entInspectScrollY_, curLblW, 20, kSetPosFlags);
        }
        if (hCtrl) {
            SetWindowPos(hCtrl, nullptr, curFldX, curY - entInspectScrollY_, rowFldW, ctrlDropH, kSetPosFlags);
        }
        curY += rowH;
    };

    // Row 1: Type
    int typeFldW = (isBesidePreview && curY < previewBottomY) ? (boxX - curFldX - 8) : std::max(20, paneW - curFldX - 8);
    SetWindowPos(hInspectStaticType_, nullptr, 0, curY + 2 - entInspectScrollY_, curLblW, 20, kSetPosFlags);
    if (isAgent) {
        ShowWindow(hInspectValueType_, SW_HIDE);
        SetWindowPos(hInspectComboType_, nullptr, curFldX, curY - entInspectScrollY_, typeFldW, 140, kSetPosFlags);
        if (hInspectComboType_) ShowWindow(hInspectComboType_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
    } else {
        if (hInspectComboType_) ShowWindow(hInspectComboType_, SW_HIDE);
        SetWindowPos(hInspectValueType_, nullptr, curFldX, curY + 2 - entInspectScrollY_, typeFldW, 20, kSetPosFlags);
        if (hInspectValueType_) ShowWindow(hInspectValueType_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
    }
    curY += 26;

    // Row 2: Health (not shown for player's agents or weapons)
    if (!isOurAgent && !isWeapon) {
        int healthFldW = (isBesidePreview && curY < previewBottomY) ? (boxX - curFldX - 8) : std::max(20, paneW - curFldX - 8);
        SetWindowPos(hInspectStaticHealth_, nullptr, 0, curY + 2 - entInspectScrollY_, curLblW, 20, kSetPosFlags);
        bool showDeadCheck = (hInspectCheckDead_ != nullptr);
        int editW = showDeadCheck ? 48 : healthFldW;
        SetWindowPos(hInspectEditHealth_, nullptr, curFldX, curY - entInspectScrollY_, editW, 22, kSetPosFlags);
        if (showDeadCheck) {
            int checkX = curFldX + editW + 6;
            int checkW = (isBesidePreview && curY < previewBottomY) ? std::max(20, boxX - checkX - 4) : std::max(20, paneW - checkX - 8);
            SetWindowPos(hInspectCheckDead_, nullptr, checkX, curY + 1 - entInspectScrollY_, checkW, 20, kSetPosFlags);
        }
        curY += 26;

        if (hInspectStaticHealth_) ShowWindow(hInspectStaticHealth_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectEditHealth_) ShowWindow(hInspectEditHealth_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectCheckDead_) ShowWindow(hInspectCheckDead_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
    } else {
        if (hInspectStaticHealth_) ShowWindow(hInspectStaticHealth_, SW_HIDE);
        if (hInspectEditHealth_) ShowWindow(hInspectEditHealth_, SW_HIDE);
        if (hInspectCheckDead_) ShowWindow(hInspectCheckDead_, SW_HIDE);
    }

    // Row 3: Direction (not shown for weapons)
    if (!isWeapon) {
        placeRow(hInspectStaticOrient_, hInspectComboOrient_, 26, 180);
        if (hInspectStaticOrient_) ShowWindow(hInspectStaticOrient_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectComboOrient_) ShowWindow(hInspectComboOrient_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
    } else {
        if (hInspectStaticOrient_) ShowWindow(hInspectStaticOrient_, SW_HIDE);
        if (hInspectComboOrient_) ShowWindow(hInspectComboOrient_, SW_HIDE);
    }

    // Ensure all subsequent controls (Cyberware, Driver, Movement, Faction, etc.) start below the preview block and span full width
    curY = std::max(curY, previewBottomY + 4);

    // Row 4+: Entity-specific dropdowns, sequenced without gaps and wrapping around preview box
    if (isOurAgent) {
        placeRow(hInspectStaticFaction_, hInspectComboFaction_, 26, 180);
        if (hInspectStaticFaction_) ShowWindow(hInspectStaticFaction_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectComboFaction_) ShowWindow(hInspectComboFaction_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectStaticCyberware_) ShowWindow(hInspectStaticCyberware_, SW_HIDE);
        if (hInspectComboCyberware_) ShowWindow(hInspectComboCyberware_, SW_HIDE);
        if (hInspectStaticMoveMode_) ShowWindow(hInspectStaticMoveMode_, SW_HIDE);
        if (hInspectComboMoveMode_) ShowWindow(hInspectComboMoveMode_, SW_HIDE);
        if (hInspectBtnAiTactics_) ShowWindow(hInspectBtnAiTactics_, SW_HIDE);
        if (hInspectStaticDriver_) ShowWindow(hInspectStaticDriver_, SW_HIDE);
        if (hInspectComboDriver_) ShowWindow(hInspectComboDriver_, SW_HIDE);
    } else if (isEnemyAgent) {
        placeRow(hInspectStaticCyberware_, hInspectComboCyberware_, 26, 180);
        placeRow(hInspectStaticFaction_, hInspectComboFaction_, 26, 180);
        placeRow(hInspectStaticMoveMode_, hInspectComboMoveMode_, 26, 180);
        {
            int btnFldW = (isBesidePreview && curY < previewBottomY) ? (boxX - curFldX - 8) : std::max(20, paneW - curFldX - 8);
            SetWindowPos(hInspectBtnAiTactics_, nullptr, curFldX, curY - entInspectScrollY_, btnFldW, 24, kSetPosFlags);
            curY += 28;
        }
        if (hInspectStaticCyberware_) ShowWindow(hInspectStaticCyberware_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectComboCyberware_) ShowWindow(hInspectComboCyberware_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectStaticFaction_) ShowWindow(hInspectStaticFaction_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectComboFaction_) ShowWindow(hInspectComboFaction_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectStaticMoveMode_) ShowWindow(hInspectStaticMoveMode_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectComboMoveMode_) ShowWindow(hInspectComboMoveMode_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectBtnAiTactics_) ShowWindow(hInspectBtnAiTactics_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectStaticDriver_) ShowWindow(hInspectStaticDriver_, SW_HIDE);
        if (hInspectComboDriver_) ShowWindow(hInspectComboDriver_, SW_HIDE);
    } else if (isVeh && !isTrainBody) {
        placeRow(hInspectStaticDriver_, hInspectComboDriver_, 26, 180);
        if (hInspectStaticDriver_) ShowWindow(hInspectStaticDriver_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectComboDriver_) ShowWindow(hInspectComboDriver_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectStaticCyberware_) ShowWindow(hInspectStaticCyberware_, SW_HIDE);
        if (hInspectComboCyberware_) ShowWindow(hInspectComboCyberware_, SW_HIDE);
        if (hInspectStaticFaction_) ShowWindow(hInspectStaticFaction_, SW_HIDE);
        if (hInspectComboFaction_) ShowWindow(hInspectComboFaction_, SW_HIDE);
        if (hInspectBtnAiTactics_) ShowWindow(hInspectBtnAiTactics_, SW_HIDE);
        if (hInspectStaticMoveMode_) ShowWindow(hInspectStaticMoveMode_, SW_HIDE);
        if (hInspectComboMoveMode_) ShowWindow(hInspectComboMoveMode_, SW_HIDE);
    } else if (isVeh && isTrainBody) {
        if (hInspectStaticDriver_) ShowWindow(hInspectStaticDriver_, SW_HIDE);
        if (hInspectComboDriver_) ShowWindow(hInspectComboDriver_, SW_HIDE);
        if (hInspectStaticCyberware_) ShowWindow(hInspectStaticCyberware_, SW_HIDE);
        if (hInspectComboCyberware_) ShowWindow(hInspectComboCyberware_, SW_HIDE);
        if (hInspectStaticFaction_) ShowWindow(hInspectStaticFaction_, SW_HIDE);
        if (hInspectComboFaction_) ShowWindow(hInspectComboFaction_, SW_HIDE);
        if (hInspectBtnAiTactics_) ShowWindow(hInspectBtnAiTactics_, SW_HIDE);
        if (hInspectStaticMoveMode_) ShowWindow(hInspectStaticMoveMode_, SW_HIDE);
        if (hInspectComboMoveMode_) ShowWindow(hInspectComboMoveMode_, SW_HIDE);
    } else if (isActor && !isOurAgent) {
        placeRow(hInspectStaticMoveMode_, hInspectComboMoveMode_, 26, 180);
        if (hInspectStaticMoveMode_) ShowWindow(hInspectStaticMoveMode_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectComboMoveMode_) ShowWindow(hInspectComboMoveMode_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectStaticDriver_) ShowWindow(hInspectStaticDriver_, SW_HIDE);
        if (hInspectComboDriver_) ShowWindow(hInspectComboDriver_, SW_HIDE);
        if (hInspectStaticCyberware_) ShowWindow(hInspectStaticCyberware_, SW_HIDE);
        if (hInspectComboCyberware_) ShowWindow(hInspectComboCyberware_, SW_HIDE);
        if (hInspectStaticFaction_) ShowWindow(hInspectStaticFaction_, SW_HIDE);
        if (hInspectComboFaction_) ShowWindow(hInspectComboFaction_, SW_HIDE);
        if (hInspectBtnAiTactics_) ShowWindow(hInspectBtnAiTactics_, SW_HIDE);
    } else {
        if (hInspectStaticDriver_) ShowWindow(hInspectStaticDriver_, SW_HIDE);
        if (hInspectComboDriver_) ShowWindow(hInspectComboDriver_, SW_HIDE);
        if (hInspectStaticCyberware_) ShowWindow(hInspectStaticCyberware_, SW_HIDE);
        if (hInspectComboCyberware_) ShowWindow(hInspectComboCyberware_, SW_HIDE);
        if (hInspectStaticFaction_) ShowWindow(hInspectStaticFaction_, SW_HIDE);
        if (hInspectComboFaction_) ShowWindow(hInspectComboFaction_, SW_HIDE);
        if (hInspectBtnAiTactics_) ShowWindow(hInspectBtnAiTactics_, SW_HIDE);
        if (hInspectStaticMoveMode_) ShowWindow(hInspectStaticMoveMode_, SW_HIDE);
        if (hInspectComboMoveMode_) ShowWindow(hInspectComboMoveMode_, SW_HIDE);
    }

    // Ensure subsequent sections clear the preview box and buttons
    curY = std::max(curY, previewBottomY + 6);

    if (hasEntity) {
        SetWindowPos(hInspectStaticPos_, nullptr, 0, curY + 2 - entInspectScrollY_, curLblW, 20, kSetPosFlags);
        SetWindowPos(hInspectValuePos_, nullptr, curFldX, curY + 2 - entInspectScrollY_, std::max(20, paneW - curFldX - 8), 20, kSetPosFlags);
        curY += 22;

        SetWindowPos(hInspectStaticLoc_, nullptr, 0, curY + 2 - entInspectScrollY_, curLblW, 20, kSetPosFlags);
        SetWindowPos(hInspectValueLoc_, nullptr, curFldX, curY + 2 - entInspectScrollY_, std::max(20, paneW - curFldX - 8), 20, kSetPosFlags);
        curY += 24;

        if (hInspectStaticPos_) ShowWindow(hInspectStaticPos_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectValuePos_) ShowWindow(hInspectValuePos_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectStaticLoc_) ShowWindow(hInspectStaticLoc_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectValueLoc_) ShowWindow(hInspectValueLoc_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
    } else {
        if (hInspectStaticPos_) ShowWindow(hInspectStaticPos_, SW_HIDE);
        if (hInspectValuePos_) ShowWindow(hInspectValuePos_, SW_HIDE);
        if (hInspectStaticLoc_) ShowWindow(hInspectStaticLoc_, SW_HIDE);
        if (hInspectValueLoc_) ShowWindow(hInspectValueLoc_, SW_HIDE);
    }

    if (isOurAgent && hasEntity) {
        curY += 6;
        SetWindowPos(hInspectBtnPlaytestConfig_, nullptr, 0, curY - entInspectScrollY_, paneW, 26, kSetPosFlags);
        curY += 32;
        if (hInspectBtnPlaytestConfig_) ShowWindow(hInspectBtnPlaytestConfig_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
    } else {
        if (hInspectBtnPlaytestConfig_) ShowWindow(hInspectBtnPlaytestConfig_, SW_HIDE);
    }

    if (showWeapons) {
        SetWindowPos(hInspectStaticWeapons_, nullptr, 0, curY - entInspectScrollY_, paneW, 18, kSetPosFlags);
        SetWindowPos(hInspectWeaponsGrid_, nullptr, 0, curY + 20 - entInspectScrollY_, paneW, 72, kSetPosFlags);
        curY += 96;
        if (hInspectStaticWeapons_) ShowWindow(hInspectStaticWeapons_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectWeaponsGrid_) ShowWindow(hInspectWeaponsGrid_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
    } else {
        if (hInspectStaticWeapons_) ShowWindow(hInspectStaticWeapons_, SW_HIDE);
        if (hInspectWeaponsGrid_) ShowWindow(hInspectWeaponsGrid_, SW_HIDE);
    }

    if (showRoute) {
        SetWindowPos(hInspectStaticRoute_, nullptr, 0, curY - entInspectScrollY_, paneW, 18, kSetPosFlags);
        curY += 20;

        int rBtnW1 = 92;
        int clearW = 55;
        int clearX = paneW - clearW;
        int checkX = rBtnW1 + 8;
        int checkW = std::max(60, clearX - 6 - checkX);
        SetWindowPos(hInspectBtnEditRoute_, nullptr, 0, curY - entInspectScrollY_, rBtnW1, 24, kSetPosFlags);
        SetWindowPos(hInspectCheckLoop_, nullptr, checkX, curY + 1 - entInspectScrollY_, checkW, 22, kSetPosFlags);
        SetWindowPos(hInspectBtnClearRoute_, nullptr, clearX, curY - entInspectScrollY_, clearW, 24, kSetPosFlags);
        curY += 28;

        int actListH = 80;
        SetWindowPos(hInspectListActions_, nullptr, 0, curY - entInspectScrollY_, paneW, actListH, kSetPosFlags);
        curY += actListH + 4;

        int actBtnGap = 3;
        int actBtnW = (paneW - 3 * actBtnGap) / 4;
        SetWindowPos(hInspectBtnAddAction_, nullptr, 0, curY - entInspectScrollY_, actBtnW, 24, kSetPosFlags);
        SetWindowPos(hInspectBtnDelAction_, nullptr, actBtnW + actBtnGap, curY - entInspectScrollY_, actBtnW, 24, kSetPosFlags);
        SetWindowPos(hInspectBtnMoveUp_, nullptr, (actBtnW + actBtnGap) * 2, curY - entInspectScrollY_, actBtnW, 24, kSetPosFlags);
        SetWindowPos(hInspectBtnMoveDown_, nullptr, (actBtnW + actBtnGap) * 3, curY - entInspectScrollY_, paneW - (actBtnW + actBtnGap) * 3, 24, kSetPosFlags);
        curY += 28;

        if (hInspectStaticRoute_) ShowWindow(hInspectStaticRoute_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectBtnEditRoute_) ShowWindow(hInspectBtnEditRoute_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectCheckLoop_) ShowWindow(hInspectCheckLoop_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectBtnClearRoute_) ShowWindow(hInspectBtnClearRoute_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectListActions_) ShowWindow(hInspectListActions_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectBtnAddAction_) ShowWindow(hInspectBtnAddAction_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectBtnDelAction_) ShowWindow(hInspectBtnDelAction_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectBtnMoveUp_) ShowWindow(hInspectBtnMoveUp_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        if (hInspectBtnMoveDown_) ShowWindow(hInspectBtnMoveDown_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
    } else {
        if (hInspectStaticRoute_) ShowWindow(hInspectStaticRoute_, SW_HIDE);
        if (hInspectBtnEditRoute_) ShowWindow(hInspectBtnEditRoute_, SW_HIDE);
        if (hInspectCheckLoop_) ShowWindow(hInspectCheckLoop_, SW_HIDE);
        if (hInspectBtnClearRoute_) ShowWindow(hInspectBtnClearRoute_, SW_HIDE);
        if (hInspectListActions_) ShowWindow(hInspectListActions_, SW_HIDE);
        if (hInspectBtnAddAction_) ShowWindow(hInspectBtnAddAction_, SW_HIDE);
        if (hInspectBtnDelAction_) ShowWindow(hInspectBtnDelAction_, SW_HIDE);
        if (hInspectBtnMoveUp_) ShowWindow(hInspectBtnMoveUp_, SW_HIDE);
        if (hInspectBtnMoveDown_) ShowWindow(hInspectBtnMoveDown_, SW_HIDE);
    }

    if (hasEntity) {
        curY += 6;
        if (isWeapon) {
            SetWindowPos(hInspectBtnDelete_, nullptr, 0, curY - entInspectScrollY_, paneW, 28, kSetPosFlags);
            curY += 34;

            if (hInspectBtnApply_) ShowWindow(hInspectBtnApply_, SW_HIDE);
            if (hInspectBtnCancel_) ShowWindow(hInspectBtnCancel_, SW_HIDE);
            if (hInspectBtnDelete_) ShowWindow(hInspectBtnDelete_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        } else {
            int bottomBtnGap = 4;
            int bottomBtnW = (paneW - 2 * bottomBtnGap) / 3;
            SetWindowPos(hInspectBtnApply_, nullptr, 0, curY - entInspectScrollY_, bottomBtnW, 28, kSetPosFlags);
            SetWindowPos(hInspectBtnCancel_, nullptr, bottomBtnW + bottomBtnGap, curY - entInspectScrollY_, bottomBtnW, 28, kSetPosFlags);
            SetWindowPos(hInspectBtnDelete_, nullptr, (bottomBtnW + bottomBtnGap) * 2, curY - entInspectScrollY_, paneW - (bottomBtnW + bottomBtnGap) * 2, 28, kSetPosFlags);
            curY += 34;

            if (hInspectBtnApply_) ShowWindow(hInspectBtnApply_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
            if (hInspectBtnCancel_) ShowWindow(hInspectBtnCancel_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
            if (hInspectBtnDelete_) ShowWindow(hInspectBtnDelete_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        }
    } else {
        if (hInspectBtnApply_) ShowWindow(hInspectBtnApply_, SW_HIDE);
        if (hInspectBtnCancel_) ShowWindow(hInspectBtnCancel_, SW_HIDE);
        if (hInspectBtnDelete_) ShowWindow(hInspectBtnDelete_, SW_HIDE);
    }

    int totalContentH = curY + 6;
    int maxScroll = std::max(0, totalContentH - paneH);
    if (entInspectScrollY_ > maxScroll) entInspectScrollY_ = maxScroll;

    SCROLLINFO si = {};
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = totalContentH;
    si.nPage = std::max(1, paneH);
    si.nPos = entInspectScrollY_;
    SetScrollInfo(hEntInspectPane_, SB_VERT, &si, TRUE);
    ShowScrollBar(hEntInspectPane_, SB_VERT, (totalContentH > paneH) ? TRUE : FALSE);
    RedrawWindow(hEntInspectPane_, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE);
}

void WinArtPanel::setEntityGridHeight(int h) {
    if (h < 60) h = 60;
    entityGridHeight_ = h;
    if (hWnd_) {
        layoutControls();
        InvalidateRect(hWnd_, nullptr, TRUE);
    }

    HWND hFocus = GetFocus();
    if (hFocus && hWnd_ && IsChild(hWnd_, hFocus) && !IsWindowVisible(hFocus)) {
        SetFocus(hWnd_);
    }
}

void WinArtPanel::setActiveTab(int tabIndex) {
    if (tabIndex < 0) tabIndex = 0;
    if (tabIndex > 2) tabIndex = 2;
    if (tabIndex != currentTab_) {
        if (!confirmDiscardEntityEdits()) return;
    }
    TabCtrl_SetCurSel(hTabControl_, tabIndex);
    updateTabVisibility(tabIndex);
    InvalidateRect(hTabControl_, nullptr, TRUE);
}

static int findBestMatchingActorIndex(rs_knl::PedInstance *pPed) {
    if (!pPed) return 0;
    // 1. Exact match on both typeId and baseAnim
    for (size_t i = 0; i < s_actorItems.size(); ++i) {
        if (s_actorItems[i].typeId == pPed->type() && s_actorItems[i].baseAnim == pPed->baseAnim()) {
            return static_cast<int>(i);
        }
    }
    // 2. Fallback: match typeId (faction/behavior is primary)
    for (size_t i = 0; i < s_actorItems.size(); ++i) {
        if (s_actorItems[i].typeId == pPed->type()) {
            return static_cast<int>(i);
        }
    }
    // 3. Fallback: match baseAnim
    for (size_t i = 0; i < s_actorItems.size(); ++i) {
        if (pPed->baseAnim() != 0 && s_actorItems[i].baseAnim == pPed->baseAnim()) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

static int findBestMatchingVehicleIndex(rs_knl::Vehicle *pVeh) {
    if (!pVeh) return 0;
    if (pVeh->getType() == rs_knl::Vehicle::kVehicleTypeTrainBody) {
        uint16_t bAnim = pVeh->baseSpriteAnim();
        for (size_t i = 0; i < s_vehicleItems.size(); ++i) {
            if (s_vehicleItems[i].typeId == pVeh->getType() && s_vehicleItems[i].baseAnim == bAnim) {
                return static_cast<int>(i);
            }
        }
    }
    for (size_t i = 0; i < s_vehicleItems.size(); ++i) {
        if (s_vehicleItems[i].typeId == pVeh->getType()) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

static int findBestMatchingStaticIndex(rs_knl::Static *pStatic) {
    if (!pStatic) return 0;
    uint8_t st = pStatic->subType();
    uint16_t curAnim = pStatic->getDrawAnimation();
    uint16_t baseAnim = pStatic->baseAnim();

    // 1. Sliding Doors (0x0C..0x0F or smt_Door)
    if (st == 0x0C || st == 0x0D || st == 0x0E || st == 0x0F || pStatic->type() == rs_knl::Static::smt_Door) {
        uint16_t animToTest = (baseAnim != 0) ? baseAnim : curAnim;
        for (size_t i = 0; i < s_staticItems.size(); ++i) {
            if (s_staticItems[i].typeId == 0x0C) {
                if (animToTest >= s_staticItems[i].baseAnim && animToTest < s_staticItems[i].baseAnim + 8) {
                    return static_cast<int>(i);
                }
            }
        }
        return 0;
    }

    // 2. Traffic Light / Semaphore (0x05..0x08, smt_Semaphore, or 1040..1043)
    if ((st >= 0x05 && st <= 0x08) || pStatic->type() == rs_knl::Static::smt_Semaphore ||
        (curAnim >= 1040 && curAnim <= 1043)) {
        for (size_t i = 0; i < s_staticItems.size(); ++i) {
            if (s_staticItems[i].typeId == 0x05) return static_cast<int>(i);
        }
    }

    // 3. Neon Signs (0x0A or smt_NeonSign or 1045..1076)
    if (st == 0x0A || pStatic->type() == rs_knl::Static::smt_NeonSign ||
        (curAnim >= 1045 && curAnim <= 1076) || (baseAnim >= 1045 && baseAnim <= 1076)) {
        uint16_t animToTest = (baseAnim != 0) ? baseAnim : curAnim;
        uint16_t normAnim = (animToTest % 2 == 0) ? (animToTest - 1) : animToTest;
        for (size_t i = 0; i < s_staticItems.size(); ++i) {
            if (s_staticItems[i].typeId == 0x0A && s_staticItems[i].baseAnim == normAnim) {
                return static_cast<int>(i);
            }
        }
    }

    // 4. Wall Advertisements (0x1F, smt_Advertisement, or 570..575)
    if (st == 0x1F || pStatic->type() == rs_knl::Static::smt_Advertisement ||
        (curAnim >= 570 && curAnim <= 575) || (baseAnim >= 570 && baseAnim <= 575)) {
        uint16_t animToTest = (baseAnim != 0) ? baseAnim : curAnim;
        if (animToTest == 573) animToTest = 572;
        else if (animToTest == 575) animToTest = 574;
        for (size_t i = 0; i < s_staticItems.size(); ++i) {
            if (s_staticItems[i].typeId == 0x1F && s_staticItems[i].baseAnim == animToTest) {
                return static_cast<int>(i);
            }
        }
    }

    // 6. Windows
    if (st == 0x13 || curAnim == 550 || curAnim == 551 || baseAnim == 550 || baseAnim == 551) {
        for (size_t i = 0; i < s_staticItems.size(); ++i) {
            if (s_staticItems[i].typeId == 0x13) return static_cast<int>(i);
        }
    }
    if (st == 0x12 || curAnim == 552 || curAnim == 553 || baseAnim == 552 || baseAnim == 553) {
        for (size_t i = 0; i < s_staticItems.size(); ++i) {
            if (s_staticItems[i].typeId == 0x12) return static_cast<int>(i);
        }
    }
    if (st == 0x15 || curAnim == 556 || curAnim == 557 || baseAnim == 556 || baseAnim == 557) {
        for (size_t i = 0; i < s_staticItems.size(); ++i) {
            if (s_staticItems[i].typeId == 0x15) return static_cast<int>(i);
        }
    }
    if (st == 0x20 || curAnim == 1959 || curAnim == 1960 || baseAnim == 1959 || baseAnim == 1960) {
        for (size_t i = 0; i < s_staticItems.size(); ++i) {
            if (s_staticItems[i].typeId == 0x20) return static_cast<int>(i);
        }
    }

    // 7. Phone Booth (0x01, or 618..619)
    if (st == 0x01 || curAnim == 618 || curAnim == 619 || baseAnim == 618 || baseAnim == 619) {
        for (size_t i = 0; i < s_staticItems.size(); ++i) {
            if (s_staticItems[i].typeId == 0x01) return static_cast<int>(i);
        }
    }

    // 8. Mailbox (0x19, or 561)
    if (st == 0x19 || curAnim == 561 || baseAnim == 561) {
        for (size_t i = 0; i < s_staticItems.size(); ++i) {
            if (s_staticItems[i].typeId == 0x19) return static_cast<int>(i);
        }
    }

    // 9. Trash Bin (0x1A, or 562)
    if (st == 0x1A || curAnim == 562 || baseAnim == 562) {
        for (size_t i = 0; i < s_staticItems.size(); ++i) {
            if (s_staticItems[i].typeId == 0x1A) return static_cast<int>(i);
        }
    }

    // 10. Large Gate (0x26, 392, 395)
    if (st == 0x26 || curAnim == 392 || curAnim == 395 || baseAnim == 392 || baseAnim == 395) {
        for (size_t i = 0; i < s_staticItems.size(); ++i) {
            if (s_staticItems[i].typeId == 0x26) return static_cast<int>(i);
        }
    }

    // 11. Exact baseAnim or curAnim matches
    for (size_t i = 0; i < s_staticItems.size(); ++i) {
        if (s_staticItems[i].baseAnim == baseAnim || s_staticItems[i].baseAnim == curAnim) {
            return static_cast<int>(i);
        }
    }

    // 12. Fallback: match by sub_type
    for (size_t i = 0; i < s_staticItems.size(); ++i) {
        if (s_staticItems[i].typeId == st) return static_cast<int>(i);
    }

    return 0;
}

static int findBestMatchingWeaponIndex(rs_knl::WeaponInstance *pWeapon) {
    if (!pWeapon) return 0;
    auto wType = pWeapon->type();
    if (!isAmericanRevoltAvailable() && rs_knl::Weapon::isAmericanRevoltOnly(wType)) {
        return 0;
    }
    for (size_t i = 0; i < s_weaponItems.size(); ++i) {
        if (s_weaponItems[i].typeId == static_cast<int>(wType)) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

void WinArtPanel::selectEntity(rs_knl::ShootableMapObject *pObj) {
    if (pSelectedEntity_ && pSelectedEntity_ != pObj) {
        if (!confirmDiscardEntityEdits()) return;
    }
    pSelectedEntity_ = pObj;
    if (pObj) {
        if (currentTab_ != 1) {
            setActiveTab(1);
            pSelectedEntity_ = pObj;
            if (pViewport_) pViewport_->setSelectedEntity(pObj);
        }

        int targetCat = 1;
        int bestIdx = 0;
        int allIdx = 0;
        if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pObj)) {
            targetCat = 1;
            bestIdx = findBestMatchingActorIndex(pPed);
            allIdx = bestIdx;
        } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pObj)) {
            targetCat = 2;
            bestIdx = findBestMatchingVehicleIndex(pVeh);
            allIdx = static_cast<int>(s_actorItems.size()) + bestIdx;
        } else if (auto *pStatic = dynamic_cast<rs_knl::Static*>(pObj)) {
            targetCat = 3;
            bestIdx = findBestMatchingStaticIndex(pStatic);
            allIdx = static_cast<int>(s_actorItems.size() + s_vehicleItems.size()) + bestIdx;
        } else if (auto *pWeapon = dynamic_cast<rs_knl::WeaponInstance*>(pObj)) {
            targetCat = 4;
            bestIdx = findBestMatchingWeaponIndex(pWeapon);
            allIdx = static_cast<int>(s_actorItems.size() + s_vehicleItems.size() + s_staticItems.size()) + bestIdx;
        }

        if (entityCategory_ == 0) {
            selectedEntityGridIdx_ = allIdx;
        } else {
            if (entityCategory_ != targetCat) {
                entityCategory_ = targetCat;
                ComboBox_SetCurSel(hComboEntityType_, targetCat);
                entityGridScrollY_ = 0;
                updateEntityGridScroll();
            }
            selectedEntityGridIdx_ = bestIdx;
        }

        if (hEntityGrid_) {
            int cols = getEntityGridColumns();
            int row = selectedEntityGridIdx_ / cols;
            int cellH = 54;
            int entTop = row * cellH;
            int entBottom = entTop + cellH + 8;
            RECT rc = {};
            GetClientRect(hEntityGrid_, &rc);
            int viewH = rc.bottom;
            if (viewH > 0) {
                if (entTop < entityGridScrollY_) {
                    entityGridScrollY_ = entTop;
                } else if (entBottom > entityGridScrollY_ + viewH) {
                    entityGridScrollY_ = entBottom - viewH;
                }
                updateEntityGridScroll();
            }
        }
        InvalidateRect(hEntityGrid_, nullptr, FALSE);

        refreshInspector();
    } else {
        refreshInspector();
    }
}

void WinArtPanel::clearSelectedEntity() {
    if (pSelectedEntity_) {
        if (!confirmDiscardEntityEdits()) return;
    }
    pSelectedEntity_ = nullptr;
    if (pViewport_ && pViewport_->selectedEntity()) {
        pViewport_->setSelectedEntity(nullptr);
    }
    refreshInspector();
}

static void populateOrient8(HWND cb) {
    ComboBox_ResetContent(cb);
    ComboBox_AddString(cb, L"South-West (0)");
    ComboBox_AddString(cb, L"South (32)");
    ComboBox_AddString(cb, L"South-East (64)");
    ComboBox_AddString(cb, L"East (96)");
    ComboBox_AddString(cb, L"North-East (128)");
    ComboBox_AddString(cb, L"North (160)");
    ComboBox_AddString(cb, L"North-West (192)");
    ComboBox_AddString(cb, L"West (224)");
}

static void populateOrientStatic(HWND cb) {
    ComboBox_ResetContent(cb);
    ComboBox_AddString(cb, L"Diagonal NW \u2194 SE (Along X)");
    ComboBox_AddString(cb, L"Diagonal SW \u2194 NE (Along Y)");
}

static void populateOrientTrain(HWND cb) {
    ComboBox_ResetContent(cb);
    ComboBox_AddString(cb, L"Diagonal SW \u2194 NE (0 / 128)");
    ComboBox_AddString(cb, L"Diagonal SE \u2194 NW (64 / 192)");
}

static void populateOrientVehicle(HWND cb) {
    ComboBox_ResetContent(cb);
    ComboBox_AddString(cb, L"South-West (0)");
    ComboBox_AddString(cb, L"South-East (64)");
    ComboBox_AddString(cb, L"North-East (128)");
    ComboBox_AddString(cb, L"North-West (192)");
}

void WinArtPanel::populateDriverCombo(rs_knl::Vehicle *pVeh) {
    if (!hInspectComboDriver_) return;

    if (pSelectedEntity_) {
        // Inspector mode
        auto *pSelectedVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_);
        bool isTrainBody = (pSelectedVeh && pSelectedVeh->getType() == rs_knl::Vehicle::kVehicleTypeTrainBody);
        if (isTrainBody) {
            ComboBox_ResetContent(hInspectComboDriver_);
            ComboBox_AddString(hInspectComboDriver_, L"Automatic Rail System");
            ComboBox_SetCurSel(hInspectComboDriver_, 0);
            EnableWindow(hInspectComboDriver_, FALSE);
            origDriver_ = nullptr;
            origDriverChoice_ = 0;
            return;
        }

        EnableWindow(hInspectComboDriver_, TRUE);
        ComboBox_ResetContent(hInspectComboDriver_);
        int idxNone = ComboBox_AddString(hInspectComboDriver_, L"None (Unoccupied)");
        ComboBox_SetItemData(hInspectComboDriver_, idxNone, reinterpret_cast<LPARAM>(nullptr));

        int idxNew = ComboBox_AddString(hInspectComboDriver_, L"+ New Civilian Driver");
        ComboBox_SetItemData(hInspectComboDriver_, idxNew, reinterpret_cast<LPARAM>(nullptr));

        rs_knl::PedInstance *curDriver = pVeh ? pVeh->getDriver() : (pSelectedVeh ? pSelectedVeh->getDriver() : nullptr);
        int selIdx = 0;

        if (pMission_) {
            for (size_t i = 0; i < pMission_->numPeds(); ++i) {
                rs_knl::PedInstance *p = pMission_->ped(i);
                if (!p) continue;
                std::wstring typeStr = L"Civ";
                if (p->type() == rs_knl::PedInstance::kPedTypeGuard) typeStr = L"Guard";
                else if (p->type() == rs_knl::PedInstance::kPedTypePolice) typeStr = L"Police";
                else if (p->type() == rs_knl::PedInstance::kPedTypeAgent) typeStr = L"Agent";
                else if (p->type() == rs_knl::PedInstance::kPedTypeCriminal) typeStr = L"Crim";

                std::wstring label = std::format(L"Ped #{:03} ({}, HP:{})", p->id(), typeStr, p->health());
                int idx = ComboBox_AddString(hInspectComboDriver_, label.c_str());
                ComboBox_SetItemData(hInspectComboDriver_, idx, reinterpret_cast<LPARAM>(p));
                if (p == curDriver) {
                    selIdx = idx;
                }
            }
        }
        ComboBox_SetCurSel(hInspectComboDriver_, selIdx);
        origDriver_ = curDriver;
        origDriverChoice_ = selIdx;
    } else {
        // Placement mode
        bool isTrainBody = false;
        const auto *pItem = selectedEntityTemplateItem();
        if (pItem && pItem->kind == EntityKind::Vehicle) {
            isTrainBody = (pItem->typeId == rs_knl::Vehicle::kVehicleTypeTrainBody);
        }
        if (isTrainBody) {
            ComboBox_ResetContent(hInspectComboDriver_);
            ComboBox_AddString(hInspectComboDriver_, L"Automatic Rail System");
            ComboBox_SetCurSel(hInspectComboDriver_, 0);
            EnableWindow(hInspectComboDriver_, FALSE);
            origDriver_ = nullptr;
            origDriverChoice_ = 0;
            return;
        }

        EnableWindow(hInspectComboDriver_, TRUE);
        ComboBox_ResetContent(hInspectComboDriver_);
        ComboBox_AddString(hInspectComboDriver_, L"None (Unoccupied)");
        for (const auto &actor : s_actorItems) {
            std::wstring label = L"+ " + std::wstring(actor.name.begin(), actor.name.end()) + L" Driver";
            ComboBox_AddString(hInspectComboDriver_, label.c_str());
        }
        if (activeDriverChoice_ < 0 || activeDriverChoice_ > static_cast<int>(s_actorItems.size())) {
            activeDriverChoice_ = 0;
        }
        ComboBox_SetCurSel(hInspectComboDriver_, activeDriverChoice_);
        SendMessageW(hInspectComboDriver_, CB_SETDROPPEDWIDTH, 230, 0);
        origDriver_ = nullptr;
        origDriverChoice_ = activeDriverChoice_;
    }
}

static const wchar_t* getCyberwareTierName(int tier, bool isCampaign = false) {
    switch (tier) {
        case 0: return isCampaign ? L"Dynamic (World Progression)" : L"Unaugmented";
        case 1: return L"Basic Cybernetics (V1)";
        case 2: return L"Advanced Cybernetics (V2)";
        case 3: return L"Apex Cybernetics (V3)";
        default: return L"Unaugmented";
    }
}

void WinArtPanel::populateCyberwareCombo(HWND hCombo, bool isOverridden, int overrideTier) {
    if (!hCombo) return;
    ComboBox_ResetContent(hCombo);

    bool isCampaign = (activeMissionId_ >= 1 && activeMissionId_ <= 50);
    int missionTier = pMission_ ? pMission_->techLevel() : 0;
    std::wstring inheritLabel = (missionTier == 0 && isCampaign)
        ? L"Inherit: Tier 0 (Dynamic - World Progression)"
        : std::format(L"Inherit: Tier {} ({})", missionTier, getCyberwareTierName(missionTier, isCampaign));
    ComboBox_AddString(hCombo, inheritLabel.c_str());

    ComboBox_AddString(hCombo, L"Override: Tier 0 (Unaugmented)");
    ComboBox_AddString(hCombo, L"Override: Tier 1 (Basic Cybernetics V1)");
    ComboBox_AddString(hCombo, L"Override: Tier 2 (Advanced Cybernetics V2)");
    ComboBox_AddString(hCombo, L"Override: Tier 3 (Apex Cybernetics V3)");

    int sel = isOverridden ? (1 + std::clamp(overrideTier, 0, 3)) : 0;
    ComboBox_SetCurSel(hCombo, sel);
    SendMessageW(hCombo, CB_SETDROPPEDWIDTH, 320, 0);
}

void WinArtPanel::populateFactionCombo(bool isPlayerAgent, int factionId) {
    if (!hInspectComboFaction_) return;
    ComboBox_ResetContent(hInspectComboFaction_);
    if (isPlayerAgent) {
        ComboBox_AddString(hInspectComboFaction_, L"Faction 0 (Player - Red)");
        ComboBox_SetCurSel(hInspectComboFaction_, 0);
        EnableWindow(hInspectComboFaction_, FALSE);
    } else {
        ComboBox_AddString(hInspectComboFaction_, L"Faction 1 (Blue)");
        ComboBox_AddString(hInspectComboFaction_, L"Faction 2 (Orange)");
        ComboBox_AddString(hInspectComboFaction_, L"Faction 3 (Yellow A)");
        ComboBox_AddString(hInspectComboFaction_, L"Faction 4 (Yellow B)");
        ComboBox_AddString(hInspectComboFaction_, L"Faction 5 (Purple)");
        ComboBox_AddString(hInspectComboFaction_, L"Faction 6 (Grey)");
        ComboBox_AddString(hInspectComboFaction_, L"Faction 7 (Cyan)");
        int fSel = (factionId >= 1 && factionId <= 7) ? (factionId - 1) : 0;
        ComboBox_SetCurSel(hInspectComboFaction_, fSel);
        EnableWindow(hInspectComboFaction_, TRUE);
    }
}

static bool isFixedDirectionStatic(uint8_t subType, uint16_t baseAnim) {
    if (subType == 0x1F) {
        // Wall ads: Billboards (572, 573, 574, 575) have 2 orientations. Posters (570, 571) are fixed.
        return (baseAnim < 572 || baseAnim > 575);
    }
    // Doors (0x0C..0x0F), Windows (0x12, 0x13, 0x15), Gate (0x26) have 2 orientations
    if ((subType >= 0x0C && subType <= 0x0F) ||
        subType == 0x12 || subType == 0x13 || subType == 0x15 ||
        subType == 0x26) {
        return false;
    }
    // All other statics (phone booth 0x01, semaphores 0x05..0x08, neon signs 0x0A,
    // trees 0x16, mailbox 0x19, trash bin 0x1A, anim window 0x20) are fixed direction.
    return true;
}

static bool isFixedDirectionStatic(const rs_knl::Static *pStatic) {
    if (!pStatic) return false;
    return isFixedDirectionStatic(pStatic->subType(), pStatic->baseAnim());
}

void WinArtPanel::setWeaponsLabelText(const std::wstring &text) {
    if (hInspectStaticWeapons_) {
        SetWindowTextW(hInspectStaticWeapons_, text.c_str());
    }
}

void WinArtPanel::toggleWeapon(int weaponIndex) {
    bool hasAR = isAmericanRevoltAvailable();
    int maxWeps = hasAR ? static_cast<int>(sizeof(kWeaponsList)/sizeof(kWeaponsList[0])) : 14;
    if (weaponIndex < 0 || weaponIndex >= maxWeps) return;
    if (selectedWeapons_.count(weaponIndex)) {
        selectedWeapons_.erase(weaponIndex);
    } else {
        selectedWeapons_.insert(weaponIndex);
    }
    markEntityDirty();
    if (hInspectWeaponsGrid_) {
        InvalidateRect(hInspectWeaponsGrid_, nullptr, FALSE);
    }
    if (!pSelectedEntity_) {
        applyEntityPlacementFromSelection();
    }
}

void WinArtPanel::refreshWeaponsGrid() {
    if (hInspectWeaponsGrid_) {
        InvalidateRect(hInspectWeaponsGrid_, nullptr, FALSE);
    }
}

void WinArtPanel::refreshActionsList() {
    if (!hInspectListActions_) return;
    int prevSel = ListBox_GetCurSel(hInspectListActions_);
    ListBox_ResetContent(hInspectListActions_);

    rs_knl::PedInstance *pRoutePed = nullptr;
    if (auto *pP = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
        pRoutePed = pP;
    } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) {
        pRoutePed = pVeh->getDriver();
    }

    if (!pRoutePed) return;

    const auto &steps = pRoutePed->scriptSteps();
    int gotoCounter = 0;
    int actionCount = 0;
    for (size_t i = 0; i < steps.size(); ++i) {
        const auto &st = steps[i];
        if (st.type == LevelData::kScenarioTypeReset) {
            continue; // Loop state is cleanly indicated by the Loop Route checkbox
        }
        actionCount++;
        std::wstring line;
        std::wstring coord = std::format(L"({}, {}, {})", st.targetPos.tx, st.targetPos.ty, st.targetPos.tz);
        switch (st.type) {
            case LevelData::kScenarioTypeGoto: {
                gotoCounter++;
                rs_knl::Vehicle *vAtStep = WinMapViewport::getPedVehicleAtStep(pMission_, pRoutePed, i);
                if (i == 0) {
                    if (vAtStep) {
                        line = std::format(L"#{:02} {} [Spawn] Drive Vehicle #{:03}", i + 1, coord, vAtStep->id());
                    } else {
                        line = std::format(L"#{:02} {} [Spawn] Waypoint 1 (Walk)", i + 1, coord);
                    }
                } else if (vAtStep) {
                    if (vAtStep->isCar()) {
                        line = std::format(L"#{:02} {} Drive Car #{:03}", i + 1, coord, vAtStep->id());
                    } else if (vAtStep->getType() == rs_knl::Vehicle::kVehicleTypeTrainHead || vAtStep->getType() == rs_knl::Vehicle::kVehicleTypeTrainBody) {
                        line = std::format(L"#{:02} {} Drive Train #{:03}", i + 1, coord, vAtStep->id());
                    } else {
                        line = std::format(L"#{:02} {} Drive Vehicle #{:03}", i + 1, coord, vAtStep->id());
                    }
                } else {
                    line = std::format(L"#{:02} {} Goto (Walk)", i + 1, coord);
                }
                break;
            }
            case LevelData::kScenarioTypeWait: {
                int sec = (st.param > 0) ? (st.param / 1000) : 5;
                if (i == 0) {
                    line = std::format(L"#{:02} Wait {}s (at Spawn)", i + 1, sec);
                } else if (gotoCounter > 0) {
                    line = std::format(L"#{:02} Wait {}s (at Waypoint #{})", i + 1, sec, gotoCounter);
                } else {
                    line = std::format(L"#{:02} Wait {}s", i + 1, sec);
                }
                break;
            }
            case LevelData::kScenarioTypeTrainWait:
                line = std::format(L"#{:02} Train Dwell Wait", i + 1);
                break;
            case LevelData::kScenarioTypeTrigger: {
                int rTiles = (st.param > 0) ? static_cast<int>(st.param / 256) : 6;
                line = std::format(L"#{:02} {} Ambush Trigger (Radius: {} tiles)", i + 1, coord, rTiles);
                break;
            }
            case LevelData::kScenarioTypeLeaveVehicle:
                line = std::format(L"#{:02} Unboard / Exit Vehicle", i + 1);
                break;
            case LevelData::kScenarioTypeEscape:
                if (st.targetPos.tx != 0 || st.targetPos.ty != 0) {
                    line = std::format(L"#{:02} {} Escape", i + 1, coord);
                } else {
                    line = std::format(L"#{:02} Escape / Evacuate", i + 1);
                }
                break;
            case LevelData::kScenarioTypeUseVehicle:
                if (pMission_ && st.targetEntityId >= 0 && static_cast<size_t>(st.targetEntityId) < pMission_->numVehicles()) {
                    rs_knl::Vehicle *v = pMission_->vehicle(st.targetEntityId);
                    line = std::format(L"#{:02} Board Vehicle #{:03}", i + 1, v ? v->id() : st.targetEntityId);
                } else {
                    line = std::format(L"#{:02} Board Vehicle #{} (Invalid / Deleted)", i + 1, st.targetEntityId);
                }
                break;
            default:
                line = std::format(L"#{:02} Action 0x{:02X}", i + 1, st.type);
                break;
        }
        ListBox_AddString(hInspectListActions_, line.c_str());
    }

    if (prevSel >= 0 && prevSel < actionCount) {
        ListBox_SetCurSel(hInspectListActions_, prevSel);
    } else if (actionCount > 0) {
        ListBox_SetCurSel(hInspectListActions_, 0);
    }
}

void WinArtPanel::refreshRouteUI() {
    auto *pSelectedPed = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_);
    bool isAgent = (pSelectedPed && pSelectedPed->type() == rs_knl::PedInstance::kPedTypeAgent);

    rs_knl::PedInstance *pRoutePed = nullptr;
    if (pSelectedPed && !isAgent) {
        pRoutePed = pSelectedPed;
    } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) {
        pRoutePed = pVeh->getDriver();
    }

    auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_);
    bool isTrainBody = (pVeh && pVeh->getType() == rs_knl::Vehicle::kVehicleTypeTrainBody);
    bool isVeh = (pSelectedEntity_ && pSelectedEntity_->is(rs_knl::MapObject::kNatureVehicle) && !isTrainBody);
    bool isActor = (pSelectedPed != nullptr && !isAgent);

    if (pRoutePed) {
        int wpCount = 0;
        for (const auto &st : pRoutePed->scriptSteps()) {
            if (st.type == LevelData::kScenarioTypeGoto ||
                st.type == LevelData::kScenarioTypeTrigger ||
                st.type == LevelData::kScenarioTypeWait ||
                st.type == 1 || st.type == 5 || st.type == 8) {
                wpCount++;
            }
        }
        std::wstring routeSummary;
        if (wpCount > 0) {
            routeSummary = std::format(L"Route: {} Waypoint{} ({})",
                wpCount,
                (wpCount == 1) ? L"" : L"s",
                pRoutePed->isScriptLooping() ? L"Looping" : L"One-way");
        } else {
            routeSummary = L"Route: None (Stationary)";
        }
        SetWindowTextW(hInspectStaticRoute_, routeSummary.c_str());
        Button_SetCheck(hInspectCheckLoop_, pRoutePed->isScriptLooping() ? BST_CHECKED : BST_UNCHECKED);

        bool isRouteToolActive = (pViewport_ && pViewport_->tool() == ViewportTool::RouteTool);
        SetWindowTextW(hInspectBtnEditRoute_, isRouteToolActive ? L"Done Route (Esc)" : L"Edit Route (R)");
        if (hInspectBtnEditRoute_) InvalidateRect(hInspectBtnEditRoute_, nullptr, TRUE);

        EnableWindow(hInspectBtnEditRoute_, TRUE);
        EnableWindow(hInspectCheckLoop_, TRUE);
        EnableWindow(hInspectBtnClearRoute_, (wpCount > 0) ? TRUE : FALSE);
    } else if (isVeh) {
        SetWindowTextW(hInspectStaticRoute_, L"Route: Requires Driver");
        Button_SetCheck(hInspectCheckLoop_, BST_UNCHECKED);
        SetWindowTextW(hInspectBtnEditRoute_, L"Edit Route (R)");
        if (hInspectBtnEditRoute_) InvalidateRect(hInspectBtnEditRoute_, nullptr, TRUE);
        EnableWindow(hInspectBtnEditRoute_, FALSE);
        EnableWindow(hInspectCheckLoop_, FALSE);
        EnableWindow(hInspectBtnClearRoute_, FALSE);
    } else {
        SetWindowTextW(hInspectStaticRoute_, L"Route: N/A");
        Button_SetCheck(hInspectCheckLoop_, BST_UNCHECKED);
        if (hInspectBtnEditRoute_) InvalidateRect(hInspectBtnEditRoute_, nullptr, TRUE);
        EnableWindow(hInspectBtnEditRoute_, FALSE);
        EnableWindow(hInspectCheckLoop_, FALSE);
        EnableWindow(hInspectBtnClearRoute_, FALSE);
    }

    refreshActionsList();
    int selAct = ListBox_GetCurSel(hInspectListActions_);
    int numActions = ListBox_GetCount(hInspectListActions_);
    EnableWindow(hInspectBtnAddAction_, pRoutePed != nullptr ? TRUE : FALSE);
    EnableWindow(hInspectBtnDelAction_, (pRoutePed != nullptr && selAct > 0 && selAct < numActions) ? TRUE : FALSE);
    EnableWindow(hInspectBtnMoveUp_, (pRoutePed != nullptr && selAct > 1 && selAct < numActions) ? TRUE : FALSE);
    EnableWindow(hInspectBtnMoveDown_, (pRoutePed != nullptr && selAct > 0 && selAct + 1 < numActions) ? TRUE : FALSE);

    if (currentTab_ == 1) {
        bool showRoute = (isActor || isVeh);
        ShowWindow(hInspectStaticRoute_, showRoute ? SW_SHOW : SW_HIDE);
        ShowWindow(hInspectBtnEditRoute_, showRoute ? SW_SHOW : SW_HIDE);
        ShowWindow(hInspectCheckLoop_, showRoute ? SW_SHOW : SW_HIDE);
        ShowWindow(hInspectBtnClearRoute_, showRoute ? SW_SHOW : SW_HIDE);
        ShowWindow(hInspectListActions_, showRoute ? SW_SHOW : SW_HIDE);
        ShowWindow(hInspectBtnAddAction_, showRoute ? SW_SHOW : SW_HIDE);
        ShowWindow(hInspectBtnDelAction_, showRoute ? SW_SHOW : SW_HIDE);
        ShowWindow(hInspectBtnMoveUp_, showRoute ? SW_SHOW : SW_HIDE);
        ShowWindow(hInspectBtnMoveDown_, showRoute ? SW_SHOW : SW_HIDE);
    }
}

void WinArtPanel::refreshInspector() {
    if (isPopulatingInspector_) return;
    if (currentTab_ != 1 || isEntitySearchActive_) {
        if (hEntInspectPane_) ShowWindow(hEntInspectPane_, SW_HIDE);
        return;
    }
    if (hEntInspectPane_) ShowWindow(hEntInspectPane_, SW_SHOW);

    if (!pSelectedEntity_) {
        isPopulatingInspector_ = true;
        ShowWindow(hInspectBtnPlaytestConfig_, SW_HIDE);
        for (auto &item : s_actorItems) {
            auto pType = static_cast<rs_knl::PedInstance::PedType>(item.typeId);
            bool isEnemy = (pType == rs_knl::PedInstance::kPedTypeAgent);
            item.defaultHealth = rs_knl::PedManager::getStats(pType, isEnemy).health;
        }
        setSelectedEntityGridIndex(selectedEntityGridIdx_);
        isPopulatingInspector_ = false;
        layoutEntInspectPane();
        if (hEntInspectPane_) RedrawWindow(hEntInspectPane_, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE);
        return;
    }

    isPopulatingInspector_ = true;
    if (hInspectHeader_) ShowWindow(hInspectHeader_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);

    bool isObscured = false;
    int roofZ = -1;
    if (pMission_ && pMission_->map()) {
        for (int z = pSelectedEntity_->tileZ() + 1; z < pMission_->map()->maxTz(); ++z) {
            auto *t = pMission_->map()->getTileAt(pSelectedEntity_->tileX(), pSelectedEntity_->tileY(), z);
            if (t && t->id() != 0 && t->notTransparent()) {
                isObscured = true;
                roofZ = z;
            }
        }
    }

    if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
        bool isPlayerAgent = pPed->isOurAgent();
        if (isPlayerAgent) {
            std::wstring hdr = std::format(L"Selected Player's Agent #{:03}", pPed->id() + 1);
            SetWindowTextW(hInspectHeader_, hdr.c_str());

            SetWindowTextW(hInspectStaticType_, L"Type:");
            ShowWindow(hInspectValueType_, SW_HIDE);
            if (hInspectComboType_) {
                ShowWindow(hInspectComboType_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
                ComboBox_SetCurSel(hInspectComboType_, 0);
            }

            ShowWindow(hInspectStaticHealth_, SW_HIDE);
            ShowWindow(hInspectEditHealth_, SW_HIDE);
            ShowWindow(hInspectCheckDead_, SW_HIDE);

            EnableWindow(hInspectComboOrient_, TRUE);
            populateOrient8(hInspectComboOrient_);
            int oriIdx = ((pPed->direction() + 16) / 32) % 8;
            ComboBox_SetCurSel(hInspectComboOrient_, oriIdx);

            ShowWindow(hInspectStaticDriver_, SW_HIDE);
            ShowWindow(hInspectComboDriver_, SW_HIDE);
            ShowWindow(hInspectStaticCyberware_, SW_HIDE);
            ShowWindow(hInspectComboCyberware_, SW_HIDE);
            ShowWindow(hInspectStaticMoveMode_, SW_HIDE);
            ShowWindow(hInspectComboMoveMode_, SW_HIDE);
            ShowWindow(hInspectStaticFaction_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
            ShowWindow(hInspectComboFaction_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
            populateFactionCombo(true, 0);
            if (hInspectBtnAiTactics_) ShowWindow(hInspectBtnAiTactics_, SW_HIDE);
            origCyberwareOverridden_ = false;
            origCyberwareTier_ = 0;
            origMovementMode_ = 0x10;
            activeMovementMode_ = 0x10;
            origFactionId_ = 0;
            activeFactionId_ = 0;

            ShowWindow(hInspectStaticWeapons_, SW_HIDE);
            ShowWindow(hInspectWeaponsGrid_, SW_HIDE);
            selectedWeapons_.clear();

            ShowWindow(hInspectStaticRoute_, SW_HIDE);
            ShowWindow(hInspectBtnEditRoute_, SW_HIDE);
            ShowWindow(hInspectCheckLoop_, SW_HIDE);
            ShowWindow(hInspectBtnClearRoute_, SW_HIDE);
            ShowWindow(hInspectListActions_, SW_HIDE);
            ShowWindow(hInspectBtnAddAction_, SW_HIDE);
            ShowWindow(hInspectBtnDelAction_, SW_HIDE);
            ShowWindow(hInspectBtnMoveUp_, SW_HIDE);
            ShowWindow(hInspectBtnMoveDown_, SW_HIDE);

            ShowWindow(hInspectBtnPlaytestConfig_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);

            origHealth_ = 100;
            origDir_ = static_cast<uint8_t>(pPed->direction());
            origWeapons_.clear();
        } else {
            ShowWindow(hInspectBtnPlaytestConfig_, SW_HIDE);
            ShowWindow(hInspectStaticHealth_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
            ShowWindow(hInspectEditHealth_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
            ShowWindow(hInspectStaticWeapons_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
            ShowWindow(hInspectWeaponsGrid_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
            std::wstring hdr = std::format(L"Selected Ped #{:03}", pPed->id());
            SetWindowTextW(hInspectHeader_, hdr.c_str());

            std::wstring typeStr;
            if (pPed->type() == rs_knl::PedInstance::kPedTypeGuard) {
                typeStr = L"Platform Guard";
            } else if (pPed->type() == rs_knl::PedInstance::kPedTypePolice) {
                typeStr = L"Police Officer";
            } else if (pPed->type() == rs_knl::PedInstance::kPedTypeAgent) {
                typeStr = L"Enemy Agent";
            } else if (pPed->type() == rs_knl::PedInstance::kPedTypeCriminal) {
                if (pPed->baseAnim() == 1123) typeStr = L"Criminal (Punk)";
                else if (pPed->baseAnim() == 1330) typeStr = L"Criminal (Merc)";
                else typeStr = L"Criminal";
            } else {
                if (pPed->baseAnim() == 1123) typeStr = L"Civilian (Punk)";
                else if (pPed->baseAnim() == 626) typeStr = L"Civilian (Female)";
                else if (pPed->baseAnim() == 1537) typeStr = L"Civilian (Male)";
                else typeStr = L"Civilian";
            }

            SetWindowTextW(hInspectStaticType_, L"Type:");
            if (pPed->type() == rs_knl::PedInstance::kPedTypeAgent) {
                ShowWindow(hInspectValueType_, SW_HIDE);
                if (hInspectComboType_) {
                    ShowWindow(hInspectComboType_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
                    ComboBox_SetCurSel(hInspectComboType_, 1);
                }
            } else {
                if (hInspectComboType_) ShowWindow(hInspectComboType_, SW_HIDE);
                ShowWindow(hInspectValueType_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
                SetWindowTextW(hInspectValueType_, typeStr.c_str());
            }

            ShowWindow(hInspectCheckDead_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
            SetWindowTextW(hInspectCheckDead_, L"Dead");
            bool isDead = (pPed->health() <= 0);
            SendMessageW(hInspectCheckDead_, BM_SETCHECK, isDead ? BST_CHECKED : BST_UNCHECKED, 0);
            EnableWindow(hInspectEditHealth_, isDead ? FALSE : TRUE);
            SetWindowTextW(hInspectEditHealth_, isDead ? L"-1" : std::to_wstring(pPed->health()).c_str());

            EnableWindow(hInspectComboOrient_, TRUE);
            populateOrient8(hInspectComboOrient_);
            int oriIdx = ((pPed->direction() + 16) / 32) % 8;
            ComboBox_SetCurSel(hInspectComboOrient_, oriIdx);

            ShowWindow(hInspectStaticDriver_, SW_HIDE);
            ShowWindow(hInspectComboDriver_, SW_HIDE);

            bool isEnemyAgent = (pPed->type() == rs_knl::PedInstance::kPedTypeAgent && !pPed->isOurAgent());
            ShowWindow(hInspectStaticCyberware_, (isEnemyAgent && currentTab_ == 1) ? SW_SHOW : SW_HIDE);
            ShowWindow(hInspectComboCyberware_, (isEnemyAgent && currentTab_ == 1) ? SW_SHOW : SW_HIDE);
            if (isEnemyAgent) {
                populateCyberwareCombo(hInspectComboCyberware_, pPed->isCyberwareOverridden(), pPed->cyberwareTier());
                origCyberwareOverridden_ = pPed->isCyberwareOverridden();
                origCyberwareTier_ = pPed->cyberwareTier();

                int fId = pPed->factionId();
                populateFactionCombo(false, fId);
                origFactionId_ = fId;
                activeFactionId_ = fId;

            } else {
                origCyberwareOverridden_ = false;
                origCyberwareTier_ = 0;
                origFactionId_ = -1;
                activeFactionId_ = -1;
            }
            ShowWindow(hInspectStaticFaction_, (isEnemyAgent && currentTab_ == 1) ? SW_SHOW : SW_HIDE);
            ShowWindow(hInspectComboFaction_, (isEnemyAgent && currentTab_ == 1) ? SW_SHOW : SW_HIDE);
            ShowWindow(hInspectBtnAiTactics_, (isEnemyAgent && currentTab_ == 1) ? SW_SHOW : SW_HIDE);

            uint8_t moveMode = (pPed->initialLevelState() & LevelData::kPeopleStateWalking) ? 0x10 : 0x00;
            ComboBox_SetCurSel(hInspectComboMoveMode_, (moveMode == 0x10) ? 0 : 1);
            origMovementMode_ = moveMode;
            activeMovementMode_ = moveMode;
            ShowWindow(hInspectStaticMoveMode_, (currentTab_ == 1) ? SW_SHOW : SW_HIDE);
            ShowWindow(hInspectComboMoveMode_, (currentTab_ == 1) ? SW_SHOW : SW_HIDE);

            EnableWindow(hInspectWeaponsGrid_, TRUE);
            SetWindowTextW(hInspectStaticWeapons_, L"Weapons Inventory (Equipped):");
            selectedWeapons_.clear();
            origWeapons_.clear();

            bool hasAR = isAmericanRevoltAvailable();
            int maxWeps = hasAR ? static_cast<int>(sizeof(kWeaponsList)/sizeof(kWeaponsList[0])) : 14;
            for (int i = 0; i < maxWeps; ++i) {
                bool hasW = (pPed->numWeapons() > 0) && pPed->hasWeapon(kWeaponsList[i].type);
                if (hasW) {
                    selectedWeapons_.insert(i);
                    origWeapons_.push_back(i);
                }
            }
            refreshWeaponsGrid();
            ShowWindow(hInspectStaticWeapons_, (currentTab_ == 1) ? SW_SHOW : SW_HIDE);
            ShowWindow(hInspectWeaponsGrid_, (currentTab_ == 1) ? SW_SHOW : SW_HIDE);

            origHealth_ = pPed->health();
            origDir_ = static_cast<uint8_t>(pPed->direction());
        }
    } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) {
        const wchar_t *vTypeName = L"Vehicle";
        uint8_t vt = pVeh->getType();
        if (vt == rs_knl::Vehicle::kVehicleTypeRegularCar) vTypeName = L"Regular Car";
        else if (vt == rs_knl::Vehicle::kVehicleTypeSmallArmored) vTypeName = L"Armored Car";
        else if (vt == rs_knl::Vehicle::kVehicleTypeLargeArmored) vTypeName = L"Large Armored";
        else if (vt == rs_knl::Vehicle::kVehicleTypePolice) vTypeName = L"Police Car";
        else if (vt == rs_knl::Vehicle::kVehicleTypeFireFighter) vTypeName = L"Firetruck";
        else if (vt == rs_knl::Vehicle::kVehicleTypeMedics) vTypeName = L"Ambulance";
        else if (vt == rs_knl::Vehicle::kVehicleTypeTrainHead) vTypeName = L"Train Engine";
        else if (vt == rs_knl::Vehicle::kVehicleTypeTrainBody) {
            if (pVeh->baseSpriteAnim() >= 440) vTypeName = L"Train Carriage (Raised Roof)";
            else if (pVeh->baseSpriteAnim() >= 434) vTypeName = L"Train Carriage (Door)";
            else vTypeName = L"Train Carriage (Windows)";
        }

        std::wstring hdr = std::format(L"Selected {} #{:03}", vTypeName, pVeh->id());
        SetWindowTextW(hInspectHeader_, hdr.c_str());

        SetWindowTextW(hInspectStaticType_, L"Type:");
        SetWindowTextW(hInspectValueType_, vTypeName);

        bool isTrain = (vt == rs_knl::Vehicle::kVehicleTypeTrainHead || vt == rs_knl::Vehicle::kVehicleTypeTrainBody);
        ShowWindow(hInspectStaticHealth_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        ShowWindow(hInspectEditHealth_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        ShowWindow(hInspectCheckDead_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        SetWindowTextW(hInspectCheckDead_, L"Destroyed");
        bool isDestroyed = (pVeh->health() <= 0);
        SendMessageW(hInspectCheckDead_, BM_SETCHECK, isDestroyed ? BST_CHECKED : BST_UNCHECKED, 0);
        EnableWindow(hInspectEditHealth_, !isDestroyed ? TRUE : FALSE);
        SetWindowTextW(hInspectEditHealth_, isDestroyed ? L"-1" : std::to_wstring(pVeh->health()).c_str());
        EnableWindow(hInspectComboOrient_, TRUE);
        if (vt == rs_knl::Vehicle::kVehicleTypeTrainBody) {
            populateOrientTrain(hInspectComboOrient_);
            int trainSel = ((pVeh->direction() + 32) / 64) % 2;
            ComboBox_SetCurSel(hInspectComboOrient_, trainSel);
        } else {
            populateOrientVehicle(hInspectComboOrient_);
            int oriIdx = ((pVeh->direction() + 32) / 64) % 4;
            ComboBox_SetCurSel(hInspectComboOrient_, oriIdx);
        }

        ShowWindow(hInspectStaticDriver_, !isTrain ? SW_SHOW : SW_HIDE);
        ShowWindow(hInspectComboDriver_, !isTrain ? SW_SHOW : SW_HIDE);
        ShowWindow(hInspectBtnPlaytestConfig_, SW_HIDE);
        ShowWindow(hInspectBtnPlaytestConfig_, SW_HIDE);
        ShowWindow(hInspectStaticCyberware_, SW_HIDE);
        ShowWindow(hInspectComboCyberware_, SW_HIDE);
        ShowWindow(hInspectStaticMoveMode_, SW_HIDE);
        ShowWindow(hInspectComboMoveMode_, SW_HIDE);
        ShowWindow(hInspectStaticFaction_, SW_HIDE);
        ShowWindow(hInspectComboFaction_, SW_HIDE);
        ShowWindow(hInspectBtnAiTactics_, SW_HIDE);
        origCyberwareOverridden_ = false;
        origCyberwareTier_ = 0;
        origMovementMode_ = 0x10;
        activeMovementMode_ = 0x10;
        origFactionId_ = -1;
        activeFactionId_ = -1;

        populateDriverCombo(pVeh);

        if (pVeh && pVeh->getDriver()) {
            EnableWindow(hInspectWeaponsGrid_, TRUE);
            SetWindowTextW(hInspectStaticWeapons_, L"Driver Weapons (Equipped):");
            selectedWeapons_.clear();
            auto *pDriver = pVeh->getDriver();
            bool hasAR = isAmericanRevoltAvailable();
            int maxWeps = hasAR ? static_cast<int>(sizeof(kWeaponsList)/sizeof(kWeaponsList[0])) : 14;
            for (int i = 0; i < maxWeps; ++i) {
                bool hasW = pDriver->hasWeapon(kWeaponsList[i].type);
                if (hasW) {
                    selectedWeapons_.insert(i);
                    origWeapons_.push_back(i);
                }
            }
            refreshWeaponsGrid();
            ShowWindow(hInspectStaticWeapons_, (currentTab_ == 1) ? SW_SHOW : SW_HIDE);
            ShowWindow(hInspectWeaponsGrid_, (currentTab_ == 1) ? SW_SHOW : SW_HIDE);
        } else {
            selectedWeapons_.clear();
            refreshWeaponsGrid();
            ShowWindow(hInspectStaticWeapons_, SW_HIDE);
            ShowWindow(hInspectWeaponsGrid_, SW_HIDE);
        }

        origHealth_ = pVeh->health();
        origDir_ = static_cast<uint8_t>(pVeh->direction());
        origWeapons_.clear();
        origDriver_ = pVeh ? pVeh->getDriver() : nullptr;
        origDriverChoice_ = ComboBox_GetCurSel(hInspectComboDriver_);
    } else if (auto *pStatic = dynamic_cast<rs_knl::Static*>(pSelectedEntity_)) {
        int bestIdx = findBestMatchingStaticIndex(pStatic);
        std::wstring typeStr = std::wstring(s_staticItems[bestIdx].name.begin(), s_staticItems[bestIdx].name.end());

        std::wstring hdr = std::format(L"Selected {} #{:03}", typeStr, pStatic->id());
        SetWindowTextW(hInspectHeader_, hdr.c_str());

        SetWindowTextW(hInspectStaticType_, L"Type:");
        SetWindowTextW(hInspectValueType_, typeStr.c_str());

        ShowWindow(hInspectCheckDead_, (currentTab_ == 1 && !isEntitySearchActive_) ? SW_SHOW : SW_HIDE);
        SetWindowTextW(hInspectCheckDead_, L"Destroyed");
        bool isDestroyed = (pStatic->health() <= 0);
        SendMessageW(hInspectCheckDead_, BM_SETCHECK, isDestroyed ? BST_CHECKED : BST_UNCHECKED, 0);
        EnableWindow(hInspectEditHealth_, isDestroyed ? FALSE : TRUE);
        SetWindowTextW(hInspectEditHealth_, isDestroyed ? L"-1" : std::to_wstring(pStatic->health()).c_str());

        if (isFixedDirectionStatic(pStatic)) {
            ComboBox_ResetContent(hInspectComboOrient_);
            ComboBox_AddString(hInspectComboOrient_, L"Fixed Direction");
            ComboBox_SetCurSel(hInspectComboOrient_, 0);
            EnableWindow(hInspectComboOrient_, FALSE);
        } else {
            EnableWindow(hInspectComboOrient_, TRUE);
            populateOrientStatic(hInspectComboOrient_);
            int curOri = pStatic->orientation();
            ComboBox_SetCurSel(hInspectComboOrient_, (curOri == rs_knl::Static::kStaticOrientationEW) ? 1 : 0);
        }

        ShowWindow(hInspectStaticDriver_, SW_HIDE);
        ShowWindow(hInspectComboDriver_, SW_HIDE);
        ShowWindow(hInspectStaticCyberware_, SW_HIDE);
        ShowWindow(hInspectComboCyberware_, SW_HIDE);
        ShowWindow(hInspectStaticMoveMode_, SW_HIDE);
        ShowWindow(hInspectComboMoveMode_, SW_HIDE);
        ShowWindow(hInspectStaticFaction_, SW_HIDE);
        ShowWindow(hInspectComboFaction_, SW_HIDE);
        ShowWindow(hInspectBtnAiTactics_, SW_HIDE);
        origCyberwareOverridden_ = false;
        origCyberwareTier_ = 0;
        origMovementMode_ = 0x10;
        activeMovementMode_ = 0x10;
        origFactionId_ = -1;
        activeFactionId_ = -1;

        selectedWeapons_.clear();
        refreshWeaponsGrid();
        ShowWindow(hInspectStaticWeapons_, SW_HIDE);
        ShowWindow(hInspectWeaponsGrid_, SW_HIDE);

        origHealth_ = pStatic->health();
        origDir_ = static_cast<uint8_t>(pStatic->orientation());
        origWeapons_.clear();
    } else if (auto *pWeapon = dynamic_cast<rs_knl::WeaponInstance*>(pSelectedEntity_)) {
        int bestIdx = findBestMatchingWeaponIndex(pWeapon);
        std::wstring wTypeName = std::wstring(s_weaponItems[bestIdx].name.begin(), s_weaponItems[bestIdx].name.end());

        std::wstring hdr = std::format(L"Selected {} #{:03}", wTypeName, pWeapon->id());
        SetWindowTextW(hInspectHeader_, hdr.c_str());

        SetWindowTextW(hInspectStaticType_, L"Type:");
        SetWindowTextW(hInspectValueType_, wTypeName.c_str());

        ShowWindow(hInspectCheckDead_, SW_HIDE);
        ShowWindow(hInspectStaticHealth_, SW_HIDE);
        ShowWindow(hInspectEditHealth_, SW_HIDE);

        ComboBox_ResetContent(hInspectComboOrient_);
        ComboBox_AddString(hInspectComboOrient_, L"Fixed Direction");
        ComboBox_SetCurSel(hInspectComboOrient_, 0);
        EnableWindow(hInspectComboOrient_, FALSE);

        ShowWindow(hInspectStaticDriver_, SW_HIDE);
        ShowWindow(hInspectComboDriver_, SW_HIDE);
        ShowWindow(hInspectStaticCyberware_, SW_HIDE);
        ShowWindow(hInspectComboCyberware_, SW_HIDE);
        ShowWindow(hInspectStaticMoveMode_, SW_HIDE);
        ShowWindow(hInspectComboMoveMode_, SW_HIDE);
        ShowWindow(hInspectStaticFaction_, SW_HIDE);
        ShowWindow(hInspectComboFaction_, SW_HIDE);
        ShowWindow(hInspectBtnAiTactics_, SW_HIDE);
        ShowWindow(hInspectBtnPlaytestConfig_, SW_HIDE);
        origCyberwareOverridden_ = false;
        origCyberwareTier_ = 0;
        origMovementMode_ = 0x10;
        activeMovementMode_ = 0x10;
        origFactionId_ = -1;
        activeFactionId_ = -1;

        selectedWeapons_.clear();
        refreshWeaponsGrid();
        ShowWindow(hInspectStaticWeapons_, SW_HIDE);
        ShowWindow(hInspectWeaponsGrid_, SW_HIDE);

        origHealth_ = 1;
        origDir_ = 0;
        origWeapons_.clear();
    }

    // Coordinates and Location for all selected entities
    std::wstring posStr = std::format(L"TX: {:02}, TY: {:02}, TZ: {:02}",
        pSelectedEntity_->tileX(), pSelectedEntity_->tileY(), pSelectedEntity_->tileZ());
    SetWindowTextW(hInspectValuePos_, posStr.c_str());
    ShowWindow(hInspectStaticPos_, (currentTab_ == 1) ? SW_SHOW : SW_HIDE);
    ShowWindow(hInspectValuePos_, (currentTab_ == 1) ? SW_SHOW : SW_HIDE);

    std::wstring locStr = isObscured ? std::format(L"Indoors / Under Roof Layer: {}", roofZ) : L"Outdoors";
    SetWindowTextW(hInspectValueLoc_, locStr.c_str());
    ShowWindow(hInspectStaticLoc_, (currentTab_ == 1) ? SW_SHOW : SW_HIDE);
    ShowWindow(hInspectValueLoc_, (currentTab_ == 1) ? SW_SHOW : SW_HIDE);

    // Populate route status and snapshot original route state
    rs_knl::PedInstance *pRoutePed = nullptr;
    if (auto *pP = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
        if (pP->type() != rs_knl::PedInstance::kPedTypeAgent) {
            pRoutePed = pP;
        }
    } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) {
        pRoutePed = pVeh->getDriver();
    }

    if (pRoutePed) {
        origSteps_ = pRoutePed->scriptSteps();
        origLoop_ = pRoutePed->isScriptLooping();
    } else {
        origSteps_.clear();
        origLoop_ = false;
    }

    refreshRouteUI();

    if (currentTab_ == 1 && !isEntitySearchActive_) {
        layoutEntInspectPane();
        if (hEntInspectPane_) RedrawWindow(hEntInspectPane_, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE);
        if (hInspectBtnApply_) ShowWindow(hInspectBtnApply_, SW_SHOW);
        if (hInspectBtnCancel_) ShowWindow(hInspectBtnCancel_, SW_SHOW);
        if (hInspectBtnDelete_) ShowWindow(hInspectBtnDelete_, SW_SHOW);
    }

    markEntityClean();
    updateEntityPreview();
    isPopulatingInspector_ = false;
    if (hWnd_) InvalidateRect(hWnd_, nullptr, TRUE);
}

static bool areScriptStepsEqual(const std::vector<rs_knl::ScriptStep> &a, const std::vector<rs_knl::ScriptStep> &b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].type != b[i].type ||
            a[i].param != b[i].param ||
            a[i].targetEntityId != b[i].targetEntityId ||
            a[i].targetPos.tx != b[i].targetPos.tx ||
            a[i].targetPos.ty != b[i].targetPos.ty ||
            a[i].targetPos.tz != b[i].targetPos.tz) {
            return false;
        }
    }
    return true;
}

void WinArtPanel::applyInspectorChanges() {
    if (!pSelectedEntity_) return;

    ModifyEntityAttributesCommand::Attrs oldAttrs;
    ModifyEntityAttributesCommand::Attrs newAttrs;

    oldAttrs.health = origHealth_;
    oldAttrs.direction = origDir_;
    oldAttrs.driverChoice = origDriverChoice_;
    oldAttrs.pDriver = origDriver_;
    oldAttrs.cyberwareOverridden = origCyberwareOverridden_;
    oldAttrs.cyberwareTier = origCyberwareTier_;
    oldAttrs.movementMode = origMovementMode_;
    oldAttrs.factionId = origFactionId_;
    for (int idx : origWeapons_) {
        if (idx >= 0 && idx < static_cast<int>(sizeof(kWeaponsList)/sizeof(kWeaponsList[0]))) {
            oldAttrs.weapons.push_back(kWeaponsList[idx].type);
        }
    }

    if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
        if (pPed->isOurAgent()) {
            int ori = ComboBox_GetCurSel(hInspectComboOrient_);
            newAttrs.health = pPed->health();
            newAttrs.direction = static_cast<uint8_t>((ori >= 0) ? ori * 32 : origDir_);
            newAttrs.weapons = oldAttrs.weapons;
        } else {
            bool isDead = (hInspectCheckDead_ && SendMessageW(hInspectCheckDead_, BM_GETCHECK, 0, 0) == BST_CHECKED);
            if (isDead) {
                newAttrs.health = -1;
            } else {
                wchar_t buf[64] = {};
                GetWindowTextW(hInspectEditHealth_, buf, 64);
                newAttrs.health = _wtoi(buf);
            }

            int ori = ComboBox_GetCurSel(hInspectComboOrient_);
            newAttrs.direction = static_cast<uint8_t>((ori >= 0) ? ori * 32 : origDir_);

            int moveSel = ComboBox_GetCurSel(hInspectComboMoveMode_);
            newAttrs.movementMode = (moveSel == 1) ? 0x00 : 0x10;

            for (int i = 0; i < static_cast<int>(sizeof(kWeaponsList)/sizeof(kWeaponsList[0])); ++i) {
                if (selectedWeapons_.count(i) > 0) {
                    newAttrs.weapons.push_back(kWeaponsList[i].type);
                }
            }

            if (pPed->type() == rs_knl::PedInstance::kPedTypeAgent) {
                int sel = ComboBox_GetCurSel(hInspectComboCyberware_);
                if (sel > 0) {
                    newAttrs.cyberwareOverridden = true;
                    newAttrs.cyberwareTier = sel - 1;
                } else {
                    newAttrs.cyberwareOverridden = false;
                    newAttrs.cyberwareTier = pMission_ ? pMission_->techLevel() : 0;
                }
                int fSel = ComboBox_GetCurSel(hInspectComboFaction_);
                newAttrs.factionId = (fSel >= 0) ? (fSel + 1) : origFactionId_;
            }
        }
    } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) {
        bool isDestroyed = (hInspectCheckDead_ && SendMessageW(hInspectCheckDead_, BM_GETCHECK, 0, 0) == BST_CHECKED);
        if (isDestroyed) {
            newAttrs.health = -1;
        } else {
            wchar_t buf[64] = {};
            GetWindowTextW(hInspectEditHealth_, buf, 64);
            newAttrs.health = _wtoi(buf);
        }

        int ori = ComboBox_GetCurSel(hInspectComboOrient_);
        newAttrs.direction = static_cast<uint8_t>((ori >= 0) ? ori * 64 : origDir_);

        if (pVeh) {
            int sel = ComboBox_GetCurSel(hInspectComboDriver_);
            newAttrs.driverChoice = sel;
            if (sel >= 2) {
                auto *pChosenPed = reinterpret_cast<rs_knl::PedInstance*>(ComboBox_GetItemData(hInspectComboDriver_, sel));
                if (pChosenPed && pChosenPed != reinterpret_cast<rs_knl::PedInstance*>(CB_ERR)) {
                    newAttrs.pDriver = pChosenPed;
                }
            }
        }
    } else if (auto *pStatic = dynamic_cast<rs_knl::Static*>(pSelectedEntity_)) {
        bool isDestroyed = (hInspectCheckDead_ && SendMessageW(hInspectCheckDead_, BM_GETCHECK, 0, 0) == BST_CHECKED);
        if (isDestroyed) {
            newAttrs.health = -1;
        } else {
            wchar_t buf[64] = {};
            GetWindowTextW(hInspectEditHealth_, buf, 64);
            newAttrs.health = _wtoi(buf);
        }

        if (!isFixedDirectionStatic(pStatic)) {
            int ori = ComboBox_GetCurSel(hInspectComboOrient_);
            newAttrs.direction = static_cast<uint8_t>((ori == 1) ? rs_knl::Static::kStaticOrientationEW : rs_knl::Static::kStaticOrientationNS);
        } else {
            newAttrs.direction = origDir_;
        }
    }

    bool attrsChanged = (oldAttrs.health != newAttrs.health ||
                         oldAttrs.direction != newAttrs.direction ||
                         oldAttrs.driverChoice != newAttrs.driverChoice ||
                         oldAttrs.pDriver != newAttrs.pDriver ||
                         oldAttrs.weapons != newAttrs.weapons ||
                         oldAttrs.cyberwareOverridden != newAttrs.cyberwareOverridden ||
                         oldAttrs.cyberwareTier != newAttrs.cyberwareTier ||
                         oldAttrs.movementMode != newAttrs.movementMode ||
                         oldAttrs.factionId != newAttrs.factionId);

    if (attrsChanged) {
        if (pViewport_ && pViewport_->commandStack()) {
            pViewport_->commandStack()->executeCommand(
                std::make_unique<ModifyEntityAttributesCommand>(pMission_, pSelectedEntity_, oldAttrs, newAttrs));
            pViewport_->notifyCommandExecuted();
        } else {
            ModifyEntityAttributesCommand cmd(pMission_, pSelectedEntity_, oldAttrs, newAttrs);
            cmd.execute();
        }
    }

    origHealth_ = newAttrs.health;
    origDir_ = newAttrs.direction;
    origCyberwareOverridden_ = newAttrs.cyberwareOverridden;
    origCyberwareTier_ = newAttrs.cyberwareTier;
    origMovementMode_ = newAttrs.movementMode;
    origFactionId_ = newAttrs.factionId;
    origWeapons_.clear();
    for (int i = 0; i < static_cast<int>(sizeof(kWeaponsList)/sizeof(kWeaponsList[0])); ++i) {
        if (selectedWeapons_.count(i) > 0) origWeapons_.push_back(i);
    }
    if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) {
        origDriver_ = pVeh->getDriver();
        origDriverChoice_ = newAttrs.driverChoice;
        populateDriverCombo(pVeh);
    }

    // Route updates staged on pRoutePed
    rs_knl::PedInstance *pRoutePed = nullptr;
    if (auto *pP = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
        if (pP->type() != rs_knl::PedInstance::kPedTypeAgent) {
            pRoutePed = pP;
        }
    } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) {
        pRoutePed = pVeh->getDriver();
    }
    bool routeStepsChanged = (pRoutePed && !areScriptStepsEqual(pRoutePed->scriptSteps(), origSteps_));
    bool routeLoopChanged = (pRoutePed && pRoutePed->isScriptLooping() != origLoop_);
    if (pRoutePed && (routeStepsChanged || routeLoopChanged)) {
        if (pViewport_ && pViewport_->commandStack()) {
            pViewport_->commandStack()->executeCommand(
                std::make_unique<ModifyPedRouteCommand>(pMission_, pRoutePed, origSteps_, pRoutePed->scriptSteps(), origLoop_, pRoutePed->isScriptLooping()));
            pViewport_->notifyCommandExecuted();
        } else {
            rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pRoutePed);
        }
        origSteps_ = pRoutePed->scriptSteps();
        origLoop_ = pRoutePed->isScriptLooping();
    }

    markEntityClean();
    refreshObjectivesList();
    if (pViewport_) InvalidateRect(pViewport_->hwnd(), nullptr, FALSE);
}

void WinArtPanel::revertInspectorChanges() {
    if (!pSelectedEntity_) return;

    isPopulatingInspector_ = true;

    if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
        if (pPed->isOurAgent()) {
            pPed->setDirection(origDir_);
            if (pMission_) {
                size_t targetSlot = (pPed->id() < 4) ? pPed->id() : 0;
                pMission_->setSquadSpawnPoint(targetSlot, pPed->position(), origDir_);
            }
            int oriIdx = ((origDir_ + 16) / 32) % 8;
            ComboBox_SetCurSel(hInspectComboOrient_, oriIdx);
        } else {
            bool wasDead = (origHealth_ <= 0);
            pPed->setHealth(origHealth_);
            SetWindowTextW(hInspectEditHealth_, wasDead ? L"-1" : std::to_wstring(origHealth_).c_str());
            EnableWindow(hInspectEditHealth_, wasDead ? FALSE : TRUE);
            if (hInspectCheckDead_) {
                SendMessageW(hInspectCheckDead_, BM_SETCHECK, wasDead ? BST_CHECKED : BST_UNCHECKED, 0);
            }
            if (wasDead) {
                pPed->playDeadAnimation();
            } else {
                pPed->playStandOrWalkAnimation();
            }

            pPed->setDirection(origDir_);
            int oriIdx = ((origDir_ + 16) / 32) % 8;
            ComboBox_SetCurSel(hInspectComboOrient_, oriIdx);

            while (pPed->numWeapons() > 0) {
                auto *w = pPed->removeWeaponAtIndex(0);
                delete w;
            }
            for (int wIdx : origWeapons_) {
                if (wIdx >= 0 && wIdx < static_cast<int>(sizeof(kWeaponsList)/sizeof(kWeaponsList[0]))) {
                    auto *pW = g_weaponMgr.getWeapon(kWeaponsList[wIdx].type);
                    if (pW) {
                        auto *inst = rs_knl::WeaponInstance::createInstance(pW);
                        if (inst) {
                            inst->setOwner(pPed);
                            pPed->addWeapon(inst);
                        }
                    }
                }
            }
            selectedWeapons_.clear();
            for (int wIdx : origWeapons_) {
                selectedWeapons_.insert(wIdx);
            }
            refreshWeaponsGrid();

            pPed->setInitialLevelState(origMovementMode_);
            if (hInspectComboMoveMode_) {
                ComboBox_SetCurSel(hInspectComboMoveMode_, (origMovementMode_ & LevelData::kPeopleStateWalking) ? 0 : 1);
            }

            if (pPed->type() == rs_knl::PedInstance::kPedTypeAgent && !pPed->isOurAgent()) {
                pPed->setCyberwareOverridden(origCyberwareOverridden_);
                if (origCyberwareOverridden_) {
                    pPed->applyCyberwareTier(origCyberwareTier_);
                } else if (pMission_) {
                    pPed->applyCyberwareTier(pMission_->techLevel());
                }
                populateCyberwareCombo(hInspectComboCyberware_, origCyberwareOverridden_, origCyberwareTier_);

                pPed->setFactionId(origFactionId_);
                if (hInspectComboFaction_) {
                    int fSel = (origFactionId_ >= 1 && origFactionId_ <= 7) ? (origFactionId_ - 1) : 0;
                    ComboBox_SetCurSel(hInspectComboFaction_, fSel);
                }
            }
        }
    } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) {
        bool wasDestroyed = (origHealth_ <= 0);
        pVeh->setHealth(origHealth_);
        SetWindowTextW(hInspectEditHealth_, wasDestroyed ? L"-1" : std::to_wstring(origHealth_).c_str());
        EnableWindow(hInspectEditHealth_, wasDestroyed ? FALSE : TRUE);
        if (hInspectCheckDead_) {
            SendMessageW(hInspectCheckDead_, BM_SETCHECK, wasDestroyed ? BST_CHECKED : BST_UNCHECKED, 0);
        }
        if (wasDestroyed) {
            pVeh->playAnimation(pVeh->burntAnimation());
        } else {
            pVeh->playAnimation(pVeh->regularAnimation());
        }

        pVeh->setDirection(origDir_);
        bool isTrain = (dynamic_cast<rs_knl::TrainHead*>(pVeh) != nullptr || dynamic_cast<rs_knl::TrainBody*>(pVeh) != nullptr);
        int oriIdx = isTrain ? (((origDir_ + 32) / 64) % 2) : (((origDir_ + 32) / 64) % 4);
        ComboBox_SetCurSel(hInspectComboOrient_, oriIdx);

        if (pVeh) {
            pVeh->setDriver(origDriver_, true);
            ComboBox_SetCurSel(hInspectComboDriver_, origDriverChoice_);
        }
    } else if (auto *pStatic = dynamic_cast<rs_knl::Static*>(pSelectedEntity_)) {
        bool wasDestroyed = (origHealth_ <= 0);
        pStatic->setHealth(origHealth_);
        SetWindowTextW(hInspectEditHealth_, wasDestroyed ? L"-1" : std::to_wstring(origHealth_).c_str());
        EnableWindow(hInspectEditHealth_, wasDestroyed ? FALSE : TRUE);
        if (hInspectCheckDead_) {
            SendMessageW(hInspectCheckDead_, BM_SETCHECK, wasDestroyed ? BST_CHECKED : BST_UNCHECKED, 0);
        }

        if (isFixedDirectionStatic(pStatic)) {
            ComboBox_SetCurSel(hInspectComboOrient_, 0);
        } else {
            pStatic->setOrientation(origDir_ == rs_knl::Static::kStaticOrientationEW ? rs_knl::Static::kStaticOrientationEW : rs_knl::Static::kStaticOrientationNS);
            ComboBox_SetCurSel(hInspectComboOrient_, (origDir_ == rs_knl::Static::kStaticOrientationEW) ? 1 : 0);
        }
    }

    // Revert Route
    rs_knl::PedInstance *pRoutePed = nullptr;
    if (auto *pP = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
        if (pP->type() != rs_knl::PedInstance::kPedTypeAgent) {
            pRoutePed = pP;
        }
    } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) {
        pRoutePed = pVeh->getDriver();
    }
    if (pRoutePed) {
        pRoutePed->setScriptSteps(origSteps_);
        pRoutePed->setScriptLooping(origLoop_);
        rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pRoutePed);
        refreshRouteUI();
    }

    markEntityClean();
    isPopulatingInspector_ = false;
    if (pViewport_) InvalidateRect(pViewport_->hwnd(), nullptr, FALSE);
}

bool WinArtPanel::confirmDiscardEntityEdits() {
    if (!isEntityDirty_ || !pSelectedEntity_) return true;

    int res = showEditorMessageBox(hWnd_,
        L"You have unsaved changes to the selected entity.\r\n\r\n"
        L"Click Yes to Apply changes, No to Discard changes, or Cancel to continue editing.",
        L"Unsaved Entity Changes",
        MB_YESNOCANCEL | MB_ICONQUESTION);

    if (res == IDYES) {
        applyInspectorChanges();
        return true;
    } else if (res == IDNO) {
        revertInspectorChanges();
        return true;
    }
    return false;
}

void WinArtPanel::markEntityDirty() {
    if (!pSelectedEntity_) return;
    isEntityDirty_ = true;
    if (hInspectBtnApply_) {
        EnableWindow(hInspectBtnApply_, TRUE);
    }
    if (hInspectBtnCancel_) {
        EnableWindow(hInspectBtnCancel_, TRUE);
    }
}

void WinArtPanel::markEntityClean() {
    isEntityDirty_ = false;
    if (hInspectBtnApply_) {
        EnableWindow(hInspectBtnApply_, FALSE);
    }
    if (hInspectBtnCancel_) {
        EnableWindow(hInspectBtnCancel_, FALSE);
    }
}

void WinArtPanel::refreshPalette() {
    int oldTab = currentTab_;
    categoryTiles_ = TileRasterizer::instance().getTilesForCategory(currentCategory_);
    updateTileGridScroll();
    refreshEntityThumbnails();
    refreshWeaponIcons();
    if (!isAmericanRevoltAvailable()) {
        auto it = selectedWeapons_.begin();
        while (it != selectedWeapons_.end()) {
            if (*it >= 14) it = selectedWeapons_.erase(it);
            else ++it;
        }
        if (pViewport_ && pViewport_->tool() == ViewportTool::PlaceWeapon) {
            if (rs_knl::Weapon::isAmericanRevoltOnly(pViewport_->weaponTypeToPlace())) {
                pViewport_->setWeaponPlacement(rs_knl::Weapon::Pistol);
            }
        }
    }
    updateEntityTypeComboLabels();
    const auto &curItems = getEntityItemsForCategory(entityCategory_);
    if (selectedEntityGridIdx_ >= static_cast<int>(curItems.size())) {
        selectedEntityGridIdx_ = curItems.empty() ? -1 : 0;
    }
    updateEntityGridScroll();
    InvalidateRect(hTileGrid_, nullptr, TRUE);
    if (hEntityGrid_) InvalidateRect(hEntityGrid_, nullptr, TRUE);
    if (hInspectWeaponsGrid_) InvalidateRect(hInspectWeaponsGrid_, nullptr, TRUE);
    setActiveTileId(activeTileId_);
    if (oldTab != 0) {
        setActiveTab(oldTab);
    }
}

void WinArtPanel::setActiveTileId(uint8_t id) {
    if (currentTab_ != 0) {
        setActiveTab(0);
    }
    activeTileId_ = id;
    if (pViewport_) pViewport_->setActiveTileId(id);

    // If activeTileId_ is not in current category, switch to the category that contains it
    auto it = std::find(categoryTiles_.begin(), categoryTiles_.end(), id);
    if (it == categoryTiles_.end()) {
        TileCategory targetCat = TileRasterizer::instance().getTileCategory(id);
        currentCategory_ = targetCat;
        if (hComboCategory_) {
            ComboBox_SetCurSel(hComboCategory_, static_cast<int>(currentCategory_));
        }
        categoryTiles_ = TileRasterizer::instance().getTilesForCategory(currentCategory_);
        updateTileGridScroll();
        it = std::find(categoryTiles_.begin(), categoryTiles_.end(), id);
    }

    // Scroll tile grid into view if necessary
    if (it != categoryTiles_.end() && hTileGrid_) {
        int idx = static_cast<int>(std::distance(categoryTiles_.begin(), it));
        int cols = getTileGridColumns();
        int row = idx / cols;
        int cellH = 54;
        int tileTop = row * cellH;
        int tileBottom = tileTop + cellH + 8;
        RECT rc = {};
        GetClientRect(hTileGrid_, &rc);
        int viewH = rc.bottom;
        if (viewH > 0) {
            if (tileTop < gridScrollY_) {
                gridScrollY_ = tileTop;
            } else if (tileBottom > gridScrollY_ + viewH) {
                gridScrollY_ = tileBottom - viewH;
            }
            SetScrollPos(hTileGrid_, SB_VERT, gridScrollY_, TRUE);
        }
    }

    refreshTileInspector();
    InvalidateRect(hTileGrid_, nullptr, TRUE);
}

void WinArtPanel::refreshTileInspector() {
    if (!hTileInspectValueType_) return;
    const WinTile &t = TileRasterizer::instance().getTile(activeTileId_);
    std::string typeStr = TileRasterizer::instance().getTileTypeName(t.type);
    bool walkable = TileRasterizer::instance().isTileWalkableSurface(activeTileId_);

    int usageCount = 0;
    if (pMission_ && pMission_->map()) {
        rs_knl::Map *m = pMission_->map();
        for (int z = 0; z < m->maxTz(); ++z) {
            for (int y = 0; y < m->maxTy(); ++y) {
                for (int x = 0; x < m->maxTx(); ++x) {
                    if (m->getTileIdAt(x, y, z) == activeTileId_) {
                        usageCount++;
                    }
                }
            }
        }
    }

    std::wstring surfaceDesc;
    if (activeTileId_ == 2) {
        surfaceDesc = L"Walkable (Invisible Ground Collider)";
    } else if (t.type == 0x0D) {
        surfaceDesc = L"Walkable (Rooftop)";
    } else if (t.type == 0x04 || t.type == 0x07) {
        surfaceDesc = L"Walkable (Stairs / Slope)";
    } else if (walkable) {
        surfaceDesc = L"Walkable";
    } else {
        surfaceDesc = L"Non-Walkable";
    }

    bool isSelectedOnMap = (pViewport_ && pViewport_->hasSelectedTile());
    if (isSelectedOnMap) {
        auto selPt = pViewport_->selectedTile();
        std::wstring posStr = std::format(L"TX: {:02}, TY: {:02}, TZ: {:02}", selPt.tx, selPt.ty, selPt.tz);
        SetWindowTextW(hTileInspectValuePos_, posStr.c_str());
        ShowWindow(hTileInspectStaticPos_, (currentTab_ == 0 && !isTileSearchActive_) ? SW_SHOW : SW_HIDE);
        ShowWindow(hTileInspectValuePos_, (currentTab_ == 0 && !isTileSearchActive_) ? SW_SHOW : SW_HIDE);
    } else {
        ShowWindow(hTileInspectStaticPos_, SW_HIDE);
        ShowWindow(hTileInspectValuePos_, SW_HIDE);
    }

    std::wstring typeDisplay = std::format(L"{} (#{:03})", std::wstring(typeStr.begin(), typeStr.end()), activeTileId_);
    SetWindowTextW(hTileInspectValueType_, typeDisplay.c_str());
    SetWindowTextW(hTileInspectValueSurface_, surfaceDesc.c_str());

    std::wstring placementsStr = std::format(L"{} instances on map", usageCount);
    SetWindowTextW(hTileInspectValuePlacements_, placementsStr.c_str());

    ShowWindow(hTilePreviewBox_, (currentTab_ == 0 && !isTileSearchActive_) ? SW_SHOW : SW_HIDE);
    ShowWindow(hTileInspectStaticType_, (currentTab_ == 0 && !isTileSearchActive_) ? SW_SHOW : SW_HIDE);
    ShowWindow(hTileInspectValueType_, (currentTab_ == 0 && !isTileSearchActive_) ? SW_SHOW : SW_HIDE);
    ShowWindow(hTileInspectStaticSurface_, (currentTab_ == 0 && !isTileSearchActive_) ? SW_SHOW : SW_HIDE);
    ShowWindow(hTileInspectValueSurface_, (currentTab_ == 0 && !isTileSearchActive_) ? SW_SHOW : SW_HIDE);
    ShowWindow(hTileInspectStaticPlacements_, (currentTab_ == 0 && !isTileSearchActive_) ? SW_SHOW : SW_HIDE);
    ShowWindow(hTileInspectValuePlacements_, (currentTab_ == 0 && !isTileSearchActive_) ? SW_SHOW : SW_HIDE);

    if (currentTab_ == 0 && !isTileSearchActive_) {
        layoutTileInspectPane();
        if (hTilePreviewBox_) InvalidateRect(hTilePreviewBox_, nullptr, FALSE);
        if (hTileInspectPane_) InvalidateRect(hTileInspectPane_, nullptr, TRUE);
    }
}

void WinArtPanel::performTileSearch(uint8_t tileId) {
    tileSearchResults_.clear();
    tileSearchScrollY_ = 0;
    tileSearchHoverRow_ = -1;
    tileSearchHoverTarget_ = false;

    if (pMission_ && pMission_->map()) {
        rs_knl::Map *pMap = pMission_->map();
        const WinTile &t = TileRasterizer::instance().getTile(tileId);
        std::string typeStr = TileRasterizer::instance().getTileTypeName(t.type);
        std::wstring typeNameW(typeStr.begin(), typeStr.end());

        for (int z = 0; z < pMap->maxTz(); ++z) {
            for (int y = 0; y < pMap->maxTy(); ++y) {
                for (int x = 0; x < pMap->maxTx(); ++x) {
                    if (pMap->getTileIdAt(x, y, z) == tileId) {
                        SearchRow row;
                        row.idStr = std::format(L"#{:03}", tileId);
                        row.typeStr = typeNameW;
                        row.coordStr = std::format(L"TX:{:02} TY:{:02} TZ:{:02}", x, y, z);
                        row.pos = rs_knl::TilePoint(x, y, z);
                        row.pEntity = nullptr;
                        tileSearchResults_.push_back(row);
                    }
                }
            }
        }
        tileSearchTitle_ = std::format(L"Search Results: Tile #{:03} ({} found)", tileId, tileSearchResults_.size());
    } else {
        tileSearchTitle_ = L"Search Results (No Mission)";
    }

    if (hTileSearchResults_) {
        RECT rc = {};
        GetClientRect(hTileSearchResults_, &rc);
        SCROLLINFO si = {};
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin = 0;
        si.nMax = static_cast<int>(tileSearchResults_.size() * 24);
        si.nPage = std::max(1, static_cast<int>(rc.bottom - 48));
        si.nPos = 0;
        SetScrollInfo(hTileSearchResults_, SB_VERT, &si, TRUE);
        InvalidateRect(hTileSearchResults_, nullptr, TRUE);
    }
}

static void getAgentFactionDisplayInfo(int factionId, std::wstring &labelOut, COLORREF &colorOut) {
    switch (factionId) {
        case 0: labelOut = L"F0 (Player)";   colorOut = RGB(245, 100, 100); break;
        case 1: labelOut = L"F1 (Blue)";     colorOut = RGB(110, 165, 255); break;
        case 2: labelOut = L"F2 (Orange)";   colorOut = RGB(250, 150, 65);  break;
        case 3: labelOut = L"F3 (Yellow A)"; colorOut = RGB(245, 220, 80);  break;
        case 4: labelOut = L"F4 (Yellow B)"; colorOut = RGB(230, 195, 65);  break;
        case 5: labelOut = L"F5 (Purple)";   colorOut = RGB(205, 125, 240); break;
        case 6: labelOut = L"F6 (Grey)";     colorOut = RGB(180, 185, 195); break;
        case 7: labelOut = L"F7 (Cyan)";     colorOut = RGB(70, 225, 225);  break;
        default:
            labelOut = std::format(L"Faction {}", factionId);
            colorOut = RGB(215, 222, 230);
            break;
    }
}

void WinArtPanel::performEntitySearch(int templateIdx) {
    entSearchResults_.clear();
    entSearchScrollY_ = 0;
    entSearchHoverRow_ = -1;
    entSearchHoverTarget_ = false;

    std::wstring templName = L"Entity";

    if (pMission_) {
        const auto &items = getEntityItemsForCategory(entityCategory_);
        if (templateIdx >= 0 && templateIdx < static_cast<int>(items.size())) {
            const auto &item = *items[templateIdx];
            templName = item.shortLabel;

            if (item.kind == EntityKind::Actor) {
                for (size_t i = 0; i < pMission_->numPeds(); ++i) {
                    auto *pPed = pMission_->ped(i);
                    if (!pPed) continue;
                    bool match = false;
                    if (pPed->type() == item.typeId) {
                        if (item.typeId == rs_knl::PedInstance::kPedTypeCivilian ||
                            item.typeId == rs_knl::PedInstance::kPedTypeCriminal) {
                            match = (pPed->baseAnim() == item.baseAnim);
                        } else {
                            match = true;
                        }
                    }
                    if (match) {
                        SearchRow row;
                        row.idStr = std::format(L"Ped #{:03}", pPed->id());
                        if (pPed->type() == rs_knl::PedInstance::kPedTypeAgent) {
                            int facId = pPed->isOurAgent() ? 0 : pPed->factionId();
                            getAgentFactionDisplayInfo(facId, row.typeStr, row.typeColor);
                            row.hasTypeColor = true;
                        } else {
                            row.typeStr = templName;
                        }
                        row.coordStr = std::format(L"TX:{:02} TY:{:02} TZ:{:02}", pPed->tileX(), pPed->tileY(), pPed->tileZ());
                        row.pos = pPed->position();
                        row.pEntity = pPed;
                        entSearchResults_.push_back(row);
                    }
                }
            } else if (item.kind == EntityKind::Vehicle) {
                uint8_t targetType = static_cast<uint8_t>(item.typeId);
                for (size_t i = 0; i < pMission_->numVehicles(); ++i) {
                    auto *pVeh = pMission_->vehicle(i);
                    if (!pVeh) continue;
                    bool match = false;
                    if (targetType == 0xFE) { // Train Group
                        match = (pVeh->getType() == rs_knl::Vehicle::kVehicleTypeTrainHead || pVeh->getType() == rs_knl::Vehicle::kVehicleTypeTrainBody);
                    } else {
                        match = (pVeh->getType() == targetType);
                    }
                    if (match) {
                        SearchRow row;
                        row.idStr = std::format(L"Veh #{:03}", pVeh->id());
                        row.typeStr = templName;
                        row.coordStr = std::format(L"TX:{:02} TY:{:02} TZ:{:02}", pVeh->tileX(), pVeh->tileY(), pVeh->tileZ());
                        row.pos = pVeh->position();
                        row.pEntity = pVeh;
                        entSearchResults_.push_back(row);
                    }
                }
            } else if (item.kind == EntityKind::Static) {
                for (size_t i = 0; i < pMission_->numStatics(); ++i) {
                    auto *pStatic = pMission_->statics(i);
                    if (!pStatic) continue;
                    int matchedIdx = findBestMatchingStaticIndex(pStatic);
                    if (matchedIdx >= 0 && static_cast<size_t>(matchedIdx) < s_staticItems.size() &&
                        &s_staticItems[matchedIdx] == &item) {
                        SearchRow row;
                        row.idStr = std::format(L"Obj #{:03}", pStatic->id());
                        row.typeStr = templName;
                        row.coordStr = std::format(L"TX:{:02} TY:{:02} TZ:{:02}", pStatic->tileX(), pStatic->tileY(), pStatic->tileZ());
                        row.pos = pStatic->position();
                        row.pEntity = pStatic;
                        entSearchResults_.push_back(row);
                    }
                }
            } else if (item.kind == EntityKind::Weapon) {
                rs_knl::Weapon::WeaponType targetType = static_cast<rs_knl::Weapon::WeaponType>(item.typeId);
                for (size_t i = 0; i < pMission_->weaponsOnGround().size(); ++i) {
                    auto *pWpn = pMission_->weaponsOnGround()[i];
                    if (!pWpn) continue;
                    if (pWpn->type() == targetType) {
                        SearchRow row;
                        row.idStr = std::format(L"Wpn #{:03}", pWpn->id());
                        row.typeStr = templName;
                        row.coordStr = std::format(L"TX:{:02} TY:{:02} TZ:{:02}", pWpn->tileX(), pWpn->tileY(), pWpn->tileZ());
                        row.pos = pWpn->position();
                        row.pEntity = pWpn;
                        entSearchResults_.push_back(row);
                    }
                }
            }
        }
        entSearchTitle_ = std::format(L"Search Results: {} ({} found)", templName, entSearchResults_.size());
    } else {
        entSearchTitle_ = L"Search Results (No Mission)";
    }

    if (hEntSearchResults_) {
        RECT rc = {};
        GetClientRect(hEntSearchResults_, &rc);
        SCROLLINFO si = {};
        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin = 0;
        si.nMax = static_cast<int>(entSearchResults_.size() * 24);
        si.nPage = std::max(1, static_cast<int>(rc.bottom - 48));
        si.nPos = 0;
        SetScrollInfo(hEntSearchResults_, SB_VERT, &si, TRUE);
        InvalidateRect(hEntSearchResults_, nullptr, TRUE);
    }
}

LRESULT CALLBACK WinArtPanel::SearchResultsProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        auto *pCS = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCS->lpCreateParams));
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    auto *pPanel = reinterpret_cast<WinArtPanel*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    if (pPanel) {
        return pPanel->handleSearchResultsMessage(hWnd, msg, wParam, lParam);
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT WinArtPanel::handleSearchResultsMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    bool isTile = (hWnd == hTileSearchResults_);
    auto &rows = isTile ? tileSearchResults_ : entSearchResults_;
    const auto &title = isTile ? tileSearchTitle_ : entSearchTitle_;
    int &scrollY = isTile ? tileSearchScrollY_ : entSearchScrollY_;
    int &hoverRow = isTile ? tileSearchHoverRow_ : entSearchHoverRow_;
    bool &hoverTarget = isTile ? tileSearchHoverTarget_ : entSearchHoverTarget_;

    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc = {};
            GetClientRect(hWnd, &rc);
            int w = rc.right;
            int h = rc.bottom;
            if (w <= 0 || h <= 0) {
                EndPaint(hWnd, &ps);
                return 0;
            }

            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hBmpMem = CreateCompatibleBitmap(hdc, w, h);
            HBITMAP hOldBmp = static_cast<HBITMAP>(SelectObject(hdcMem, hBmpMem));

            // Background
            HBRUSH hBg = CreateSolidBrush(RGB(20, 23, 28));
            FillRect(hdcMem, &rc, hBg);
            DeleteObject(hBg);

            HFONT hBoldFont = hBoldFont_ ? hBoldFont_ : static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            HFONT hRegFont = hFont_ ? hFont_ : static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            HFONT hOldFont = static_cast<HFONT>(SelectObject(hdcMem, hBoldFont));

            // 1. Title Banner (y = 0..24)
            RECT rcTitle = { 0, 0, w, 24 };
            HBRUSH hTitleBr = CreateSolidBrush(RGB(26, 30, 36));
            FillRect(hdcMem, &rcTitle, hTitleBr);
            DeleteObject(hTitleBr);

            SetBkMode(hdcMem, TRANSPARENT);
            SetTextColor(hdcMem, RGB(0, 229, 255));
            SelectObject(hdcMem, hHeaderFont_ ? hHeaderFont_ : hBoldFont);
            RECT rcTitleText = { 6, 2, w - 6, 22 };
            DrawTextW(hdcMem, title.c_str(), -1, &rcTitleText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            SelectObject(hdcMem, hBoldFont);

            // Dividing line under title
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(45, 52, 60));
            HPEN hOldPen = static_cast<HPEN>(SelectObject(hdcMem, hPen));
            MoveToEx(hdcMem, 0, 24, nullptr);
            LineTo(hdcMem, w, 24);

            // 2. Column Headers (y = 25..46)
            RECT rcColHdr = { 0, 25, w, 47 };
            HBRUSH hColBr = CreateSolidBrush(RGB(30, 34, 42));
            FillRect(hdcMem, &rcColHdr, hColBr);
            DeleteObject(hColBr);

            int coordsX = std::max(182, w - 110);
            int typeRight = coordsX - 4;

            SetTextColor(hdcMem, RGB(160, 175, 190));
            RECT rcHdrAim = { 4, 25, 28, 47 };
            DrawTextW(hdcMem, L"Aim", -1, &rcHdrAim, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            RECT rcHdrId = { 32, 25, 88, 47 };
            DrawTextW(hdcMem, L"Number/ID", -1, &rcHdrId, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            RECT rcHdrType = { 92, 25, typeRight, 47 };
            DrawTextW(hdcMem, (!isTile && !rows.empty() && rows[0].hasTypeColor) ? L"Faction" : L"Type",
                      -1, &rcHdrType, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            RECT rcHdrCoords = { coordsX, 25, w - 4, 47 };
            DrawTextW(hdcMem, L"Coordinates", -1, &rcHdrCoords, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            MoveToEx(hdcMem, 0, 47, nullptr);
            LineTo(hdcMem, w, 47);
            SelectObject(hdcMem, hOldPen);
            DeleteObject(hPen);

            // 3. Rows
            SelectObject(hdcMem, hRegFont);
            if (rows.empty()) {
                RECT rcEmpty = { 10, 60, w - 10, 120 };
                SetTextColor(hdcMem, RGB(130, 140, 150));
                DrawTextW(hdcMem, L"No instances found on current map.", -1, &rcEmpty, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            } else {
                for (size_t i = 0; i < rows.size(); ++i) {
                    int rowY = 48 - scrollY + static_cast<int>(i) * 24;
                    if (rowY + 24 < 48) continue;
                    if (rowY > h) break;

                    RECT rcRow = { 0, rowY, w, rowY + 24 };
                    COLORREF rowBg = (static_cast<int>(i) == hoverRow) ? RGB(36, 44, 56) :
                                     ((i % 2 == 1) ? RGB(24, 28, 34) : RGB(19, 22, 27));
                    HBRUSH hRowBr = CreateSolidBrush(rowBg);
                    FillRect(hdcMem, &rcRow, hRowBr);
                    DeleteObject(hRowBr);

                    // Row divider line
                    HPEN hRowPen = CreatePen(PS_SOLID, 1, RGB(32, 36, 44));
                    HPEN hPrevPen = static_cast<HPEN>(SelectObject(hdcMem, hRowPen));
                    MoveToEx(hdcMem, 0, rowY + 23, nullptr);
                    LineTo(hdcMem, w, rowY + 23);
                    SelectObject(hdcMem, hPrevPen);
                    DeleteObject(hRowPen);

                    // Target Button (18x18, icon 16x16)
                    int btnX = 5;
                    int btnY = rowY + 3;
                    RECT rcTargetBtn = { btnX, btnY, btnX + 18, btnY + 18 };
                    bool isTargetHover = (static_cast<int>(i) == hoverRow && hoverTarget);
                    HBRUSH hTBr = CreateSolidBrush(isTargetHover ? RGB(24, 48, 64) : RGB(30, 36, 44));
                    FillRect(hdcMem, &rcTargetBtn, hTBr);
                    DeleteObject(hTBr);

                    HPEN hTBorder = CreatePen(PS_SOLID, 1, isTargetHover ? RGB(0, 229, 255) : RGB(50, 60, 72));
                    HPEN hTPrev = static_cast<HPEN>(SelectObject(hdcMem, hTBorder));
                    HBRUSH hTOldBr = static_cast<HBRUSH>(SelectObject(hdcMem, GetStockObject(NULL_BRUSH)));
                    Rectangle(hdcMem, rcTargetBtn.left, rcTargetBtn.top, rcTargetBtn.right, rcTargetBtn.bottom);
                    SelectObject(hdcMem, hTOldBr);
                    SelectObject(hdcMem, hTPrev);
                    DeleteObject(hTBorder);

                    if (hBmpTarget_) {
                        HDC hdcIcon = CreateCompatibleDC(hdcMem);
                        HBITMAP hOldIBmp = static_cast<HBITMAP>(SelectObject(hdcIcon, hBmpTarget_));
                        BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
                        AlphaBlend(hdcMem, btnX + 1, btnY + 1, 16, 16, hdcIcon, 0, 0, 16, 16, bf);
                        SelectObject(hdcIcon, hOldIBmp);
                        DeleteDC(hdcIcon);
                    }

                    // Text Columns
                    COLORREF textCol = (static_cast<int>(i) == hoverRow) ? RGB(255, 255, 255) : RGB(215, 222, 230);
                    SetTextColor(hdcMem, textCol);

                    RECT rcTextId = { 32, rowY, 88, rowY + 24 };
                    DrawTextW(hdcMem, rows[i].idStr.c_str(), -1, &rcTextId, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

                    if (rows[i].hasTypeColor) {
                        SetTextColor(hdcMem, rows[i].typeColor);
                    }
                    RECT rcTextType = { 92, rowY, typeRight, rowY + 24 };
                    DrawTextW(hdcMem, rows[i].typeStr.c_str(), -1, &rcTextType, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

                    SetTextColor(hdcMem, (static_cast<int>(i) == hoverRow) ? RGB(140, 235, 255) : RGB(100, 200, 225));
                    RECT rcTextCoords = { coordsX, rowY, w - 4, rowY + 24 };
                    DrawTextW(hdcMem, rows[i].coordStr.c_str(), -1, &rcTextCoords, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
                }
            }

            SelectObject(hdcMem, hOldFont);

            BitBlt(hdc, 0, 0, w, h, hdcMem, 0, 0, SRCCOPY);
            SelectObject(hdcMem, hOldBmp);
            DeleteObject(hBmpMem);
            DeleteDC(hdcMem);

            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_SIZE: {
            RECT rc = {};
            GetClientRect(hWnd, &rc);
            int visibleH = std::max(1, static_cast<int>(rc.bottom - 48));
            int maxScroll = std::max(0, static_cast<int>(rows.size() * 24) - visibleH);
            if (scrollY > maxScroll) scrollY = maxScroll;
            SCROLLINFO si = {};
            si.cbSize = sizeof(si);
            si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
            si.nMin = 0;
            si.nMax = static_cast<int>(rows.size() * 24);
            si.nPage = visibleH;
            si.nPos = scrollY;
            SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        }

        case WM_VSCROLL: {
            int action = LOWORD(wParam);
            int curPos = scrollY;
            RECT rc = {};
            GetClientRect(hWnd, &rc);
            int visibleH = std::max(1, static_cast<int>(rc.bottom - 48));
            int maxScroll = std::max(0, static_cast<int>(rows.size() * 24) - visibleH);

            switch (action) {
                case SB_LINEUP: curPos -= 24; break;
                case SB_LINEDOWN: curPos += 24; break;
                case SB_PAGEUP: curPos -= visibleH; break;
                case SB_PAGEDOWN: curPos += visibleH; break;
                case SB_THUMBTRACK:
                case SB_THUMBPOSITION: {
                    SCROLLINFO si = { sizeof(SCROLLINFO), SIF_TRACKPOS };
                    GetScrollInfo(hWnd, SB_VERT, &si);
                    curPos = si.nTrackPos;
                    break;
                }
            }
            if (curPos < 0) curPos = 0;
            if (curPos > maxScroll) curPos = maxScroll;
            if (curPos != scrollY) {
                scrollY = curPos;
                SetScrollPos(hWnd, SB_VERT, scrollY, TRUE);
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            return 0;
        }

        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            RECT rc = {};
            GetClientRect(hWnd, &rc);
            int visibleH = std::max(1, static_cast<int>(rc.bottom - 48));
            int maxScroll = std::max(0, static_cast<int>(rows.size() * 24) - visibleH);
            scrollY -= (delta / WHEEL_DELTA) * (24 * 3);
            if (scrollY < 0) scrollY = 0;
            if (scrollY > maxScroll) scrollY = maxScroll;
            SetScrollPos(hWnd, SB_VERT, scrollY, TRUE);
            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        }

        case WM_MOUSEMOVE: {
            int mx = GET_X_LPARAM(lParam);
            int my = GET_Y_LPARAM(lParam);
            int newHoverRow = -1;
            bool newHoverTarget = false;

            if (my >= 48) {
                int relY = my - 48 + scrollY;
                int rIdx = relY / 24;
                if (rIdx >= 0 && rIdx < static_cast<int>(rows.size())) {
                    newHoverRow = rIdx;
                    if (mx >= 4 && mx <= 26) {
                        newHoverTarget = true;
                    }
                }
            }

            if (newHoverRow != hoverRow || newHoverTarget != hoverTarget) {
                hoverRow = newHoverRow;
                hoverTarget = newHoverTarget;
                InvalidateRect(hWnd, nullptr, FALSE);
                TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hWnd, 0 };
                TrackMouseEvent(&tme);
            }
            return 0;
        }

        case WM_MOUSELEAVE: {
            hoverRow = -1;
            hoverTarget = false;
            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            SetFocus(hWnd);
            int mx = GET_X_LPARAM(lParam);
            int my = GET_Y_LPARAM(lParam);
            (void)mx;
            if (my >= 48) {
                int relY = my - 48 + scrollY;
                int rIdx = relY / 24;
                if (rIdx >= 0 && rIdx < static_cast<int>(rows.size())) {
                    const auto &row = rows[rIdx];
                    if (isTile) {
                        if (pViewport_) {
                            pViewport_->centerOnTile(row.pos);
                            pViewport_->highlightSearchTile(row.pos);
                        }
                    } else {
                        if (pViewport_ && row.pEntity) {
                            pViewport_->centerOnEntity(row.pEntity);
                            pViewport_->highlightSearchEntity(row.pEntity);
                        }
                    }
                    InvalidateRect(hWnd, nullptr, FALSE);
                }
            }
            return 0;
        }

        case WM_LBUTTONDBLCLK: {
            int mx = GET_X_LPARAM(lParam);
            int my = GET_Y_LPARAM(lParam);
            (void)mx;
            if (my >= 48) {
                int relY = my - 48 + scrollY;
                int rIdx = relY / 24;
                if (rIdx >= 0 && rIdx < static_cast<int>(rows.size())) {
                    const auto &row = rows[rIdx];
                    if (isTile) {
                        if (pViewport_) {
                            pViewport_->clearSearchHighlight();
                            pViewport_->setSelectedTile(row.pos);
                            pViewport_->setTool(ViewportTool::SelectTile);
                            pViewport_->centerOnTile(row.pos);
                        }
                        isTileSearchActive_ = false;
                        ShowWindow(hTileSearchResults_, SW_HIDE);
                        ShowWindow(hTileInspectPane_, SW_SHOW);
                        refreshTileInspector();
                        updateToolSelection();
                        layoutControls();
                    } else {
                        if (row.pEntity) {
                            if (pViewport_) {
                                pViewport_->clearSearchHighlight();
                                pViewport_->setSelectedEntity(row.pEntity);
                                pViewport_->setTool(ViewportTool::SelectEntity);
                                pViewport_->centerOnEntity(row.pEntity);
                            }
                            isEntitySearchActive_ = false;
                            ShowWindow(hEntSearchResults_, SW_HIDE);
                            ShowWindow(hEntInspectPane_, SW_SHOW);
                            selectEntity(row.pEntity);
                            updateToolSelection();
                            layoutControls();
                        }
                    }
                    InvalidateRect(hWnd_, nullptr, TRUE);
                }
            }
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK WinArtPanel::InspectPaneProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        auto *pCS = reinterpret_cast<CREATESTRUCT*>(lParam);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCS->lpCreateParams));
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    auto *pPanel = reinterpret_cast<WinArtPanel*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    if (pPanel) {
        return pPanel->handleInspectPaneMessage(hWnd, msg, wParam, lParam);
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT WinArtPanel::handleInspectPaneMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    bool isTile = (hWnd == hTileInspectPane_);
    int &scrollY = isTile ? tileInspectScrollY_ : entInspectScrollY_;

    switch (msg) {
        case WM_COMMAND:
        case WM_NOTIFY:
        case WM_DRAWITEM:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORLISTBOX:
            if (hWnd_) {
                return SendMessageW(hWnd_, msg, wParam, lParam);
            }
            break;

        case WM_LBUTTONDOWN:
            SetFocus(hWnd);
            return 0;

        case WM_SIZE: {
            if (isTile) layoutTileInspectPane();
            else layoutEntInspectPane();
            return 0;
        }

        case WM_VSCROLL: {
            RECT rc = {};
            GetClientRect(hWnd, &rc);
            int paneH = rc.bottom;
            SCROLLINFO si = {};
            si.cbSize = sizeof(si);
            si.fMask = SIF_ALL;
            GetScrollInfo(hWnd, SB_VERT, &si);

            int oldY = scrollY;
            int action = LOWORD(wParam);
            int trackPos = HIWORD(wParam);

            switch (action) {
                case SB_LINEUP:        scrollY -= 20; break;
                case SB_LINEDOWN:      scrollY += 20; break;
                case SB_PAGEUP:        scrollY -= std::max(20, paneH - 20); break;
                case SB_PAGEDOWN:      scrollY += std::max(20, paneH - 20); break;
                case SB_THUMBTRACK:
                case SB_THUMBPOSITION: scrollY = trackPos; break;
                default: break;
            }

            int maxScroll = std::max(0, si.nMax - paneH);
            if (scrollY < 0) scrollY = 0;
            if (scrollY > maxScroll) scrollY = maxScroll;

            if (scrollY != oldY) {
                if (isTile) layoutTileInspectPane();
                else layoutEntInspectPane();
                RedrawWindow(hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE);
            }
            return 0;
        }

        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            RECT rc = {};
            GetClientRect(hWnd, &rc);
            int paneH = rc.bottom;
            SCROLLINFO si = {};
            si.cbSize = sizeof(si);
            si.fMask = SIF_ALL;
            GetScrollInfo(hWnd, SB_VERT, &si);

            int oldY = scrollY;
            int maxScroll = std::max(0, si.nMax - paneH);
            scrollY -= (delta / WHEEL_DELTA) * 32;
            if (scrollY < 0) scrollY = 0;
            if (scrollY > maxScroll) scrollY = maxScroll;

            if (scrollY != oldY) {
                if (isTile) layoutTileInspectPane();
                else layoutEntInspectPane();
                RedrawWindow(hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE);
            }
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);
            HBRUSH hBg = CreateSolidBrush(RGB(20, 22, 26));
            FillRect(hdc, &rc, hBg);
            DeleteObject(hBg);
            EndPaint(hWnd, &ps);
            return 0;
        }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void WinArtPanel::activatePencilTool() {
    if (!confirmDiscardEntityEdits()) return;
    if (currentTab_ == 2) {
        if (hTabControl_) TabCtrl_SetCurSel(hTabControl_, 0);
        currentTab_ = 0;
    }
    if (currentTab_ == 1) {
        isEntitySearchActive_ = false;
        isTileSearchActive_ = false;
        activateEntityPlaceTool();
        updateTabVisibility(1);
    } else {
        isTileSearchActive_ = false;
        isEntitySearchActive_ = false;
        if (pViewport_) pViewport_->setTool(ViewportTool::Pencil);
        updateTabVisibility(0);
    }
    layoutControls();
    updateToolSelection();
}

void WinArtPanel::activateBoxFillTool() {
    if (!confirmDiscardEntityEdits()) return;
    if (currentTab_ != 0) {
        if (hTabControl_) TabCtrl_SetCurSel(hTabControl_, 0);
        currentTab_ = 0;
    }
    isTileSearchActive_ = false;
    isEntitySearchActive_ = false;
    if (pViewport_) pViewport_->setTool(ViewportTool::BoxFill);
    updateTabVisibility(0);
    layoutControls();
    updateToolSelection();
}

void WinArtPanel::activateSelectTool() {
    if (!confirmDiscardEntityEdits()) return;
    if (currentTab_ == 2) {
        if (hTabControl_) TabCtrl_SetCurSel(hTabControl_, 0);
        currentTab_ = 0;
    }
    if (currentTab_ == 1) {
        isEntitySearchActive_ = false;
        isTileSearchActive_ = false;
        if (pViewport_) pViewport_->setTool(ViewportTool::SelectEntity);
        updateTabVisibility(1);
    } else {
        isTileSearchActive_ = false;
        isEntitySearchActive_ = false;
        if (pViewport_) pViewport_->setTool(ViewportTool::SelectTile);
        updateTabVisibility(0);
    }
    layoutControls();
    updateToolSelection();
}

void WinArtPanel::activateEntityPlaceTool() {
    if (!confirmDiscardEntityEdits()) return;
    clearSelectedEntity();
    applyEntityPlacementFromSelection();
    if (pViewport_) {
        const auto *pItem = selectedEntityTemplateItem();
        if (pItem) {
            if (pItem->kind == EntityKind::Actor) pViewport_->setTool(ViewportTool::PlaceActor);
            else if (pItem->kind == EntityKind::Vehicle) pViewport_->setTool(ViewportTool::PlaceVehicle);
            else if (pItem->kind == EntityKind::Static) pViewport_->setTool(ViewportTool::PlaceStatic);
            else if (pItem->kind == EntityKind::Weapon) pViewport_->setTool(ViewportTool::PlaceWeapon);
        } else {
            pViewport_->setTool(ViewportTool::PlaceActor);
        }
    }
    updateToolSelection();
}

void WinArtPanel::activateSearchTool() {
    if (!confirmDiscardEntityEdits()) return;
    if (currentTab_ == 2) {
        if (hTabControl_) TabCtrl_SetCurSel(hTabControl_, 0);
        currentTab_ = 0;
    }
    if (currentTab_ == 1) {
        isEntitySearchActive_ = true;
        isTileSearchActive_ = false;
        clearSelectedEntity();
        performEntitySearch(selectedEntityGridIdx_);
        updateTabVisibility(1);
        layoutControls();
        updateToolSelection();
    } else {
        isTileSearchActive_ = true;
        isEntitySearchActive_ = false;
        clearSelectedEntity();
        performTileSearch(activeTileId_);
        updateTabVisibility(0);
        layoutControls();
        updateToolSelection();
    }
}

void WinArtPanel::setSelectedEntityGridIndex(int idx) {
    const auto &items = getEntityItemsForCategory(entityCategory_);
    if (items.empty()) return;
    if (idx < 0) idx = 0;
    if (idx >= static_cast<int>(items.size())) idx = static_cast<int>(items.size()) - 1;
    selectedEntityGridIdx_ = idx;

    const auto &item = *items[idx];

    // If no entity is selected on map, configure the bottom panel as placement template
    if (!pSelectedEntity_) {
        isPopulatingInspector_ = true;
        if (hInspectHeader_) ShowWindow(hInspectHeader_, SW_HIDE);
        if (hInspectStaticPos_) ShowWindow(hInspectStaticPos_, SW_HIDE);
        if (hInspectValuePos_) ShowWindow(hInspectValuePos_, SW_HIDE);
        if (hInspectStaticLoc_) ShowWindow(hInspectStaticLoc_, SW_HIDE);
        if (hInspectValueLoc_) ShowWindow(hInspectValueLoc_, SW_HIDE);

        SetWindowTextW(hInspectStaticType_, L"Type:");
        bool isAgentItem = (item.kind == EntityKind::Actor && item.typeId == rs_knl::PedInstance::kPedTypeAgent);
        if (isAgentItem) {
            ShowWindow(hInspectValueType_, SW_HIDE);
            if (hInspectComboType_) {
                ShowWindow(hInspectComboType_, (currentTab_ == 1) ? SW_SHOW : SW_HIDE);
                if (pMission_ && pMission_->countPlayerAgents() >= 4 && templateAgentType_ == 0) {
                    templateAgentType_ = 1;
                }
                ComboBox_SetCurSel(hInspectComboType_, templateAgentType_);
            }
        } else {
            if (hInspectComboType_) ShowWindow(hInspectComboType_, SW_HIDE);
            ShowWindow(hInspectValueType_, (currentTab_ == 1) ? SW_SHOW : SW_HIDE);
            SetWindowTextW(hInspectValueType_, std::wstring(item.name.begin(), item.name.end()).c_str());
        }
        ShowWindow(hInspectStaticType_, (currentTab_ == 1) ? SW_SHOW : SW_HIDE);

        bool isOurAgentTemplate = (isAgentItem && templateAgentType_ == 0);

        if (isOurAgentTemplate || item.kind == EntityKind::Weapon) {
            ShowWindow(hInspectStaticHealth_, SW_HIDE);
            ShowWindow(hInspectEditHealth_, SW_HIDE);
            ShowWindow(hInspectCheckDead_, SW_HIDE);
        } else {
            ShowWindow(hInspectStaticHealth_, (currentTab_ == 1) ? SW_SHOW : SW_HIDE);
            ShowWindow(hInspectEditHealth_, (currentTab_ == 1) ? SW_SHOW : SW_HIDE);
            ShowWindow(hInspectCheckDead_, (currentTab_ == 1) ? SW_SHOW : SW_HIDE);
            if (item.kind == EntityKind::Actor) {
                SetWindowTextW(hInspectCheckDead_, L"Dead");
            } else {
                SetWindowTextW(hInspectCheckDead_, L"Destroyed");
            }
            SendMessageW(hInspectCheckDead_, BM_SETCHECK, BST_UNCHECKED, 0);
            int defaultHp = item.defaultHealth;
            if (item.kind == EntityKind::Actor) {
                auto pType = static_cast<rs_knl::PedInstance::PedType>(item.typeId);
                bool isEnemy = (pType == rs_knl::PedInstance::kPedTypeAgent);
                defaultHp = rs_knl::PedManager::getStats(pType, isEnemy).health;
            }
            SetWindowTextW(hInspectEditHealth_, std::to_wstring(defaultHp).c_str());
            EnableWindow(hInspectEditHealth_, TRUE);
        }

        if (item.kind == EntityKind::Weapon) {
            ShowWindow(hInspectStaticOrient_, SW_HIDE);
            ShowWindow(hInspectComboOrient_, SW_HIDE);
            EnableWindow(hInspectComboOrient_, FALSE);
        } else {
            ShowWindow(hInspectStaticOrient_, (currentTab_ == 1) ? SW_SHOW : SW_HIDE);
            ShowWindow(hInspectComboOrient_, (currentTab_ == 1) ? SW_SHOW : SW_HIDE);
            EnableWindow(hInspectComboOrient_, TRUE);
            if (item.kind == EntityKind::Static) {
                if (isFixedDirectionStatic(static_cast<uint8_t>(item.typeId), item.baseAnim)) {
                    ComboBox_ResetContent(hInspectComboOrient_);
                    ComboBox_AddString(hInspectComboOrient_, L"Fixed Direction");
                    ComboBox_SetCurSel(hInspectComboOrient_, 0);
                    EnableWindow(hInspectComboOrient_, FALSE);
                } else {
                    EnableWindow(hInspectComboOrient_, TRUE);
                    populateOrientStatic(hInspectComboOrient_);
                    ComboBox_SetCurSel(hInspectComboOrient_, (activeOrientation_ == 1) ? 1 : 0);
                }
            } else if (item.kind == EntityKind::Vehicle && item.typeId == rs_knl::Vehicle::kVehicleTypeTrainBody) {
                EnableWindow(hInspectComboOrient_, TRUE);
                populateOrientTrain(hInspectComboOrient_);
                int oriIdx = ((activeOrientation_ + 32) / 64) % 2;
                ComboBox_SetCurSel(hInspectComboOrient_, oriIdx);
            } else if (item.kind == EntityKind::Vehicle) {
                EnableWindow(hInspectComboOrient_, TRUE);
                populateOrientVehicle(hInspectComboOrient_);
                int oriIdx = ((activeOrientation_ + 32) / 64) % 4;
                ComboBox_SetCurSel(hInspectComboOrient_, oriIdx);
            } else {
                EnableWindow(hInspectComboOrient_, TRUE);
                populateOrient8(hInspectComboOrient_);
                int oriIdx = ((activeOrientation_ + 16) / 32) % 8;
                ComboBox_SetCurSel(hInspectComboOrient_, oriIdx);
            }
        }

        bool isVeh = (item.kind == EntityKind::Vehicle);
        ShowWindow(hInspectStaticDriver_, (isVeh && currentTab_ == 1) ? SW_SHOW : SW_HIDE);
        ShowWindow(hInspectComboDriver_, (isVeh && currentTab_ == 1) ? SW_SHOW : SW_HIDE);
        if (isVeh) {
            populateDriverCombo(nullptr);
        }

        bool isEnemyAgent = (item.kind == EntityKind::Actor && item.typeId == rs_knl::PedInstance::kPedTypeAgent && templateAgentType_ == 1);
        ShowWindow(hInspectStaticCyberware_, (isEnemyAgent && currentTab_ == 1) ? SW_SHOW : SW_HIDE);
        ShowWindow(hInspectComboCyberware_, (isEnemyAgent && currentTab_ == 1) ? SW_SHOW : SW_HIDE);
        if (isEnemyAgent) {
            populateCyberwareCombo(hInspectComboCyberware_, activeCyberwareOverridden_, activeCyberwareTier_);
        }

        bool isActor = (item.kind == EntityKind::Actor);
        bool isOurAgentTempl = (item.kind == EntityKind::Actor && item.typeId == rs_knl::PedInstance::kPedTypeAgent && templateAgentType_ == 0);
        if (isEnemyAgent && activeMovementMode_ == 0x10) {
            // Default placed enemy agents to stationary sentry so they hold positions until tactical AI triggers
            activeMovementMode_ = 0x00;
        }
        ShowWindow(hInspectStaticMoveMode_, (isActor && !isOurAgentTempl && currentTab_ == 1) ? SW_SHOW : SW_HIDE);
        ShowWindow(hInspectComboMoveMode_, (isActor && !isOurAgentTempl && currentTab_ == 1) ? SW_SHOW : SW_HIDE);
        if (isActor && !isOurAgentTempl) {
            ComboBox_SetCurSel(hInspectComboMoveMode_, (activeMovementMode_ == 0x10) ? 0 : 1);
        }

        bool isAnyAgentItem = (item.kind == EntityKind::Actor && item.typeId == rs_knl::PedInstance::kPedTypeAgent);
        ShowWindow(hInspectStaticFaction_, (isAnyAgentItem && currentTab_ == 1) ? SW_SHOW : SW_HIDE);
        ShowWindow(hInspectComboFaction_, (isAnyAgentItem && currentTab_ == 1) ? SW_SHOW : SW_HIDE);
        ShowWindow(hInspectBtnAiTactics_, (isEnemyAgent && currentTab_ == 1) ? SW_SHOW : SW_HIDE);
        if (isAnyAgentItem) {
            if (templateAgentType_ == 0) {
                populateFactionCombo(true, 0);
            } else {
                populateFactionCombo(false, activeFactionId_);
            }
        }

        ShowWindow(hInspectBtnPlaytestConfig_, SW_HIDE);
        if (hInspectBtnApply_) ShowWindow(hInspectBtnApply_, SW_HIDE);
        if (hInspectBtnCancel_) ShowWindow(hInspectBtnCancel_, SW_HIDE);
        if (hInspectBtnDelete_) ShowWindow(hInspectBtnDelete_, SW_HIDE);

        if (item.kind == EntityKind::Actor && !isOurAgentTempl) {
            EnableWindow(hInspectWeaponsGrid_, TRUE);
            SetWindowTextW(hInspectStaticWeapons_, L"Weapons Inventory (Equipped):");
            selectedWeapons_.clear();
            bool hasAR = isAmericanRevoltAvailable();
            int maxWeps = hasAR ? static_cast<int>(sizeof(kWeaponsList)/sizeof(kWeaponsList[0])) : 14;
            for (int i = 0; i < maxWeps; ++i) {
                bool hasW = false;
                if (item.typeId == rs_knl::PedInstance::kPedTypeAgent) {
                    if (kWeaponsList[i].type == rs_knl::Weapon::Pistol || kWeaponsList[i].type == rs_knl::Weapon::Minigun) {
                        hasW = true;
                    }
                } else if (item.typeId == rs_knl::PedInstance::kPedTypeGuard) {
                    if (kWeaponsList[i].type == rs_knl::Weapon::Shotgun) hasW = true;
                } else if (item.typeId == rs_knl::PedInstance::kPedTypePolice) {
                    if (kWeaponsList[i].type == rs_knl::Weapon::Pistol) hasW = true;
                } else if (item.typeId == rs_knl::PedInstance::kPedTypeCriminal) {
                    if (kWeaponsList[i].type == rs_knl::Weapon::Uzi) hasW = true;
                }
                if (hasW) selectedWeapons_.insert(i);
            }
            refreshWeaponsGrid();
            ShowWindow(hInspectStaticWeapons_, (currentTab_ == 1) ? SW_SHOW : SW_HIDE);
            ShowWindow(hInspectWeaponsGrid_, (currentTab_ == 1) ? SW_SHOW : SW_HIDE);
        } else {
            selectedWeapons_.clear();
            refreshWeaponsGrid();
            ShowWindow(hInspectStaticWeapons_, SW_HIDE);
            ShowWindow(hInspectWeaponsGrid_, SW_HIDE);
        }

        if (currentTab_ != 1) {
            if (hInspectHeader_) ShowWindow(hInspectHeader_, SW_HIDE);
            if (hInspectStaticType_) ShowWindow(hInspectStaticType_, SW_HIDE);
            if (hInspectValueType_) ShowWindow(hInspectValueType_, SW_HIDE);
            if (hInspectStaticHealth_) ShowWindow(hInspectStaticHealth_, SW_HIDE);
            if (hInspectEditHealth_) ShowWindow(hInspectEditHealth_, SW_HIDE);
            if (hInspectCheckDead_) ShowWindow(hInspectCheckDead_, SW_HIDE);
            if (hInspectStaticOrient_) ShowWindow(hInspectStaticOrient_, SW_HIDE);
            if (hInspectComboOrient_) ShowWindow(hInspectComboOrient_, SW_HIDE);
            if (hInspectStaticDriver_) ShowWindow(hInspectStaticDriver_, SW_HIDE);
            if (hInspectComboDriver_) ShowWindow(hInspectComboDriver_, SW_HIDE);
            if (hInspectStaticCyberware_) ShowWindow(hInspectStaticCyberware_, SW_HIDE);
            if (hInspectComboCyberware_) ShowWindow(hInspectComboCyberware_, SW_HIDE);
            if (hInspectStaticMoveMode_) ShowWindow(hInspectStaticMoveMode_, SW_HIDE);
            if (hInspectComboMoveMode_) ShowWindow(hInspectComboMoveMode_, SW_HIDE);
            if (hInspectStaticFaction_) ShowWindow(hInspectStaticFaction_, SW_HIDE);
            if (hInspectComboFaction_) ShowWindow(hInspectComboFaction_, SW_HIDE);
            if (hInspectBtnAiTactics_) ShowWindow(hInspectBtnAiTactics_, SW_HIDE);
            if (hInspectBtnPlaytestConfig_) ShowWindow(hInspectBtnPlaytestConfig_, SW_HIDE);
            if (hInspectStaticPos_) ShowWindow(hInspectStaticPos_, SW_HIDE);
            if (hInspectValuePos_) ShowWindow(hInspectValuePos_, SW_HIDE);
            if (hInspectStaticLoc_) ShowWindow(hInspectStaticLoc_, SW_HIDE);
            if (hInspectValueLoc_) ShowWindow(hInspectValueLoc_, SW_HIDE);
            if (hInspectBtnApply_) ShowWindow(hInspectBtnApply_, SW_HIDE);
            if (hInspectBtnCancel_) ShowWindow(hInspectBtnCancel_, SW_HIDE);
            if (hInspectBtnDelete_) ShowWindow(hInspectBtnDelete_, SW_HIDE);
        } else {
            layoutControls();
            InvalidateRect(hWnd_, nullptr, TRUE);
        }

        isPopulatingInspector_ = false;
        applyEntityPlacementFromSelection();
    }

    updateEntityPreview();
    InvalidateRect(hEntityGrid_, nullptr, FALSE);
}

void WinArtPanel::applyEntityPlacementFromSelection() {
    if (!pViewport_) return;

    const auto *pItem = selectedEntityTemplateItem();
    if (!pItem) return;

    int hp = pItem->defaultHealth > 0 ? pItem->defaultHealth : 30;
    wchar_t hpBuf[64] = {};
    GetWindowTextW(hInspectEditHealth_, hpBuf, 64);
    if (wcslen(hpBuf) > 0) {
        hp = _wtoi(hpBuf);
    }
    if (hInspectCheckDead_ && SendMessageW(hInspectCheckDead_, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        hp = -1;
    }

    int oriSel = ComboBox_GetCurSel(hInspectComboOrient_);
    if (oriSel < 0) oriSel = 0;

    ViewportTool curTool = pViewport_->tool();
    bool isPlacing = (curTool == ViewportTool::PlaceActor ||
                      curTool == ViewportTool::PlaceVehicle ||
                      curTool == ViewportTool::PlaceStatic ||
                      curTool == ViewportTool::PlaceWeapon);

    if (pItem->kind == EntityKind::Actor) {
        rs_knl::PedInstance::PedType pType = static_cast<rs_knl::PedInstance::PedType>(pItem->typeId);
        uint8_t ori = static_cast<uint8_t>(oriSel * 32);
        activeOrientation_ = ori;

        bool isOurAgent = (pType == rs_knl::PedInstance::kPedTypeAgent && templateAgentType_ == 0);

        std::vector<int> weapons;
        if (!isOurAgent) {
            for (int i = 0; i < static_cast<int>(sizeof(kWeaponsList)/sizeof(kWeaponsList[0])); ++i) {
                if (selectedWeapons_.count(i) > 0) {
                    weapons.push_back(static_cast<int>(kWeaponsList[i].type));
                }
            }
        }

        bool cyberOverridden = false;
        int cyberTier = pMission_ ? pMission_->techLevel() : 0;
        if (pType == rs_knl::PedInstance::kPedTypeAgent && !isOurAgent && hInspectComboCyberware_ && IsWindowVisible(hInspectComboCyberware_)) {
            int sel = ComboBox_GetCurSel(hInspectComboCyberware_);
            if (sel > 0) {
                cyberOverridden = true;
                cyberTier = sel - 1;
            } else {
                cyberOverridden = false;
                cyberTier = pMission_ ? pMission_->techLevel() : 0;
            }
            activeCyberwareOverridden_ = cyberOverridden;
            activeCyberwareTier_ = cyberTier;
        }

        if (!isOurAgent && hInspectComboMoveMode_ && IsWindowVisible(hInspectComboMoveMode_)) {
            int sel = ComboBox_GetCurSel(hInspectComboMoveMode_);
            activeMovementMode_ = (sel == 1) ? 0x00 : 0x10;
        }

        if (pType == rs_knl::PedInstance::kPedTypeAgent) {
            if (isOurAgent) {
                activeFactionId_ = 0;
            } else if (hInspectComboFaction_ && IsWindowVisible(hInspectComboFaction_)) {
                int fSel = ComboBox_GetCurSel(hInspectComboFaction_);
                activeFactionId_ = (fSel >= 0) ? (fSel + 1) : 1;
            }
        }

        pViewport_->setActorPlacement(pType, ori, pItem->baseAnim, hp, weapons, cyberOverridden, cyberTier,
                                      activeMovementMode_, activeFactionId_, isOurAgent);
        if (isPlacing) {
            pViewport_->setTool(ViewportTool::PlaceActor);
        }
    } else if (pItem->kind == EntityKind::Vehicle) {
        uint8_t vType = static_cast<uint8_t>(pItem->typeId);
        uint8_t ori = static_cast<uint8_t>(oriSel * 64);
        activeOrientation_ = ori;

        int driverChoice = (hInspectComboDriver_ && IsWindowVisible(hInspectComboDriver_)) ? ComboBox_GetCurSel(hInspectComboDriver_) : 0;
        if (driverChoice < 0) driverChoice = 0;
        activeDriverChoice_ = driverChoice;

        int driverPedType = 0;
        uint16_t driverBaseAnim = 0;
        if (driverChoice >= 1 && driverChoice <= static_cast<int>(s_actorItems.size())) {
            const auto &actor = s_actorItems[driverChoice - 1];
            driverPedType = actor.typeId;
            driverBaseAnim = actor.baseAnim;
        }

        pViewport_->setVehiclePlacement(vType, ori, hp, driverPedType, driverBaseAnim, pItem->baseAnim);
        if (isPlacing) {
            pViewport_->setTool(ViewportTool::PlaceVehicle);
        }
    } else if (pItem->kind == EntityKind::Static) {
        uint16_t sId = static_cast<uint16_t>(pItem->typeId);
        uint8_t ori = 0;
        if (!isFixedDirectionStatic(static_cast<uint8_t>(pItem->typeId), pItem->baseAnim)) {
            ori = static_cast<uint8_t>((oriSel == 1) ? rs_knl::Static::kStaticOrientationEW : rs_knl::Static::kStaticOrientationNS);
        }
        activeOrientation_ = static_cast<uint8_t>(oriSel);

        pViewport_->setStaticPlacement(sId, pItem->baseAnim, ori);
        if (isPlacing) {
            pViewport_->setTool(ViewportTool::PlaceStatic);
        }
    } else if (pItem->kind == EntityKind::Weapon) {
        rs_knl::Weapon::WeaponType wType = static_cast<rs_knl::Weapon::WeaponType>(pItem->typeId);
        pViewport_->setWeaponPlacement(wType);
        if (isPlacing) {
            pViewport_->setTool(ViewportTool::PlaceWeapon);
        }
    }
}

static const wchar_t* getPedTypeName(rs_knl::PedInstance *p) {
    if (!p) return L"Ped";
    if (p->isOurAgent()) return L"Agent";
    if (p->type() == rs_knl::PedInstance::kPedTypeGuard) return L"Guard";
    if (p->type() == rs_knl::PedInstance::kPedTypePolice) return L"Police";
    if (p->type() == rs_knl::PedInstance::kPedTypeAgent) return L"Enemy Agent";
    if (p->type() == rs_knl::PedInstance::kPedTypeCriminal) {
        if (p->baseAnim() == 1123) return L"Punk";
        if (p->baseAnim() == 1330) return L"Merc";
        return L"Criminal";
    }
    if (p->baseAnim() == 1123) return L"Civ (P)";
    if (p->baseAnim() == 626) return L"Civ (F)";
    if (p->baseAnim() == 1537) return L"Civ (S)";
    return L"Civ (M)";
}

static const wchar_t* getVehicleTypeName(uint8_t type) {
    if (type == rs_knl::Vehicle::kVehicleTypeRegularCar) return L"Car";
    if (type == rs_knl::Vehicle::kVehicleTypePolice) return L"Police Car";
    if (type == rs_knl::Vehicle::kVehicleTypeMedics) return L"Ambulance";
    if (type == rs_knl::Vehicle::kVehicleTypeFireFighter) return L"Firetruck";
    if (type == rs_knl::Vehicle::kVehicleTypeSmallArmored) return L"Small Armored";
    if (type == rs_knl::Vehicle::kVehicleTypeLargeArmored) return L"Large Armored";
    if (type == rs_knl::Vehicle::kVehicleTypeLargeArmoredDamaged) return L"Damaged Armored";
    if (type == rs_knl::Vehicle::kVehicleTypeTrainHead) return L"Train Engine";
    if (type == rs_knl::Vehicle::kVehicleTypeTrainBody) return L"Train Carriage";
    return L"Vehicle";
}

static int findVehicleIndex(rs_knl::Mission *pMission, rs_knl::Vehicle *t) {
    if (!pMission || !t) return 0;
    for (size_t i = 0; i < pMission->numVehicles(); ++i) {
        if (pMission->vehicle(i) == t) return static_cast<int>(i);
    }
    return 0;
}

void WinArtPanel::refreshObjectivesList() {
    if (!hListObjectives_) return;

    int curSel = ListBox_GetCurSel(hListObjectives_);
    SendMessageW(hListObjectives_, LB_RESETCONTENT, 0, 0);

    if (!pMission_) {
        SetWindowTextW(hEditProperties_, L"No mission loaded.\r\n\r\nUse File -> New Mission (Ctrl+N) or Open Mission (Ctrl+O) to begin.");
        return;
    }

    int agentCount = 0;
    for (size_t i = 0; i < pMission_->numPeds(); ++i) {
        if (pMission_->ped(i) && pMission_->ped(i)->isOurAgent()) {
            agentCount++;
        }
    }
    std::wstring overview = std::format(
        L"Mission Overview & Properties\r\n"
        L"-----------------------------\r\n"
        L"Map Dimensions: {}x{} tiles\r\n"
        L"Player's Agents: {} of 4 placed\r\n"
        L"Total Map Peds: {}\r\n"
        L"Total Vehicles: {}\r\n"
        L"Static Objects: {}\r\n"
        L"Ground Weapons: {}\r\n"
        L"Objectives: {}\r\n\r\n"
        L"Entity Inspection:\r\n"
        L"Use Select / Move Entity (V) and click any ped, vehicle, static, or weapon drop on the map to inspect or edit properties.\r\n\r\n"
        L"Placement:\r\n"
        L"Use the Entities tab to place Player's Agents (1-4), enemy agents, civilians, guards, police, vehicles, statics, or weapon drops.",
        pMission_->map() ? pMission_->map()->maxTx() : 128,
        pMission_->map() ? pMission_->map()->maxTy() : 128,
        agentCount,
        pMission_->numPeds(),
        pMission_->numVehicles(),
        pMission_->numStatics(),
        pMission_->weaponsOnGround().size(),
        pMission_->numObjectives()
    );
    SetWindowTextW(hEditProperties_, overview.c_str());

    for (size_t i = 0; i < pMission_->numObjectives(); ++i) {
        auto *obj = pMission_->objective(i);
        std::wstring line;

        if (!obj) {
            line = std::format(L"#{:02}: [Unknown Objective]", i + 1);
        } else if (auto *oa = dynamic_cast<rs_knl::ObjAssassinate*>(obj)) {
            auto *t = dynamic_cast<rs_knl::PedInstance*>(oa->target());
            if (t) {
                line = std::format(L"#{:02}: [Assassinate] Ped #{:03} [{}] at (TX:{:02}, TY:{:02}, TZ:{:02})",
                    i + 1, t->id(), getPedTypeName(t), t->tileX(), t->tileY(), t->tileZ());
            } else {
                line = std::format(L"#{:02}: [Assassinate] Target Ped", i + 1);
            }
        } else if (auto *op = dynamic_cast<rs_knl::ObjPersuade*>(obj)) {
            auto *t = dynamic_cast<rs_knl::PedInstance*>(op->target());
            if (t) {
                line = std::format(L"#{:02}: [Persuade] Ped #{:03} [{}] at (TX:{:02}, TY:{:02}, TZ:{:02})",
                    i + 1, t->id(), getPedTypeName(t), t->tileX(), t->tileY(), t->tileZ());
            } else {
                line = std::format(L"#{:02}: [Persuade] Target Ped", i + 1);
            }
        } else if (auto *opr = dynamic_cast<rs_knl::ObjProtect*>(obj)) {
            auto *t = dynamic_cast<rs_knl::PedInstance*>(opr->target());
            if (t) {
                line = std::format(L"#{:02}: [Protect] Ped #{:03} [{}] at (TX:{:02}, TY:{:02}, TZ:{:02})",
                    i + 1, t->id(), getPedTypeName(t), t->tileX(), t->tileY(), t->tileZ());
            } else {
                line = std::format(L"#{:02}: [Protect] Target Ped", i + 1);
            }
        } else if (auto *oe = dynamic_cast<rs_knl::ObjEliminate*>(obj)) {
            line = std::format(L"#{:02}: [Combat Sweep] Eliminate Enemies", i + 1);
        } else if (auto *odv = dynamic_cast<rs_knl::ObjDestroyVehicle*>(obj)) {
            auto *t = dynamic_cast<rs_knl::Vehicle*>(odv->target());
            if (t) {
                int vIdx = findVehicleIndex(pMission_, t);
                line = std::format(L"#{:02}: [Destroy Vehicle] Veh #{:02} [{}] at (TX:{:02}, TY:{:02}, TZ:{:02})",
                    i + 1, vIdx, getVehicleTypeName(t->getType()), t->tileX(), t->tileY(), t->tileZ());
            } else {
                line = std::format(L"#{:02}: [Destroy Vehicle] Target Vehicle", i + 1);
            }
        } else if (auto *ouv = dynamic_cast<rs_knl::ObjUseVehicle*>(obj)) {
            auto *t = dynamic_cast<rs_knl::Vehicle*>(ouv->target());
            if (t) {
                int vIdx = findVehicleIndex(pMission_, t);
                line = std::format(L"#{:02}: [Use / Steal Vehicle] Veh #{:02} [{}] at (TX:{:02}, TY:{:02}, TZ:{:02})",
                    i + 1, vIdx, getVehicleTypeName(t->getType()), t->tileX(), t->tileY(), t->tileZ());
            } else {
                line = std::format(L"#{:02}: [Use / Steal Vehicle] Target Vehicle", i + 1);
            }
        } else if (auto *oevac = dynamic_cast<rs_knl::ObjEvacuate*>(obj)) {
            auto loc = oevac->location();
            line = std::format(L"#{:02}: [Evacuate] Zone at (TX:{:02}, TY:{:02}, TZ:{:02})",
                i + 1, loc.x / 256, loc.y / 256, loc.z / 128);
        } else {
            line = std::format(L"#{:02}: [Objective]", i + 1);
        }

        SendMessageW(hListObjectives_, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(line.c_str()));
    }

    if (curSel >= 0 && curSel < static_cast<int>(pMission_->numObjectives())) {
        ListBox_SetCurSel(hListObjectives_, curSel);
    }
}

void WinArtPanel::setMission(rs_knl::Mission *pMission, int missionId) {
    pMission_ = pMission;
    activeMissionId_ = missionId;
    pSelectedEntity_ = nullptr;
    activeCyberwareOverridden_ = false;
    activeCyberwareTier_ = pMission_ ? pMission_->techLevel() : 0;
    if (hInspectComboCyberware_) {
        populateCyberwareCombo(hInspectComboCyberware_, activeCyberwareOverridden_, activeCyberwareTier_);
    }
    refreshObjectivesList();
    refreshTileInspector();
    refreshInspector();
    updateTabVisibility(currentTab_);
}

void WinArtPanel::updateProperties(const std::string &info) {
    std::wstring wstr(info.begin(), info.end());
    SetWindowTextW(hEditProperties_, wstr.c_str());
}

int WinArtPanel::getTileGridColumns() const {
    if (!hTileGrid_) return 4;
    RECT rc = {};
    GetClientRect(hTileGrid_, &rc);
    int availW = rc.right;
    int marginX = 4;
    int cellWidth = 66;
    int cellSpacingX = 72;
    if (availW < marginX + cellWidth) return 1;
    int cols = 1 + (availW - marginX - cellWidth) / cellSpacingX;
    return std::max(1, cols);
}

void WinArtPanel::updateTileGridScroll() {
    if (!hTileGrid_) return;

    RECT rc;
    GetClientRect(hTileGrid_, &rc);
    int cols = getTileGridColumns();
    int cellH = 54;
    int totalRows = (static_cast<int>(categoryTiles_.size()) + cols - 1) / cols;
    int contentH = totalRows * cellH + 8;

    SCROLLINFO si = {};
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = contentH > 0 ? contentH : 1;
    si.nPage = rc.bottom > 0 ? rc.bottom : 1;
    int maxPos = std::max(0, contentH - static_cast<int>(rc.bottom));
    if (gridScrollY_ > maxPos) {
        gridScrollY_ = maxPos;
    }
    si.nPos = gridScrollY_;
    SetScrollInfo(hTileGrid_, SB_VERT, &si, TRUE);
}

LRESULT CALLBACK WinArtPanel::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WinArtPanel *pThis = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<WinArtPanel*>(cs->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } else {
        pThis = reinterpret_cast<WinArtPanel*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->handleMessage(hWnd, msg, wParam, lParam);
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT WinArtPanel::handleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);
            FillRect(hdc, &rc, hBgBrush_);
            // 1px left separator line from splitter
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(45, 52, 60));
            HPEN hOld = static_cast<HPEN>(SelectObject(hdc, hPen));
            MoveToEx(hdc, 0, 0, nullptr);
            LineTo(hdc, 0, rc.bottom);
            SelectObject(hdc, hOld);
            DeleteObject(hPen);

            // If on Tiles or Entities tab, draw horizontal splitter bar between grid and inspector / search results
            if (currentTab_ == 0 || currentTab_ == 1) {
                int listH = (currentTab_ == 0) ? getClampedTileGridHeight(rc.bottom) : getClampedEntityGridHeight(rc.bottom);
                int splitY = 72 + listH; // 8px tall area: [splitY, splitY + 8]

                HPEN hSplitPen = CreatePen(PS_SOLID, 1, RGB(55, 62, 70));
                HPEN hOldSplit = static_cast<HPEN>(SelectObject(hdc, hSplitPen));
                MoveToEx(hdc, 6, splitY + 3, nullptr);
                LineTo(hdc, rc.right - 6, splitY + 3);
                SelectObject(hdc, hOldSplit);
                DeleteObject(hSplitPen);

                // Draw centered 3-dot grip handle
                int midX = rc.right / 2;
                int dotY = splitY + 3;
                HBRUSH hGripBr = CreateSolidBrush(RGB(110, 120, 135));
                for (int dx = -10; dx <= 10; dx += 10) {
                    RECT rcDot = { midX + dx - 1, dotY - 1, midX + dx + 2, dotY + 2 };
                    FillRect(hdc, &rcDot, hGripBr);
                }
                DeleteObject(hGripBr);
            }

            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_CTLCOLORSTATIC: {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            HWND hCtrl = reinterpret_cast<HWND>(lParam);
            SetBkMode(hdc, TRANSPARENT);
            if (hCtrl == hInspectHeader_) {
                SetTextColor(hdc, RGB(0, 229, 255));
            } else {
                SetTextColor(hdc, RGB(220, 225, 230));
            }
            return reinterpret_cast<LRESULT>(hBgBrush_);
        }

        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX: {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetBkColor(hdc, RGB(20, 22, 26));
            SetTextColor(hdc, RGB(240, 245, 250));
            return reinterpret_cast<LRESULT>(hInputBgBrush_);
        }

        case WM_CTLCOLORBTN: {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetBkColor(hdc, RGB(38, 42, 48));
            SetTextColor(hdc, RGB(220, 225, 230));
            return reinterpret_cast<LRESULT>(hBtnBrush_);
        }

        case WM_DRAWITEM: {
            DRAWITEMSTRUCT *pDIS = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            if (!pDIS) break;

            if (pDIS->CtlID == IDC_TAB_CONTROL) {
                int tabIndex = pDIS->itemID;
                bool isCurSel = (TabCtrl_GetCurSel(hTabControl_) == tabIndex);
                RECT rc = pDIS->rcItem;

                COLORREF bgCol = isCurSel ? RGB(38, 44, 52) : RGB(26, 29, 34);
                HBRUSH hBr = CreateSolidBrush(bgCol);
                FillRect(pDIS->hDC, &rc, hBr);
                DeleteObject(hBr);

                // Top accent bar for current selection
                if (isCurSel) {
                    RECT rcAccent = { rc.left, rc.top, rc.right, rc.top + 2 };
                    HBRUSH hAccentBr = CreateSolidBrush(RGB(0, 229, 255));
                    FillRect(pDIS->hDC, &rcAccent, hAccentBr);
                    DeleteObject(hAccentBr);
                }

                // Subtle border
                HPEN hPen = CreatePen(PS_SOLID, 1, isCurSel ? RGB(58, 66, 76) : RGB(40, 44, 50));
                HPEN hOldPen = static_cast<HPEN>(SelectObject(pDIS->hDC, hPen));
                MoveToEx(pDIS->hDC, rc.left, rc.top, nullptr);
                LineTo(pDIS->hDC, rc.right, rc.top);
                MoveToEx(pDIS->hDC, rc.right - 1, rc.top, nullptr);
                LineTo(pDIS->hDC, rc.right - 1, rc.bottom);
                SelectObject(pDIS->hDC, hOldPen);
                DeleteObject(hPen);

                // Draw Tab icon (3 tabs: Tiles, Entities, Objectives) - 24x24
                HIMAGELIST hTabs = IconManager::instance().tabsImageList();
                if (hTabs && tabIndex >= 0 && tabIndex < 3) {
                    int iconY = rc.top + (rc.bottom - rc.top - 24) / 2;
                    ImageList_Draw(hTabs, tabIndex, pDIS->hDC, rc.left + 8, iconY, ILD_TRANSPARENT);
                }

                // Draw Tab text
                TCITEMW tie = {};
                wchar_t textBuf[64] = {};
                tie.mask = TCIF_TEXT;
                tie.pszText = textBuf;
                tie.cchTextMax = 64;
                TabCtrl_GetItem(hTabControl_, tabIndex, &tie);

                RECT rcText = rc;
                rcText.left += 38;
                rcText.right -= 4;

                SetBkMode(pDIS->hDC, TRANSPARENT);
                SetTextColor(pDIS->hDC, isCurSel ? RGB(255, 255, 255) : RGB(165, 175, 185));
                HFONT hFont = hHeaderFont_ ? hHeaderFont_ : reinterpret_cast<HFONT>(SendMessageW(hTabControl_, WM_GETFONT, 0, 0));
                if (!hFont) hFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
                HFONT hOldFont = static_cast<HFONT>(SelectObject(pDIS->hDC, hFont));

                DrawTextW(pDIS->hDC, textBuf, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

                SelectObject(pDIS->hDC, hOldFont);
                return TRUE;
            }

            // Tool buttons on Tiles and Entities tabs
            if (pDIS->CtlID == IDC_BTN_TOOL_PENCIL ||
                pDIS->CtlID == IDC_BTN_TOOL_BOX ||
                pDIS->CtlID == IDC_BTN_TOOL_TILE_SELECT ||
                pDIS->CtlID == IDC_BTN_TOOL_TILE_SEARCH ||
                pDIS->CtlID == IDC_BTN_ENT_SELECT ||
                pDIS->CtlID == IDC_BTN_ENT_PLACE ||
                pDIS->CtlID == IDC_BTN_ENT_SEARCH) {

                ViewportTool activeTool = pViewport_ ? pViewport_->tool() : ViewportTool::SelectTile;
                bool isPicking = pViewport_ && pViewport_->isObjectivePicking();
                bool isActive = false;
                HBITMAP hBmp = nullptr;

                if (!isPicking) {
                    if (pDIS->CtlID == IDC_BTN_TOOL_PENCIL) {
                        isActive = (!isTileSearchActive_ && currentTab_ == 0 && (activeTool == ViewportTool::Pencil ||
                                                         activeTool == ViewportTool::PlaceActor ||
                                                         activeTool == ViewportTool::PlaceVehicle ||
                                                         activeTool == ViewportTool::PlaceStatic));
                        hBmp = hBmpPencil_;
                    } else if (pDIS->CtlID == IDC_BTN_TOOL_BOX) {
                        isActive = (!isTileSearchActive_ && currentTab_ == 0 && activeTool == ViewportTool::BoxFill);
                        hBmp = hBmpBox_;
                    } else if (pDIS->CtlID == IDC_BTN_TOOL_TILE_SELECT) {
                        isActive = (!isTileSearchActive_ && currentTab_ == 0 && (activeTool == ViewportTool::SelectTile ||
                                                         activeTool == ViewportTool::SelectEntity));
                        hBmp = hBmpSelect_;
                    } else if (pDIS->CtlID == IDC_BTN_TOOL_TILE_SEARCH) {
                        isActive = (currentTab_ == 0 && isTileSearchActive_);
                        hBmp = hBmpSearch_;
                    } else if (pDIS->CtlID == IDC_BTN_ENT_PLACE) {
                        isActive = (!isEntitySearchActive_ && currentTab_ == 1 && (activeTool == ViewportTool::PlaceActor ||
                                                         activeTool == ViewportTool::PlaceVehicle ||
                                                         activeTool == ViewportTool::PlaceStatic ||
                                                         activeTool == ViewportTool::PlaceWeapon ||
                                                         activeTool == ViewportTool::Pencil ||
                                                         activeTool == ViewportTool::BoxFill));
                        hBmp = hBmpPencil_;
                    } else if (pDIS->CtlID == IDC_BTN_ENT_SELECT) {
                        isActive = (!isEntitySearchActive_ && currentTab_ == 1 && (activeTool == ViewportTool::SelectEntity ||
                                                         activeTool == ViewportTool::SelectTile));
                        hBmp = hBmpSelect_;
                    } else if (pDIS->CtlID == IDC_BTN_ENT_SEARCH) {
                        isActive = (currentTab_ == 1 && isEntitySearchActive_);
                        hBmp = hBmpSearch_;
                    }
                } else {
                    if (pDIS->CtlID == IDC_BTN_TOOL_PENCIL) hBmp = hBmpPencil_;
                    else if (pDIS->CtlID == IDC_BTN_TOOL_BOX) hBmp = hBmpBox_;
                    else if (pDIS->CtlID == IDC_BTN_TOOL_TILE_SELECT) hBmp = hBmpSelect_;
                    else if (pDIS->CtlID == IDC_BTN_TOOL_TILE_SEARCH) hBmp = hBmpSearch_;
                    else if (pDIS->CtlID == IDC_BTN_ENT_PLACE) hBmp = hBmpPencil_;
                    else if (pDIS->CtlID == IDC_BTN_ENT_SELECT) hBmp = hBmpSelect_;
                    else if (pDIS->CtlID == IDC_BTN_ENT_SEARCH) hBmp = hBmpSearch_;
                }

                bool isPressed = (pDIS->itemState & ODS_SELECTED) != 0;
                RECT rc = pDIS->rcItem;

                COLORREF bgCol = isPressed ? RGB(16, 36, 48) : (isActive ? RGB(22, 50, 68) : RGB(34, 38, 44));
                COLORREF borderCol = isActive ? RGB(0, 220, 245) : (isPressed ? RGB(0, 180, 210) : RGB(58, 65, 74));

                HBRUSH hBr = CreateSolidBrush(bgCol);
                FillRect(pDIS->hDC, &rc, hBr);
                DeleteObject(hBr);

                HPEN hPen = CreatePen(PS_SOLID, 1, borderCol);
                HPEN hOldPen = static_cast<HPEN>(SelectObject(pDIS->hDC, hPen));
                HBRUSH hOldBr = static_cast<HBRUSH>(SelectObject(pDIS->hDC, GetStockObject(NULL_BRUSH)));
                Rectangle(pDIS->hDC, rc.left, rc.top, rc.right, rc.bottom);
                SelectObject(pDIS->hDC, hOldBr);
                SelectObject(pDIS->hDC, hOldPen);
                DeleteObject(hPen);

                if (hBmp) {
                    HDC hdcMem = CreateCompatibleDC(pDIS->hDC);
                    HBITMAP hOldBmp = static_cast<HBITMAP>(SelectObject(hdcMem, hBmp));
                    int bmpW = 20, bmpH = 20;
                    int bx = rc.left + (rc.right - rc.left - bmpW) / 2;
                    int by = rc.top + (rc.bottom - rc.top - bmpH) / 2;
                    if (isPressed) { bx++; by++; }
                    BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
                    AlphaBlend(pDIS->hDC, bx, by, bmpW, bmpH, hdcMem, 0, 0, bmpW, bmpH, bf);
                    SelectObject(hdcMem, hOldBmp);
                    DeleteDC(hdcMem);
                }
                return TRUE;
            }

            // Custom buttons
            const wchar_t *btnText = nullptr;
            bool isDanger = false;
            bool isPrimary = false;

            switch (pDIS->CtlID) {
                case IDC_BTN_ADD_OBJ: btnText = L"Add..."; break;
                case IDC_BTN_EDIT_OBJ: btnText = L"Edit..."; break;
                case IDC_BTN_DEL_OBJ: btnText = L"Delete"; isDanger = true; break;
                case IDC_BTN_CLEAR_OBJ: btnText = L"Clear All Objectives"; isDanger = true; break;
                case IDC_INSPECT_APPLY: btnText = L"Apply Changes"; isPrimary = true; break;
                case IDC_INSPECT_CANCEL: btnText = L"Cancel"; break;
                case IDC_INSPECT_BTN_AI_TACTICS: btnText = L"Edit Faction AI Tactics..."; break;
                case IDC_INSPECT_BTN_PLAYTEST_CONFIG: btnText = L"Playtest Configuration..."; isPrimary = true; break;
                case IDC_INSPECT_DELETE: {
                    bool isWpn = (pSelectedEntity_ && dynamic_cast<rs_knl::WeaponInstance*>(pSelectedEntity_));
                    btnText = isWpn ? L"Delete Weapon" : L"Delete Entity";
                    isDanger = true;
                    break;
                }
                case IDC_INSPECT_BTN_ROUTE: {
                    bool isRouteActive = (pViewport_ && pViewport_->tool() == ViewportTool::RouteTool);
                    btnText = isRouteActive ? L"Done Route (Esc)" : L"Edit Route (R)";
                    isPrimary = isRouteActive;
                    break;
                }
                case IDC_INSPECT_BTN_CLEAR_ROUTE: btnText = L"Clear"; isDanger = true; break;
                case IDC_INSPECT_BTN_ADD_ACTION: btnText = L"+ Action"; isPrimary = true; break;
                case IDC_INSPECT_BTN_DEL_ACTION: btnText = L"Delete"; isDanger = true; break;
                case IDC_INSPECT_BTN_MOVE_UP: btnText = L"▲ Up"; break;
                case IDC_INSPECT_BTN_MOVE_DOWN: btnText = L"▼ Down"; break;
                default: break;
            }

            if (btnText) {
                drawDarkButton(pDIS, btnText, isDanger, isPrimary);
                return TRUE;
            }
            break;
        }

        case WM_NOTIFY: {
            NMHDR *pnm = reinterpret_cast<NMHDR*>(lParam);
            if ((pnm->hwndFrom == hTabControl_ || pnm->idFrom == IDC_TAB_CONTROL) && pnm->code == TCN_SELCHANGE) {
                int sel = TabCtrl_GetCurSel(hTabControl_);
                if (sel != currentTab_) {
                    if (!confirmDiscardEntityEdits()) {
                        TabCtrl_SetCurSel(hTabControl_, currentTab_);
                        return 0;
                    }
                }
                updateTabVisibility(sel);
                InvalidateRect(hTabControl_, nullptr, TRUE);
                return 0;
            }
            break;
        }

        case WM_MOUSEWHEEL: {
            if (currentTab_ == 0 && hTileGrid_) {
                return SendMessage(hTileGrid_, WM_MOUSEWHEEL, wParam, lParam);
            }
            if (currentTab_ == 1 && hEntityGrid_) {
                return SendMessage(hEntityGrid_, WM_MOUSEWHEEL, wParam, lParam);
            }
            break;
        }

        case WM_SETCURSOR: {
            if (LOWORD(lParam) == HTCLIENT && (currentTab_ == 0 || currentTab_ == 1)) {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hWnd, &pt);
                RECT rc;
                GetClientRect(hWnd, &rc);
                int listH = (currentTab_ == 0) ? getClampedTileGridHeight(rc.bottom) : getClampedEntityGridHeight(rc.bottom);
                int splitY = 72 + listH;
                bool isDragging = (currentTab_ == 0) ? isDraggingTileSplitter_ : isDraggingEntitySplitter_;
                if (isDragging || (pt.y >= splitY - 2 && pt.y <= splitY + 8 && pt.x >= 0 && pt.x <= rc.right)) {
                    SetCursor(LoadCursor(nullptr, IDC_SIZENS));
                    return TRUE;
                }
            }
            break;
        }

        case WM_LBUTTONDOWN: {
            if (currentTab_ == 0 || currentTab_ == 1) {
                int my = GET_Y_LPARAM(lParam);
                RECT rc;
                GetClientRect(hWnd, &rc);
                int listH = (currentTab_ == 0) ? getClampedTileGridHeight(rc.bottom) : getClampedEntityGridHeight(rc.bottom);
                int splitY = 72 + listH;
                if (my >= splitY - 2 && my <= splitY + 8) {
                    if (currentTab_ == 0) {
                        isDraggingTileSplitter_ = true;
                        splitterDragStartY_ = my;
                        initialSplitterGridHeight_ = tileGridHeight_;
                    } else {
                        isDraggingEntitySplitter_ = true;
                        splitterDragStartY_ = my;
                        initialSplitterGridHeight_ = entityGridHeight_;
                    }
                    SetCapture(hWnd);
                    return 0;
                }
            }
            break;
        }

        case WM_MOUSEMOVE: {
            if (isDraggingTileSplitter_) {
                int my = GET_Y_LPARAM(lParam);
                int dy = my - splitterDragStartY_;
                RECT rc;
                GetClientRect(hWnd, &rc);
                int maxH = std::max(60, static_cast<int>(rc.bottom - 120));
                int newH = std::clamp(initialSplitterGridHeight_ + dy, 60, maxH);
                if (newH != tileGridHeight_) {
                    setTileGridHeight(newH);
                }
                return 0;
            }
            if (isDraggingEntitySplitter_) {
                int my = GET_Y_LPARAM(lParam);
                int dy = my - splitterDragStartY_;
                RECT rc;
                GetClientRect(hWnd, &rc);
                int maxH = std::max(60, static_cast<int>(rc.bottom - 120));
                int newH = std::clamp(initialSplitterGridHeight_ + dy, 60, maxH);
                if (newH != entityGridHeight_) {
                    setEntityGridHeight(newH);
                }
                return 0;
            }
            break;
        }

        case WM_LBUTTONUP: {
            if (isDraggingTileSplitter_) {
                isDraggingTileSplitter_ = false;
                ReleaseCapture();
                if (onSplitterChangedCallback_) {
                    onSplitterChangedCallback_();
                }
                return 0;
            }
            if (isDraggingEntitySplitter_) {
                isDraggingEntitySplitter_ = false;
                ReleaseCapture();
                if (onSplitterChangedCallback_) {
                    onSplitterChangedCallback_();
                }
                return 0;
            }
            break;
        }

        case WM_CANCELMODE: {
            if (isDraggingTileSplitter_) {
                isDraggingTileSplitter_ = false;
                ReleaseCapture();
            }
            if (isDraggingEntitySplitter_) {
                isDraggingEntitySplitter_ = false;
                ReleaseCapture();
            }
            break;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            int code = HIWORD(wParam);

            if (pViewport_ && pViewport_->isObjectivePicking()) {
                return 0;
            }

            if (id == IDC_COMBO_CATEGORY && code == CBN_SELCHANGE) {
                int sel = ComboBox_GetCurSel(hComboCategory_);
                currentCategory_ = static_cast<TileCategory>(sel);
                categoryTiles_ = TileRasterizer::instance().getTilesForCategory(currentCategory_);
                gridScrollY_ = 0;
                updateTileGridScroll();
                InvalidateRect(hTileGrid_, nullptr, TRUE);
                return 0;
            }

            if (id == IDC_BTN_TOOL_PENCIL) {
                activatePencilTool();
                return 0;
            }
            if (id == IDC_BTN_TOOL_BOX) {
                activateBoxFillTool();
                return 0;
            }
            if (id == IDC_BTN_TOOL_TILE_SELECT) {
                activateSelectTool();
                return 0;
            }
            if (id == IDC_BTN_TOOL_TILE_SEARCH) {
                activateSearchTool();
                return 0;
            }

            if (id == IDC_BTN_ENT_PLACE) {
                activatePencilTool();
                return 0;
            }
            if (id == IDC_BTN_ENT_SELECT) {
                activateSelectTool();
                return 0;
            }
            if (id == IDC_BTN_ENT_SEARCH) {
                activateSearchTool();
                return 0;
            }

            if (id == IDC_COMBO_ENTITY_TYPE && code == CBN_SELCHANGE) {
                int sel = ComboBox_GetCurSel(hComboEntityType_);
                entityCategory_ = sel;
                entityGridScrollY_ = 0;
                updateEntityGridScroll();
                setSelectedEntityGridIndex(0);
                InvalidateRect(hEntityGrid_, nullptr, TRUE);
                return 0;
            }

            if (isPopulatingInspector_) {
                if (id == IDC_INSPECT_ORIENT || id == IDC_INSPECT_HEALTH ||
                    id == IDC_INSPECT_WEAPONS || id == IDC_INSPECT_DRIVER ||
                    id == IDC_INSPECT_CYBERWARE || id == IDC_INSPECT_CHECK_DEAD ||
                    id == IDC_INSPECT_MOVEMENT_MODE || id == IDC_INSPECT_FACTION) {
                    return 0;
                }
            }

            if (id == IDC_INSPECT_COMBO_TYPE && code == CBN_SELCHANGE) {
                int sel = ComboBox_GetCurSel(hInspectComboType_);
                if (pSelectedEntity_) {
                    if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
                        if (pPed->type() == rs_knl::PedInstance::kPedTypeAgent) {
                            if (sel == 0 && !pPed->isOurAgent()) {
                                // Convert Enemy Agent -> Player's Agent
                                if (pMission_ && pMission_->countPlayerAgents() >= 4) {
                                    showEditorMessageBox(hWnd_,
                                        L"Cannot convert to Player's Agent:\nMap already contains 4 Player's Agents (the maximum allowed).\nDelete an existing Player's Agent first.",
                                        L"Player's Agent Limit", MB_OK | MB_ICONWARNING);
                                    ComboBox_SetCurSel(hInspectComboType_, 1);
                                    return 0;
                                }
                                int freeSlot = pMission_ ? pMission_->findNextFreePlayerSlot() : -1;
                                if (freeSlot < 0) {
                                    showEditorMessageBox(hWnd_,
                                        L"Cannot convert to Player's Agent:\nNo available squad slots (0-3).",
                                        L"Player's Agent Limit", MB_OK | MB_ICONWARNING);
                                    ComboBox_SetCurSel(hInspectComboType_, 1);
                                    return 0;
                                }
                                rs_knl::TilePoint pos = pPed->position();
                                uint8_t dir = static_cast<uint8_t>(pPed->direction());
                                pMission_->removePed(pPed);
                                auto *pNew = pMission_->spawnPlayerAgent(freeSlot, pos, dir);
                                pSelectedEntity_ = pNew;
                                if (pViewport_) {
                                    pViewport_->setSelectedEntity(pNew);
                                    InvalidateRect(pViewport_->hwnd(), nullptr, FALSE);
                                }
                                markEntityDirty();
                                refreshInspector();
                                updateEntityPreview();
                            } else if (sel == 1 && pPed->isOurAgent()) {
                                // Convert Player's Agent -> Enemy Agent
                                int targetFac = 1;
                                if (pMission_ && pMission_->countFactionAgents(targetFac) >= 8) {
                                    showEditorMessageBox(hWnd_,
                                        std::format(L"Cannot convert to Enemy Agent:\nFaction {} already has the maximum capacity of 8 agents.", targetFac).c_str(),
                                        L"Faction Capacity Limit", MB_OK | MB_ICONWARNING);
                                    ComboBox_SetCurSel(hInspectComboType_, 0);
                                    return 0;
                                }
                                rs_knl::TilePoint pos = pPed->position();
                                uint8_t dir = static_cast<uint8_t>(pPed->direction());
                                pMission_->setSquadSpawnPoint(pPed->id(), rs_knl::TilePoint(-1, -1, 0), 0);
                                pMission_->removePed(pPed);
                                auto *pNew = pMission_->spawnPed(rs_knl::PedInstance::kPedTypeAgent, pos, dir, 1, targetFac);
                                if (pNew) {
                                    pNew->setInitialLevelState(0x00);
                                    pNew->setCyberwareOverridden(false);
                                    pNew->applyCyberwareTier(pMission_->techLevel());
                                    pNew->setFactionId(targetFac);
                                }
                                pSelectedEntity_ = pNew;
                                if (pViewport_) {
                                    pViewport_->setSelectedEntity(pNew);
                                    InvalidateRect(pViewport_->hwnd(), nullptr, FALSE);
                                }
                                markEntityDirty();
                                refreshInspector();
                                updateEntityPreview();
                            }
                        }
                    }
                } else {
                    // Template mode
                    if (sel == 0) {
                        if (pMission_ && pMission_->countPlayerAgents() >= 4) {
                            showEditorMessageBox(hWnd_,
                                L"Cannot select Player's Agent:\nMap already contains 4 Player's Agents (the maximum allowed).\nDelete an existing Player's Agent first.",
                                L"Player's Agent Limit", MB_OK | MB_ICONWARNING);
                            ComboBox_SetCurSel(hInspectComboType_, 1);
                            templateAgentType_ = 1;
                            return 0;
                        }
                        templateAgentType_ = 0;
                    } else {
                        templateAgentType_ = 1;
                    }
                    applyEntityPlacementFromSelection();
                    refreshInspector();
                    updateEntityPreview();
                }
                return 0;
            }

            if (id == IDC_INSPECT_ROT_LEFT && code == BN_CLICKED) {
                rotatePreviewEntity(-1);
                return 0;
            }

            if (id == IDC_INSPECT_ROT_RIGHT && code == BN_CLICKED) {
                rotatePreviewEntity(1);
                return 0;
            }

            if (id == IDC_INSPECT_PLAY_ANIM && code == BN_CLICKED) {
                togglePreviewAnimation();
                return 0;
            }

            if (id == IDC_INSPECT_ORIENT && code == CBN_SELCHANGE) {
                int sel = ComboBox_GetCurSel(hInspectComboOrient_);
                if (sel >= 0) {
                    if (pSelectedEntity_) {
                        if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
                            pPed->setDirection(sel * 32);
                            if (pPed->isOurAgent() && pMission_) {
                                size_t targetSlot = (pPed->id() < 4) ? pPed->id() : 0;
                                pMission_->setSquadSpawnPoint(targetSlot, pPed->position(), static_cast<uint8_t>(sel * 32));
                            }
                        } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) {
                            pVeh->setDirection(sel * 64);
                        } else if (auto *pStatic = dynamic_cast<rs_knl::Static*>(pSelectedEntity_)) {
                            if (!isFixedDirectionStatic(pStatic)) {
                                pStatic->setOrientation(sel == 1 ? rs_knl::Static::kStaticOrientationEW : rs_knl::Static::kStaticOrientationNS);
                            }
                        }
                        markEntityDirty();
                        if (pViewport_) InvalidateRect(pViewport_->hwnd(), nullptr, FALSE);
                    } else {
                        const auto *pItem = selectedEntityTemplateItem();
                        if (pItem && pItem->kind == EntityKind::Static) {
                            activeOrientation_ = static_cast<uint8_t>(sel);
                        } else if (pItem && pItem->kind == EntityKind::Vehicle) {
                            activeOrientation_ = static_cast<uint8_t>(sel * 64);
                        } else {
                            activeOrientation_ = static_cast<uint8_t>(sel * 32);
                        }
                        applyEntityPlacementFromSelection();
                    }
                    updateEntityPreview();
                }
                return 0;
            }

            if (id == IDC_INSPECT_CHECK_DEAD && code == BN_CLICKED) {
                bool isDead = (SendMessageW(hInspectCheckDead_, BM_GETCHECK, 0, 0) == BST_CHECKED);
                EnableWindow(hInspectEditHealth_, isDead ? FALSE : TRUE);
                if (isDead) {
                    SetWindowTextW(hInspectEditHealth_, L"-1");
                    if (pSelectedEntity_) {
                        if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
                            pPed->setHealth(-1);
                            pPed->playDeadAnimation();
                        } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) {
                            pVeh->setHealth(-1);
                            pVeh->playAnimation(pVeh->burntAnimation());
                        } else if (auto *pStatic = dynamic_cast<rs_knl::Static*>(pSelectedEntity_)) {
                            pStatic->setHealth(-1);
                        }
                        markEntityDirty();
                        if (pViewport_) InvalidateRect(pViewport_->hwnd(), nullptr, FALSE);
                    } else {
                        applyEntityPlacementFromSelection();
                    }
                } else {
                    int restoredHp = 2;
                    if (pSelectedEntity_) {
                        if (origHealth_ > 0) {
                            restoredHp = origHealth_;
                        } else if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
                            restoredHp = (pPed->startHealth() > 0) ? pPed->startHealth()
                                : rs_knl::PedManager::getStats(pPed->type(), pPed->type() == rs_knl::PedInstance::kPedTypeAgent).health;
                        } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) {
                            if (pVeh->startHealth() > 0) {
                                restoredHp = pVeh->startHealth();
                            } else if (dynamic_cast<rs_knl::TrainHead*>(pVeh)) {
                                restoredHp = 100;
                            } else if (dynamic_cast<rs_knl::TrainBody*>(pVeh)) {
                                restoredHp = 80;
                            } else {
                                switch (pVeh->getType()) {
                                    case 0x01: restoredHp = 600; break;
                                    case 0x0D: restoredHp = 30;  break;
                                    case 0x11: restoredHp = 40;  break;
                                    case 0x1C: restoredHp = 10;  break;
                                    case 0x24: restoredHp = 120; break;
                                    case 0x28: restoredHp = 115; break;
                                    default:   restoredHp = 30;  break;
                                }
                            }
                        }
                        if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
                            pPed->setHealth(restoredHp);
                            pPed->playStandOrWalkAnimation();
                        } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) {
                            pVeh->setHealth(restoredHp);
                            pVeh->playAnimation(pVeh->regularAnimation());
                        } else if (auto *pStatic = dynamic_cast<rs_knl::Static*>(pSelectedEntity_)) {
                            pStatic->setHealth(restoredHp);
                        }
                        SetWindowTextW(hInspectEditHealth_, std::to_wstring(restoredHp).c_str());
                        markEntityDirty();
                        if (pViewport_) InvalidateRect(pViewport_->hwnd(), nullptr, FALSE);
                    } else {
                        const auto *pItem = selectedEntityTemplateItem();
                        restoredHp = pItem ? pItem->defaultHealth : 2;
                        SetWindowTextW(hInspectEditHealth_, std::to_wstring(restoredHp).c_str());
                        applyEntityPlacementFromSelection();
                    }
                }
                updateEntityPreview();
                return 0;
            }

            if (id == IDC_INSPECT_HEALTH && code == EN_CHANGE) {
                wchar_t hpBuf[64] = {};
                GetWindowTextW(hInspectEditHealth_, hpBuf, 64);
                int hp = _wtoi(hpBuf);
                if (hInspectCheckDead_) {
                    bool shouldCheck = (hp <= 0 && wcslen(hpBuf) > 0 && wcscmp(hpBuf, L"-") != 0);
                    SendMessageW(hInspectCheckDead_, BM_SETCHECK, shouldCheck ? BST_CHECKED : BST_UNCHECKED, 0);
                }
                if (pSelectedEntity_) {
                    if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
                        pPed->setHealth(hp);
                        if (hp <= 0 && wcslen(hpBuf) > 0 && wcscmp(hpBuf, L"-") != 0) {
                            pPed->playDeadAnimation();
                        } else if (hp > 0) {
                            pPed->playStandOrWalkAnimation();
                        }
                    } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) {
                        pVeh->setHealth(hp);
                        if (hp <= 0 && wcslen(hpBuf) > 0 && wcscmp(hpBuf, L"-") != 0) {
                            pVeh->playAnimation(pVeh->burntAnimation());
                        } else if (hp > 0) {
                            pVeh->playAnimation(pVeh->regularAnimation());
                        }
                    } else if (auto *pStatic = dynamic_cast<rs_knl::Static*>(pSelectedEntity_)) {
                        pStatic->setHealth(hp);
                    }
                    markEntityDirty();
                    if (pViewport_) InvalidateRect(pViewport_->hwnd(), nullptr, FALSE);
                } else {
                    applyEntityPlacementFromSelection();
                }
                updateEntityPreview();
                return 0;
            }

            if (id == IDC_INSPECT_WEAPONS && code == LBN_SELCHANGE) {
                if (pSelectedEntity_) {
                    markEntityDirty();
                } else {
                    applyEntityPlacementFromSelection();
                }
                return 0;
            }

            if (id == IDC_INSPECT_DRIVER && code == CBN_SELCHANGE) {
                if (pSelectedEntity_) {
                    markEntityDirty();
                } else {
                    activeDriverChoice_ = ComboBox_GetCurSel(hInspectComboDriver_);
                    applyEntityPlacementFromSelection();
                }
                return 0;
            }

            if (id == IDC_INSPECT_CYBERWARE && code == CBN_SELCHANGE) {
                if (pSelectedEntity_) {
                    markEntityDirty();
                } else {
                    int sel = ComboBox_GetCurSel(hInspectComboCyberware_);
                    if (sel > 0) {
                        activeCyberwareOverridden_ = true;
                        activeCyberwareTier_ = sel - 1;
                    } else {
                        activeCyberwareOverridden_ = false;
                        activeCyberwareTier_ = pMission_ ? pMission_->techLevel() : 0;
                    }
                    applyEntityPlacementFromSelection();
                }
                return 0;
            }

            if (id == IDC_INSPECT_MOVEMENT_MODE && code == CBN_SELCHANGE) {
                if (pSelectedEntity_) {
                    markEntityDirty();
                } else {
                    int sel = ComboBox_GetCurSel(hInspectComboMoveMode_);
                    activeMovementMode_ = (sel == 1) ? 0x00 : 0x10;
                    applyEntityPlacementFromSelection();
                }
                return 0;
            }

            if (id == IDC_INSPECT_FACTION && code == CBN_SELCHANGE) {
                if (pSelectedEntity_) {
                    if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
                        int sel = ComboBox_GetCurSel(hInspectComboFaction_);
                        if (sel >= 0) {
                            int targetFac = sel + 1;
                            if (pMission_ && targetFac != pPed->factionId()) {
                                if (pMission_->countFactionAgents(targetFac, pPed) >= 8) {
                                    showEditorMessageBox(hWnd_,
                                        std::format(L"Cannot assign faction:\nFaction {} already has the maximum capacity of 8 agents.", targetFac).c_str(),
                                        L"Faction Capacity Limit", MB_OK | MB_ICONWARNING);
                                    int curSel = (pPed->factionId() >= 1 && pPed->factionId() <= 7) ? (pPed->factionId() - 1) : 0;
                                    ComboBox_SetCurSel(hInspectComboFaction_, curSel);
                                    return 0;
                                }
                                int allocated = pMission_->allocateFactionSlot(targetFac, pPed);
                                if (allocated < 0) {
                                    showEditorMessageBox(hWnd_,
                                        std::format(L"Cannot assign faction:\nFaction {} has no available pedestrian slots.", targetFac).c_str(),
                                        L"Faction Assignment Failed", MB_OK | MB_ICONWARNING);
                                    int curSel = (pPed->factionId() >= 1 && pPed->factionId() <= 7) ? (pPed->factionId() - 1) : 0;
                                    ComboBox_SetCurSel(hInspectComboFaction_, curSel);
                                    return 0;
                                }
                                refreshObjectivesList();
                            }
                            pPed->setFactionId(targetFac);
                            activeFactionId_ = targetFac;
                        }
                    }
                    markEntityDirty();
                    if (pViewport_) InvalidateRect(pViewport_->hwnd(), nullptr, FALSE);
                    updateEntityPreview();
                } else {
                    int sel = ComboBox_GetCurSel(hInspectComboFaction_);
                    activeFactionId_ = (sel >= 0) ? (sel + 1) : 1;
                    applyEntityPlacementFromSelection();
                    updateEntityPreview();
                }
                return 0;
            }

            if (id == IDC_INSPECT_BTN_AI_TACTICS) {
                if (pMission_) {
                    int targetFac = 1;
                    if (pSelectedEntity_) {
                        if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
                            targetFac = pPed->factionId();
                        }
                    } else {
                        int sel = ComboBox_GetCurSel(hInspectComboFaction_);
                        targetFac = (sel >= 0 && sel < 7) ? (sel + 1) : 1;
                    }
                    EditorCommandStack *pCmd = (pViewport_ ? pViewport_->commandStack() : nullptr);
                    HWND hRoot = GetAncestor(hWnd_, GA_ROOT);
                    AiTacticsDialog::show(hRoot ? hRoot : hWnd_, pViewport_, pMission_, pCmd, targetFac);
                    if (pViewport_) pViewport_->notifyCommandExecuted();
                }
                return 0;
            }

            if (id == IDC_INSPECT_BTN_PLAYTEST_CONFIG) {
                HWND hRoot = GetAncestor(hWnd_, GA_ROOT);
                if (hRoot) {
                    SendMessageW(hRoot, WM_COMMAND, MAKEWPARAM(IDM_MISSION_LOADOUT, 0), 0);
                }
                return 0;
            }



            if (id == IDC_BTN_ADD_OBJ) {
                if (pMission_) {
                    ObjectiveDialogData d;
                    if (ObjectiveDialog::show(hWnd_, pViewport_, pMission_, d, false, -1)) {
                        if (pViewport_ && pViewport_->commandStack()) {
                            pViewport_->commandStack()->executeCommand(
                                std::make_unique<AddObjectiveCommand>(pMission_, d));
                            pViewport_->notifyCommandExecuted();
                        } else {
                            AddObjectiveCommand cmd(pMission_, d);
                            cmd.execute();
                        }
                        refreshObjectivesList();
                        ListBox_SetCurSel(hListObjectives_, static_cast<int>(pMission_->numObjectives() - 1));
                    }
                    if (pViewport_) InvalidateRect(pViewport_->hwnd(), nullptr, FALSE);
                }
                return 0;
            }

            if (id == IDC_BTN_EDIT_OBJ || (id == IDC_LIST_OBJECTIVES && code == LBN_DBLCLK)) {
                if (pMission_) {
                    int sel = ListBox_GetCurSel(hListObjectives_);
                    if (sel >= 0 && sel < static_cast<int>(pMission_->numObjectives())) {
                        rs_knl::ObjectiveDesc *cur = pMission_->objective(sel);
                        ObjectiveDialogData oldData = ObjectiveDialog::extractDataFromObjective(pMission_, cur);
                        ObjectiveDialogData newData = oldData;
                        if (ObjectiveDialog::show(hWnd_, pViewport_, pMission_, newData, true, sel)) {
                            if (pViewport_ && pViewport_->commandStack()) {
                                pViewport_->commandStack()->executeCommand(
                                    std::make_unique<EditObjectiveCommand>(pMission_, sel, oldData, newData));
                                pViewport_->notifyCommandExecuted();
                            } else {
                                EditObjectiveCommand cmd(pMission_, sel, oldData, newData);
                                cmd.execute();
                            }
                            refreshObjectivesList();
                            ListBox_SetCurSel(hListObjectives_, sel);
                        }
                    }
                    if (pViewport_) InvalidateRect(pViewport_->hwnd(), nullptr, FALSE);
                }
                return 0;
            }

            if (id == IDC_BTN_DEL_OBJ) {
                if (pMission_) {
                    int sel = ListBox_GetCurSel(hListObjectives_);
                    if (sel >= 0 && sel < static_cast<int>(pMission_->numObjectives())) {
                        rs_knl::ObjectiveDesc *cur = pMission_->objective(sel);
                        ObjectiveDialogData oldData = ObjectiveDialog::extractDataFromObjective(pMission_, cur);
                        if (pViewport_ && pViewport_->commandStack()) {
                            pViewport_->commandStack()->executeCommand(
                                std::make_unique<DeleteObjectiveCommand>(pMission_, sel, oldData));
                            pViewport_->notifyCommandExecuted();
                        } else {
                            DeleteObjectiveCommand cmd(pMission_, sel, oldData);
                            cmd.execute();
                        }
                        refreshObjectivesList();
                        int newCount = static_cast<int>(pMission_->numObjectives());
                        if (newCount > 0) {
                            int newSel = std::min(sel, newCount - 1);
                            ListBox_SetCurSel(hListObjectives_, newSel);
                        }
                        if (pViewport_) InvalidateRect(pViewport_->hwnd(), nullptr, FALSE);
                    }
                }
                return 0;
            }

            if (id == IDC_BTN_CLEAR_OBJ) {
                if (pMission_ && pMission_->numObjectives() > 0) {
                    if (showEditorMessageBox(hWnd_, L"Are you sure you want to clear all mission objectives?", L"Confirm Clear", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                        std::vector<ObjectiveDialogData> oldDataList;
                        oldDataList.reserve(pMission_->numObjectives());
                        for (size_t i = 0; i < pMission_->numObjectives(); ++i) {
                            oldDataList.push_back(ObjectiveDialog::extractDataFromObjective(pMission_, pMission_->objective(i)));
                        }
                        if (pViewport_ && pViewport_->commandStack()) {
                            pViewport_->commandStack()->executeCommand(
                                std::make_unique<ClearObjectivesCommand>(pMission_, oldDataList));
                            pViewport_->notifyCommandExecuted();
                        } else {
                            ClearObjectivesCommand cmd(pMission_, oldDataList);
                            cmd.execute();
                        }
                        refreshObjectivesList();
                        if (pViewport_) InvalidateRect(pViewport_->hwnd(), nullptr, FALSE);
                    }
                }
                return 0;
            }

            if (id == IDC_INSPECT_APPLY) {
                applyInspectorChanges();
                return 0;
            }

            if (id == IDC_INSPECT_CANCEL) {
                revertInspectorChanges();
                return 0;
            }

            if (id == IDC_INSPECT_DELETE) {
                if (pMission_ && pSelectedEntity_) {
                    auto *pObjToDelete = pSelectedEntity_;
                    pSelectedEntity_ = nullptr;
                    if (pViewport_) {
                        pViewport_->clearEntityReferences(pObjToDelete);
                    }
                    if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pObjToDelete)) {
                        if (pViewport_ && pViewport_->commandStack()) {
                            pViewport_->commandStack()->executeCommand(std::make_unique<DeletePedCommand>(pMission_, pPed));
                        } else {
                            if (pPed->isOurAgent() && pPed->id() < 4) {
                                pMission_->setSquadSpawnPoint(pPed->id(), rs_knl::TilePoint(-1, -1, 0), 0);
                            }
                            pMission_->removePed(pPed);
                        }
                    } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pObjToDelete)) {
                        if (pViewport_ && pViewport_->commandStack()) {
                            pViewport_->commandStack()->executeCommand(std::make_unique<DeleteVehicleCommand>(pMission_, pVeh));
                        } else {
                            pMission_->removeVehicle(pVeh);
                        }
                    } else if (auto *pStatic = dynamic_cast<rs_knl::Static*>(pObjToDelete)) {
                        if (pViewport_ && pViewport_->commandStack()) {
                            pViewport_->commandStack()->executeCommand(std::make_unique<DeleteStaticCommand>(pMission_, pStatic));
                        } else {
                            pMission_->removeStatic(pStatic);
                        }
                    } else if (auto *pWeapon = dynamic_cast<rs_knl::WeaponInstance*>(pObjToDelete)) {
                        if (pViewport_ && pViewport_->commandStack()) {
                            pViewport_->commandStack()->executeCommand(std::make_unique<DeleteWeaponCommand>(pMission_, pWeapon));
                        } else {
                            pMission_->removeGroundWeapon(pWeapon);
                        }
                    }
                    clearSelectedEntity();
                    refreshObjectivesList();
                    if (pViewport_) {
                        pViewport_->notifyCommandExecuted();
                        InvalidateRect(pViewport_->hwnd(), nullptr, FALSE);
                    }
                }
                return 0;
            }

            if (id == IDC_INSPECT_BTN_ROUTE) {
                if (pViewport_ && pViewport_->getActiveRoutePed()) {
                    auto *pPed = pViewport_->getActiveRoutePed();
                    if (pViewport_->tool() == ViewportTool::RouteTool) {
                        pViewport_->setTool(ViewportTool::SelectEntity);
                    } else {
                        if (pPed->scriptSteps().empty()) {
                            rs_knl::Vehicle *v0 = WinMapViewport::getPedVehicleAtStep(pMission_, pPed, 0);
                            rs_knl::TilePoint spawnPos = (v0 != nullptr) ? v0->position() : pPed->position();
                            rs_knl::ScriptStep spawnStep;
                            spawnStep.type = LevelData::kScenarioTypeGoto;
                            spawnStep.targetPos = spawnPos;
                            pPed->addScriptStep(spawnStep);
                            pPed->setScriptLooping(true);
                            if (pMission_) {
                                rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pPed);
                            }
                        }
                        pViewport_->setTool(ViewportTool::RouteTool);
                        SetFocus(pViewport_->hwnd());
                    }
                    refreshInspector();
                    InvalidateRect(pViewport_->hwnd(), nullptr, FALSE);
                    InvalidateRect(hWnd_, nullptr, FALSE);
                }
                return 0;
            }

            if (id == IDC_INSPECT_CHECK_LOOP) {
                rs_knl::PedInstance *pPed = nullptr;
                if (auto *pP = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
                    if (pP->type() != rs_knl::PedInstance::kPedTypeAgent) pPed = pP;
                } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) {
                    pPed = pVeh->getDriver();
                }
                if (pPed) {
                    bool wantLoop = (Button_GetCheck(hInspectCheckLoop_) == BST_CHECKED);
                    pPed->setScriptLooping(wantLoop);
                    rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pPed);
                    refreshRouteUI();
                    markEntityDirty();
                    if (pViewport_) InvalidateRect(pViewport_->hwnd(), nullptr, FALSE);
                }
                return 0;
            }

            if (id == IDC_INSPECT_BTN_CLEAR_ROUTE) {
                rs_knl::PedInstance *pPed = nullptr;
                if (auto *pP = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
                    if (pP->type() != rs_knl::PedInstance::kPedTypeAgent) pPed = pP;
                } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) {
                    pPed = pVeh->getDriver();
                }
                if (pPed && !pPed->scriptSteps().empty()) {
                    if (showEditorMessageBox(hWnd_, L"Are you sure you want to clear all waypoints for this route?", L"Confirm Clear Route", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                        std::vector<rs_knl::ScriptStep> newSteps;
                        pPed->setScriptSteps(newSteps);
                        rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pPed);
                        refreshRouteUI();
                        markEntityDirty();
                        if (pViewport_) InvalidateRect(pViewport_->hwnd(), nullptr, FALSE);
                    }
                }
                return 0;
            }

            if (id == IDC_INSPECT_LIST_ACTIONS) {
                if (code == LBN_SELCHANGE) {
                    rs_knl::PedInstance *pPed = nullptr;
                    if (auto *pP = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
                        if (pP->type() != rs_knl::PedInstance::kPedTypeAgent) pPed = pP;
                    } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) pPed = pVeh->getDriver();
                    int curSel = ListBox_GetCurSel(hInspectListActions_);
                    int numActions = ListBox_GetCount(hInspectListActions_);
                    EnableWindow(hInspectBtnDelAction_, (pPed && curSel > 0 && curSel < numActions) ? TRUE : FALSE);
                    EnableWindow(hInspectBtnMoveUp_, (pPed && curSel > 1 && curSel < numActions) ? TRUE : FALSE);
                    EnableWindow(hInspectBtnMoveDown_, (pPed && curSel > 0 && curSel + 1 < numActions) ? TRUE : FALSE);
                } else if (code == LBN_DBLCLK) {
                    SendMessageW(hWnd_, WM_COMMAND, MAKEWPARAM(IDC_INSPECT_BTN_ADD_ACTION, BN_CLICKED), reinterpret_cast<LPARAM>(hInspectBtnAddAction_));
                }
                return 0;
            }

            if (id == IDC_INSPECT_BTN_ADD_ACTION) {
                rs_knl::PedInstance *pPed = nullptr;
                if (auto *pP = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
                    if (pP->type() != rs_knl::PedInstance::kPedTypeAgent) pPed = pP;
                } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) {
                    pPed = pVeh->getDriver();
                }
                if (pPed) {
                    RECT btnRc = {};
                    GetWindowRect(hInspectBtnAddAction_, &btnRc);
                    HMENU hMenu = CreatePopupMenu();
                    int curSel = ListBox_GetCurSel(hInspectListActions_);

                    auto getFriendlyVehName = [&](size_t vi) -> std::wstring {
                        rs_knl::Vehicle *v = pMission_->vehicle(vi);
                        if (!v) return std::format(L"Vehicle #{}", vi);
                        const wchar_t *typeName = L"Vehicle";
                        uint8_t vt = v->getType();
                        if (vt == rs_knl::Vehicle::kVehicleTypeRegularCar) typeName = L"Regular Car";
                        else if (vt == rs_knl::Vehicle::kVehicleTypeSmallArmored) typeName = L"Armored Car";
                        else if (vt == rs_knl::Vehicle::kVehicleTypeLargeArmored) typeName = L"Large Armored";
                        else if (vt == rs_knl::Vehicle::kVehicleTypePolice) typeName = L"Police Car";
                        else if (vt == rs_knl::Vehicle::kVehicleTypeFireFighter) typeName = L"Firetruck";
                        else if (vt == rs_knl::Vehicle::kVehicleTypeMedics) typeName = L"Ambulance";
                        else if (vt == rs_knl::Vehicle::kVehicleTypeTrainHead) typeName = L"Train Engine";
                        else if (vt == rs_knl::Vehicle::kVehicleTypeTrainBody) typeName = L"Train Carriage";
                        return std::format(L"Vehicle #{:03} [{}]", v->id(), typeName);
                    };

                    if (curSel >= 0 && curSel < static_cast<int>(pPed->scriptSteps().size())) {
                        std::wstring hdr = std::format(L"-- CURRENT STEP #{} ACTION --", curSel + 1);
                        AppendMenuW(hMenu, MF_STRING | MF_DISABLED, 999, hdr.c_str());
                        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

                        UINT uGoto = (pPed->scriptSteps()[curSel].type == LevelData::kScenarioTypeGoto) ? MF_CHECKED : MF_UNCHECKED;
                        rs_knl::Vehicle *vAtSel = WinMapViewport::getPedVehicleAtStep(pMission_, pPed, curSel);
                        std::wstring gotoMenuText = L"Goto (Walk)";
                        if (vAtSel) {
                            if (vAtSel->isCar()) gotoMenuText = std::format(L"Drive Car #{:03}", vAtSel->id());
                            else if (vAtSel->getType() == rs_knl::Vehicle::kVehicleTypeTrainHead || vAtSel->getType() == rs_knl::Vehicle::kVehicleTypeTrainBody)
                                gotoMenuText = std::format(L"Drive Train #{:03}", vAtSel->id());
                            else gotoMenuText = std::format(L"Drive Vehicle #{:03}", vAtSel->id());
                        }
                        AppendMenuW(hMenu, MF_STRING | uGoto, 1005, gotoMenuText.c_str());

                        UINT uWait5 = (pPed->scriptSteps()[curSel].type == LevelData::kScenarioTypeWait && pPed->scriptSteps()[curSel].param == 5000) ? MF_CHECKED : MF_UNCHECKED;
                        AppendMenuW(hMenu, MF_STRING | uWait5, 1002, L"Wait 5s (Pause here)");

                        UINT uWait10 = (pPed->scriptSteps()[curSel].type == LevelData::kScenarioTypeWait && pPed->scriptSteps()[curSel].param == 10000) ? MF_CHECKED : MF_UNCHECKED;
                        AppendMenuW(hMenu, MF_STRING | uWait10, 1003, L"Wait 10s (Pause here)");

                        int dwellTime = rs_knl::VehicleManager::getStats(rs_knl::Vehicle::kVehicleTypeTrainHead).dwell_time;
                        std::wstring dwellLabel = std::format(L"Train Station Dwell ({}s)", dwellTime / 1000);
                        UINT uTrainWait = (pPed->scriptSteps()[curSel].type == LevelData::kScenarioTypeTrainWait) ? MF_CHECKED : MF_UNCHECKED;
                        AppendMenuW(hMenu, MF_STRING | uTrainWait, 1009, dwellLabel.c_str());

                        UINT uTrig = (pPed->scriptSteps()[curSel].type == LevelData::kScenarioTypeTrigger) ? MF_CHECKED : MF_UNCHECKED;
                        AppendMenuW(hMenu, MF_STRING | uTrig, 1004, L"Proximity Ambush Trigger (6 tiles)");

                        HMENU hRadMenu = CreatePopupMenu();
                        static const int kRadii[] = { 2, 4, 6, 8, 10, 12, 16 };
                        int curRadTiles = (pPed->scriptSteps()[curSel].param > 0) ? static_cast<int>(pPed->scriptSteps()[curSel].param / 256) : 6;
                        for (int r : kRadii) {
                            std::wstring rLabel = std::format(L"{} Tiles ({} units{})", r, r * 256, (r == 6) ? L" - Default" : L"");
                            UINT uRadFlags = (pPed->scriptSteps()[curSel].type == LevelData::kScenarioTypeTrigger && curRadTiles == r) ? MF_CHECKED : MF_UNCHECKED;
                            AppendMenuW(hRadMenu, MF_STRING | uRadFlags, 1300 + r, rLabel.c_str());
                        }
                        AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hRadMenu), L"Ambush Trigger Radius");

                        UINT uLeave = (pPed->scriptSteps()[curSel].type == LevelData::kScenarioTypeLeaveVehicle) ? MF_CHECKED : MF_UNCHECKED;
                        AppendMenuW(hMenu, MF_STRING | uLeave, 1007, L"Unboard / Exit Vehicle");

                        UINT uEsc = (pPed->scriptSteps()[curSel].type == LevelData::kScenarioTypeEscape) ? MF_CHECKED : MF_UNCHECKED;
                        AppendMenuW(hMenu, MF_STRING | uEsc, 1008, L"Target Escape / Evacuate");

                        if (pMission_ && pMission_->numVehicles() > 0) {
                            HMENU hVehMenu = CreatePopupMenu();
                            for (size_t vi = 0; vi < pMission_->numVehicles() && vi < 30; ++vi) {
                                std::wstring vName = getFriendlyVehName(vi);
                                UINT uVehFlags = (pPed->scriptSteps()[curSel].type == LevelData::kScenarioTypeUseVehicle &&
                                                  pPed->scriptSteps()[curSel].targetEntityId == static_cast<int>(vi)) ? MF_CHECKED : MF_UNCHECKED;
                                AppendMenuW(hVehMenu, MF_STRING | uVehFlags, 1100 + static_cast<UINT>(vi), vName.c_str());
                            }
                            AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hVehMenu), L"Board Vehicle");
                        }
                        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
                    }

                    AppendMenuW(hMenu, MF_STRING | MF_DISABLED, 998, L"-- ADD ACTION TO ROUTE --");
                    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

                    rs_knl::Vehicle *vAtEnd = WinMapViewport::getPedVehicleAtStep(pMission_, pPed, pPed->scriptSteps().size());
                    std::wstring addGotoText = L"+ Add Goto at Ped Position (Walk)";
                    if (vAtEnd) {
                        if (vAtEnd->isCar()) addGotoText = std::format(L"+ Add Drive Car #{:03} at Current Pos", vAtEnd->id());
                        else if (vAtEnd->getType() == rs_knl::Vehicle::kVehicleTypeTrainHead || vAtEnd->getType() == rs_knl::Vehicle::kVehicleTypeTrainBody)
                            addGotoText = std::format(L"+ Add Drive Train #{:03} at Current Pos", vAtEnd->id());
                        else addGotoText = std::format(L"+ Add Drive Vehicle #{:03} at Current Pos", vAtEnd->id());
                    }
                    AppendMenuW(hMenu, MF_STRING, 1020, addGotoText.c_str());
                    AppendMenuW(hMenu, MF_STRING, 1025, L"+ Add Ambush Trigger (6 tiles)");

                    HMENU hAddRadMenu = CreatePopupMenu();
                    static const int kRadii[] = { 2, 4, 6, 8, 10, 12, 16 };
                    for (int r : kRadii) {
                        std::wstring rLabel = std::format(L"+ Add Ambush Trigger ({} tiles)", r);
                        AppendMenuW(hAddRadMenu, MF_STRING, 1320 + r, rLabel.c_str());
                    }
                    AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hAddRadMenu), L"+ Add Ambush Trigger with Radius...");

                    AppendMenuW(hMenu, MF_STRING, 1021, L"+ Add Wait 5s (Pause here)");
                    AppendMenuW(hMenu, MF_STRING, 1022, L"+ Add Wait 10s (Pause here)");
                    AppendMenuW(hMenu, MF_STRING, 1026, L"+ Add Train Station Dwell (10s)");
                    AppendMenuW(hMenu, MF_STRING, 1023, L"+ Add Unboard / Exit Vehicle");
                    AppendMenuW(hMenu, MF_STRING, 1024, L"+ Add Target Escape / Evacuate");

                    if (pMission_ && pMission_->numVehicles() > 0) {
                        HMENU hAddVehMenu = CreatePopupMenu();
                        for (size_t vi = 0; vi < pMission_->numVehicles() && vi < 30; ++vi) {
                            std::wstring vName = getFriendlyVehName(vi);
                            AppendMenuW(hAddVehMenu, MF_STRING, 1200 + static_cast<UINT>(vi), vName.c_str());
                        }
                        AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hAddVehMenu), L"+ Add Board Vehicle");
                    }

                    int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_BOTTOMALIGN, btnRc.left, btnRc.top, 0, hWnd_, nullptr);
                    DestroyMenu(hMenu);

                    if (cmd > 0) {
                        auto oldSteps = pPed->scriptSteps();
                        auto newSteps = oldSteps;
                        bool wasLoop = pPed->isScriptLooping();
                        if (!newSteps.empty() && newSteps.back().type == LevelData::kScenarioTypeReset) {
                            newSteps.pop_back();
                        }

                        if (newSteps.empty() && cmd >= 1020) {
                            rs_knl::Vehicle *v0 = WinMapViewport::getPedVehicleAtStep(pMission_, pPed, 0);
                            rs_knl::TilePoint spawnPos = (v0 != nullptr) ? v0->position() : pPed->position();
                            rs_knl::ScriptStep spawnStep;
                            spawnStep.type = LevelData::kScenarioTypeGoto;
                            spawnStep.targetPos = spawnPos;
                            newSteps.push_back(spawnStep);
                        }

                        if (cmd == 1020) {
                            rs_knl::ScriptStep step;
                            step.type = LevelData::kScenarioTypeGoto;
                            step.targetPos = pPed->position();
                            step.param = 0;
                            step.targetEntityId = -1;
                            newSteps.push_back(step);
                        } else if (cmd == 1021) {
                            rs_knl::ScriptStep step;
                            step.type = LevelData::kScenarioTypeWait;
                            step.targetPos = rs_knl::TilePoint(0, 0, 0);
                            step.param = 5000;
                            step.targetEntityId = -1;
                            newSteps.push_back(step);
                        } else if (cmd == 1022) {
                            rs_knl::ScriptStep step;
                            step.type = LevelData::kScenarioTypeWait;
                            step.targetPos = rs_knl::TilePoint(0, 0, 0);
                            step.param = 10000;
                            step.targetEntityId = -1;
                            newSteps.push_back(step);
                        } else if (cmd == 1026) {
                            rs_knl::ScriptStep step;
                            step.type = LevelData::kScenarioTypeTrainWait;
                            step.targetPos = rs_knl::TilePoint(0, 0, 0);
                            step.param = rs_knl::VehicleManager::getStats(rs_knl::Vehicle::kVehicleTypeTrainHead).dwell_time;
                            step.targetEntityId = -1;
                            newSteps.push_back(step);
                        } else if (cmd == 1023) {
                            rs_knl::ScriptStep step;
                            step.type = LevelData::kScenarioTypeLeaveVehicle;
                            step.targetPos = rs_knl::TilePoint(0, 0, 0);
                            step.param = 0;
                            step.targetEntityId = -1;
                            newSteps.push_back(step);
                        } else if (cmd == 1024) {
                            rs_knl::ScriptStep step;
                            step.type = LevelData::kScenarioTypeEscape;
                            step.targetPos = rs_knl::TilePoint(0, 0, 0);
                            step.param = 0;
                            step.targetEntityId = -1;
                            newSteps.push_back(step);
                        } else if (cmd == 1025) {
                            rs_knl::ScriptStep step;
                            step.type = LevelData::kScenarioTypeTrigger;
                            step.targetPos = pPed->position();
                            step.param = 6 * 256;
                            step.targetEntityId = -1;
                            newSteps.push_back(step);
                        } else if (cmd >= 1321 && cmd <= 1340) {
                            int rTiles = cmd - 1320;
                            rs_knl::ScriptStep step;
                            step.type = LevelData::kScenarioTypeTrigger;
                            step.targetPos = pPed->position();
                            step.param = rTiles * 256;
                            step.targetEntityId = -1;
                            newSteps.push_back(step);
                        } else if (cmd >= 1200 && cmd < 1250) {
                            rs_knl::ScriptStep step;
                            step.type = LevelData::kScenarioTypeUseVehicle;
                            step.targetPos = rs_knl::TilePoint(0, 0, 0);
                            step.param = 0;
                            step.targetEntityId = cmd - 1200;
                            newSteps.push_back(step);
                        } else if (curSel >= 0 && curSel < static_cast<int>(newSteps.size())) {
                            if (cmd == 1005) {
                                newSteps[curSel].type = LevelData::kScenarioTypeGoto;
                                newSteps[curSel].param = 0;
                                if (newSteps[curSel].targetPos.tx == 0 && newSteps[curSel].targetPos.ty == 0) {
                                    newSteps[curSel].targetPos = pPed->position();
                                }
                            } else if (cmd == 1002) {
                                newSteps[curSel].type = LevelData::kScenarioTypeWait;
                                newSteps[curSel].param = 5000;
                                newSteps[curSel].targetPos = rs_knl::TilePoint(0, 0, 0);
                            } else if (cmd == 1003) {
                                newSteps[curSel].type = LevelData::kScenarioTypeWait;
                                newSteps[curSel].param = 10000;
                                newSteps[curSel].targetPos = rs_knl::TilePoint(0, 0, 0);
                            } else if (cmd == 1009) {
                                newSteps[curSel].type = LevelData::kScenarioTypeTrainWait;
                                newSteps[curSel].param = rs_knl::VehicleManager::getStats(rs_knl::Vehicle::kVehicleTypeTrainHead).dwell_time;
                                newSteps[curSel].targetPos = rs_knl::TilePoint(0, 0, 0);
                            } else if (cmd == 1004) {
                                newSteps[curSel].type = LevelData::kScenarioTypeTrigger;
                                if (newSteps[curSel].param == 0) newSteps[curSel].param = 6 * 256;
                                if (newSteps[curSel].targetPos.tx == 0 && newSteps[curSel].targetPos.ty == 0) {
                                    newSteps[curSel].targetPos = pPed->position();
                                }
                            } else if (cmd >= 1301 && cmd <= 1320) {
                                int rTiles = cmd - 1300;
                                newSteps[curSel].type = LevelData::kScenarioTypeTrigger;
                                newSteps[curSel].param = rTiles * 256;
                                if (newSteps[curSel].targetPos.tx == 0 && newSteps[curSel].targetPos.ty == 0) {
                                    newSteps[curSel].targetPos = pPed->position();
                                }
                            } else if (cmd == 1007) {
                                newSteps[curSel].type = LevelData::kScenarioTypeLeaveVehicle;
                                newSteps[curSel].param = 0;
                                newSteps[curSel].targetPos = rs_knl::TilePoint(0, 0, 0);
                            } else if (cmd == 1008) {
                                newSteps[curSel].type = LevelData::kScenarioTypeEscape;
                                newSteps[curSel].param = 0;
                            } else if (cmd >= 1100 && cmd < 1150) {
                                newSteps[curSel].type = LevelData::kScenarioTypeUseVehicle;
                                newSteps[curSel].targetEntityId = cmd - 1100;
                                newSteps[curSel].targetPos = rs_knl::TilePoint(0, 0, 0);
                            }
                        }

                        pPed->setScriptSteps(newSteps);
                        pPed->setScriptLooping(wasLoop);
                        rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pPed);
                        refreshRouteUI();
                        markEntityDirty();
                        if (pViewport_) InvalidateRect(pViewport_->hwnd(), nullptr, FALSE);
                    }
                }
                return 0;
            }

            if (id == IDC_INSPECT_BTN_DEL_ACTION) {
                rs_knl::PedInstance *pPed = nullptr;
                if (auto *pP = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
                    if (pP->type() != rs_knl::PedInstance::kPedTypeAgent) pPed = pP;
                } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) pPed = pVeh->getDriver();
                if (pPed) {
                    int curSel = ListBox_GetCurSel(hInspectListActions_);
                    int numActions = ListBox_GetCount(hInspectListActions_);
                    if (curSel > 0 && curSel < numActions) {
                        bool wasLoop = pPed->isScriptLooping();
                        auto newSteps = pPed->scriptSteps();
                        if (!newSteps.empty() && newSteps.back().type == LevelData::kScenarioTypeReset) {
                            newSteps.pop_back();
                        }
                        if (curSel < static_cast<int>(newSteps.size())) {
                            newSteps.erase(newSteps.begin() + curSel);
                        }
                        pPed->setScriptSteps(newSteps);
                        pPed->setScriptLooping(wasLoop);
                        rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pPed);
                        refreshRouteUI();
                        markEntityDirty();
                        if (pViewport_) InvalidateRect(pViewport_->hwnd(), nullptr, FALSE);
                    }
                }
                return 0;
            }

            if (id == IDC_INSPECT_BTN_MOVE_UP) {
                rs_knl::PedInstance *pPed = nullptr;
                if (auto *pP = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
                    if (pP->type() != rs_knl::PedInstance::kPedTypeAgent) pPed = pP;
                } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) pPed = pVeh->getDriver();
                if (pPed) {
                    int curSel = ListBox_GetCurSel(hInspectListActions_);
                    int numActions = ListBox_GetCount(hInspectListActions_);
                    if (curSel > 1 && curSel < numActions) {
                        bool wasLoop = pPed->isScriptLooping();
                        auto newSteps = pPed->scriptSteps();
                        if (!newSteps.empty() && newSteps.back().type == LevelData::kScenarioTypeReset) {
                            newSteps.pop_back();
                        }
                        if (curSel < static_cast<int>(newSteps.size())) {
                            std::swap(newSteps[curSel], newSteps[curSel - 1]);
                        }
                        pPed->setScriptSteps(newSteps);
                        pPed->setScriptLooping(wasLoop);
                        rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pPed);
                        refreshRouteUI();
                        ListBox_SetCurSel(hInspectListActions_, curSel - 1);
                        markEntityDirty();
                        if (pViewport_) InvalidateRect(pViewport_->hwnd(), nullptr, FALSE);
                    }
                }
                return 0;
            }

            if (id == IDC_INSPECT_BTN_MOVE_DOWN) {
                rs_knl::PedInstance *pPed = nullptr;
                if (auto *pP = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
                    if (pP->type() != rs_knl::PedInstance::kPedTypeAgent) pPed = pP;
                } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) pPed = pVeh->getDriver();
                if (pPed) {
                    int curSel = ListBox_GetCurSel(hInspectListActions_);
                    int numActions = ListBox_GetCount(hInspectListActions_);
                    if (curSel > 0 && curSel + 1 < numActions) {
                        bool wasLoop = pPed->isScriptLooping();
                        auto newSteps = pPed->scriptSteps();
                        if (!newSteps.empty() && newSteps.back().type == LevelData::kScenarioTypeReset) {
                            newSteps.pop_back();
                        }
                        if (curSel + 1 < static_cast<int>(newSteps.size())) {
                            std::swap(newSteps[curSel], newSteps[curSel + 1]);
                        }
                        pPed->setScriptSteps(newSteps);
                        pPed->setScriptLooping(wasLoop);
                        rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pPed);
                        refreshRouteUI();
                        ListBox_SetCurSel(hInspectListActions_, curSel + 1);
                        markEntityDirty();
                        if (pViewport_) InvalidateRect(pViewport_->hwnd(), nullptr, FALSE);
                    }
                }
                return 0;
            }
            break;
        }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK WinArtPanel::TileGridProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WinArtPanel *pPanel = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        pPanel = reinterpret_cast<WinArtPanel*>(cs->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pPanel));
    } else {
        pPanel = reinterpret_cast<WinArtPanel*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (pPanel) {
        return pPanel->handleTileGridMessage(hWnd, msg, wParam, lParam);
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT WinArtPanel::handleTileGridMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            drawTileGrid(hdc);
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_SIZE: {
            updateTileGridScroll();
            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        }

        case WM_VSCROLL: {
            int action = LOWORD(wParam);
            int curPos = gridScrollY_;
            int cellH = 54;
            RECT rc = {};
            GetClientRect(hWnd, &rc);
            int pageH = rc.bottom > 0 ? rc.bottom : cellH * 4;
            switch (action) {
                case SB_LINEUP: curPos -= cellH; break;
                case SB_LINEDOWN: curPos += cellH; break;
                case SB_PAGEUP: curPos -= pageH; break;
                case SB_PAGEDOWN: curPos += pageH; break;
                case SB_THUMBTRACK:
                case SB_THUMBPOSITION: {
                    SCROLLINFO trackSi = { sizeof(SCROLLINFO), SIF_TRACKPOS };
                    if (GetScrollInfo(hWnd, SB_VERT, &trackSi)) {
                        curPos = trackSi.nTrackPos;
                    } else {
                        curPos = HIWORD(wParam);
                    }
                    break;
                }
            }
            SCROLLINFO si = { sizeof(SCROLLINFO), SIF_RANGE | SIF_PAGE };
            GetScrollInfo(hWnd, SB_VERT, &si);
            int maxPos = std::max(0, si.nMax - static_cast<int>(si.nPage));
            gridScrollY_ = std::clamp(curPos, 0, maxPos);
            SetScrollPos(hWnd, SB_VERT, gridScrollY_, TRUE);
            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        }

        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            int scroll = (delta > 0) ? -54 : 54;
            SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(scroll < 0 ? SB_LINEUP : SB_LINEDOWN, 0), 0);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            SetFocus(hWnd);
            int cols = getTileGridColumns();
            int mx = GET_X_LPARAM(lParam) - 4;
            int my = GET_Y_LPARAM(lParam) + gridScrollY_ - 4;
            if (mx >= 0 && my >= 0) {
                int col = mx / 72;
                int row = my / 54;
                if (col >= 0 && col < cols) {
                    size_t idx = static_cast<size_t>(row) * cols + col;
                    if (idx < categoryTiles_.size()) {
                        uint8_t tid = categoryTiles_[idx];
                        setActiveTileId(tid);
                        if (isTileSearchActive_) {
                            performTileSearch(tid);
                        } else {
                            if (pViewport_) {
                                                        ViewportTool curTool = pViewport_->tool();
                                if (curTool != ViewportTool::Pencil && curTool != ViewportTool::BoxFill) {
                                    pViewport_->setTool(ViewportTool::Pencil);
                                }
                            }
                            clearSelectedEntity();
                        }
                        InvalidateRect(hWnd_, nullptr, FALSE);
                    }
                }
            }
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void WinArtPanel::drawTileGrid(HDC hdc) {
    RECT rc;
    GetClientRect(hTileGrid_, &rc);
    int w = rc.right;
    int h = rc.bottom;
    if (w <= 0 || h <= 0) return;

    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hBmpMem = CreateCompatibleBitmap(hdc, w, h);
    HBITMAP hOldBmp = static_cast<HBITMAP>(SelectObject(hdcMem, hBmpMem));

    HBRUSH hBgBrush = CreateSolidBrush(RGB(18, 22, 18));
    FillRect(hdcMem, &rc, hBgBrush);
    DeleteObject(hBgBrush);

    HDC hdcThumb = CreateCompatibleDC(hdcMem);

    HPEN hPenSel = CreatePen(PS_SOLID, 2, RGB(255, 220, 40)); // Bright yellow for selected
    HPEN hPenNormal = CreatePen(PS_SOLID, 1, RGB(40, 70, 40)); // Dark green cell border
    HFONT hTagFont = CreateFontW(
        -10, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    HFONT hOldFont = static_cast<HFONT>(SelectObject(hdcMem, hTagFont ? hTagFont : GetStockObject(DEFAULT_GUI_FONT)));
    SetBkMode(hdcMem, TRANSPARENT);

    int cellW = 72;
    int cellH = 54;
    int cols = getTileGridColumns();

    for (size_t i = 0; i < categoryTiles_.size(); ++i) {
        int col = static_cast<int>(i % cols);
        int row = static_cast<int>(i / cols);
        int cellX = 4 + col * cellW;
        int cellY = 4 + row * cellH - gridScrollY_;

        if (cellY + cellH < 0 || cellY > h) continue;

        uint8_t tid = categoryTiles_[i];
        bool isSel = (tid == activeTileId_);

        // Cell border
        SelectObject(hdcMem, isSel ? hPenSel : hPenNormal);
        Rectangle(hdcMem, cellX, cellY, cellX + 66, cellY + 50);

        // Thumbnail
        const WinTile &tile = TileRasterizer::instance().getTile(tid);
        if (tile.hThumb) {
            HBITMAP hOldThumb = static_cast<HBITMAP>(SelectObject(hdcThumb, tile.hThumb));
            BitBlt(hdcMem, cellX + 1, cellY + 1, 64, 48, hdcThumb, 0, 0, SRCCOPY);
            SelectObject(hdcThumb, hOldThumb);
        }

        // Compact unique Tile ID badge (#000-#255) in top-left corner
        RECT rcTag = { cellX + 2, cellY + 2, cellX + 28, cellY + 14 };
        HBRUSH hTagBg = CreateSolidBrush(isSel ? RGB(52, 46, 12) : RGB(14, 20, 16));
        FillRect(hdcMem, &rcTag, hTagBg);
        DeleteObject(hTagBg);
        SetTextColor(hdcMem, isSel ? RGB(255, 225, 60) : RGB(180, 210, 185));
        std::wstring idTag = std::format(L"#{:03}", tid);
        DrawTextW(hdcMem, idTag.c_str(), -1, &rcTag, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }

    SelectObject(hdcMem, hOldFont);
    if (hTagFont) DeleteObject(hTagFont);
    DeleteObject(hPenSel);
    DeleteObject(hPenNormal);
    DeleteDC(hdcThumb);

    // Clean 1px dark border around the tile grid
    HPEN hGridBorder = CreatePen(PS_SOLID, 1, RGB(58, 65, 74));
    HPEN hOldP = static_cast<HPEN>(SelectObject(hdcMem, hGridBorder));
    HBRUSH hOldB = static_cast<HBRUSH>(SelectObject(hdcMem, GetStockObject(NULL_BRUSH)));
    Rectangle(hdcMem, 0, 0, w, h);
    SelectObject(hdcMem, hOldB);
    SelectObject(hdcMem, hOldP);
    DeleteObject(hGridBorder);

    BitBlt(hdc, 0, 0, w, h, hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hBmpMem);
    DeleteDC(hdcMem);
}

void WinArtPanel::updateToolSelection() {
    if (hBtnPencil_) InvalidateRect(hBtnPencil_, nullptr, FALSE);
    if (hBtnBoxFill_) InvalidateRect(hBtnBoxFill_, nullptr, FALSE);
    if (hBtnTileSelect_) InvalidateRect(hBtnTileSelect_, nullptr, FALSE);
    if (hBtnTileSearch_) InvalidateRect(hBtnTileSearch_, nullptr, FALSE);
    if (hBtnEntPlace_) InvalidateRect(hBtnEntPlace_, nullptr, FALSE);
    if (hBtnEntSelect_) InvalidateRect(hBtnEntSelect_, nullptr, FALSE);
    if (hBtnEntSearch_) InvalidateRect(hBtnEntSearch_, nullptr, FALSE);
    refreshTileInspector();
    refreshRouteUI();
    if (hInspectBtnEditRoute_) InvalidateRect(hInspectBtnEditRoute_, nullptr, TRUE);
}

int WinArtPanel::getEntityGridColumns() const {
    if (!hEntityGrid_) return 4;
    RECT rc = {};
    GetClientRect(hEntityGrid_, &rc);
    int availW = rc.right;
    int marginX = 4;
    int cellWidth = 66;
    int cellSpacingX = 72;
    if (availW < marginX + cellWidth) return 1;
    int cols = 1 + (availW - marginX - cellWidth) / cellSpacingX;
    return std::max(1, cols);
}

void WinArtPanel::updateEntityGridScroll() {
    if (!hEntityGrid_) return;

    const auto &items = getEntityItemsForCategory(entityCategory_);
    int totalCount = static_cast<int>(items.size());

    RECT rc = {};
    GetClientRect(hEntityGrid_, &rc);
    int cols = getEntityGridColumns();
    int cellH = 54;
    int totalRows = (totalCount + cols - 1) / cols;
    int contentH = totalRows * cellH + 8;

    SCROLLINFO si = {};
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = contentH > 0 ? contentH : 1;
    si.nPage = rc.bottom > 0 ? rc.bottom : 1;
    int maxPos = std::max(0, contentH - static_cast<int>(rc.bottom));
    if (entityGridScrollY_ > maxPos) {
        entityGridScrollY_ = maxPos;
    }
    si.nPos = entityGridScrollY_;
    SetScrollInfo(hEntityGrid_, SB_VERT, &si, TRUE);
}

LRESULT CALLBACK WinArtPanel::EntityGridProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WinArtPanel *pPanel = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        pPanel = reinterpret_cast<WinArtPanel*>(cs->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pPanel));
    } else {
        pPanel = reinterpret_cast<WinArtPanel*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (pPanel) {
        return pPanel->handleEntityGridMessage(hWnd, msg, wParam, lParam);
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT WinArtPanel::handleEntityGridMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            drawEntityGrid(hdc);
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_SIZE: {
            updateEntityGridScroll();
            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        }

        case WM_VSCROLL: {
            int action = LOWORD(wParam);
            int curPos = entityGridScrollY_;
            int cellH = 54;
            RECT rc = {};
            GetClientRect(hWnd, &rc);
            int pageH = rc.bottom > 0 ? rc.bottom : cellH * 4;
            switch (action) {
                case SB_LINEUP: curPos -= cellH; break;
                case SB_LINEDOWN: curPos += cellH; break;
                case SB_PAGEUP: curPos -= pageH; break;
                case SB_PAGEDOWN: curPos += pageH; break;
                case SB_THUMBTRACK:
                case SB_THUMBPOSITION: {
                    SCROLLINFO trackSi = { sizeof(SCROLLINFO), SIF_TRACKPOS };
                    if (GetScrollInfo(hWnd, SB_VERT, &trackSi)) {
                        curPos = trackSi.nTrackPos;
                    } else {
                        curPos = HIWORD(wParam);
                    }
                    break;
                }
            }
            SCROLLINFO si = { sizeof(SCROLLINFO), SIF_RANGE | SIF_PAGE };
            GetScrollInfo(hWnd, SB_VERT, &si);
            int maxPos = std::max(0, si.nMax - static_cast<int>(si.nPage));
            entityGridScrollY_ = std::clamp(curPos, 0, maxPos);
            SetScrollPos(hWnd, SB_VERT, entityGridScrollY_, TRUE);
            InvalidateRect(hWnd, nullptr, FALSE);
            return 0;
        }

        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            int scroll = (delta > 0) ? -54 : 54;
            SendMessage(hWnd, WM_VSCROLL, MAKEWPARAM(scroll < 0 ? SB_LINEUP : SB_LINEDOWN, 0), 0);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            SetFocus(hWnd);
            const auto &items = getEntityItemsForCategory(entityCategory_);
            if (items.empty()) return 0;

            int cols = getEntityGridColumns();
            int mx = GET_X_LPARAM(lParam) - 4;
            int my = GET_Y_LPARAM(lParam) + entityGridScrollY_ - 4;
            if (mx >= 0 && my >= 0) {
                int col = mx / 72;
                int row = my / 54;
                if (col >= 0 && col < cols) {
                    size_t idx = static_cast<size_t>(row) * cols + col;
                    if (idx < items.size()) {
                        if (isEntitySearchActive_) {
                            setSelectedEntityGridIndex(static_cast<int>(idx));
                            performEntitySearch(static_cast<int>(idx));
                        } else {
                            if (pViewport_) {
                                                        const auto *pItem = items[idx];
                                ViewportTool placeTool = ViewportTool::PlaceActor;
                                if (pItem) {
                                    if (pItem->kind == EntityKind::Actor) placeTool = ViewportTool::PlaceActor;
                                    else if (pItem->kind == EntityKind::Vehicle) placeTool = ViewportTool::PlaceVehicle;
                                    else if (pItem->kind == EntityKind::Static) placeTool = ViewportTool::PlaceStatic;
                                    else if (pItem->kind == EntityKind::Weapon) placeTool = ViewportTool::PlaceWeapon;
                                }
                                pViewport_->setTool(placeTool);
                            }
                            clearSelectedEntity();
                            setSelectedEntityGridIndex(static_cast<int>(idx));
                        }
                        InvalidateRect(hWnd_, nullptr, FALSE);
                    }
                }
            }
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void WinArtPanel::drawEntityGrid(HDC hdc) {
    RECT rc;
    GetClientRect(hEntityGrid_, &rc);
    int w = rc.right;
    int h = rc.bottom;
    if (w <= 0 || h <= 0) return;

    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hBmpMem = CreateCompatibleBitmap(hdc, w, h);
    HBITMAP hOldBmp = static_cast<HBITMAP>(SelectObject(hdcMem, hBmpMem));

    HBRUSH hBgBrush = CreateSolidBrush(RGB(18, 20, 24));
    FillRect(hdcMem, &rc, hBgBrush);
    DeleteObject(hBgBrush);

    HFONT hOldFont = static_cast<HFONT>(SelectObject(hdcMem, hBoldFont_ ? hBoldFont_ : GetStockObject(DEFAULT_GUI_FONT)));
    SetBkMode(hdcMem, TRANSPARENT);

    HDC hdcThumb = CreateCompatibleDC(hdcMem);

    HPEN hPenSel = CreatePen(PS_SOLID, 2, RGB(0, 229, 255)); // Cyan for selected entity
    HPEN hPenNormal = CreatePen(PS_SOLID, 1, RGB(45, 52, 60)); // Dark slate border

    int cellW = 72;
    int cellH = 54;
    int cols = getEntityGridColumns();

    const auto &items = getEntityItemsForCategory(entityCategory_);
    for (size_t i = 0; i < items.size(); ++i) {
        int col = static_cast<int>(i % cols);
        int row = static_cast<int>(i / cols);
        int cellX = 4 + col * cellW;
        int cellY = 4 + row * cellH - entityGridScrollY_;

        if (cellY + cellH < 0 || cellY > h) continue;

        const auto &item = *items[i];
        bool isSel = (static_cast<int>(i) == selectedEntityGridIdx_);

            // Cell border
            SelectObject(hdcMem, isSel ? hPenSel : hPenNormal);
            Rectangle(hdcMem, cellX, cellY, cellX + 66, cellY + 50);

            // Thumbnail
            if (item.hThumb) {
                HBITMAP hOldThumb = static_cast<HBITMAP>(SelectObject(hdcThumb, item.hThumb));
                BitBlt(hdcMem, cellX + 1, cellY + 1, 64, 46, hdcThumb, 0, 0, SRCCOPY);
                SelectObject(hdcThumb, hOldThumb);
            }

            // Lower-left label banner
            RECT rcBanner = { cellX + 1, cellY + 34, cellX + 65, cellY + 49 };
            HBRUSH hBannerBr = CreateSolidBrush(isSel ? RGB(10, 45, 60) : RGB(14, 16, 20));
            FillRect(hdcMem, &rcBanner, hBannerBr);
            DeleteObject(hBannerBr);

            // Label text in lower-left corner
            SetTextColor(hdcMem, isSel ? RGB(0, 255, 255) : RGB(210, 220, 230));
            RECT rcText = { cellX + 3, cellY + 34, cellX + 63, cellY + 49 };
            DrawTextW(hdcMem, item.shortLabel.c_str(), -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        }

    DeleteObject(hPenSel);
    DeleteObject(hPenNormal);
    DeleteDC(hdcThumb);

    SelectObject(hdcMem, hOldFont);

    // Clean 1px dark border around the entity grid
    HPEN hGridBorder = CreatePen(PS_SOLID, 1, RGB(58, 65, 74));
    HPEN hOldP = static_cast<HPEN>(SelectObject(hdcMem, hGridBorder));
    HBRUSH hOldB = static_cast<HBRUSH>(SelectObject(hdcMem, GetStockObject(NULL_BRUSH)));
    Rectangle(hdcMem, 0, 0, w, h);
    SelectObject(hdcMem, hOldB);
    SelectObject(hdcMem, hOldP);
    DeleteObject(hGridBorder);

    BitBlt(hdc, 0, 0, w, h, hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hBmpMem);
    DeleteDC(hdcMem);
}

LRESULT CALLBACK WinArtPanel::EntityPreviewProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WinArtPanel *pThis = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<WinArtPanel*>(cs->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } else {
        pThis = reinterpret_cast<WinArtPanel*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }
    if (pThis) {
        return pThis->handleEntityPreviewMessage(hWnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT WinArtPanel::handleEntityPreviewMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            if (hWnd == hTilePreviewBox_) {
                drawTilePreview(hdc);
            } else {
                drawEntityPreview(hdc);
            }
            EndPaint(hWnd, &ps);
            return 0;
        }
        case WM_TIMER: {
            if (hWnd != hTilePreviewBox_ && wParam == 101 && previewAnimating_) {
                previewFrame_++;
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

void WinArtPanel::rotatePreviewEntity(int step) {
    if (pSelectedEntity_) {
        if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
            int curDir = ((pPed->direction() + 16) / 32) % 8;
            int newDir = (curDir + step + 8) % 8;
            previewDir_ = newDir;
            pPed->setDirection(newDir * 32);
            if (pPed->isOurAgent() && pMission_) {
                size_t slot = (pPed->id() < 4) ? pPed->id() : 0;
                pMission_->setSquadSpawnPoint(slot, pPed->position(), static_cast<uint8_t>(newDir * 32));
            }
            ComboBox_SetCurSel(hInspectComboOrient_, newDir);
            markEntityDirty();
        } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) {
            int curDir = ((pVeh->direction() + 32) / 64) % 4;
            int newDir = (curDir + step + 4) % 4;
            previewDir_ = newDir;
            pVeh->setDirection(newDir * 64);
            ComboBox_SetCurSel(hInspectComboOrient_, newDir);
            markEntityDirty();
        } else if (auto *pStatic = dynamic_cast<rs_knl::Static*>(pSelectedEntity_)) {
            if (!isFixedDirectionStatic(pStatic)) {
                int curOri = (pStatic->orientation() == rs_knl::Static::kStaticOrientationEW) ? 1 : 0;
                int newOri = (curOri + 1) % 2;
                previewDir_ = newOri;
                pStatic->setOrientation(newOri == 1 ? rs_knl::Static::kStaticOrientationEW : rs_knl::Static::kStaticOrientationNS);
                ComboBox_SetCurSel(hInspectComboOrient_, newOri);
                markEntityDirty();
            }
        }
        if (pViewport_) InvalidateRect(pViewport_->hwnd(), nullptr, FALSE);
    } else {
        // Template mode
        const auto *pItem = selectedEntityTemplateItem();
        if (pItem) {
            if (pItem->kind == EntityKind::Actor) {
                int curDir = ((activeOrientation_ + 16) / 32) % 8;
                int newDir = (curDir + step + 8) % 8;
                previewDir_ = newDir;
                activeOrientation_ = static_cast<uint8_t>(newDir * 32);
                ComboBox_SetCurSel(hInspectComboOrient_, newDir);
                applyEntityPlacementFromSelection();
            } else if (pItem->kind == EntityKind::Vehicle) {
                if (pItem->typeId == rs_knl::Vehicle::kVehicleTypeTrainBody) {
                    int curDir = ((activeOrientation_ + 32) / 64) % 2;
                    int newDir = (curDir + step + 2) % 2;
                    previewDir_ = newDir;
                    activeOrientation_ = static_cast<uint8_t>(newDir * 64);
                    ComboBox_SetCurSel(hInspectComboOrient_, newDir);
                    applyEntityPlacementFromSelection();
                } else {
                    int curDir = ((activeOrientation_ + 32) / 64) % 4;
                    int newDir = (curDir + step + 4) % 4;
                    previewDir_ = newDir;
                    activeOrientation_ = static_cast<uint8_t>(newDir * 64);
                    ComboBox_SetCurSel(hInspectComboOrient_, newDir);
                    applyEntityPlacementFromSelection();
                }
            } else if (pItem->kind == EntityKind::Static) {
                if (!isFixedDirectionStatic(static_cast<uint8_t>(pItem->typeId), pItem->baseAnim)) {
                    int newDir = (activeOrientation_ + 1) % 2;
                    previewDir_ = newDir;
                    activeOrientation_ = static_cast<uint8_t>(newDir);
                    ComboBox_SetCurSel(hInspectComboOrient_, newDir);
                    applyEntityPlacementFromSelection();
                }
            }
        }
    }
    if (hEntPreviewBox_) InvalidateRect(hEntPreviewBox_, nullptr, FALSE);
    if (hInspectBtnRotLeft_) InvalidateRect(hInspectBtnRotLeft_, nullptr, TRUE);
    if (hInspectBtnPlayAnim_) InvalidateRect(hInspectBtnPlayAnim_, nullptr, TRUE);
    if (hInspectBtnRotRight_) InvalidateRect(hInspectBtnRotRight_, nullptr, TRUE);
}

void WinArtPanel::togglePreviewAnimation() {
    previewAnimating_ = !previewAnimating_;
    if (hInspectBtnPlayAnim_) {
        SetWindowTextW(hInspectBtnPlayAnim_, previewAnimating_ ? L"||" : L"\u25B6");
    }
    if (previewAnimating_) {
        SetTimer(hEntPreviewBox_, 101, 120, nullptr);
    } else {
        KillTimer(hEntPreviewBox_, 101);
        previewFrame_ = 0;
    }
    if (hEntPreviewBox_) InvalidateRect(hEntPreviewBox_, nullptr, FALSE);
}

void WinArtPanel::updateEntityPreview() {
    bool canRotate = true;
    bool hasAnim = true;

    if (pSelectedEntity_) {
        if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
            previewDir_ = ((pPed->direction() + 16) / 32) % 8;
            canRotate = true;
            hasAnim = true;
        } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) {
            previewDir_ = ((pVeh->direction() + 32) / 64) % 4;
            canRotate = true;
            hasAnim = (pVeh->getDrawAnimation() != 0);
        } else if (auto *pStatic = dynamic_cast<rs_knl::Static*>(pSelectedEntity_)) {
            canRotate = !isFixedDirectionStatic(pStatic);
            previewDir_ = (pStatic->orientation() == rs_knl::Static::kStaticOrientationEW) ? 1 : 0;
            auto *animMgr = rs_eng::AnimationManager::singletonPtr();
            hasAnim = (animMgr && animMgr->lastFrame(pStatic->getDrawAnimation()) > 0);
        } else if (auto *pWeapon = dynamic_cast<rs_knl::WeaponInstance*>(pSelectedEntity_)) {
            canRotate = false;
            previewDir_ = 0;
            auto *animMgr = rs_eng::AnimationManager::singletonPtr();
            hasAnim = (animMgr && animMgr->lastFrame(pWeapon->getDrawAnimation()) > 0);
        }
    } else {
        const auto *pItem = selectedEntityTemplateItem();
        if (pItem) {
            if (pItem->kind == EntityKind::Actor) {
                previewDir_ = ((activeOrientation_ + 16) / 32) % 8;
                canRotate = true;
                hasAnim = true;
            } else if (pItem->kind == EntityKind::Vehicle) {
                previewDir_ = ((activeOrientation_ + 32) / 64) % 4;
                canRotate = true;
                hasAnim = true;
            } else if (pItem->kind == EntityKind::Static) {
                canRotate = !isFixedDirectionStatic(static_cast<uint8_t>(pItem->typeId), pItem->baseAnim);
                previewDir_ = activeOrientation_ % 2;
                auto *animMgr = rs_eng::AnimationManager::singletonPtr();
                hasAnim = (animMgr && animMgr->lastFrame(pItem->drawAnim) > 0);
            } else if (pItem->kind == EntityKind::Weapon) {
                canRotate = false;
                previewDir_ = 0;
                auto *animMgr = rs_eng::AnimationManager::singletonPtr();
                hasAnim = (animMgr && animMgr->lastFrame(pItem->drawAnim) > 0);
            }
        }
    }

    if (hInspectBtnRotLeft_) EnableWindow(hInspectBtnRotLeft_, canRotate);
    if (hInspectBtnRotRight_) EnableWindow(hInspectBtnRotRight_, canRotate);
    if (hInspectBtnPlayAnim_) {
        EnableWindow(hInspectBtnPlayAnim_, hasAnim);
        if (!hasAnim && previewAnimating_) {
            togglePreviewAnimation();
        }
    }

    if (hEntPreviewBox_) InvalidateRect(hEntPreviewBox_, nullptr, FALSE);
}

void WinArtPanel::drawEntityPreview(HDC hdc) {
    if (!hEntPreviewBox_) return;
    RECT rc;
    GetClientRect(hEntPreviewBox_, &rc);
    int w = rc.right;
    int h = rc.bottom;
    if (w <= 0 || h <= 0) return;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdcMem = CreateCompatibleDC(hdc);
    uint32_t *pBits = nullptr;
    HBITMAP hBmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&pBits), nullptr, 0);
    HBITMAP hOldBmp = static_cast<HBITMAP>(SelectObject(hdcMem, hBmp));

    if (pBits) {
        // Draw dark background with subtle grid checker
        for (int i = 0; i < w * h; ++i) {
            int px = i % w;
            int py = i / w;
            bool check = ((px / 6) + (py / 6)) % 2 == 0;
            pBits[i] = check ? 0xFF14171A : 0xFF1A1E22;
        }

        uint16_t animId = 0;
        int frameId = 0;
        auto *animMgr = rs_eng::AnimationManager::singletonPtr();

        if (pSelectedEntity_) {
            if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
                uint16_t base = pPed->baseAnim();
                if (base == 0) base = rs_knl::PedManager::getDefaultBaseAnim(pPed->type());
                if (pPed->health() <= 0) {
                    animId = base + 206;
                    frameId = 0;
                } else if (previewAnimating_) {
                    animId = base + 8 + (previewDir_ % 8);
                    uint8_t lf = animMgr ? animMgr->lastFrame(animId) : 5;
                    frameId = (lf > 0) ? (previewFrame_ % (lf + 1)) : 0;
                } else {
                    animId = base + (previewDir_ % 8);
                    frameId = 0;
                }
            } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pSelectedEntity_)) {
                bool isTrainHead = (pVeh->getType() == rs_knl::Vehicle::kVehicleTypeTrainHead);
                bool isTrainBody = (pVeh->getType() == rs_knl::Vehicle::kVehicleTypeTrainBody);
                uint16_t base = pVeh->baseSpriteAnim();
                if (base == 0) base = pVeh->regularAnimation();
                if (pVeh->health() <= 0) {
                    if (isTrainHead) {
                        animId = base + 8 + (previewDir_ % 4);
                    } else if (isTrainBody) {
                        animId = base + 4 + ((previewDir_ % 4) % 2);
                    } else {
                        animId = base + 12 + (previewDir_ % 4);
                    }
                    frameId = 0;
                } else {
                    animId = base;
                    if (isTrainHead) {
                        animId += (previewDir_ % 4);
                    } else if (isTrainBody) {
                        animId += ((previewDir_ % 4) % 2);
                    } else {
                        animId += (previewDir_ % 4) * 2;
                    }
                    uint8_t lf = animMgr ? animMgr->lastFrame(animId) : 0;
                    frameId = (previewAnimating_ && lf > 0) ? (previewFrame_ % (lf + 1)) : 0;
                }
            } else if (auto *pStatic = dynamic_cast<rs_knl::Static*>(pSelectedEntity_)) {
                animId = pStatic->getDrawAnimation();
                if (animId == 0) animId = pStatic->baseAnim();
                uint8_t lf = animMgr ? animMgr->lastFrame(animId) : 0;
                frameId = (previewAnimating_ && lf > 0) ? (previewFrame_ % (lf + 1)) : 0;
            } else if (auto *pWeapon = dynamic_cast<rs_knl::WeaponInstance*>(pSelectedEntity_)) {
                animId = pWeapon->getDrawAnimation();
                if (animId == 0) animId = pWeapon->baseAnim();
                uint8_t lf = animMgr ? animMgr->lastFrame(animId) : 0;
                frameId = (previewAnimating_ && lf > 0) ? (previewFrame_ % (lf + 1)) : 0;
            }
        } else {
            // Template mode
            const auto *pItem = selectedEntityTemplateItem();
            if (pItem) {
                bool isDeadTemplate = (hInspectCheckDead_ && SendMessageW(hInspectCheckDead_, BM_GETCHECK, 0, 0) == BST_CHECKED);
                if (pItem->kind == EntityKind::Actor) {
                    uint16_t base = pItem->baseAnim;
                    if (isDeadTemplate) {
                        animId = base + 206;
                        frameId = 0;
                    } else if (previewAnimating_) {
                        animId = base + 8 + (previewDir_ % 8);
                        uint8_t lf = animMgr ? animMgr->lastFrame(animId) : 5;
                        frameId = (lf > 0) ? (previewFrame_ % (lf + 1)) : 0;
                    } else {
                        animId = base + (previewDir_ % 8);
                        frameId = 0;
                    }
                } else if (pItem->kind == EntityKind::Vehicle) {
                    bool isTrainHead = (pItem->typeId == rs_knl::Vehicle::kVehicleTypeTrainHead || pItem->typeId == 0xFE);
                    bool isTrainBody = (pItem->typeId == rs_knl::Vehicle::kVehicleTypeTrainBody);
                    uint16_t base = pItem->drawAnim;
                    if (isDeadTemplate) {
                        if (isTrainHead) {
                            animId = base + 8 + (previewDir_ % 4);
                        } else if (isTrainBody) {
                            animId = base + 4 + ((previewDir_ % 4) % 2);
                        } else {
                            animId = base + 12 + (previewDir_ % 4);
                        }
                        frameId = 0;
                    } else {
                        animId = base;
                        if (isTrainHead) {
                            animId += (previewDir_ % 4);
                        } else if (isTrainBody) {
                            animId += ((previewDir_ % 4) % 2);
                        } else {
                            animId += (previewDir_ % 4) * 2;
                        }
                        uint8_t lf = animMgr ? animMgr->lastFrame(animId) : 0;
                        frameId = (previewAnimating_ && lf > 0) ? (previewFrame_ % (lf + 1)) : 0;
                    }
                } else if (pItem->kind == EntityKind::Static) {
                    animId = pItem->drawAnim;
                    if (!isFixedDirectionStatic(static_cast<uint8_t>(pItem->typeId), pItem->baseAnim)) {
                        animId += (previewDir_ % 2);
                    }
                    uint8_t lf = animMgr ? animMgr->lastFrame(animId) : 0;
                    frameId = (previewAnimating_ && lf > 0) ? (previewFrame_ % (lf + 1)) : 0;
                } else if (pItem->kind == EntityKind::Weapon) {
                    animId = pItem->drawAnim;
                    uint8_t lf = animMgr ? animMgr->lastFrame(animId) : 0;
                    frameId = (previewAnimating_ && lf > 0) ? (previewFrame_ % (lf + 1)) : 0;
                }
            }
        }

        int previewFactionId = -1;
        if (pSelectedEntity_) {
            if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pSelectedEntity_)) {
                if (pPed->type() == rs_knl::PedInstance::kPedTypeAgent) {
                    if (pPed->isOurAgent()) {
                        previewFactionId = 0;
                    } else {
                        previewFactionId = (activeFactionId_ > 0) ? activeFactionId_ : pPed->factionId();
                    }
                }
            }
        } else {
            const auto *pItem = selectedEntityTemplateItem();
            if (pItem && pItem->kind == EntityKind::Actor && pItem->typeId == rs_knl::PedInstance::kPedTypeAgent) {
                if (templateAgentType_ == 0) {
                    previewFactionId = 0;
                } else {
                    previewFactionId = (activeFactionId_ > 0) ? activeFactionId_ : 1;
                }
            }
        }

        if (animId > 0) {
            int minX = 0, minY = 0, maxX = 0, maxY = 0;
            if (SpriteRasterizer::instance().getAnimFrameBounds(animId, frameId, minX, minY, maxX, maxY)) {
                float sprW = static_cast<float>(maxX - minX);
                float sprH = static_cast<float>(maxY - minY);
                float maxW = static_cast<float>(w - 8);
                float maxH = static_cast<float>(h - 8);
                float scale = 1.6f; // Large preview render
                if (sprW * scale > maxW || sprH * scale > maxH) {
                    scale = std::min(maxW / sprW, maxH / sprH);
                }
                int anchorX = static_cast<int>(w / 2.0f - ((minX + maxX) / 2.0f) * scale);
                int anchorY = static_cast<int>(h / 2.0f - ((minY + maxY) / 2.0f) * scale);
                SpriteRasterizer::instance().drawAnimFrame(pBits, w, h, animId, frameId, anchorX, anchorY, scale, 255, previewFactionId);
            }
        }
    }

    BitBlt(hdc, 0, 0, w, h, hdcMem, 0, 0, SRCCOPY);

    // Draw border
    HPEN hBorderPen = CreatePen(PS_SOLID, 1, RGB(45, 52, 60));
    HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hBorderPen));
    HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
    Rectangle(hdc, 0, 0, w, h);
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hBorderPen);

    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
}

void WinArtPanel::drawTilePreview(HDC hdc) {
    if (!hTilePreviewBox_) return;
    RECT rc;
    GetClientRect(hTilePreviewBox_, &rc);
    int w = rc.right;
    int h = rc.bottom;
    if (w <= 0 || h <= 0) return;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdcMem = CreateCompatibleDC(hdc);
    uint32_t *pBits = nullptr;
    HBITMAP hBmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&pBits), nullptr, 0);
    HBITMAP hOldBmp = static_cast<HBITMAP>(SelectObject(hdcMem, hBmp));

    if (pBits) {
        for (int i = 0; i < w * h; ++i) {
            int px = i % w;
            int py = i / w;
            bool check = ((px / 6) + (py / 6)) % 2 == 0;
            pBits[i] = check ? 0xFF14171A : 0xFF1A1E22;
        }

        float scale = std::min(static_cast<float>(w - 8) / 64.0f, static_cast<float>(h - 8) / 48.0f);
        if (scale < 0.25f) scale = 0.25f;
        int drawW = static_cast<int>(64.0f * scale);
        int drawH = static_cast<int>(48.0f * scale);
        int drawX = (w - drawW) / 2;
        int drawY = (h - drawH) / 2;
        TileRasterizer::instance().drawTileScaled(pBits, w, h, activeTileId_, drawX, drawY, scale);
    }

    BitBlt(hdc, 0, 0, w, h, hdcMem, 0, 0, SRCCOPY);

    HPEN hBorderPen = CreatePen(PS_SOLID, 1, RGB(45, 52, 60));
    HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hBorderPen));
    HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
    Rectangle(hdc, 0, 0, w, h);
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hBorderPen);

    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
}

} // namespace rs_win_editor
