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

#ifndef EDITOR_CORE_MISSION_VALIDATOR_H_
#define EDITOR_CORE_MISSION_VALIDATOR_H_

#include <string>
#include <vector>
#include <format>
#include "rs-kernel/model/mission.h"
#include "rs-kernel/model/position.h"

namespace rs_win_editor {

enum class ValidationSeverity {
    Warning,
    Error
};

struct ValidationIssue {
    ValidationSeverity severity;
    std::string entityDesc;
    rs_knl::TilePoint position;
    std::string message;
};

struct ValidationReport {
    std::vector<ValidationIssue> issues;
    int errorCount = 0;
    int warningCount = 0;

    bool hasIssues() const { return !issues.empty(); }
    bool hasErrors() const { return errorCount > 0; }
    bool hasWarnings() const { return warningCount > 0; }
    bool passed() const { return issues.empty(); }

    void addError(const std::string &entityDesc, const rs_knl::TilePoint &pos, const std::string &msg) {
        issues.push_back({ ValidationSeverity::Error, entityDesc, pos, msg });
        errorCount++;
    }

    void addWarning(const std::string &entityDesc, const rs_knl::TilePoint &pos, const std::string &msg) {
        issues.push_back({ ValidationSeverity::Warning, entityDesc, pos, msg });
        warningCount++;
    }
};

class MissionValidator {
public:
    //! Validates all entities, map bounds, supporting tiles, walkability, and objectives in a mission.
    static ValidationReport validate(rs_knl::Mission *pMission);
};

} // namespace rs_win_editor

#endif // EDITOR_CORE_MISSION_VALIDATOR_H_
