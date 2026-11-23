/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
 *   Copyright (C) 2011  Joey Parrish  <joey.parrish@gmail.com>
 *   Copyright (C) 2010, 2025-2026  Benoit Blancard <benblan@users.sourceforge.net>
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

#include "rs-utils/io/file.h"

#include <stdio.h>
#include <stdlib.h>
#include <cctype>
#include <iostream>
#include <iterator>
#include <fstream>
#include <sstream>
#include <algorithm>

#include <chrono>
#include <format>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

#include "rs-utils/common.h"
#include "rs-utils/io/dernc.h"
#include "rs-utils/log/log.h"
#include "rs-utils/io/portablefile.h"
#include "rs-utils/io/formatversion.h"

namespace rs_utl {

    fs::path File::dataPath_ = "";
    fs::path File::ourDataPath_ = "./data/";
    fs::path File::savePath_ = ".";
    fs::path File::userConfFolderPath_ = "";

    fs::path File::getExecutableDirectory() {
#ifdef _WIN32
        wchar_t path[32768];
        DWORD length = GetModuleFileNameW(nullptr, path, std::size(path));
        if (length && length < std::size(path)) return fs::path(path).parent_path();
#elif defined(__APPLE__)
        uint32_t size = 0;
        _NSGetExecutablePath(nullptr, &size);
        std::vector<char> path(size);
        if (_NSGetExecutablePath(path.data(), &size) == 0)
            return fs::canonical(path.data()).parent_path();
#endif
        return fs::current_path();
    }

    bool File::getOrCreateUserConfFolder(const std::string& userConfFolder) {
        if (userConfFolder.size() != 0) {
            // The user has given a path using the cli so use it
            userConfFolderPath_ = userConfFolder;
            if (!fs::exists(userConfFolderPath_)) {
                RSERR(Log::k_FLG_GFX, "File", "getOrCreateUserConfFolder", ("Directory does not exist: %s.", userConfFolder.c_str()));
                return false;
            }

            return true;
        }

        // We need to use default path and create folder if it does not exist
        // On all platforms, we use the user's home folder.
        // On Windows, home is defined using USERPROFILE env variable
#ifdef _WIN32
        const char *up = getenv("USERPROFILE");
#else
        const char *up = getenv("HOME");
#endif
        userConfFolderPath_ = up ? fs::path(up) : fs::current_path();

        userConfFolderPath_ /= ".resynd";

        if (!fs::exists(userConfFolderPath_)) {
            LOG(Log::k_FLG_IO, "File", "getOrCreateUserConfFolder", ("Creating user config folder %s", userConfFolderPath_.string().c_str()));
            if (!fs::create_directories(userConfFolderPath_)) {
                RSERR(Log::k_FLG_GFX, "File", "getOrCreateUserConfFolder", ("Could not create user conf folder."));
                return false;
            }
        }

        return true;
    }

    void File::getDefaultSaveFolder(std::string& confFolderPath) {
        fs::path savepath = userConfFolderPath_ / "save";
        confFolderPath.append(savepath.string());
    }

    bool File::getResyndConfFullPath(fs::path& confFullPath) {
        if (userConfFolderPath_.empty()) {
            getOrCreateUserConfFolder("");
        }
        confFullPath = userConfFolderPath_ / "resynd.conf";
        return fs::exists(confFullPath);
    }

    /*!
    * The methods returns a string composed of the root path and given file name.
    * No control is made on the result format or file existence.
    * \param filename The relative path to one of the original data files.
    * \param uppercase If true, the resulting string will uppercased.
    */
    std::string File::getOriginalDataFullPath(const std::string& filename, bool uppercase) {
        std::string second_part = filename;

        if (uppercase) {
            std::transform(second_part.begin(), second_part.end(), second_part.begin(),
                        [](unsigned char c){
                            return (std::toupper(c)); }
                    );
        } else {
            std::transform(second_part.begin(), second_part.end(), second_part.begin(),
                        [](unsigned char c){
                            return (std::tolower(c)); }
                    );
        }


        return (dataPath_ / second_part).string();
    }

    /*!
    * The methods returns a string composed of the root path and given file name.
    * No control is made on the result format or file existence.
    * \param filename The relative path to one of our data files.
    */
    std::string File::getResyndDataFullPath(const std::string& filename) {
        return (ourDataPath_ / filename).string();
    }

    void File::getFullPathForSaveSlot(int slot, std::string &path) {
        std::ostringstream filename;
        if (slot < 10) {
            filename << "0";
        }
        filename << slot << ".fsg";

        path.assign((savePath_ / filename.str()).string());
    }

    static std::string getDataDiskAlias(const std::string &lower) {
        if (lower == "hspr-0.dat") return "hspr-0-d.dat";
        if (lower == "hspr-0.tab") return "hspr-0-d.tab";
        if (lower == "hspr-1.dat") return "hspr-1-d.dat";
        if (lower == "hspr-1.tab") return "hspr-1-d.tab";
        if (lower == "mspr-0.dat") return "mspr-0-d.dat";
        if (lower == "mspr-0.tab") return "mspr-0-d.tab";
        if (lower == "sound-0.dat") return "sound-d.dat";
        if (lower == "sound-0.tab") return "sound-d.tab";
        if (lower == "gsound-0.dat") return "gsound-d.dat";
        if (lower == "gsound-0.tab") return "gsound-d.tab";

        if (lower == "hspr-0-d.dat") return "hspr-0.dat";
        if (lower == "hspr-0-d.tab") return "hspr-0.tab";
        if (lower == "hspr-1-d.dat") return "hspr-1.dat";
        if (lower == "hspr-1-d.tab") return "hspr-1.tab";
        if (lower == "mspr-0-d.dat") return "mspr-0.dat";
        if (lower == "mspr-0-d.tab") return "mspr-0.tab";
        if (lower == "sound-d.dat") return "sound-0.dat";
        if (lower == "sound-d.tab") return "sound-0.tab";
        if (lower == "gsound-d.dat") return "gsound-0.dat";
        if (lower == "gsound-d.tab") return "gsound-0.tab";
        return "";
    }

    /*!
    * Loads raw file from given directory to memory without decompressing.
    * Tries lowercase, uppercase, exact filename, and Data Disk (-d) aliases.
    * \return NULL if file cannot be read.
    */
    uint8_t *File::loadFileFromDirToMem(const fs::path &dir, const std::string &filename, size_t &filesize) {
        std::string lowerName = filename;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        });
        std::string upperName = filename;
        std::transform(upperName.begin(), upperName.end(), upperName.begin(), [](unsigned char c) {
            return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        });

        fs::path pLower = dir / lowerName;
        fs::path pUpper = dir / upperName;
        fs::path pExact = dir / filename;

        FILE *fp = nullptr;

        // Preference Data Disk (-d / -D) superset assets first when loading core sprites/audio
        std::string alias = getDataDiskAlias(lowerName);
        if (!alias.empty() && alias.find("-d") != std::string::npos) {
            std::string aliasUpper = alias;
            std::transform(aliasUpper.begin(), aliasUpper.end(), aliasUpper.begin(), [](unsigned char c) {
                return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            });
            fp = fopen((dir / alias).string().c_str(), "rb");
            if (!fp) fp = fopen((dir / aliasUpper).string().c_str(), "rb");
        }

        // Try primary requested filename (lower, upper, exact)
        if (!fp) fp = fopen((dir / lowerName).string().c_str(), "rb");
        if (!fp) fp = fopen((dir / upperName).string().c_str(), "rb");
        if (!fp) fp = fopen((dir / filename).string().c_str(), "rb");

        // Fallback to non-DataDisk alias if caller explicitly requested a -d filename but directory only has base
        if (!fp && !alias.empty()) {
            std::string aliasUpper = alias;
            std::transform(aliasUpper.begin(), aliasUpper.end(), aliasUpper.begin(), [](unsigned char c) {
                return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            });
            fp = fopen((dir / alias).string().c_str(), "rb");
            if (!fp) fp = fopen((dir / aliasUpper).string().c_str(), "rb");
        }

        if (fp) {
            fseek(fp, 0, SEEK_END);
            long size = ftell(fp);
            if (size >= 0) {
                filesize = static_cast<size_t>(size);
                uint8_t *mem = new uint8_t[filesize + 1];
                mem[filesize] = '\0';
                fseek(fp, 0, SEEK_SET);
                size_t n = fread(mem, 1, filesize, fp);
                fclose(fp);
                if (n == 0 && filesize > 0) {
                    Error::setError(Log::k_FLG_IO, "File", "loadFileToMem", "File '{}' in path {} is empty", filename, dir.string());
                    delete[] mem;
                    filesize = 0;
                    return nullptr;
                }
                return mem;
            }
            fclose(fp);
        }

        filesize = 0;
        return nullptr;
    }

    uint8_t *File::loadOriginalFileToMem(const std::string& filename, size_t &filesize) {
        uint8_t *data = loadFileFromDirToMem(dataPath_, filename, filesize);
        if (!data) {
            Error::setError(Log::k_FLG_IO, "File", "loadFileToMem", "Couldn't open file '{}' in path {}", filename, dataPath_.string());
        }
        return data;
    }

    FILE *File::openOriginalFile(const std::string& filename) {
        // try lowercase, then uppercase, then exact.
        FILE *fp = fopen(getOriginalDataFullPath(filename, false).c_str(), "r");
        if (!fp) fp = fopen(getOriginalDataFullPath(filename, true).c_str(), "r");
        if (!fp) fp = fopen((dataPath_ / filename).string().c_str(), "r");
        return fp;
    }

    /*!
    * Adds a trailing slash if needed to the given string
    */
    void File::addMissingSlash(std::string& str) {
        if (str[str.length() - 1] != '/') str.push_back('/');
    }

    void File::setOriginalDataFolder(const std::string& path) {
        if (path.size() != 0) {
            dataPath_ = path;
        } else {
            dataPath_ = ourDataPath_;
        }
        LOG(Log::k_FLG_IO, "File", "setOriginalDataPath", ("set data path to %s", dataPath_.string().c_str()));
    }

    const fs::path& File::getOriginalDataFolder() {
        return dataPath_;
    }

    /*!
    * @brief Sets path to ReSyndicated bundled data folder. If empty, uses getExecutableDirectory()/data or ./data.
    * @param path
    */
    void File::setResyndDataFolder(const std::string& path) {
        if (path.size() != 0) {
            ourDataPath_ = path;
        } else {
            // Prioritize live workspace data directory when running in dev/repo tree
            fs::path exeDir = getExecutableDirectory();
#ifdef __APPLE__
            // A Finder launch can have an unrelated working directory.
            fs::path resources = exeDir.parent_path() / "Resources" / "data";
            if (exeDir.filename() == "MacOS" && fs::is_directory(resources)) {
                ourDataPath_ = resources;
                return;
            }
#endif
            if (fs::exists(fs::current_path() / "data" / "ref" / "characters.dat")) {
                ourDataPath_ = fs::current_path() / "data";
            } else if (fs::exists(exeDir / ".." / ".." / ".." / "data" / "ref" / "characters.dat")) {
                std::error_code ec;
                auto canon = fs::weakly_canonical(exeDir / ".." / ".." / ".." / "data", ec);
                ourDataPath_ = ec ? (exeDir / ".." / ".." / ".." / "data") : canon;
            } else if (fs::exists(exeDir / "data")) {
                ourDataPath_ = exeDir / "data";
            } else if (fs::exists(fs::current_path() / "data")) {
                ourDataPath_ = fs::current_path() / "data";
            } else {
                ourDataPath_ = exeDir / "data";
            }
        }
        LOG(Log::k_FLG_IO, "File", "setOurDataPath", ("set our data path to %s", ourDataPath_.string().c_str()));
    }

    const fs::path& File::getResyndDataFolder() {
        if (ourDataPath_.empty()) {
            setResyndDataFolder("");
        }
        return ourDataPath_;
    }

    void File::upsertSaveDataFolder(const std::string& path) {
        savePath_ = path;

        if (!fs::exists(savePath_)) {
            if (!fs::create_directories(savePath_)) {
                RSERR(Log::k_FLG_GFX, "File", "upsertSaveDataFolder", ("Could not create save folder.\n"));
            }
        }

        LOG(Log::k_FLG_IO, "File", "setSaveDataFolder", ("set save path to %s", path.c_str()));
    }

    uint8_t *File::loadOriginalFileFromDir(const fs::path &dir, const std::string &filename, size_t &filesize) {
        uint8_t *data = loadFileFromDirToMem(dir, filename, filesize);
        if (data) {
            if (rnc::isRncCompressed(data)) { // File is RNC compressed
                rnc::RncRetCode result = rnc::unpackedLength(data, filesize);

                if (result != 0) {
                    Error::setError(Log::k_FLG_IO, "File", "loadFile", "Error reading length for file {} : {}", filename, rnc::errorString(result));
                    delete[] data;
                    filesize = 0;
                    return nullptr;
                } else if (filesize == 0) {
                    Error::setError(Log::k_FLG_IO, "File", "loadFile", "Read length is zero for file '{}'", filename);
                    delete[] data;
                    return nullptr;
                }

                uint8_t *buffer = new uint8_t[filesize + 1];
                buffer[filesize] = '\0';
                size_t realSize = 0;

                result = rnc::unpack(data, buffer, realSize);
                delete[] data;

                if (result < 0) {
                    Error::setError(Log::k_FLG_IO, "File", "loadFile", "Error loading file '{}': {}", filename, rnc::errorString(result));
                    filesize = 0;
                    delete[] buffer;
                    return nullptr;
                }

                if (realSize != filesize) {
                    Error::setError(Log::k_FLG_IO, "File", "loadFile", "Uncompressed size mismatch for file '{}'", filename);
                    filesize = 0;
                    delete[] buffer;
                    return nullptr;
                }

                return buffer;
            }
        }
        return data;
    }

    uint8_t *File::loadOriginalFile(const std::string& filename, size_t &filesize) {
        return loadOriginalFileFromDir(dataPath_, filename, filesize);
    }

    /** \brief
     *
     * \param filename const std::string&
     * \param files std::vector<std::string>&
     * \return void
     *
     */
    void File::addSaveFilenameAtIndex(const fs::path& filename, std::vector<std::string> &files) {

        if (filename.extension().compare(".fsg") == 0) {
            std::istringstream iss( filename.stem().string() );
            size_t index;
            iss >> index;
            if (index < 10) {
                rs_utl::PortableFile infile;
                infile.open_to_read(filename.string().c_str());

                if (infile) {
                    // FIXME: detect original game saves
                    // Read version first
                    unsigned char vMaj = infile.read8();
                    unsigned char vMin = infile.read8();
                    rs_utl::FormatVersion v(vMaj, vMin);
                    // Read slot name
                    if (v == 0x0100) {
                        files[index] = infile.read_string(25, true);
                    } else {
                        files[index] = infile.read_string(31, true);
                    }
                }
            }
        }
    }

    /*!
    * Returns the list of names to display in load/save menu.
    * \param files
    */
    void File::getGameSavedNames(std::vector<std::string> &files) {
        for (const auto & entry : fs::directory_iterator(savePath_)) {
            addSaveFilenameAtIndex(entry.path(), files);
        }
    }

    std::string File::computeDataSignature(const fs::path &dir) {
        std::error_code ec;
        if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) {
            return "";
        }
        size_t count = 0;
        uint64_t totalSize = 0;
        uint64_t maxMtime = 0;

        for (const auto &entry : fs::directory_iterator(dir, fs::directory_options::skip_permission_denied, ec)) {
            if (ec) break;
            if (entry.is_regular_file(ec)) {
                count++;
                auto sz = entry.file_size(ec);
                if (!ec) totalSize += sz;
                auto lwt = entry.last_write_time(ec);
                if (!ec) {
                    auto s = std::chrono::duration_cast<std::chrono::seconds>(lwt.time_since_epoch()).count();
                    if (static_cast<uint64_t>(s) > maxMtime) {
                        maxMtime = static_cast<uint64_t>(s);
                    }
                }
            }
        }
        if (count == 0) return "";
        return std::format("{}:{}:{}", count, totalSize, maxMtime);
    }

    bool File::verifyOriginalData(const fs::path &originalDir, std::string *outError) {
        auto setErr = [&](const std::string &msg) {
            if (outError) *outError = msg;
            RSERR(Log::k_FLG_IO, "File", "verifyOriginalData", ("%s", msg.c_str()));
        };

        std::error_code ec;
        if (!fs::exists(originalDir, ec) || !fs::is_directory(originalDir, ec)) {
            setErr("Data directory does not exist or is not a directory: " + originalDir.string());
            return false;
        }

        // 1. Tile Graphics: hblk01.dat
        {
            size_t sz = 0;
            std::unique_ptr<uint8_t[]> data(loadOriginalFileFromDir(originalDir, "hblk01.dat", sz));
            if (!data || sz < 6144) {
                setErr("Missing or invalid tile graphics: hblk01.dat");
                return false;
            }
            bool hasSubtile = false;
            for (int i = 0; i < 1536; ++i) {
                uint32_t off = READ_LE_UINT32(data.get() + i * 4);
                if (off >= 6144) {
                    if (static_cast<size_t>(off) + 320 > sz) {
                        setErr("Tile subtile offset out of bounds in hblk01.dat");
                        return false;
                    }
                    hasSubtile = true;
                }
            }
            if (!hasSubtile) {
                setErr("Tile graphics hblk01.dat has no valid subtiles");
                return false;
            }
        }

        // 2. Collision Data: col01.dat
        {
            size_t sz = 0;
            std::unique_ptr<uint8_t[]> data(loadOriginalFileFromDir(originalDir, "col01.dat", sz));
            if (!data || sz < 256) {
                setErr("Missing or invalid collision data: col01.dat");
                return false;
            }
        }

        // 3. Palettes: hpal01.dat and hpal04.dat
        {
            const char *pals[2] = { "hpal01.dat", "hpal04.dat" };
            for (const char *pal : pals) {
                size_t sz = 0;
                std::unique_ptr<uint8_t[]> data(loadOriginalFileFromDir(originalDir, pal, sz));
                if (!data || sz < 768) {
                    setErr(std::string("Missing or invalid palette file: ") + pal);
                    return false;
                }
            }
        }

        // 4. Gameplay Sprites: hspr-0.dat and hspr-0.tab
        {
            size_t tabSz = 0, datSz = 0;
            std::unique_ptr<uint8_t[]> tab(loadOriginalFileFromDir(originalDir, "hspr-0.tab", tabSz));
            std::unique_ptr<uint8_t[]> dat(loadOriginalFileFromDir(originalDir, "hspr-0.dat", datSz));
            if (!tab || !dat || tabSz < 6 || tabSz % 6 != 0 || datSz == 0) {
                setErr("Missing or invalid gameplay sprites: hspr-0.dat / hspr-0.tab");
                return false;
            }
            size_t count = tabSz / 6;
            if (count < 50) {
                setErr("Gameplay sprite table contains unexpectedly few entries: hspr-0.tab");
                return false;
            }
            for (size_t i = 0; i < count; ++i) {
                uint32_t off = READ_LE_UINT32(tab.get() + i * 6);
                uint8_t w = tab[i * 6 + 4];
                uint8_t h = tab[i * 6 + 5];
                if (w > 0 && h > 0) {
                    size_t sprBytes = static_cast<size_t>(h) * ((static_cast<size_t>(w) + 7) / 8) * 5;
                    if (static_cast<size_t>(off) + sprBytes > datSz) {
                        setErr("Gameplay sprite offset out of bounds in hspr-0.dat");
                        return false;
                    }
                }
            }
        }

        // 5. Menu Sprites: mspr-0.dat and mspr-0.tab
        {
            size_t tabSz = 0, datSz = 0;
            std::unique_ptr<uint8_t[]> tab(loadOriginalFileFromDir(originalDir, "mspr-0.tab", tabSz));
            std::unique_ptr<uint8_t[]> dat(loadOriginalFileFromDir(originalDir, "mspr-0.dat", datSz));
            if (!tab || !dat || tabSz < 6 || tabSz % 6 != 0 || datSz == 0) {
                setErr("Missing or invalid menu sprites: mspr-0.dat / mspr-0.tab");
                return false;
            }
            size_t count = tabSz / 6;
            for (size_t i = 0; i < count; ++i) {
                uint32_t off = READ_LE_UINT32(tab.get() + i * 6);
                uint8_t w = tab[i * 6 + 4];
                uint8_t h = tab[i * 6 + 5];
                if (w > 0 && h > 0) {
                    size_t sprBytes = static_cast<size_t>(h) * ((static_cast<size_t>(w) + 7) / 8) * 5;
                    if (static_cast<size_t>(off) + sprBytes > datSz) {
                        setErr("Menu sprite offset out of bounds in mspr-0.dat");
                        return false;
                    }
                }
            }
        }

        // 6. Animation Sequences: hele-0.ani, hfra-0.ani, hsta-0.ani
        {
            size_t eleSz = 0, fraSz = 0, staSz = 0;
            std::unique_ptr<uint8_t[]> ele(loadOriginalFileFromDir(originalDir, "hele-0.ani", eleSz));
            std::unique_ptr<uint8_t[]> fra(loadOriginalFileFromDir(originalDir, "hfra-0.ani", fraSz));
            std::unique_ptr<uint8_t[]> sta(loadOriginalFileFromDir(originalDir, "hsta-0.ani", staSz));
            if (!ele || eleSz < 10 || eleSz % 10 != 0 ||
                !fra || fraSz < 8 || fraSz % 8 != 0 ||
                !sta || staSz < 2 || staSz % 2 != 0) {
                setErr("Missing or invalid animation sequence files (*.ani)");
                return false;
            }
            size_t numElements = eleSz / 10;
            size_t numFrames = fraSz / 8;
            size_t numStates = staSz / 2;

            for (size_t i = 0; i < std::min<size_t>(numElements, 100); ++i) {
                uint16_t nextEle = READ_LE_UINT16(ele.get() + i * 10 + 8);
                if (nextEle >= numElements && nextEle != 0) {
                    setErr("Animation element next-pointer out of range in hele-0.ani");
                    return false;
                }
            }
            for (size_t i = 0; i < std::min<size_t>(numFrames, 100); ++i) {
                uint16_t firstEle = READ_LE_UINT16(fra.get() + i * 8);
                uint16_t nextFra = READ_LE_UINT16(fra.get() + i * 8 + 6);
                if (firstEle >= numElements || nextFra >= numFrames) {
                    setErr("Animation frame pointer out of range in hfra-0.ani");
                    return false;
                }
            }
            for (size_t i = 0; i < std::min<size_t>(numStates, 50); ++i) {
                uint16_t firstFra = READ_LE_UINT16(sta.get() + i * 2);
                if (firstFra >= numFrames) {
                    setErr("Animation state frame index out of range in hsta-0.ani");
                    return false;
                }
            }
        }

        // 7. Missions and Maps: scan and verify all game*.dat and map*.dat files
        {
            std::vector<std::string> gameFiles;
            std::vector<std::string> mapFiles;

            for (const auto &entry : fs::directory_iterator(originalDir, ec)) {
                if (ec) break;
                if (!entry.is_regular_file(ec)) continue;
                std::string fname = entry.path().filename().string();
                std::string lower = fname;
                for (char &c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

                if (lower.rfind("game", 0) == 0 && lower.size() >= 8 && lower.rfind(".dat") == lower.size() - 4) {
                    gameFiles.push_back(fname);
                } else if (lower.rfind("map", 0) == 0 && lower.size() >= 7 && lower.rfind(".dat") == lower.size() - 4) {
                    mapFiles.push_back(fname);
                }
            }

            if (gameFiles.empty()) {
                setErr("No mission level files (game*.dat) found in: " + originalDir.string());
                return false;
            }
            if (mapFiles.empty()) {
                setErr("No map files (map*.dat) found in: " + originalDir.string());
                return false;
            }

            // Verify all game*.dat mission files
            for (const auto &gName : gameFiles) {
                size_t gSz = 0;
                std::unique_ptr<uint8_t[]> gDat(loadOriginalFileFromDir(originalDir, gName.c_str(), gSz));
                if (!gDat || gSz < 34816) {
                    setErr("Missing, corrupted, or truncated mission level data: " + gName);
                    return false;
                }
                // If mission has full DOS header with mapinfos, verify referenced map exists
                if (gSz >= 114060) {
                    uint16_t mapNum = READ_LE_UINT16(gDat.get() + 113960);
                    if (mapNum > 0) {
                        std::string expectedMap = std::format("map{:02}.dat", mapNum);
                        bool mapFound = false;
                        for (const auto &mName : mapFiles) {
                            if (std::equal(mName.begin(), mName.end(), expectedMap.begin(), expectedMap.end(),
                                    [](unsigned char a, unsigned char b) { return std::tolower(a) == b; })) {
                                mapFound = true;
                                break;
                            }
                        }
                        if (!mapFound) {
                            setErr(std::format("Mission {} references missing map file: {}", gName, expectedMap));
                            return false;
                        }
                    }
                }
            }

            // Verify all map*.dat map files
            for (const auto &mName : mapFiles) {
                size_t mSz = 0;
                std::unique_ptr<uint8_t[]> mDat(loadOriginalFileFromDir(originalDir, mName.c_str(), mSz));
                if (!mDat || mSz < 12) {
                    setErr("Missing, corrupted, or truncated map data: " + mName);
                    return false;
                }
                uint32_t maxX = READ_LE_UINT32(mDat.get() + 0);
                uint32_t maxY = READ_LE_UINT32(mDat.get() + 4);
                uint32_t maxZ = READ_LE_UINT32(mDat.get() + 8);
                if (maxX == 0 || maxX > 512 || maxY == 0 || maxY > 512 || maxZ == 0 || maxZ > 128) {
                    setErr("Invalid map grid dimensions in: " + mName);
                    return false;
                }
                if (mSz < 12 + (maxX * maxY * 4)) {
                    setErr("Map tile index column table truncated in: " + mName);
                    return false;
                }
            }
        }

        RSINFO(Log::k_FLG_IO, "File", "verifyOriginalData",
            ("Syndicate original data structural verification passed for directory: %s", originalDir.string().c_str()));
        return true;
    }

    bool File::testOriginalData() {
        std::string err;
        bool ok = verifyOriginalData(dataPath_, &err);
        if (!ok) {
            RSERR(Log::k_FLG_IO, "File", "testOriginalData",
                ("Original Syndicate data verification failed: %s", err.c_str()));
        }
        return ok;
    }

    bool File::hasAmericanRevoltData(const fs::path &originalDir) {
        fs::path targetDir = originalDir.empty() ? dataPath_ : originalDir;
        std::error_code ec;
        if (!fs::exists(targetDir, ec) || !fs::is_directory(targetDir, ec)) {
            return false;
        }

        // Check for presence of American Revolt Data Disk files
        for (const auto &entry : fs::directory_iterator(targetDir, ec)) {
            if (ec) break;
            if (!entry.is_regular_file(ec)) continue;
            std::string fname = entry.path().filename().string();
            std::string lower = fname;
            for (char &c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

            // American Revolt files: mspr-0-d.dat, sound-d.dat, hspr-0-d.dat
            if (lower == "mspr-0-d.dat" || lower == "sound-d.dat" || lower == "mspr-0-d.tab") {
                return true;
            }
            // In a merged install where mspr-0.dat was replaced with the AR 144KB version:
            if (lower == "mspr-0.dat") {
                auto sz = entry.file_size(ec);
                if (!ec && sz >= 100000) { // AR mspr-0.dat is ~144KB, base is ~28KB
                    return true;
                }
            }
        }
        return false;
    }

};