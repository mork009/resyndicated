/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2011, 2024-2025  Benoit Blancard <benblan@users.sourceforge.net>
 *   Copyright (C) 2011  Joey Parrish  <joey.parrish@gmail.com>         *
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
#ifndef RESEARCHMANAGER_H
#define RESEARCHMANAGER_H

#include <list>
#include <fstream>

#include "rs-utils/misc/seqmodel.h"
#include "rs-utils/io/portablefile.h"
#include "rs-utils/io/formatversion.h"
#include "rs-kernel/model/research.h"
#include "rs-kernel/mgr/weaponmanager.h"
#include "rs-kernel/mgr/modmanager.h"

namespace rs_knl {
/*!
 * This class manages all research functionality.
 */
class ResearchManager {
public:
    //! Default constructor
    ResearchManager(WeaponManager *pWeaponManager, ModManager *pModManager);
    ~ResearchManager();

    bool reset();
    /*!
     * Returns a list of all available search on mods.
     * \return List can be empty but not null.
     */
    SequenceModel * getAvailableModsSearch() { return &availableModsSearch_; }
    /*!
     * Returns a list of all available search on weapons.
     * \return List can be empty but not null.
     */
    SequenceModel * getAvailableWeaponsSearch() { return &availableWeaponsSearch_; }

    /*!
     * Returns the current active research.
     * \returb NULL if no research is active
     */
    Research *getActiveSearch() { return pCurrResearch_; }

    //! An unknown weapon has been found -> it will boost research
    bool handleWeaponDiscovered(Weapon *pWeapon);
    //! Starts research and suspends current search
    void start(Research *pResearch);
    // Processes all engaged research
    int process(int hourElapsed, int moneyLeft);

    //! Save instance to file
    bool saveToFile(rs_utl::PortableFile &file);
    //! Load instance from file
    bool loadFromFile(rs_utl::PortableFile &infile, const rs_utl::FormatVersion& v);

protected:
    Research *loadResearch(Weapon::WeaponType wt);
    Research *loadResearch(Mod::EModType mt, Mod::EModVersion version);
    //! Terminate given search
    void complete(Research *pResearch);
    void removeSearch(Research *pOldSearch);
    void replaceSearch(Research *pOldSearch, Research *pNewSearch);
    void destroy();

protected:
    /*! List of all currently available research on mods.*/
    VectorModel<Research *> availableModsSearch_;
    /*! List of all currently available research on weapons.*/
    VectorModel<Research *> availableWeaponsSearch_;
    /*! There is only one active search at a time.*/
    Research *pCurrResearch_;
    WeaponManager *pWeaponManager_;
    ModManager *pModManager_;
};

}
#endif //RESEARCHMANAGER_H
