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

#include "markdown.hpp"
#include "window.hpp"
#include <cmark.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "verbosity.hpp"

void usage() {
    std::cerr << "Usage: barkdown [options] /path/to/markdown.md\n";
    std::cerr << "Options:\n\n";
    std::cerr << "/path/to/markdown.md: Path to the Markdown file to render.\n"; 
    std::cerr << "-v, --verbose (optional): Print verbose output on standard error, such as Markdown rendering information.\n\n";
}

int main(int argc, char *argv[]) {
    if (argc > 3) {
        std::cerr << "Wrong number of arguments given, expected at most 3, got " << argc
                  << ".\n";
        usage();
        std::exit(1);
    }

    std::string filename;

    // start from index 1 since index 0 is the invoked filename
    for(int i = 1; i < argc; i++) {
        std::string s = argv[i];
        if (s == "-v" || s == "--verbose") {
            barkdown_verbose = true;
        } else {
            // we cannot tolerate two filenames provided to the command
            if (!filename.empty()) {
                std::cerr << "Multiple filenames provided.\n";
                usage();
                std::exit(1);
            }
            filename = argv[i];
        }
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

