/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
 *   Copyright (C) 2011  Joey Parrish  <joey.parrish@gmail.com>
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
#ifndef MODMANAGER_H
#define MODMANAGER_H

#include <fstream>

#include "rs-utils/common.h"
#include "rs-kernel/model/mod.h"
#include "rs-utils/misc/seqmodel.h"
#include "rs-utils/misc/singleton.h"
#include "rs-utils/io/portablefile.h"
#include "rs-utils/io/formatversion.h"

namespace rs_knl {

/*!
 * Modifications manager class.
 */
class ModManager : public Singleton < ModManager > {
public:
    //! Constructor
    ModManager();
    //! Destructor
    ~ModManager();

    //! Resources destruction
    void destroy();
    //! Reset Data
    void reset();
    //! Cheating mode to enable all mods
    void cheatEnableAllMods();

    Mod *getMod(Mod::EModType mt, Mod::EModVersion ver);

    //! Enable mod of given type and version
    void enableMod(Mod::EModType mt, Mod::EModVersion ver);
    //! Returns the list of currently available weapons
    SequenceModel * getAvalaibleMods() { return &mods_; }

    //! Save instance to file
    bool saveToFile(rs_utl::PortableFile &file);
    //! Load instance from file
    bool loadFromFile(rs_utl::PortableFile &infile, const rs_utl::FormatVersion& v);
    Mod *getHighestVersion(Mod::EModType mt);

protected:
    //! Loads the mod from file
    Mod *loadMod(Mod::EModType mt, Mod::EModVersion ver);

protected:
    /*! This vector is used to store necessary but unavailable mods until there
     * are made available.*/
    std::vector<Mod *> preFetch_;
    //! The list of currently available mods
    VectorModel<Mod *> mods_;
};

}

#define g_modMgr    rs_knl::ModManager::singleton()

#endif
