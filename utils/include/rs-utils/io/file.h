/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
 *   Copyright (C) 2010, 2024-2026  Benoit Blancard <benblan@users.sourceforge.net>
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

#ifndef FILE_H
#define FILE_H

#include <string>
#include <vector>
#include <stdio.h>
#include <filesystem>

#include "rs-utils/common.h"
#include "rs-utils/io/configfile.h"

namespace fs = std::filesystem;

namespace rs_utl {
    /*!
    * File class gives a list of convenient methods to retrieve
    * files and directories important for the application.
    */
    class File {
    public:
        //! Return the path for the unified resynd.conf file
        static bool getResyndConfFullPath(fs::path& confFullPath);
        //! Return the path for the user folder
        static bool getOrCreateUserConfFolder(const std::string& userConfFolder);
        //! Sets the path to the original data files.*/
        static void setOriginalDataFolder(const std::string& path);
        //! Returns the path to the original data files.
        static const fs::path& getOriginalDataFolder();
        //! Sets the path to our data files.*/
        static void setResyndDataFolder(const std::string& path);
        //! Returns the path to our bundled data files.
        static const fs::path& getResyndDataFolder();
        //! Sets the path where to save all files.*/
        static void upsertSaveDataFolder(const std::string& path);
        static void getDefaultSaveFolder(std::string& confFolderPath);

        //*************************************
        // Original files apis
        //*************************************
        static uint8_t *loadOriginalFile(const std::string& filename, size_t &filesize);
        static uint8_t *loadFileFromDirToMem(const std::filesystem::path &dir, const std::string &filename, size_t &filesize);
        static uint8_t *loadOriginalFileFromDir(const std::filesystem::path &dir, const std::string &filename, size_t &filesize);
        static FILE *openOriginalFile(const std::string& filename);

        //! Performs full structural and format verification of Syndicate original data in dir
        static bool verifyOriginalData(const std::filesystem::path &originalDir, std::string *outError = nullptr);
        //! Computes fast size/date signature for a data directory (count:total_size:max_mtime)
        static std::string computeDataSignature(const std::filesystem::path &dir);
        //! Tests Syndicate original data for existence and structural correctness in configured data path
        static bool testOriginalData();
        //! Returns true if the specified directory (or currently configured dataPath_) contains American Revolt expansion assets
        static bool hasAmericanRevoltData(const std::filesystem::path &originalDir = "");

        //*************************************
        // ReSyndicated files apis
        //*************************************
        //! Returns the full path of the given resource using the current root path.
        static std::string getResyndDataFullPath(const std::string& filename);

        //! Sets the filename fullpath for the given slot (from 0 to 9)
        static void getFullPathForSaveSlot(int slot, std::string &path);
        //! Returns the list of game saved names
        static void getGameSavedNames(std::vector<std::string> &files);
        static uint8_t *loadOriginalFileToMem(const std::string& filename, size_t &filesize);
        //! Returns the full path of the given original game resource using the current root path.
        static std::string getOriginalDataFullPath(const std::string& filename, bool uppercase = false);

    private:
        static void addSaveFilenameAtIndex(const fs::path& filename, std::vector<std::string> &files);
        //! Adds a trailing slash to the string
        static void addMissingSlash(std::string& str);
        

    private:
        /*! The path to the original game data.*/
        static fs::path dataPath_;
        /*! The path to our data files.*/
        static fs::path ourDataPath_;
        //! The path to the folder containing the user conf file
        static fs::path userConfFolderPath_;
        /*! The path to the save directory.*/
        static fs::path savePath_;
    };
};

#endif
