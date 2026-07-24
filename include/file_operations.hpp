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
#include <iostream>
#include <string>

// define some file errors that we can encounter during reading or opening files
enum file_errors {
    FILE_NOT_FOUND,
    FILE_WRONG_EXTENSION,
    FILE_IS_BINARY,
    FILE_READING_FAILED,
    FILE_READ_SUCCESSFUL,
    FILE_EMPTY,
};

// read_file reads the file on the provided path. Returns a buffer of file
// contents back to the caller.
enum file_errors read_file(std::string, std::string &);

// ends_with compares the end of two provided strings and returns true if the
// strings match. The amount of letters that will be compared depends on the
// length of the 2nd string.
bool ends_with(const std::string &, const std::string &);
