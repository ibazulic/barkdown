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

#include "render.hpp"
#include "markdown.hpp"
#include <iostream>

// resolve_font returns an enum corresponding to the specific font we need to
// load based on the kind of node we are currently in.
enum FontStyle resolve_font(const markdown_node &current_node) {
    if (current_node.type == CODE_BLOCK || current_node.type == INLINE_CODE ||
        current_node.type == HTML_BLOCK) {
        return Code;
    }
    if (current_node.is_bold && current_node.is_italic) {
        return BoldItalic;
    }
    if (current_node.is_bold) {
        return Bold;
    }
    if (current_node.is_italic) {
        return Italic;
    }
    return Regular;
}

// get_heading_scale returns the size of the heading for each of the heading
// type nodes.
float get_heading_scale(const markdown_node &current_node) {
    switch (current_node.type) {
    case HEADING_1:
        return 2.0f;
    case HEADING_2:
        return 1.7f;
    case HEADING_3:
        return 1.4f;
    case HEADING_4:
        return 1.2f;
    case HEADING_5:
        return 1.0f;
    case HEADING_6:
        return 0.85f;
    default:
        return 1.0f;
    }
}

// render_text_segment renders the text segment using the ImDrawList.
void render_text_segment(ImDrawList *draw_list, ImFont *font, float font_size,
                         const char *text, const char *text_end,
                         float &cursor_x, float &cursor_y, float line_start_x,
                         float wrap_width, float line_height, ImU32 color) {

    float remaining;
    const char *breakpoint = nullptr;

    while (text < text_end) {
        // calculate remaining width
        remaining = wrap_width - cursor_x;
        breakpoint =
            font->CalcWordWrapPosition(font_size, text, text_end, remaining);

        // if breakpoint is all text, but the cursor is over line start
        if ((breakpoint == text) && (cursor_x > line_start_x)) {
            cursor_y += line_height;
            cursor_x = line_start_x;
            continue;
        }

        // if our breakpoint is the entire text and we are on the start of the
        // line (Example: we have a word that is longer than the entire line)
        if ((breakpoint == text) && (cursor_x == line_start_x)) {
            const char *new_breakpoint = nullptr;
            font->CalcTextSizeA(font_size, wrap_width - line_start_x, 0.0f,
                                text, text_end, &new_breakpoint);
            // if our new calculated breakpoint is still over text, add one
            // character
            if (new_breakpoint == text) {
                unsigned char lead =
                    static_cast<unsigned char>(*new_breakpoint);
                if (lead < 0x80)
                    new_breakpoint += 1;
                else if (lead < 0xE0)
                    new_breakpoint += 2;
                else if (lead < 0xF0)
                    new_breakpoint += 3;
                else
                    new_breakpoint += 4;
                if (new_breakpoint > text_end) {
                    new_breakpoint = text_end;
                }
            }
            breakpoint = new_breakpoint;
        }

        draw_list->AddText(font, font_size, ImVec2(cursor_x, cursor_y), color,
                           text, breakpoint);
        ImVec2 drawn =
            font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text, breakpoint);
        cursor_x += drawn.x;
        while (breakpoint < text_end && *breakpoint == ' ') {
            breakpoint++;
        }
        text = breakpoint;

        if (breakpoint < text_end) {
            cursor_y += line_height;
            cursor_x = line_start_x;
        }
    }
}

// render_document renders the document and all its segments.
void render_document(const std::vector<markdown_node> &parsed,
                     ImFont *fontRegular, ImFont *fontBold, ImFont *fontItalic,
                     ImFont *fontBoldItalic, ImFont *fontMono, bool dark_theme,
                     image_loader &loader, int actual_width) {

    ImDrawList *draw_list =
        ImGui::GetWindowDrawList();                 // list of elements to draw
    ImVec2 start_pos = ImGui::GetCursorScreenPos(); // drawing starting position

    float wrap_width =
        ImGui::GetContentRegionAvail()
            .x; // check how big the available region is and wrap on boundary

    float base_font_size = ImGui::GetFontSize();     // base font size
    ImU32 color = ImGui::GetColorU32(ImGuiCol_Text); // base font color

    // starting cursor (x, y) coordinates
    float cursor_x = start_pos.x;
    float cursor_y = start_pos.y;

    // set the beginning of the line to the cursor starting position
    float line_start_x = start_pos.x;

    // base line height must be equal to the base font size, otherwise we have
    // overlap
    float line_height = base_font_size;
    float previous_line_height = base_font_size;

    // default font for a specific segment
    ImFont *font_segment = fontRegular;

    // size of one character in pixels
    float one_char_size =
        font_segment->CalcTextSizeA(base_font_size, FLT_MAX, 0.0f, " ").x;

    // size of the visible render area
    // float right_edge = start_pos.x + wrap_width;
    float right_edge = std::min(start_pos.x + wrap_width, (float)actual_width);

    // rectangle padding
    float padding = 15.0f;

    bool on_new_line = true;
    bool last_line_was_list = false;

    for (unsigned long int i = 0; i < parsed.size(); i++) {
        // calculate font scale
        float font_scale = get_heading_scale(parsed[i]) * base_font_size;
        line_height = font_scale;

        if (parsed[i].starts_new_line) {
            on_new_line = true;
        }

        // find which font we need to use for a specific segment
        switch (resolve_font(parsed[i])) {
        case Code:
            font_segment = fontMono;
            break;
        case BoldItalic:
            font_segment = fontBoldItalic;
            break;
        case Bold:
            font_segment = fontBold;
            break;
        case Italic:
            font_segment = fontItalic;
            break;
        case Regular:
        default:
            font_segment = fontRegular;
        }

        // if we need to go on the new line, advance forward and reset cursor to
        // the beginning of the line
        if (on_new_line) {
            if (i > 0) {
                bool is_list = (parsed[i].type == LIST_ITEM_ORDERED ||
                                parsed[i].type == LIST_ITEM_UNORDERED);

                if (!is_list || !last_line_was_list) {
                    cursor_y += std::max(previous_line_height, line_height);
                }
                last_line_was_list = is_list;
            }
            cursor_x = start_pos.x;
            if (parsed[i].depth) {
                cursor_x += one_char_size * parsed[i].depth;
                // line_start_x = cursor_x;
            }
            line_start_x = cursor_x;
            on_new_line = false;
        }

        // check if the element of the list is an ordered item or unordered item
        // and add the numbering accordingly
        if (parsed[i].type == LIST_ITEM_UNORDERED &&
            parsed[i].starts_new_line) {
            std::string text = "\u2022 ";
            float bullet_size =
                fontRegular
                    ->CalcTextSizeA(base_font_size, FLT_MAX, 0.0f, text.c_str())
                    .x;
            draw_list->AddText(ImVec2(cursor_x, cursor_y), color, text.c_str(),
                               nullptr);
            cursor_x += bullet_size;
        }
        if (parsed[i].type == LIST_ITEM_ORDERED && parsed[i].starts_new_line) {
            std::string text =
                std::to_string(parsed[i].ordered_item_number) + ".  ";
            float text_size =
                fontRegular
                    ->CalcTextSizeA(base_font_size, FLT_MAX, 0.0f, text.c_str())
                    .x;
            draw_list->AddText(ImVec2(cursor_x, cursor_y), color, text.c_str(),
                               nullptr);
            cursor_x += text_size;
        }

        const char *text = parsed[i].textblock.c_str();
        const char *end_character = text + parsed[i].textblock.size();

        // if we have inline code
        if (parsed[i].type == INLINE_CODE) {
            float text_size = font_segment
                                  ->CalcTextSizeA(font_scale, FLT_MAX, 0.0f,
                                                  text, end_character)
                                  .x;
            draw_list->AddRectFilled(
                ImVec2(cursor_x, cursor_y),
                ImVec2(cursor_x + text_size, cursor_y + line_height),
                IM_COL32(60, 60, 60, 255), 0.0f, 0);
        }

        // we want to draw a rectangle around a code block or a HTML block so we
        // need to detect how many code/HTML lines we have:
        bool is_code_or_html =
            (parsed[i].type == CODE_BLOCK || parsed[i].type == HTML_BLOCK);
        if (is_code_or_html &&
            (i == 0 || parsed[i - 1].type != parsed[i].type)) {

            unsigned long int j = i;

            float total_height = 0.0;
            float max_text_width = 0.0f;
            float available_wrap = right_edge - cursor_x;

            while ((j < parsed.size()) && (parsed[j].type == parsed[i].type)) {
                // calculate line size for jth element and compare the two
                const char *_text = parsed[j].textblock.c_str();
                const char *_end_character = _text + parsed[j].textblock.size();

                ImVec2 _size = font_segment->CalcTextSizeA(
                    font_scale, FLT_MAX, available_wrap, _text, _end_character);
                max_text_width = std::max(max_text_width, _size.x);
                total_height += _size.y;

                j++;
            }

            // limit right end rendering to only the longest line plus padding
            draw_list->AddRectFilled(
                ImVec2(cursor_x - padding, cursor_y - padding),
                ImVec2(cursor_x + max_text_width + padding,
                       cursor_y + total_height + padding),
                IM_COL32(60, 60, 60, 255), 20.0f, 0);
        }

        // if we have a horizontal ruler
        if (parsed[i].type == HORIZONTAL_RULE) {
            ImU32 override = IM_COL32(0, 0, 0, 255);
            if (dark_theme == true) {
                override = IM_COL32(255, 255, 255, 255);
            }
            draw_list->AddLine(
                ImVec2(cursor_x, cursor_y + (line_height / 2.0f)),
                ImVec2(right_edge, cursor_y + (line_height / 2.0f)), override,
                2.0f);
        }

        // if we encounter a URL block
        if (parsed[i].type == LINK) {
            std::string link_value = "(" + parsed[i].url + ")";

            // calc the size of the text
            const char *url_description = parsed[i].textblock.c_str();
            const char *end_character =
                url_description + parsed[i].textblock.size();
            render_text_segment(draw_list, font_segment, font_scale,
                                url_description, end_character, cursor_x,
                                cursor_y, line_start_x, right_edge, line_height,
                                color);

            // add the description
            ImU32 override = IM_COL32(51, 102, 204, 255);
            const char *url_link = link_value.c_str();
            const char *end_url_character = url_link + link_value.size();
            render_text_segment(draw_list, fontMono, font_scale, url_link,
                                end_url_character, cursor_x, cursor_y,
                                line_start_x, right_edge, line_height,
                                override);

            continue;
        }

        // if we encounter an image
        if (parsed[i].type == IMAGE) {
            loader.request_image(parsed[i].url);
            auto image = loader.get_entry(parsed[i].url);
            // if the image is downloaded, meaning it's in UPLOADED state
            if (image.has_value() && (image->state == UPLOADED)) {
                // move two lines down and position to the the 1/3 of the render
                // area from the left side
                // cursor_y += 2 * base_font_size;

                float scale = 1.0f;
                float available_width = wrap_width - (cursor_x - start_pos.x);

                cursor_x = line_start_x + padding;

                if (image->width > available_width) {
                    scale = available_width / image->width;
                    cursor_x = line_start_x + line_start_x * scale;
                }
                float draw_width = image->width * scale;
                float draw_height = image->height * scale;

                draw_list->AddImage(
                    (ImTextureID)(intptr_t)image->texture_id,
                    ImVec2(cursor_x, cursor_y),
                    ImVec2(cursor_x + draw_width, cursor_y + draw_height));

                // advance the cursor for the size of the image plus one line,
                // we'll move for an additional line at the end of the loop
                cursor_y = cursor_y + draw_height;
                on_new_line = true;
                continue;
            } else {
                // show a message at the center saying that the image is loading
                cursor_y += 2 * base_font_size;
                cursor_x = line_start_x + (1.0f / 30.0f) * wrap_width;
                std::string text = "Loading image...";
                const char *end_character = text.c_str() + text.size();

                render_text_segment(draw_list, fontMono, font_scale,
                                    text.c_str(), end_character, cursor_x,
                                    cursor_y, line_start_x, right_edge,
                                    line_height, color);

                // advance by one line
                cursor_y += base_font_size;
                on_new_line = true;
                continue;
            }
        }

        // if we have a HTML block
        if (parsed[i].type == HTML_BLOCK) {
            ImU32 override = IM_COL32(173, 216, 230, 255);
            render_text_segment(draw_list, font_segment, font_scale, text,
                                end_character, cursor_x, cursor_y, line_start_x,
                                right_edge, line_height, override);
        } else if (parsed[i].type == CODE_BLOCK ||
                   parsed[i].type == INLINE_CODE) {
            ImU32 override = IM_COL32(255, 160, 50, 255);
            render_text_segment(draw_list, font_segment, font_scale, text,
                                end_character, cursor_x, cursor_y, line_start_x,
                                right_edge, line_height, override);
        } else {
            render_text_segment(draw_list, font_segment, font_scale, text,
                                end_character, cursor_x, cursor_y, line_start_x,
                                right_edge, line_height, color);
        }

        // advamce one line if a code or HTML block have just ended
        if (is_code_or_html && ((i + 1 >= parsed.size()) ||
                                parsed[i + 1].type != parsed[i].type)) {
            cursor_y += line_height;
        }

        // advance one line if we have a horizontal ruler at the end of
        // rendering
        if (parsed[i].type == HORIZONTAL_RULE) {
            cursor_y += line_height;
        }

        // if the current node is a text but next node is a list, advance by one
        bool is_list = (parsed[i].type == LIST_ITEM_ORDERED ||
                        parsed[i].type == LIST_ITEM_UNORDERED);
        bool next_is_list = (i + 1 < parsed.size()) &&
                            (parsed[i + 1].type == LIST_ITEM_ORDERED ||
                             parsed[i + 1].type == LIST_ITEM_UNORDERED);

        if (!is_list && next_is_list) {
            cursor_y += line_height;
        }

        // if we're the **last** text block in a list meaning that previous item
        // is a list, but next item is **not** a list member, then advance by
        // one line
        bool is_list_item = (parsed[i].type == LIST_ITEM_ORDERED ||
                             parsed[i].type == LIST_ITEM_UNORDERED);
        bool prev_is_list =
            (i > 0) && (parsed[i - 1].type == LIST_ITEM_ORDERED ||
                        parsed[i - 1].type == LIST_ITEM_UNORDERED);

        if (!is_list_item && prev_is_list && !next_is_list) {
            bool next_is_same_depth =
                (i + 1 < parsed.size()) &&
                (parsed[i + 1].depth == parsed[i].depth) &&
                parsed[i + 1].is_inline;
            if (!next_is_same_depth) {
                cursor_y += line_height;
            }
        }

        // advance one line if current node is **not** a code block, but next
        // one is a code or a HTML block
        if (parsed[i].type != CODE_BLOCK && parsed[i].type != HTML_BLOCK &&
            (i + 1 < parsed.size() && (parsed[i + 1].type == CODE_BLOCK ||
                                       parsed[i + 1].type == HTML_BLOCK))) {
            cursor_y += line_height;
        }

        // restore original line height
        previous_line_height = line_height;

        // if a current node is a heading but next node isn't, then we add a new
        // line
        if (parsed[i].type == HEADING_1 || parsed[i].type == HEADING_2 ||
            parsed[i].type == HEADING_3 || parsed[i].type == HEADING_4 ||
            parsed[i].type == HEADING_5 || parsed[i].type == HEADING_6) {
            on_new_line = true;
            previous_line_height = line_height * 1.2f;
        }

        line_height = base_font_size;
    }
    // add a new empty line at the of the document when we finish rendering
    float bottom_padding = base_font_size * 2.0f;
    ImGui::Dummy(ImVec2(wrap_width,
                        cursor_y - start_pos.y + line_height + bottom_padding));
}
