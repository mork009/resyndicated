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

#include "rs-utils/io/resyndconfig.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <thread>

namespace rs_utl {

namespace {

std::string trimCopy(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string toLowerCopy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

} // namespace

bool ResyndConfig::load(const std::filesystem::path &path) {
    sections_.clear();
    if (path.empty() || !std::filesystem::exists(path)) {
        return false;
    }

    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        return false;
    }

    ResyndConfigSection currentSec;
    currentSec.name = "";

    std::string line;
    while (std::getline(ifs, line)) {
        std::string trimmed = trimCopy(line);
        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }

        if (trimmed.front() == '[' && trimmed.back() == ']') {
            if (!currentSec.name.empty() || !currentSec.entries.empty()) {
                sections_.push_back(std::move(currentSec));
                currentSec = ResyndConfigSection{};
            }
            currentSec.name = trimCopy(trimmed.substr(1, trimmed.length() - 2));
            continue;
        }

        size_t eqPos = trimmed.find('=');
        if (eqPos != std::string::npos) {
            std::string key = trimCopy(trimmed.substr(0, eqPos));
            std::string val = trimCopy(trimmed.substr(eqPos + 1));
            if (!key.empty()) {
                currentSec.entries.push_back({ key, val });
            }
        }
    }

    if (!currentSec.name.empty() || !currentSec.entries.empty()) {
        sections_.push_back(std::move(currentSec));
    }

    return true;
}

bool ResyndConfig::save(const std::filesystem::path &path) const {
    if (path.empty()) return false;

    std::error_code ec;
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path(), ec);
    }

    std::ostringstream oss;
    oss << "# ReSyndicated Configuration\n";
    for (const auto &sec : sections_) {
        if (!sec.name.empty()) {
            oss << "\n[" << sec.name << "]\n";
        } else if (!sec.entries.empty()) {
            oss << "\n";
        }
        for (const auto &kv : sec.entries) {
            oss << kv.first << "=" << kv.second << "\n";
        }
    }

    const std::string payload = oss.str();

    std::filesystem::path tmpPath = path;
    std::ostringstream suffix;
    suffix << ".tmp." << std::hash<std::thread::id>{}(std::this_thread::get_id());
    tmpPath += suffix.str();

    {
        std::ofstream ofs(tmpPath, std::ios::out | std::ios::trunc);
        if (!ofs.is_open()) {
            return false;
        }
        ofs << payload;
        ofs.flush();
        if (!ofs.good()) {
            ofs.close();
            std::filesystem::remove(tmpPath, ec);
            return false;
        }
    }

    std::filesystem::rename(tmpPath, path, ec);
    if (ec) {
        std::filesystem::remove(tmpPath, ec);
        std::ofstream ofs(path, std::ios::out | std::ios::trunc);
        if (!ofs.is_open()) {
            return false;
        }
        ofs << payload;
        return ofs.good();
    }

    return true;
}

bool ResyndConfig::hasSection(const std::string &section) const {
    for (const auto &sec : sections_) {
        if (sec.name == section) {
            return true;
        }
    }
    return false;
}

const std::string* ResyndConfig::findValue(const std::string &section, const std::string &key) const {
    for (const auto &sec : sections_) {
        if (sec.name == section) {
            for (const auto &kv : sec.entries) {
                if (kv.first == key) {
                    return &kv.second;
                }
            }
            return nullptr;
        }
    }
    return nullptr;
}

bool ResyndConfig::hasKey(const std::string &section, const std::string &key) const {
    return findValue(section, key) != nullptr;
}

std::string ResyndConfig::getString(const std::string &section, const std::string &key, const std::string &defaultVal) const {
    const std::string *val = findValue(section, key);
    return val ? *val : defaultVal;
}

int ResyndConfig::getInt(const std::string &section, const std::string &key, int defaultVal) const {
    const std::string *val = findValue(section, key);
    if (!val || val->empty()) return defaultVal;
    try {
        size_t idx = 0;
        int parsed = std::stoi(*val, &idx);
        return idx > 0 ? parsed : defaultVal;
    } catch (...) {
        return defaultVal;
    }
}

uint32_t ResyndConfig::getUInt(const std::string &section, const std::string &key, uint32_t defaultVal) const {
    const std::string *val = findValue(section, key);
    if (!val || val->empty()) return defaultVal;
    try {
        size_t idx = 0;
        unsigned long parsed = std::stoul(*val, &idx);
        return idx > 0 ? static_cast<uint32_t>(parsed) : defaultVal;
    } catch (...) {
        return defaultVal;
    }
}

double ResyndConfig::getDouble(const std::string &section, const std::string &key, double defaultVal) const {
    const std::string *val = findValue(section, key);
    if (!val || val->empty()) return defaultVal;
    try {
        size_t idx = 0;
        double parsed = std::stod(*val, &idx);
        return idx > 0 ? parsed : defaultVal;
    } catch (...) {
        return defaultVal;
    }
}

bool ResyndConfig::getBool(const std::string &section, const std::string &key, bool defaultVal) const {
    const std::string *val = findValue(section, key);
    if (!val || val->empty()) return defaultVal;
    std::string lower = toLowerCopy(*val);
    if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") return true;
    if (lower == "false" || lower == "0" || lower == "no" || lower == "off") return false;
    return defaultVal;
}

void ResyndConfig::setString(const std::string &section, const std::string &key, const std::string &val) {
    for (auto &sec : sections_) {
        if (sec.name == section) {
            for (auto &kv : sec.entries) {
                if (kv.first == key) {
                    kv.second = val;
                    return;
                }
            }
            sec.entries.push_back({ key, val });
            return;
        }
    }
    ResyndConfigSection newSec;
    newSec.name = section;
    newSec.entries.push_back({ key, val });
    sections_.push_back(std::move(newSec));
}

void ResyndConfig::setInt(const std::string &section, const std::string &key, int val) {
    setString(section, key, std::to_string(val));
}

void ResyndConfig::setUInt(const std::string &section, const std::string &key, uint32_t val) {
    setString(section, key, std::to_string(val));
}

void ResyndConfig::setDouble(const std::string &section, const std::string &key, double val) {
    std::ostringstream oss;
    oss << val;
    setString(section, key, oss.str());
}

void ResyndConfig::setBool(const std::string &section, const std::string &key, bool val) {
    setString(section, key, val ? "true" : "false");
}

void ResyndConfig::removeKey(const std::string &section, const std::string &key) {
    for (auto &sec : sections_) {
        if (sec.name == section) {
            sec.entries.erase(
                std::remove_if(sec.entries.begin(), sec.entries.end(),
                    [&](const std::pair<std::string, std::string> &kv) { return kv.first == key; }),
                sec.entries.end());
            return;
        }
    }
}

std::vector<std::pair<std::string, std::string>> ResyndConfig::getSectionEntries(const std::string &section) const {
    for (const auto &sec : sections_) {
        if (sec.name == section) {
            return sec.entries;
        }
    }
    return {};
}

void ResyndConfig::setSectionEntries(const std::string &section, const std::vector<std::pair<std::string, std::string>> &entries) {
    for (auto &sec : sections_) {
        if (sec.name == section) {
            sec.entries = entries;
            return;
        }
    }
    ResyndConfigSection newSec;
    newSec.name = section;
    newSec.entries = entries;
    sections_.push_back(std::move(newSec));
}

} // namespace rs_utl
