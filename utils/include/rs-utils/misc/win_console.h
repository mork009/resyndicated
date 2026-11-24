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

#ifndef RS_UTILS_MISC_WIN_CONSOLE_H_
#define RS_UTILS_MISC_WIN_CONSOLE_H_

namespace rs_utl {

/*!
 * Attaches a Windows GUI (SUBSYSTEM:WINDOWS) executable back to its parent
 * terminal or inherited stdout/stderr handles (e.g. for --help output),
 * and cleanly releases the console on destruction so the shell prompt returns.
 * No-op on POSIX platforms where stdout/stderr are already attached.
 */
class WinParentConsole {
public:
    WinParentConsole() = default;
    ~WinParentConsole();

    WinParentConsole(const WinParentConsole &) = delete;
    WinParentConsole &operator=(const WinParentConsole &) = delete;

    void attach();
    void detach();

private:
#ifdef _WIN32
    bool attached_ = false;
#endif
};

} // namespace rs_utl

#endif // RS_UTILS_MISC_WIN_CONSOLE_H_
