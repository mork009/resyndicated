// SPDX-License-Identifier: GPL-3.0-or-later
#import <AppKit/AppKit.h>
#include <algorithm>
#include <cmath>
#include "game_menu_macos.h"
#include "sdl/system_sdl.h"
#include "rs-engine/ui/settings_dialog.h"

using rs_eng::SystemSDL;

@interface RSGameMenuController : NSObject <NSMenuItemValidation>
@property(nonatomic, assign) SystemSDL *system;
@property(nonatomic, assign) SDL_Window *window;
@property(nonatomic, strong) NSMutableArray<NSMenuItem *> *installed;
@end

@implementation RSGameMenuController
- (void)runCommand:(NSMenuItem *)sender {
    self.system->handleMenuCommand(static_cast<int>(sender.tag));
}

- (BOOL)validateMenuItem:(NSMenuItem *)item {
    NSInteger tag = item.tag;
    bool checked = false;
    bool enabled = true;
    if (tag >= 2001 && tag <= 2006) {
        int w = 0, h = 0;
        SDL_GetWindowSize(self.window, &w, &h);
        int scale = w / rs_eng::kScreenWidth;
        bool preset = scale >= 1 && scale <= 4 && w == scale * rs_eng::kScreenWidth &&
                      h == scale * rs_eng::kScreenHeight;
        if (tag == 2005) checked = (SDL_GetWindowFlags(self.window) & SDL_WINDOW_FULLSCREEN) != 0;
        else if (tag == 2006) checked = !preset;
        else {
            checked = preset && scale == tag - 2000;
            enabled = self.system->isScaleAllowed(static_cast<int>(tag - 2000));
        }
    } else if (tag >= 4001 && tag <= 4005) {
        int selected = 4004;
        if (std::abs(rs_eng::g_mapZoom - 1.0) < 0.01) selected = 4001;
        else if (std::abs(rs_eng::g_mapZoom - 1.5) < 0.01) selected = 4002;
        else if (std::abs(rs_eng::g_mapZoom - 2.0) < 0.01) selected = 4003;
        else if (std::abs(rs_eng::g_mapZoom - 0.5) < 0.01) selected = 4005;
        checked = tag == selected;
    } else if (tag == 5001) checked = self.system->getShowCoordinates();
    else if (tag == 5002) checked = self.system->getShowAgentPaths();
    else if (tag == 5003) checked = self.system->getShowStats();
    item.state = checked ? NSControlStateValueOn : NSControlStateValueOff;
    return enabled;
}

- (NSMenu *)submenu:(NSString *)title inMenu:(NSMenu *)parent {
    NSMenuItem *item = [parent itemWithTitle:title];
    if (item.submenu) return item.submenu;
    item = [parent addItemWithTitle:title action:nil keyEquivalent:@""];
    item.submenu = [[NSMenu alloc] initWithTitle:title];
    [self.installed addObject:item];
    return item.submenu;
}

- (void)add:(NSString *)title command:(int)command key:(NSString *)key to:(NSMenu *)menu {
    NSMenuItem *item = [menu addItemWithTitle:title action:@selector(runCommand:) keyEquivalent:key];
    item.tag = command;
    item.target = self;
    [self.installed addObject:item];
}
@end

namespace rs_eng {

void *installMacGameMenus(SystemSDL *system, SDL_Window *window) {
    RSGameMenuController *controller = [[RSGameMenuController alloc] init];
    controller.system = system;
    controller.window = window;
    controller.installed = [NSMutableArray array];
    NSMenu *bar = NSApp.mainMenu;
    if (!bar) NSApp.mainMenu = bar = [[NSMenu alloc] initWithTitle:@""];
    if (!bar.numberOfItems) {
        NSMenu *app = [controller submenu:@"ReSyndicated" inMenu:bar];
        [controller add:@"About ReSyndicated" command:6001 key:@"" to:app];
        [controller add:@"Quit ReSyndicated" command:1001 key:@"q" to:app];
    }
    NSMenu *file = [controller submenu:@"File" inMenu:bar];
    [controller add:@"Settings…" command:1002 key:@"," to:file];
    NSMenu *view = [controller submenu:@"View" inMenu:bar];
    NSMenu *sizes = [controller submenu:@"Window Size" inMenu:view];
    for (int s = 1; s <= 4; ++s) {
        NSString *name = [NSString stringWithFormat:@"%d× (%d × %d)", s, s * kScreenWidth, s * kScreenHeight];
        [controller add:name command:2000 + s key:@"" to:sizes];
    }
    [controller add:@"Custom Size…" command:2006 key:@"" to:sizes];
    NSMenu *zoom = [controller submenu:@"Map Zoom" inMenu:view];
    [controller add:@"0.5×" command:4005 key:@"" to:zoom];
    [controller add:@"1×" command:4001 key:@"" to:zoom];
    [controller add:@"1.5×" command:4002 key:@"" to:zoom];
    [controller add:@"2×" command:4003 key:@"" to:zoom];
    [controller add:@"Custom Zoom…" command:4004 key:@"" to:zoom];
    [controller add:@"Full Screen" command:2005 key:@"" to:view];

    NSMenu *debug = [controller submenu:@"Debug" inMenu:bar];
    [controller add:@"Show Tile Coordinates" command:5001 key:@"" to:debug];
    [controller add:@"Show Agent Paths" command:5002 key:@"" to:debug];
    [controller add:@"Show Stats" command:5003 key:@"" to:debug];
    NSMenu *cheats = [controller submenu:@"Cheats" inMenu:debug];
    NSArray *names = @[@"Enable All Cheats", @"Play Any Mission", @"Add Max Funds ($100M)",
                       @"Unlock All Weapons & Upgrades", @"Equip All V3 Upgrades", @"Equip Top Weapons & Items"];
    for (NSUInteger i = 0; i < names.count; ++i)
        [controller add:names[i] command:3001 + static_cast<int>(i) key:@"" to:cheats];
    return (__bridge_retained void *)controller;
}

void removeMacGameMenus(void *handle) {
    RSGameMenuController *controller = (__bridge_transfer RSGameMenuController *)handle;
    for (NSMenuItem *item in controller.installed.reverseObjectEnumerator)
        [item.menu removeItem:item];
}

bool chooseMacDataFolder(const std::string &current, std::string &selected) {
    bool visible = SDL_CursorVisible();
    SDL_ShowCursor();
    bool changed = showGameDataSetup(current, selected);
    std::error_code ec;
    if (changed && !std::filesystem::equivalent(current, selected, ec)) {
        NSAlert *alert = [[NSAlert alloc] init];
        alert.messageText = @"Reload game data from this folder?";
        alert.informativeText = [NSString stringWithUTF8String:selected.c_str()];
        [alert addButtonWithTitle:@"Reload"];
        [alert addButtonWithTitle:@"Cancel"];
        changed = [alert runModal] == NSAlertFirstButtonReturn;
    } else {
        changed = false;
    }
    if (!visible) SDL_HideCursor();
    return changed;
}

static bool promptValues(NSString *title, NSString *message, NSArray<NSTextField *> *fields) {
    NSStackView *stack = [NSStackView stackViewWithViews:fields];
    stack.frame = NSMakeRect(0, 0, 250, 28);
    stack.distribution = NSStackViewDistributionFillEqually;
    NSAlert *alert = [[NSAlert alloc] init];
    alert.messageText = title;
    alert.informativeText = message;
    alert.accessoryView = stack;
    [alert addButtonWithTitle:@"Apply"];
    [alert addButtonWithTitle:@"Cancel"];
    alert.window.initialFirstResponder = fields.firstObject;
    bool visible = SDL_CursorVisible();
    SDL_ShowCursor();
    bool accepted = [alert runModal] == NSAlertFirstButtonReturn;
    if (!visible) SDL_HideCursor();
    return accepted;
}

bool showCustomSizeDialog(SDL_Window *, int &w, int &h, int minW, int minH, int maxW, int maxH) {
    NSTextField *width = [NSTextField textFieldWithString:[@(w) stringValue]];
    NSTextField *height = [NSTextField textFieldWithString:[@(h) stringValue]];
    NSString *message = [NSString stringWithFormat:@"Width × height in pixels (%d × %d to %d × %d).", minW, minH, maxW, maxH];
    if (!promptValues(@"Custom Window Size", message, @[width, height])) return false;
    w = std::clamp(width.intValue, std::min(minW, maxW), maxW);
    h = std::clamp(height.intValue, std::min(minH, maxH), maxH);
    return true;
}

bool showCustomZoomDialog(SDL_Window *, double &zoom) {
    NSTextField *field = [NSTextField textFieldWithString:[@(zoom) stringValue]];
    if (!promptValues(@"Custom Map Zoom", @"Enter a zoom factor from 0.5 to 4.", @[field])) return false;
    double value = field.doubleValue;
    zoom = (std::isfinite(value) && value > 0) ? std::clamp(value, 0.5, 4.0) : 1.0;
    return true;
}

void showGameAboutDialog(SDL_Window *) {
    [NSApp orderFrontStandardAboutPanel:nil];
}

} // namespace rs_eng
