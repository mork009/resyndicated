// SPDX-License-Identifier: GPL-3.0-or-later
#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include "rs-engine/ui/settings_dialog.h"
#include "sdl/system_sdl.h"

namespace rs_eng {

bool showGameDataSetup(const std::string &initialPath, std::string &selectedPath) {
    @autoreleasepool {
        // First-run setup runs before SDL creates the game window.
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        [NSApp activateIgnoringOtherApps:YES];

        NSOpenPanel *panel = [NSOpenPanel openPanel];
        panel.message = @"Choose the DATA folder from your original copy of Syndicate.";
        panel.canChooseDirectories = YES;
        panel.canChooseFiles = NO;
        if (!initialPath.empty()) {
            panel.directoryURL = [NSURL fileURLWithFileSystemRepresentation:initialPath.c_str()
                                                                isDirectory:YES relativeToURL:nil];
        }
        while ([panel runModal] == NSModalResponseOK) {
            std::filesystem::path path(panel.URL.fileSystemRepresentation);
            auto result = verifyDataDirectories(path);
            if (result.isValid) {
                selectedPath = path.string();
                return true;
            }
            NSAlert *alert = [[NSAlert alloc] init];
            alert.messageText = @"Could not load game data";
            if (result.bundledDataValid) {
                NSMutableArray<NSString *> *errors = [NSMutableArray array];
                for (const auto &check : result.originalChecks) {
                    if (!check.isPresent) {
                        std::string error = check.filename + ": " + check.detail;
                        [errors addObject:[NSString stringWithUTF8String:error.c_str()]];
                    }
                }
                alert.informativeText = [errors componentsJoinedByString:@"\n"];
            } else {
                alert.informativeText = @"The bundled ReSyndicated data folder is missing or incomplete.";
            }
            [alert runModal];
        }
        return false;
    }
}

bool SystemSDL::browseFile(const char *title, const char *filterDesc, const char *filterPattern,
                           std::filesystem::path &outPath) {
    @autoreleasepool {
        NSOpenPanel *panel = [NSOpenPanel openPanel];
        if (title) panel.message = [NSString stringWithUTF8String:title];
        NSMutableArray<UTType *> *types = [NSMutableArray array];
        NSString *patterns = filterPattern ? [NSString stringWithUTF8String:filterPattern] : nil;
        for (NSString *pattern in [patterns componentsSeparatedByString:@";"]) {
            if (!pattern.pathExtension.length) continue;
            UTType *type = [UTType typeWithFilenameExtension:pattern.pathExtension];
            if (type && ![types containsObject:type]) [types addObject:type];
        }
        if (types.count) panel.allowedContentTypes = types;

        bool wasVisible = SDL_CursorVisible();
        SDL_ShowCursor();
        bool selected = [panel runModal] == NSModalResponseOK;
        if (!wasVisible) SDL_HideCursor();
        if (selected) outPath = panel.URL.fileSystemRepresentation;
        return selected;
    }
}

} // namespace rs_eng
