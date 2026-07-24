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
#include "imgui.h"
#include "markdown.hpp"

// Font enum
enum FontStyle {
    Code,
    Regular,
    Bold,
    Italic,
    BoldItalic,
};

// resolve_font returns an enum corresponding to the specific font we need to
// load based on the kind of node we are currently in.
enum FontStyle resolve_font(const markdown_node &current_node);

// get_heading_scale returns the size of the heading for each of the heading
// type nodes.
float get_heading_scale(const markdown_node &current_node);

// render_text_segment renders the text segment using the ImDrawList.
void render_text_segment(ImDrawList *draw_list, ImFont *font, float font_size,
                         const char *text, const char *text_end,
                         float &cursor_x, float &cursor_y, float line_start_x,
                         float wrap_width, float line_height, ImU32 color);

// render_document renders the document and all its segments.
void render_document(const std::vector<markdown_node> &parsed,
                     ImFont *fontRegular, ImFont *fontBold, ImFont *fontItalic,
                     ImFont *fontBoldItalic, ImFont *fontMono, bool dark_theme,
                     image_loader &loader, int actual_width);
