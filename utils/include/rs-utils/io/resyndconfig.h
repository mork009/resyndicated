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

#ifndef RS_UTILS_IO_RESYNDCONFIG_H_
#define RS_UTILS_IO_RESYNDCONFIG_H_

#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace rs_utl {

struct ResyndConfigSection {
    std::string name;
    std::vector<std::pair<std::string, std::string>> entries;
};

/*!
 * Section-aware INI/configuration file reader and writer for resynd.conf.
 * Preserves sections and keys across concurrent updates from both game and editor.
 */
class ResyndConfig {
public:
    bool load(const std::filesystem::path &path);
    bool save(const std::filesystem::path &path) const;

    bool hasSection(const std::string &section) const;
    bool hasKey(const std::string &section, const std::string &key) const;

    std::string getString(const std::string &section, const std::string &key, const std::string &defaultVal = "") const;
    int getInt(const std::string &section, const std::string &key, int defaultVal = 0) const;
    uint32_t getUInt(const std::string &section, const std::string &key, uint32_t defaultVal = 0) const;
    double getDouble(const std::string &section, const std::string &key, double defaultVal = 0.0) const;
    bool getBool(const std::string &section, const std::string &key, bool defaultVal = false) const;

    void setString(const std::string &section, const std::string &key, const std::string &val);
    void setInt(const std::string &section, const std::string &key, int val);
    void setUInt(const std::string &section, const std::string &key, uint32_t val);
    void setDouble(const std::string &section, const std::string &key, double val);
    void setBool(const std::string &section, const std::string &key, bool val);
    void removeKey(const std::string &section, const std::string &key);

    std::vector<std::pair<std::string, std::string>> getSectionEntries(const std::string &section) const;
    void setSectionEntries(const std::string &section, const std::vector<std::pair<std::string, std::string>> &entries);
    const std::vector<ResyndConfigSection>& sections() const { return sections_; }

private:
    const std::string* findValue(const std::string &section, const std::string &key) const;
    std::vector<ResyndConfigSection> sections_;
};

} // namespace rs_utl

#endif // RS_UTILS_IO_RESYNDCONFIG_H_
