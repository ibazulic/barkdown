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

#include "file_operations.hpp"
#include "markdown.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

// ends_with extracts the extension from a file name and converts it to lower
// letters and compares it with the proper suffix. Returns true extensions
// match.
bool ends_with(const std::string &filename, const std::string &suffix) {
    if (filename.length() >= suffix.length()) {
        std::string extension =
            filename.substr(filename.length() - suffix.length());
        std::transform(extension.begin(), extension.end(), extension.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return extension == suffix;
    }
    return false;
}

// read_file reads the file on the provided path. Returns a buffer of file
// contents back to the caller.
enum file_errors read_file(std::string path, std::string &buffer) {
    // check if the path is valid since we will be using it for both command
    // line reading and for dialog reading
    std::ifstream file(path);

    if (!file.is_open()) {
        return FILE_NOT_FOUND;
    }

    // check file extension
    if (!ends_with(path, ".md")) {
        return FILE_WRONG_EXTENSION;
    }

    std::ostringstream ss;
    ss << file.rdbuf();

    if (file.bad()) {
        return FILE_READING_FAILED;
    }

    std::string input = ss.str();
    if (input.empty()) {
        return FILE_EMPTY;
    }

    buffer = input;
    return FILE_READ_SUCCESSFUL;
}
