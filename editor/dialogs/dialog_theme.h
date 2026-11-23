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

#ifndef EDITOR_DIALOG_THEME_H_
#define EDITOR_DIALOG_THEME_H_

#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <string>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

namespace rs_win_editor {

// Core Dark Cybernetic Palette
constexpr COLORREF kColorDlgBg           = RGB(28, 30, 34);    // #1C1E22
constexpr COLORREF kColorControlBg       = RGB(20, 22, 25);    // #141619
constexpr COLORREF kColorBorder          = RGB(58, 65, 74);    // #3A414A
constexpr COLORREF kColorBorderHighlight = RGB(85, 98, 115);   // #556273
constexpr COLORREF kColorText            = RGB(225, 230, 236); // #E1E6EC
constexpr COLORREF kColorTextMuted       = RGB(160, 168, 178); // #A0A8B2
constexpr COLORREF kColorAccentCyan      = RGB(0, 220, 245);   // Syndicate Cyan
constexpr COLORREF kColorAccentRed       = RGB(235, 60, 70);    // EuroCorp Crimson
constexpr COLORREF kColorAccentAmber     = RGB(255, 185, 40);   // Amber Telemetry

// Window & control dark mode enablement
void enableDarkModeForWindow(HWND hWnd);
void enableDarkModeForControl(HWND hWnd);

// Apply complete dark styling to a dialog and all its child controls
void applyDialogTheme(HWND hDlg);
void applyComboBoxTheme(HWND hCombo);
void applyCheckBoxTheme(HWND hBtn);

// Central handler for WM_CTLCOLOR... messages in dialogs
LRESULT handleDialogColorMessages(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, bool &handled);

// High-contrast, dark-themed replacement for MessageBoxW
int showEditorMessageBox(HWND hWndParent, const wchar_t *lpText, const wchar_t *lpCaption, UINT uType);
inline int showEditorMessageBox(HWND hWndParent, const std::wstring &text, const std::wstring &caption, UINT uType) {
    return showEditorMessageBox(hWndParent, text.c_str(), caption.c_str(), uType);
}

// Dedicated dark dialogs
void showShortcutsDialog(HWND hParent);
void showToolsGuideDialog(HWND hParent);
inline void showHowToDialog(HWND hParent) { showToolsGuideDialog(hParent); }
void showAboutDialog(HWND hParent);

} // namespace rs_win_editor

#endif // EDITOR_DIALOG_THEME_H_
