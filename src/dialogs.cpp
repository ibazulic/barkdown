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

#include "dialogs.hpp"
#include "algorithm"
#include "filesystem"

void FileDialog::refresh_entries() {
    entries_.clear();

    try {
        // traverse through the filesystem and attempt to read all files and directories
        for (const auto& it : std::filesystem::directory_iterator(current_dir_)) {
            dir_entry current;

            if (it.is_directory()) {
                current.is_directory = true;
                current.name = it.path().filename().string();
                entries_.push_back(current);
                continue;
            }
            // check extension to match .md
            else {
                std::string extension = it.path().extension().string();
                std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c){
                    return std::tolower(c); 
                });

                if (extension == ".md") {
                    current.is_directory = false;
                    current.name = it.path().filename().string();
                    entries_.push_back(current);
                }
            }
        }
    } catch (const std::filesystem::filesystem_error &e) {
        // just pass if exception is raised
    }

    // sort all elements in the array
    std::sort(entries_.begin(), entries_.end(), [](const dir_entry &a, const dir_entry &b){
        // sort first by moving all directories at the beginning
        if (a.is_directory != b.is_directory) {
            return a.is_directory;
        }
        // then sort alphabetically
        return a.name < b.name;
    });

    // set selected index to -1, meaning nothing selected
    selected_index_ = -1;
}

void FileDialog::open(const std::string &start_path) {
    // set current working directory to the specified path
    // if no path is specified or the path is a file, use the current path we work in
    if (!start_path.empty()) {
        if (std::filesystem::is_directory(start_path)) {
            current_dir_ = start_path;
        }
    } else {
        current_dir_ = std::filesystem::current_path();
    }

    // load directory content
    refresh_entries();

    should_open_ = true;
}

bool FileDialog::render(std::string &out_path) {
    if (should_open_) {
        ImGui::OpenPopup("Open File");
        should_open_ = false;
    }

    ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);
    if (ImGui::BeginPopupModal("Open File", NULL, ImGuiWindowFlags_NoCollapse)) {
        // set font scale
        ImGui::SetWindowFontScale(UI_FONT_SCALE);
        
        ImGui::Text("%s", current_dir_.string().c_str());

        ImGui::BeginChild("FileList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);

        if (ImGui::Selectable("../", false, ImGuiSelectableFlags_AllowDoubleClick)) {
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                current_dir_ = std::filesystem::path(current_dir_).parent_path().string();
                refresh_entries();
            }
        }

        for (int i = 0; i < (int)entries_.size(); i++) {
            // visually distinguish directories from files
            std::string label = entries_[i].is_directory ? "[Dir] " + entries_[i].name : "" + entries_[i].name;

            bool is_selected = (selected_index_ == i);

            if (ImGui::Selectable(label.c_str(), is_selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                selected_index_ = i;
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    if (entries_[i].is_directory) {
                        current_dir_ = current_dir_ / entries_[i].name;
                        refresh_entries();
                    } else {
                        out_path = (current_dir_ / entries_[i].name).string();
                        ImGui::CloseCurrentPopup();
                        ImGui::EndChild();
                        ImGui::EndPopup();
                        return true;
                    }
                }
            }
        }

        ImGui::EndChild();

        bool selected = selected_index_ >=0 && selected_index_ < (int)entries_.size();

        if (!selected) {
            ImGui::BeginDisabled();
        }
        if (ImGui::Button("Open")) {
            if (entries_[selected_index_].is_directory) {
                current_dir_ = current_dir_ / entries_[selected_index_].name;
                refresh_entries();
            } else {
                out_path = (current_dir_ / entries_[selected_index_].name).string();
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return true;
            }
        }

        if (!selected) {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    return false;
}

void MessageDialog::show(const std::string &title, const std::string &message, severity sev) {
    title_ = title;
    message_ = message;
    sev_ = sev;
    should_open_ = true;
}

void MessageDialog::render() {
    std::string label = title_ + "###MessageDialog";

    if (should_open_) {
        ImGui::OpenPopup(label.c_str());
        should_open_ = false;
    }

    if(ImGui::BeginPopupModal(label.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        // set font scale
        ImGui::SetWindowFontScale(UI_FONT_SCALE);
        
        // icon size and position
        float r = ImGui::GetTextLineHeight();
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImDrawList *dl = ImGui::GetWindowDrawList();

        // reserve icon space
        ImGui::Dummy(ImVec2(2.5f * r, 2.5f * r));

        switch(sev_) {
            case DIALOG_ERROR: {
                // adds circle to the center
                dl->AddCircleFilled(ImVec2(p.x + r, p.y +r), r, IM_COL32(200, 40, 40, 255));
                // add lines
                float k = r * 0.45f;
                dl->AddLine(ImVec2(p.x + r - k, p.y + r - k), ImVec2(p.x + r + k, p.y + r + k), 
                    IM_COL32(255, 255, 255, 255), 2.0f);
                dl->AddLine(ImVec2(p.x + r - k, p.y + r + k), ImVec2(p.x + r + k, p.y + r - k), 
                    IM_COL32(255, 255, 255, 255), 2.0f);
                break;
            }
            case DIALOG_WARNING:
                dl->AddTriangleFilled(ImVec2(p.x + r , p.y), ImVec2(p.x, p.y + 2*r), 
                    ImVec2(p.x + 2*r, p.y + 2*r), IM_COL32(240, 180, 0, 255));
                dl->AddLine(ImVec2(p.x + r, p.y + 0.8f*r), ImVec2(p.x + r, p.y + 1.4f*r), 
                        IM_COL32(30, 30, 30, 255), 2.0f);
                dl->AddCircleFilled(ImVec2(p.x + r, p.y + 1.7f*r), r/10, IM_COL32(30, 30, 30, 255));
                break;
            case DIALOG_INFORMATION:
                dl->AddCircleFilled(ImVec2(p.x + r, p.y + r), r, IM_COL32(0, 0, 255, 255));
                dl->AddCircleFilled(ImVec2(p.x + r, p.y + 0.55f*r), r/10, IM_COL32(255, 255, 255, 255));
                dl->AddLine(ImVec2(p.x + r, p.y + 0.85f*r), ImVec2(p.x + r, p.y + 1.5f*r), 
                    IM_COL32(255, 255, 255, 255), 2.0f);
                break;
            default:
                break;
        }
        
        // continue on the same line
        ImGui::SameLine();

        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 25.0f);
        ImGui::TextWrapped("%s", message_.c_str());
        ImGui::PopTextWrapPos();

        ImGui::Separator();

        // set button to the center of the dialog screen
        ImGuiStyle& style = ImGui::GetStyle();
        float button_width = ImGui::CalcTextSize("Ok").x + style.FramePadding.x * 2.0f;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - button_width) * 0.5f);

        // button
        // check if we pressed the button or Enter or Esc keys on the keyboard
        bool ok_clicked = ImGui::Button("Ok");
        bool enter_pressed = ImGui::IsKeyPressed(ImGuiKey_Enter) || 
            ImGui::IsKeyPressed(ImGuiKey_KeypadEnter);
        bool esc_pressed = ImGui::IsKeyPressed(ImGuiKey_Escape);

        if(ok_clicked || enter_pressed || esc_pressed) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::SetItemDefaultFocus();

        ImGui::EndPopup();
    }
}
