// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <SDL3/SDL_video.h>
#include <string>

namespace rs_eng {
class SystemSDL;
void *installMacGameMenus(SystemSDL *system, SDL_Window *window);
void removeMacGameMenus(void *controller);
bool chooseMacDataFolder(const std::string &current, std::string &selected);
bool showCustomSizeDialog(SDL_Window *, int &width, int &height, int minW, int minH, int maxW, int maxH);
bool showCustomZoomDialog(SDL_Window *, double &zoom);
void showGameAboutDialog(SDL_Window *);
}
