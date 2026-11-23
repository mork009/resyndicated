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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <filesystem>
#include <string>
#include <iostream>
#include <fstream>

#include "main_window.h"
#include "editor_config.h"
#include "playtest_loadout_dialog.h"
#include "editor_settings_dialog.h"
#include "dialog_theme.h"
#include "resource.h"
#include "rs-utils/io/file.h"
#include "rs-utils/io/configfile.h"

#include <csignal>
#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")

static void logStackTrace(const char *reason) {
    std::filesystem::path crashLog = rs_win_editor::EditorConfig::getConfigPath().parent_path() / "editor_crash.log";
    std::ofstream out(crashLog, std::ios::out | std::ios::app);
    out << "=== CRASH / ABORT: " << reason << " ===" << std::endl;
    
    void *stack[64];
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);
    WORD frames = CaptureStackBackTrace(0, 64, stack, NULL);

    SYMBOL_INFO *symbol = (SYMBOL_INFO *)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
    if (symbol) {
        symbol->MaxNameLen = 255;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

        IMAGEHLP_LINE64 line;
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD displacement;

        for (WORD i = 0; i < frames; i++) {
            DWORD64 address = (DWORD64)(stack[i]);
            SymFromAddr(process, address, 0, symbol);
            if (SymGetLineFromAddr64(process, address, &displacement, &line)) {
                out << "[" << i << "] " << symbol->Name << " (" << line.FileName << ":" << line.LineNumber << ")" << std::endl;
            } else {
                out << "[" << i << "] " << symbol->Name << " (0x" << std::hex << address << std::dec << ")" << std::endl;
            }
        }
        free(symbol);
    }
    out.flush();
}

static void abortHandler(int) {
    logStackTrace("SIGABRT / abort() called");
    _exit(1);
}

static LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS *pExceptionInfo) {
    std::string reason = std::format("Unhandled Exception: 0x{:08X}", pExceptionInfo->ExceptionRecord->ExceptionCode);
    logStackTrace(reason.c_str());
    return EXCEPTION_CONTINUE_SEARCH;
}

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace fs = std::filesystem;

static bool configureAndVerifyDataDirectories(HINSTANCE /*hInstance*/,
                                             const std::wstring &customData,
                                             rs_win_editor::EditorPathsConfig &outConfiguredPaths) {
    using namespace rs_win_editor;

    // ReSyndicated data files are bundled with the application
    rs_utl::File::setResyndDataFolder("");

    // 1. Explicit CLI argument: -d <path> or --data <path>
    if (!customData.empty()) {
        std::string sData = fs::path(customData).string();
        auto val = rs_eng::verifyDataDirectories(sData);
        if (val.isValid) {
            outConfiguredPaths.dataDir = sData;
            outConfiguredPaths.dataSignature = rs_utl::File::computeDataSignature(sData);
            EditorConfig::savePathsConfig(outConfiguredPaths);
            rs_utl::File::setOriginalDataFolder(outConfiguredPaths.dataDir);
            return true;
        } else {
            EditorPathsConfig inPaths{ sData };
            EditorPathsConfig selected;
            if (showDataDirSetupDialog(nullptr, inPaths, selected)) {
                outConfiguredPaths = selected;
                outConfiguredPaths.dataSignature = rs_utl::File::computeDataSignature(outConfiguredPaths.dataDir);
                EditorConfig::savePathsConfig(outConfiguredPaths);
                rs_utl::File::setOriginalDataFolder(outConfiguredPaths.dataDir);
                return true;
            }
            return false;
        }
    }

    // 2. Check saved editor configuration (~/.resynd/resynd.conf)
    EditorPathsConfig pathsCfg;
    bool hasSaved = EditorConfig::loadPathsConfig(pathsCfg) && !pathsCfg.dataDir.empty();
    if (hasSaved) {
        std::string curSig = rs_utl::File::computeDataSignature(pathsCfg.dataDir);
        bool fastMatch = !pathsCfg.dataSignature.empty() && (pathsCfg.dataSignature == curSig);
        if (fastMatch) {
            outConfiguredPaths = pathsCfg;
            rs_utl::File::setOriginalDataFolder(outConfiguredPaths.dataDir);
            return true;
        }

        // Fast check did not match (files changed/touched or first run after config): run structural verification
        auto val = rs_eng::verifyDataDirectories(pathsCfg.dataDir);
        if (val.isValid) {
            pathsCfg.dataSignature = curSig;
            outConfiguredPaths = pathsCfg;
            EditorConfig::savePathsConfig(outConfiguredPaths);
            rs_utl::File::setOriginalDataFolder(outConfiguredPaths.dataDir);
            return true;
        } else {
            std::cerr << "Configured data directory invalid or modified incorrectly. Prompting setup..." << std::endl;
            EditorPathsConfig selected;
            if (showDataDirSetupDialog(nullptr, pathsCfg, selected)) {
                outConfiguredPaths = selected;
                outConfiguredPaths.dataSignature = rs_utl::File::computeDataSignature(outConfiguredPaths.dataDir);
                EditorConfig::savePathsConfig(outConfiguredPaths);
                rs_utl::File::setOriginalDataFolder(outConfiguredPaths.dataDir);
                return true;
            }
            return false;
        }
    }

    // 3. First-time setup: prompt user through setup flow
    EditorPathsConfig initial;
    EditorPathsConfig selected;
    if (showDataDirSetupDialog(nullptr, initial, selected)) {
        outConfiguredPaths = selected;
        outConfiguredPaths.dataSignature = rs_utl::File::computeDataSignature(outConfiguredPaths.dataDir);
        EditorConfig::savePathsConfig(outConfiguredPaths);
        rs_utl::File::setOriginalDataFolder(outConfiguredPaths.dataDir);
        return true;
    }

    return false;
}

namespace {

struct StreamRedirector {
    std::ofstream file;
    std::streambuf *oldCerr = nullptr;
    std::streambuf *oldCout = nullptr;

    explicit StreamRedirector(const std::filesystem::path &logPath) {
        std::error_code ec;
        if (logPath.has_parent_path()) {
            std::filesystem::create_directories(logPath.parent_path(), ec);
        }
        file.open(logPath, std::ios::out | std::ios::trunc);
        if (file.is_open()) {
            oldCerr = std::cerr.rdbuf(file.rdbuf());
            oldCout = std::cout.rdbuf(file.rdbuf());
        }
    }

    ~StreamRedirector() {
        if (oldCerr) std::cerr.rdbuf(oldCerr);
        if (oldCout) std::cout.rdbuf(oldCout);
    }
};

} // namespace

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, PWSTR pCmdLine, int nCmdShow) {
    signal(SIGABRT, abortHandler);
    SetUnhandledExceptionFilter(unhandledExceptionFilter);

    std::filesystem::path logPath = rs_win_editor::EditorConfig::getConfigPath().parent_path() / "editor.log";
    if (logPath.empty() || logPath.parent_path().empty()) {
        logPath = "editor.log";
    }
    StreamRedirector redirector(logPath);

    // Enable modern Per-Monitor V2 DPI awareness
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // Enable system dark mode for Win32 menus, standard dialogs, and controls
    HMODULE hUxTheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (hUxTheme) {
        using fnSetPreferredAppMode = int(WINAPI*)(int);
        using fnFlushMenuThemes = void(WINAPI*)();
        auto setPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(GetProcAddress(hUxTheme, MAKEINTRESOURCEA(135)));
        if (setPreferredAppMode) {
            setPreferredAppMode(2); // 2 = ForceDark
        }
        auto flushMenuThemes = reinterpret_cast<fnFlushMenuThemes>(GetProcAddress(hUxTheme, MAKEINTRESOURCEA(136)));
        if (flushMenuThemes) {
            flushMenuThemes();
        }
    }

    // Initialize Common Controls 6.0
    INITCOMMONCONTROLSEX icex = {};
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_TAB_CLASSES | ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    // Parse command line arguments
    int argc = 0;
    LPWSTR *argv = CommandLineToArgvW(pCmdLine, &argc);

    int startMissionId = -1;
    std::wstring customData;
    bool showSettingsDirectly = false;

    if (argv) {
        for (int i = 0; i < argc; ++i) {
            std::wstring arg = argv[i];
            if ((arg == L"-m" || arg == L"--mission") && i + 1 < argc) {
                startMissionId = std::stoi(argv[++i]);
            } else if ((arg == L"-d" || arg == L"--data") && i + 1 < argc) {
                customData = argv[++i];
            } else if (arg == L"--settings") {
                showSettingsDirectly = true;
            } else if (arg == L"-h" || arg == L"--help" || arg == L"/?") {
                MessageBoxW(nullptr,
                    L"ReSyndicated Mission Editor\n\n"
                    L"Usage: resynd_editor.exe [options]\n\n"
                    L"Options:\n"
                    L"  -m, --mission <id>   Open specified mission ID directly (e.g. -m 1)\n"
                    L"  -d, --data <path>    Specify path to original Syndicate data directory\n"
                    L"  --settings           Open settings dialog to configure data path\n"
                    L"  -h, --help           Display this help message",
                    L"ReSyndicated Editor Help", MB_OK | MB_ICONINFORMATION);
                return 0;
            }
        }
        LocalFree(argv);
    }

    // Setup and verify asset data directories
    rs_win_editor::EditorPathsConfig configuredPaths;
    if (!configureAndVerifyDataDirectories(hInstance, customData, configuredPaths)) {
        std::cerr << "Editor startup canceled or data directory not configured." << std::endl;
        return 0;
    }

    if (showSettingsDirectly) {
        rs_win_editor::EditorPathsConfig outPaths = configuredPaths;
        rs_win_editor::showEditorSettingsDialog(nullptr, outPaths);
        return 0;
    }

    rs_win_editor::EditorConfig::ensureDefaultLoadoutSection();
    if (!rs_utl::File::hasAmericanRevoltData()) {
        rs_win_editor::PlaytestLoadoutData playtestData;
        rs_win_editor::EditorConfig::loadLoadoutConfig(playtestData);
    }

    // Create Main Window
    rs_win_editor::MainWindow mainWindow;
    try {
        if (!mainWindow.create(hInstance, nCmdShow, 1600, 960)) {
            rs_win_editor::showEditorMessageBox(nullptr, L"Failed to create ReSyndicated Editor window.", L"Error", MB_ICONERROR);
            return 1;
        }

        if (startMissionId > 0) {
            mainWindow.loadMission(startMissionId);
        }
    } catch (const std::exception &ex) {
        std::wstring msg = std::format(L"Error initializing editor:\n\n{}\n\nPlease verify your asset directory settings.", 
            std::wstring(ex.what(), ex.what() + strlen(ex.what())));
        rs_win_editor::showEditorMessageBox(nullptr, msg.c_str(), L"ReSyndicated Editor Initialization Error", MB_ICONERROR);
        return 1;
    } catch (...) {
        rs_win_editor::showEditorMessageBox(nullptr, L"An unexpected error occurred while initializing the editor.", L"ReSyndicated Editor Error", MB_ICONERROR);
        return 1;
    }

    // Load accelerator shortcuts
    HACCEL hAccel = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(IDR_ACCELERATOR));

    // Win32 Message Loop
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        bool translateAccel = true;
        if (msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST) {
            HWND hFocus = GetFocus();
            if (hFocus) {
                wchar_t className[64] = {};
                GetClassNameW(hFocus, className, 64);
                if (_wcsicmp(className, L"Edit") == 0) {
                    // When an Edit control has input focus, don't let single-key shortcuts
                    // (like '1'-'3', '+', '-', 'P', 'B', 'V', 'Z') hijack text typing.
                    // Only pass accelerators with Ctrl (e.g. Ctrl+S, Ctrl+Z) or function keys.
                    bool hasCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                    bool isFKey = (msg.wParam >= VK_F1 && msg.wParam <= VK_F24);
                    if (!hasCtrl && !isFKey) {
                        translateAccel = false;
                    }
                }
            }
        }
        if (!translateAccel || !TranslateAcceleratorW(mainWindow.hwnd(), hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return static_cast<int>(msg.wParam);
}
