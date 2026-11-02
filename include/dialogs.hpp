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
#include <string>
#include <imgui.h>
#include <filesystem>
#include <vector>

struct dir_entry {
    std::string name;
    bool is_directory;
};

// severity represents the severity of the dialog popup.
enum severity {
    DIALOG_INFORMATION,
    DIALOG_WARNING,
    DIALOG_ERROR,
};

// fixed UI font scale for all dialog screens and the main application
constexpr float UI_FONT_SCALE = 2.0f;

// FileDialog class implements methods and functions needed for opening
// Markdown documents.
class FileDialog {
    public:
        // open opens the file dialog screen and displays it on the screen.
        void open(const std::string &start_path = "");

        // render renders the dialog screen constantly until a file has been chosen.
        // Returns true when a file is picked for rendering
        bool render(std::string &out_path);
    private:
        bool should_open_ = false;
        std::filesystem::path current_dir_;
        // tracks which file is selected in the dialog screen.
        int selected_index_ = -1;
        std::vector<dir_entry> entries_;
        // refresh_entries refreshes the entries in the dialog screen after directory change.
        void refresh_entries();
};

// MessageDialog implements methods and functions needed to display various messages during
// program execution.
class MessageDialog {
    public:
        void show(const std::string &title, const std::string &message, severity sev);
        void render();
    private:
        bool should_open_ = false;
        severity sev_ = DIALOG_INFORMATION;
        std::string title_;
        std::string message_;
};
