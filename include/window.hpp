/*
Copyright (c) 2026. Ivan Bazulic

Barkdown is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Barkdown is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Barkdown.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once
#include "image_loader.hpp"
#include <iostream>

// error codes for window creation and rendering
enum errorcodes {
    SUCCESSFUL_EXIT,
    DETERMINE_WINDOW_SIZE_FAILED,
    WINDOW_CREATION_FAILED,
    RENDERER_CREATION_FAILED,
    SDL_INIT_FAILED,
    GL_CONTEXT_CREATE_FAILED,
};

enum icon {
    ICON_WARNING,
    ICON_STOP,
    ICON_INFORMATION,
};

// main_window creates an SDL window with the provided title and size. Pass the
// filename of the file for rendering if provided.
enum errorcodes main_window(const char *window_title, int height, int width,
                            std::string filename);

// create_ok_modal_dialog creates a modal dialog with the OK button. Consumes
// the icon information and text to render.
void create_ok_modal_dialog(std::string window_title, std::string msg,
                            enum icon);
