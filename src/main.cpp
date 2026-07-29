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

#include "image_loader.hpp"
#include "markdown.hpp"
#include "window.hpp"
#include <cmark.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char *argv[]) {
    if (argc > 2) {
        std::cerr << "Wrong number of arguments given, expected 2, got " << argc
                  << ".\n";
        std::cerr << "Usage: barkdown filename.md\n";
        std::exit(1);
    }

    std::string filename;

    if (argc == 2) {
        // intepret the 2nd provided parameter as a file name.
        filename = argv[1];
        std::cerr << "File to load: " << filename << ".\n";
    }

    enum errorcodes state =
        main_window("Barkdown Renderer", 720, 1280, filename);
    switch (state) {
    case SUCCESSFUL_EXIT:
        return 0;
    case DETERMINE_WINDOW_SIZE_FAILED:
    case WINDOW_CREATION_FAILED:
    case RENDERER_CREATION_FAILED:
    case SDL_INIT_FAILED:
    case GL_CONTEXT_CREATE_FAILED:
        return 1;
    }
}
