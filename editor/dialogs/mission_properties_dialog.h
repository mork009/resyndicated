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

#ifndef EDITOR_MISSION_PROPERTIES_DIALOG_H_
#define EDITOR_MISSION_PROPERTIES_DIALOG_H_

#include <windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <format>

namespace rs_win_editor {

struct MissionBriefingLang {
    bool available = false;
    bool modified = false;
    bool sessionEdited = false;
    std::string baseBriefing;
    std::string info1Briefing;
    std::string info2Briefing;
    std::string info3Briefing;
    std::vector<std::string> infoCostLines;
    std::vector<std::string> enhCostLines;
    std::vector<std::string> briefingSections;
};

struct MissionPropertiesData {
    int missionId = 1;
    int mapId = 1;         // 1..50, assigned in mapinfos.map[0]
    int language = 0;      // 0=English, 1=French, 2=Italian, 3=German (active/selected language in UI)
    int techLevel = 0;     // 0..3: Enemy Agent cyberware tier
    int initialTechLevel = 0;
    int infoCost1 = 100;
    int infoCost2 = 250;
    int infoCost3 = 400;
    int enhCost1 = 100;
    int enhCost2 = 250;
    int enhCost3 = 400;

    std::filesystem::path sourceDir;
    std::filesystem::path gameFilePath;

    // Active language briefing fields (for direct access & backward compatibility):
    std::string baseBriefing;
    std::string info1Briefing;
    std::string info2Briefing;
    std::string info3Briefing;

    // Preserved raw line structures from the file to prevent data loss
    std::vector<std::string> infoCostLines;
    std::vector<std::string> enhCostLines;
    std::vector<std::string> briefingSections;

    // Multi-language briefing slots (0=English, 1=French, 2=Italian, 3=German)
    MissionBriefingLang langs[4];

    void syncActiveToLang(int langIdx) {
        if (langIdx < 0 || langIdx > 3) return;
        langs[langIdx].available = true;
        langs[langIdx].baseBriefing = baseBriefing;
        langs[langIdx].info1Briefing = info1Briefing;
        langs[langIdx].info2Briefing = info2Briefing;
        langs[langIdx].info3Briefing = info3Briefing;
        langs[langIdx].infoCostLines = infoCostLines;
        langs[langIdx].enhCostLines = enhCostLines;
        langs[langIdx].briefingSections = briefingSections;
    }

    void syncLangToActive(int langIdx) {
        if (langIdx < 0 || langIdx > 3) return;
        language = langIdx;
        baseBriefing = langs[langIdx].baseBriefing;
        info1Briefing = langs[langIdx].info1Briefing;
        info2Briefing = langs[langIdx].info2Briefing;
        info3Briefing = langs[langIdx].info3Briefing;
        infoCostLines = langs[langIdx].infoCostLines;
        enhCostLines = langs[langIdx].enhCostLines;
        briefingSections = langs[langIdx].briefingSections;
    }
};

class MissionPropertiesDialog {
public:
    static bool show(HWND hParent, MissionPropertiesData &data);
    static bool loadBriefingData(int missionId, int language, MissionPropertiesData &data, const std::filesystem::path &dir = {});
    static bool loadAllBriefingLanguages(int missionId, const std::filesystem::path &dir, MissionPropertiesData &data);
    static bool saveBriefingData(int missionId, int language, const MissionPropertiesData &data, const std::string &customPath = "");
    static bool loadBriefingDataFromPath(const std::string &path, MissionPropertiesData &data);
    static bool loadBriefingLangFromPath(const std::string &path, MissionPropertiesData &data, int langIdx);
    static bool saveBriefingDataToPath(const std::string &path, const MissionPropertiesData &data);
    static bool saveBriefingLangToPath(const std::string &path, const MissionPropertiesData &data, int langIdx);
    static inline std::string getBriefingFilename(int missionId, int language) {
        const char *prefix = "";
        if (language == 1) prefix = "1";
        else if (language == 2) prefix = "2";
        else if (language == 3) prefix = "3";
        return std::format("miss{}{:02}.dat", prefix, missionId);
    }

    static inline const wchar_t* getLanguageName(int language) {
        switch (language) {
            case 1: return L"French";
            case 2: return L"Italian";
            case 3: return L"German";
            default: return L"English";
        }
    }
};

} // namespace rs_win_editor

#endif // EDITOR_MISSION_PROPERTIES_DIALOG_H_
