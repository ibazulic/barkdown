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
#include <cmark.h>
#include <string>
#include <vector>

// types of nodes
enum node_type {
    HEADING_1,
    HEADING_2,
    HEADING_3,
    HEADING_4,
    HEADING_5,
    HEADING_6,
    PARAGRAPH,
    TEXT,
    STRONG,
    EMPHASIS,
    LINEBREAK,
    SOFTBREAK,
    CODE_BLOCK,
    INLINE_CODE,
    QUOTE_BLOCK,
    LINK,
    IMAGE,
    INLINE_HTML,
    HTML_BLOCK,
    LIST_ITEM_UNORDERED,
    LIST_ITEM_ORDERED,
    HORIZONTAL_RULE,
};

// markdown_node represents a structure of our nodes along with indentation and
// type
struct markdown_node {
        std::string textblock;
        int depth;
        enum node_type type;
        bool is_inline;
        int ordered_item_number;

        std::string url;

        // text properties
        bool is_italic;
        bool is_bold;
        bool starts_new_line;
};

// get_heading_type returns the enum type of the heading node.
enum node_type get_heading_type(int);

// is_block_node returns true if the CMARK node is a block node
bool is_block_node(cmark_node_type type);

// parse_document parses the MarkDown document and returns a vector of
// structures representing nodes of the document.
std::vector<markdown_node> parse_document(const char *, size_t);

// parse_and_store parses the node string and splits it on the new line
// character on multiple strings and then stores each individual split string
// with its attributes into the markdown_node struct.
void parse_and_store(std::string, struct markdown_node,
                     std::vector<markdown_node> &);
