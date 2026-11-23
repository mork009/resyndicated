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

#include "rs-kernel/mgr/mission_triad.h"

#include <fstream>
#include <vector>
#include <format>
#include <regex>
#include <algorithm>
#include <cwctype>

#include "rs-utils/io/file.h"
#include "rs-utils/io/dernc.h"
#include "rs-utils/common.h"

namespace rs_knl {

int MissionTriad::readMapIdFromGameFile(const std::filesystem::path &gameFilePath) {
    std::ifstream in(gameFilePath, std::ios::binary | std::ios::ate);
    if (!in.is_open()) return 0;
    std::streamsize sz = in.tellg();
    if (sz <= 0) return 0;
    in.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(static_cast<size_t>(sz));
    if (!in.read(reinterpret_cast<char*>(buffer.data()), sz)) return 0;

    std::vector<uint8_t> unpacked;
    if (rnc::isRncCompressed(buffer.data())) {
        size_t unpLen = 0;
        if (rnc::unpackedLength(buffer.data(), unpLen) == rnc::kOk && unpLen >= 113962) {
            unpacked.resize(unpLen);
            size_t realSize = 0;
            if (rnc::unpack(buffer.data(), unpacked.data(), realSize) >= 0 && realSize >= 113962) {
                return rs_utl::READ_LE_UINT16(unpacked.data() + 113960);
            }
        }
    } else if (buffer.size() >= 113962) {
        return rs_utl::READ_LE_UINT16(buffer.data() + 113960);
    }
    return 0;
}

bool MissionTriad::resolveFromAnyFile(const std::filesystem::path &chosenPath, MissionTriadPaths &outPaths) {
    if (chosenPath.empty()) return false;

    std::filesystem::path dir = chosenPath.parent_path();
    std::string filename = chosenPath.filename().string();
    std::string lowerName = filename;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                   [](unsigned char c) -> char { return static_cast<char>(std::tolower(c)); });

    int parsedId = 1;
    std::smatch match;
    std::regex reDigits("(\\d+)");
    if (std::regex_search(lowerName, match, reDigits)) {
        try {
            parsedId = std::stoi(match[1].str());
        } catch (...) {
            parsedId = 1;
        }
    }

    outPaths.missionId = parsedId;

    // 1. Resolve gameXX.dat
    if (lowerName.rfind("game", 0) == 0) {
        outPaths.gameFile = chosenPath;
    } else {
        std::filesystem::path candDir = dir.empty() ? std::filesystem::path(std::format("game{:02}.dat", parsedId)) : (dir / std::format("game{:02}.dat", parsedId));
        if (std::filesystem::exists(candDir)) {
            outPaths.gameFile = candDir;
        } else {
            std::filesystem::path candData = rs_utl::File::getOriginalDataFullPath(std::format("game{:02}.dat", parsedId));
            if (std::filesystem::exists(candData)) {
                outPaths.gameFile = candData;
            } else {
                outPaths.gameFile = candDir;
            }
        }
    }

    // 2. Determine Map ID
    int referencedMapId = readMapIdFromGameFile(outPaths.gameFile);
    if (referencedMapId <= 0) {
        referencedMapId = parsedId;
    }
    outPaths.mapId = referencedMapId;

    // 3. Resolve mapYY.dat
    if (lowerName.rfind("map", 0) == 0) {
        outPaths.mapFile = chosenPath;
        outPaths.mapId = parsedId;
    } else {
        std::filesystem::path cand1 = dir.empty() ? std::filesystem::path(std::format("map{:02}.dat", referencedMapId)) : (dir / std::format("map{:02}.dat", referencedMapId));
        std::filesystem::path cand2 = dir.empty() ? std::filesystem::path(std::format("map{:02}.dat", parsedId)) : (dir / std::format("map{:02}.dat", parsedId));
        std::filesystem::path candData1 = rs_utl::File::getOriginalDataFullPath(std::format("map{:02}.dat", referencedMapId));
        std::filesystem::path candData2 = rs_utl::File::getOriginalDataFullPath(std::format("map{:02}.dat", parsedId));

        if (std::filesystem::exists(cand1)) {
            outPaths.mapFile = cand1;
        } else if (std::filesystem::exists(cand2)) {
            outPaths.mapFile = cand2;
            outPaths.mapId = parsedId;
        } else if (std::filesystem::exists(candData1)) {
            outPaths.mapFile = candData1;
        } else if (std::filesystem::exists(candData2)) {
            outPaths.mapFile = candData2;
            outPaths.mapId = parsedId;
        } else {
            outPaths.mapFile = cand2;
            outPaths.mapId = parsedId;
        }
    }

    // 4. Resolve missXX.dat
    if (lowerName.rfind("miss", 0) == 0) {
        outPaths.missFile = chosenPath;
    } else {
        std::filesystem::path candMiss = dir.empty() ? std::filesystem::path(std::format("miss{:02}.dat", parsedId)) : (dir / std::format("miss{:02}.dat", parsedId));
        std::filesystem::path candDataMiss = rs_utl::File::getOriginalDataFullPath(std::format("miss{:02}.dat", parsedId));
        if (std::filesystem::exists(candMiss)) {
            outPaths.missFile = candMiss;
        } else if (std::filesystem::exists(candDataMiss)) {
            outPaths.missFile = candDataMiss;
        } else {
            outPaths.missFile = candMiss;
        }
    }

    return true;
}

MissionTriadPaths MissionTriad::resolveFromId(int missionId, const std::filesystem::path &customDir) {
    MissionTriadPaths paths;
    paths.missionId = missionId;
    paths.mapId = missionId;

    if (!customDir.empty()) {
        paths.gameFile = customDir / std::format("game{:02}.dat", missionId);
        int refMap = readMapIdFromGameFile(paths.gameFile);
        if (refMap > 0) paths.mapId = refMap;
        paths.mapFile = customDir / std::format("map{:02}.dat", paths.mapId);
        paths.missFile = customDir / std::format("miss{:02}.dat", missionId);
    } else {
        std::filesystem::path baseDir = rs_utl::File::getOriginalDataFolder();
        if (baseDir.empty() || !std::filesystem::exists(baseDir / std::format("game{:02}.dat", missionId))) {
            std::filesystem::path cur = std::filesystem::current_path();
            for (int i = 0; i < 6; ++i) {
                if (std::filesystem::exists(cur / "synd" / "data" / std::format("game{:02}.dat", missionId))) {
                    baseDir = cur / "synd" / "data";
                    break;
                }
                if (std::filesystem::exists(cur / "data" / std::format("game{:02}.dat", missionId))) {
                    baseDir = cur / "data";
                    break;
                }
                if (!cur.has_parent_path() || cur == cur.parent_path()) break;
                cur = cur.parent_path();
            }
        }
        if (baseDir.empty()) {
            baseDir = "data";
        }
        paths.gameFile = baseDir / std::format("game{:02}.dat", missionId);
        int refMap = readMapIdFromGameFile(paths.gameFile);
        if (refMap > 0) paths.mapId = refMap;
        paths.mapFile = baseDir / std::format("map{:02}.dat", paths.mapId);
        paths.missFile = baseDir / std::format("miss{:02}.dat", missionId);
    }

    return paths;
}

TriadValidationResult MissionTriad::validate(const MissionTriadPaths &paths) {
    TriadValidationResult res;
    res.gameOk = false;
    res.mapOk = false;
    res.missOk = false;

    // 1. Validate gameFile
    if (paths.gameFile.empty() || !std::filesystem::exists(paths.gameFile)) {
        res.gameStatus = L"MISSING - File not found";
    } else {
        std::ifstream in(paths.gameFile, std::ios::binary | std::ios::ate);
        if (!in.is_open()) {
            res.gameStatus = L"INVALID - Cannot open file";
        } else {
            std::streamsize sz = in.tellg();
            if (sz <= 18) {
                res.gameStatus = L"INVALID - File is empty or too small";
            } else {
                in.seekg(0, std::ios::beg);
                std::vector<uint8_t> buf(static_cast<size_t>(sz));
                in.read(reinterpret_cast<char*>(buf.data()), sz);

                if (rnc::isRncCompressed(buf.data())) {
                    size_t unpLen = 0;
                    if (rnc::unpackedLength(buf.data(), unpLen) == rnc::kOk && unpLen >= 114058) {
                        res.gameOk = true;
                        res.gameStatus = L"OK (Compressed RNC)";
                    } else {
                        res.gameStatus = L"INVALID - Corrupted RNC compression or truncated level data";
                    }
                } else if (buf.size() >= 114058) {
                    res.gameOk = true;
                    res.gameStatus = L"OK (Uncompressed Syndicate Level)";
                } else {
                    res.gameStatus = L"INVALID - Level data smaller than Syndicate specification (114,058 bytes)";
                }
            }
        }
    }

    // 2. Validate mapFile
    if (paths.mapFile.empty() || !std::filesystem::exists(paths.mapFile)) {
        res.mapStatus = L"MISSING - File not found";
    } else {
        std::ifstream in(paths.mapFile, std::ios::binary | std::ios::ate);
        if (!in.is_open()) {
            res.mapStatus = L"INVALID - Cannot open file";
        } else {
            std::streamsize sz = in.tellg();
            if (sz <= 18) {
                res.mapStatus = L"INVALID - File is empty or too small";
            } else {
                in.seekg(0, std::ios::beg);
                std::vector<uint8_t> buf(static_cast<size_t>(sz));
                in.read(reinterpret_cast<char*>(buf.data()), sz);

                std::vector<uint8_t> unpBuf;
                uint8_t *pData = buf.data();
                size_t mapSize = buf.size();

                if (rnc::isRncCompressed(buf.data())) {
                    size_t unpLen = 0;
                    if (rnc::unpackedLength(buf.data(), unpLen) == rnc::kOk && unpLen > 12) {
                        unpBuf.resize(unpLen);
                        size_t realSize = 0;
                        if (rnc::unpack(buf.data(), unpBuf.data(), realSize) >= 0) {
                            pData = unpBuf.data();
                            mapSize = realSize;
                        }
                    }
                }

                if (mapSize >= 12) {
                    uint32_t maxX = rs_utl::READ_LE_UINT32(pData + 0);
                    uint32_t maxY = rs_utl::READ_LE_UINT32(pData + 4);
                    uint32_t maxZ = rs_utl::READ_LE_UINT32(pData + 8);
                    if (maxX > 0 && maxX <= 512 && maxY > 0 && maxY <= 512 && maxZ > 0 && maxZ <= 64) {
                        res.mapOk = true;
                        res.mapStatus = std::format(L"OK ({}x{}x{})", maxX, maxY, maxZ);
                    } else {
                        res.mapStatus = L"INVALID - Invalid map dimensions";
                    }
                } else {
                    res.mapStatus = L"INVALID - Map header too small";
                }
            }
        }
    }

    // 3. Validate missFile
    if (paths.missFile.empty() || !std::filesystem::exists(paths.missFile)) {
        res.missStatus = L"MISSING - File not found";
    } else {
        std::ifstream in(paths.missFile, std::ios::binary | std::ios::ate);
        if (!in.is_open()) {
            res.missStatus = L"INVALID - Cannot open file";
        } else {
            std::streamsize sz = in.tellg();
            if (sz == 0) {
                res.missStatus = L"INVALID - File is empty";
            } else {
                in.seekg(0, std::ios::beg);
                std::vector<uint8_t> buf(static_cast<size_t>(sz));
                in.read(reinterpret_cast<char*>(buf.data()), sz);
                if (rnc::isRncCompressed(buf.data())) {
                    size_t unpLen = 0;
                    if (rnc::unpackedLength(buf.data(), unpLen) == rnc::kOk && unpLen > 0) {
                        std::string unpacked(unpLen, '\0');
                        size_t realSize = 0;
                        if (rnc::unpack(buf.data(), reinterpret_cast<uint8_t*>(&unpacked[0]), realSize) >= 0 &&
                            unpacked.find('|') != std::string::npos) {
                            res.missOk = true;
                            res.missStatus = L"OK (Compressed RNC Briefing)";
                        } else {
                            res.missStatus = L"INVALID - Corrupted RNC compression or unrecognized briefing structure";
                        }
                    } else {
                        res.missStatus = L"INVALID - Corrupted RNC compression in briefing file";
                    }
                } else {
                    std::string content(reinterpret_cast<const char*>(buf.data()), buf.size());
                    if (content.find('|') != std::string::npos) {
                        res.missOk = true;
                        res.missStatus = L"OK (Briefing Document)";
                    } else {
                        res.missStatus = L"INVALID - Unrecognized briefing structure";
                    }
                }
            }
        }
    }

    res.isValid = (res.gameOk && res.mapOk && res.missOk);

    if (!res.isValid) {
        std::wstring gPath = paths.gameFile.empty() ? L"(none)" : paths.gameFile.wstring();
        std::wstring mPath = paths.mapFile.empty() ? L"(none)" : paths.mapFile.wstring();
        std::wstring bPath = paths.missFile.empty() ? L"(none)" : paths.missFile.wstring();

        res.summaryMessage = std::format(
            L"Cannot load mission because one or more required files are missing or invalid.\n\n"
            L"All 3 files in the Syndicate mission triad are required:\n\n"
            L"\u2022 Level Data (game{:02}.dat): {}\n  {}\n\n"
            L"\u2022 Map Binary (map{:02}.dat): {}\n  {}\n\n"
            L"\u2022 Briefing File (miss{:02}.dat): {}\n  {}\n\n"
            L"Please verify that all 3 files exist and are valid.",
            paths.missionId, res.gameStatus, gPath,
            paths.mapId, res.mapStatus, mPath,
            paths.missionId, res.missStatus, bPath);
    }

    return res;
}

TriadOverwriteInfo MissionTriad::checkOverwrite(const MissionTriadPaths &paths) {
    TriadOverwriteInfo info;
    std::error_code ec;

    info.gameExists = !paths.gameFile.empty() && std::filesystem::exists(paths.gameFile, ec);
    info.mapExists = !paths.mapFile.empty() && std::filesystem::exists(paths.mapFile, ec);
    info.missExists = !paths.missFile.empty() && std::filesystem::exists(paths.missFile, ec);

    info.anyExists = (info.gameExists || info.mapExists || info.missExists);

    if (info.anyExists) {
        std::filesystem::path folder;
        if (!paths.gameFile.empty()) folder = paths.gameFile.parent_path();
        else if (!paths.mapFile.empty()) folder = paths.mapFile.parent_path();
        else if (!paths.missFile.empty()) folder = paths.missFile.parent_path();

        info.warningMessage = std::format(
            L"One or more destination files already exist in:\n{}\n\n"
            L"The following existing file(s) will be overwritten:\n",
            folder.wstring());
        if (info.gameExists) {
            info.warningMessage += std::format(L"\u2022 {}\n", paths.gameFile.filename().wstring());
        }
        if (info.mapExists) {
            info.warningMessage += std::format(L"\u2022 {}\n", paths.mapFile.filename().wstring());
        }
        if (info.missExists) {
            info.warningMessage += std::format(L"\u2022 {}\n", paths.missFile.filename().wstring());
        }
        info.warningMessage += L"\nDo you want to proceed and overwrite?";
    }

    return info;
}

} // namespace rs_knl
