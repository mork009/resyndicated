// SPDX-License-Identifier: GPL-3.0-or-later
#import <AppKit/AppKit.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <unistd.h>
#include <vector>
#include "rs-utils/io/resyndconfig.h"

namespace fs = std::filesystem;

static int launchError(NSString *detail) {
    [NSApplication sharedApplication];
    NSAlert *alert = [[NSAlert alloc] init];
    alert.messageText = @"ReSyndicated Editor could not start";
    alert.informativeText = detail;
    [alert runModal];
    return 1;
}

static std::string winePath(fs::path path) {
    std::string result = "Z:" + fs::absolute(path).lexically_normal().string();
    std::replace(result.begin(), result.end(), '/', '\\');
    return result;
}

int main(int argc, char **argv) {
    @autoreleasepool {
        fs::path resources(NSBundle.mainBundle.resourceURL.fileSystemRepresentation);
        fs::path wine = resources / "Wine Devel.app/Contents/Resources/wine/bin/wine";
        fs::path editor = resources / "editor/resynd_editor.exe";
        if (!fs::exists(wine) || !fs::exists(editor))
            return launchError(@"Build the resynd_editor target to install the editor and its Wine runtime.");

        if (!std::getenv("WINEPREFIX")) {
            NSURL *support = [NSFileManager.defaultManager URLsForDirectory:NSApplicationSupportDirectory
                                                                 inDomains:NSUserDomainMask].firstObject;
            fs::path prefix = fs::path(support.fileSystemRepresentation) / "ReSyndicated/EditorWine";
            std::error_code ec;
            fs::create_directories(prefix, ec);
            if (ec) return launchError([NSString stringWithUTF8String:ec.message().c_str()]);
            setenv("WINEPREFIX", prefix.c_str(), 1);
        }

        std::vector<std::string> args{wine.string(), editor.string()};
        bool explicitData = false;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            args.push_back(arg);
            if ((arg == "-d" || arg == "--data") && i + 1 < argc) {
                std::string path = argv[++i];
                args.push_back((path.size() >= 2 && path[1] == ':') ? path : winePath(path));
                explicitData = true;
            }
        }

        // Reuse the native game's choice on the first run. Later launches respect
        // changes made through File > Settings in the editor's own Wine profile.
        NSString *user = NSUserName();
        fs::path editorConf = fs::path(std::getenv("WINEPREFIX")) / "drive_c/users" /
                              user.fileSystemRepresentation / ".resynd/resynd.conf";
        if (!explicitData && !fs::exists(editorConf)) {
            rs_utl::ResyndConfig cfg;
            if (cfg.load(fs::path(NSHomeDirectory().fileSystemRepresentation) / ".resynd/resynd.conf")) {
                auto path = cfg.getString("Paths", "data_dir", "");
                if (!path.empty() && fs::is_directory(path)) {
                    args.push_back("--data");
                    args.push_back(winePath(path));
                }
            }
        }
        std::vector<char *> arguments;
        for (auto &arg : args) arguments.push_back(arg.data());
        arguments.push_back(nullptr);
        if (chdir(editor.parent_path().c_str()) == 0)
            execv(wine.c_str(), arguments.data());
        return launchError([NSString stringWithFormat:@"Wine could not run (%s). On Apple Silicon, the x64 editor requires Rosetta.", std::strerror(errno)]);
    }
}
