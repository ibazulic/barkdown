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
#include <algorithm>
#include <cctype>
#include <cmark.h>
#include <iostream>
#include <sstream>
#include <vector>

// is_block_node returns true if the node we're in is a block node.
bool is_block_node(cmark_node_type type) {
    switch (type) {
    // case CMARK_NODE_DOCUMENT:
    case CMARK_NODE_BLOCK_QUOTE:
    case CMARK_NODE_LIST:
    case CMARK_NODE_ITEM:
    case CMARK_NODE_CUSTOM_BLOCK:
    case CMARK_NODE_PARAGRAPH:
    case CMARK_NODE_HEADING:
        return true;
    default:
        return false;
    }
}

// get_heading_type is a helper function that returns the heading type to the
// caller.
node_type get_heading_type(int heading_level) {
    switch (heading_level) {
    case 1:
        return HEADING_1;
    case 2:
        return HEADING_2;
    case 3:
        return HEADING_3;
    case 4:
        return HEADING_4;
    case 5:
        return HEADING_5;
    case 6:
        return HEADING_6;
    default:
        return HEADING_1;
    }
}

// parse_and_store parses the node string and splits it on the new line
// character on multiple strings and then stores each individual split string
// with its attributes into the markdown_node struct.
void parse_and_store(std::string init_string, struct markdown_node current_data,
                     std::vector<markdown_node> &parsed_document) {
    size_t pos = init_string.find('\n');
    // remove the trailing new line at the end, if it's there
    if (!init_string.empty() && init_string.back() == '\n') {
        init_string.pop_back();
    }

    if (init_string.empty()) {
        return;
    }

    if (pos != std::string::npos && pos < init_string.length() - 1) {
        std::istringstream input_stream(init_string);
        std::string cur_line;
        while (std::getline(input_stream, cur_line, '\n')) {
            current_data.textblock = cur_line;
            parsed_document.push_back(current_data);
            if (current_data.type == CODE_BLOCK ||
                current_data.type == HTML_BLOCK) {
                current_data.starts_new_line = true;
            } else {
                current_data.starts_new_line = false;
            }
        }
    } else {
        current_data.textblock = init_string;
        parsed_document.push_back(current_data);
    }
}

// Parses the MarkDown document and returns a vector of structures representing
// nodes of the document.
std::vector<markdown_node> parse_document(const char *readBuffer,
                                          size_t length) {
    std::vector<markdown_node> parsed_document;
    cmark_node *root_node =
        cmark_parse_document(readBuffer, length, CMARK_OPT_DEFAULT);

    bool is_currently_bold = false;
    bool is_currently_italic = false;

    int header_level = 0;
    bool is_currently_header = false;

    std::string current_link_url = "";

    bool is_inside_image = false;

    if (root_node) {
        cmark_iter *iter = cmark_iter_new(root_node);
        cmark_event_type event;
        std::vector<int> depth = {0};
        std::vector<int> item_lists;

        node_type current_block_type = TEXT;
        bool start_new_line = false;
        bool suppress_next_new_line = false;

        while ((event = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
            // force reinitialization of the data before we start writing
            // anything to the node.
            struct markdown_node current_data{};
            cmark_node *cur_node = cmark_iter_get_node(iter);
            cmark_node_type type = cmark_node_get_type(cur_node);

            switch (event) {
            case CMARK_EVENT_ENTER: {
                if (is_block_node(type)) {
                    // check if we need to suppress a new line, particularly
                    // when entering a list block
                    if (suppress_next_new_line) {
                        suppress_next_new_line = false;
                        start_new_line = false;
                    } else {
                        start_new_line = true;
                    }

                    if (type == CMARK_NODE_LIST ||
                        type == CMARK_NODE_BLOCK_QUOTE) {
                        if (type == CMARK_NODE_LIST) {
                            item_lists.push_back(1);
                        }
                        depth.push_back(depth.back() + 4);
                    } else {
                        depth.push_back(depth.back());
                    }
                }

                switch (type) {
                case CMARK_NODE_HEADING: {
                    header_level = cmark_node_get_heading_level(cur_node);
                    is_currently_header = true;
                    current_block_type = get_heading_type(header_level);
                    break;
                }
                case CMARK_NODE_EMPH:
                    is_currently_italic = true;
                    break;
                case CMARK_NODE_STRONG:
                    is_currently_bold = true;
                    break;

                case CMARK_NODE_THEMATIC_BREAK:
                    current_data.type = HORIZONTAL_RULE;
                    current_data.starts_new_line = true;
                    suppress_next_new_line = false;
                    current_data.is_bold = false;
                    current_data.is_inline = false;
                    current_data.is_italic = false;
                    parsed_document.push_back(current_data);
                    break;

                case CMARK_NODE_ITEM: {
                    cmark_list_type list_type =
                        cmark_node_get_list_type(cmark_node_parent(cur_node));
                    if (list_type == CMARK_ORDERED_LIST) {
                        int cur_list_pos = item_lists.back();
                        item_lists.pop_back();
                        current_data.ordered_item_number = cur_list_pos;
                        current_data.type = LIST_ITEM_ORDERED;
                        item_lists.push_back(cur_list_pos + 1);
                    } else {
                        current_data.type = LIST_ITEM_UNORDERED;
                    }
                    
                    current_data.is_inline = true;
                    current_data.depth = depth.back();
                    current_data.starts_new_line = start_new_line;
                    suppress_next_new_line = true;

                    parsed_document.push_back(current_data);
                    break;
                }

                case CMARK_NODE_PARAGRAPH:
                    current_block_type = TEXT;
                    break;
                case CMARK_NODE_TEXT:
                case CMARK_NODE_CODE:
                case CMARK_NODE_HTML_INLINE: {
                    if (is_inside_image) {
                        continue;
                    }
                    
                    const char *cur_node_value =
                        cmark_node_get_literal(cur_node);

                    // check that we don't manipulate an empty string
                    if (!cur_node_value)
                        break;

                    std::string input_string = cur_node_value;

                    // check exactly which node type ths is
                    if (type == CMARK_NODE_CODE) {
                        current_data.type = INLINE_CODE;
                    } else if (type == CMARK_NODE_HTML_INLINE) {
                        current_data.type = INLINE_HTML;
                    } else {
                        current_data.type = current_block_type;
                    }

                    if (!current_link_url.empty()) {
                        current_data.type = LINK;
                        current_data.url = current_link_url;
                    }

                    // this is always true for inline elements
                    current_data.is_inline = true;

                    // checks if we need to apply specific formatting
                    current_data.is_bold = is_currently_bold;
                    current_data.is_italic = is_currently_italic;

                    current_data.depth = depth.back();
                    current_data.starts_new_line = start_new_line;
                    start_new_line = false;

                    parse_and_store(cur_node_value, current_data,
                                    parsed_document);
                    break;
                }

                case CMARK_NODE_HTML_BLOCK:
                case CMARK_NODE_CODE_BLOCK: {
                    const char *cur_node_value =
                        cmark_node_get_literal(cur_node);

                    if (type == CMARK_NODE_HTML_BLOCK) {
                        current_data.type = HTML_BLOCK;
                    } else
                        current_data.type = CODE_BLOCK;

                    // check that the string is not empty
                    if (!cur_node_value)
                        break;

                    // handle indentation
                    int indentation = depth.back();
                    if (indentation == 0)
                        indentation += 4;

                    current_data.is_inline = false;
                    current_data.depth = indentation;
                    current_data.starts_new_line = true;

                    parse_and_store(cur_node_value, current_data,
                                    parsed_document);
                    break;
                }

                case CMARK_NODE_LINEBREAK:
                    current_data.type = LINEBREAK;
                    current_data.textblock = "";
                    current_data.is_inline = true;

                    current_data.depth = depth.back();
                    current_data.is_bold = is_currently_bold;
                    current_data.is_italic = is_currently_italic;
                    current_data.starts_new_line = start_new_line;
                    start_new_line = false;

                    parsed_document.push_back(current_data);
                    break;
                case CMARK_NODE_SOFTBREAK:
                    current_data.type = SOFTBREAK;
                    current_data.textblock = " ";
                    current_data.is_inline = true;

                    current_data.depth = depth.back();
                    current_data.is_bold = is_currently_bold;
                    current_data.is_italic = is_currently_italic;
                    current_data.starts_new_line = start_new_line;
                    start_new_line = false;

                    parsed_document.push_back(current_data);
                    break;
                case CMARK_NODE_LINK:
                    current_link_url = cmark_node_get_url(cur_node);
                    break;
                case CMARK_NODE_IMAGE:
                    current_data.type = IMAGE;
                    current_data.url = cmark_node_get_url(cur_node);
                    current_data.is_inline = true;
                    current_data.depth = depth.back();

                    current_data.starts_new_line = start_new_line;
                    start_new_line = false;
                    is_inside_image = true;

                    parsed_document.push_back(current_data);
                    break;
                default:
                    break;
                }
                break;
            }
            case CMARK_EVENT_EXIT: {
                if (is_block_node(type)) {
                    if (!depth.empty()) {
                        depth.pop_back();
                    }
                }
                switch (type) {
                case CMARK_NODE_LIST:
                    if (!item_lists.empty()) {
                        item_lists.pop_back();
                    }
                    break;
                case CMARK_NODE_HEADING:
                    is_currently_header = false;
                    break;
                case CMARK_NODE_EMPH:
                    is_currently_italic = false;
                    break;
                case CMARK_NODE_STRONG:
                    is_currently_bold = false;
                    break;
                case CMARK_NODE_LINK:
                    current_link_url = "";
                    break;
                case CMARK_NODE_IMAGE:
                    is_inside_image = false;
                    break;
                default:
                    break;
                }
            }
            default:
                break;
            }
        }
        cmark_iter_free(iter);
        cmark_node_free(root_node);
    }

     for (unsigned long int i = 0; i < parsed_document.size(); i++) {
            printf("%lu: type=%d depth=%d new_line=%d bold=%d text='%s' url='%s'\n", 
                i, parsed_document[i].type, parsed_document[i].depth, parsed_document[i].starts_new_line, 
                parsed_document[i].is_bold, parsed_document[i].textblock.c_str(), parsed_document[i].url.c_str());
        }

    return parsed_document;
}
