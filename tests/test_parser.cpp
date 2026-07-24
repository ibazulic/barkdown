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

#include <gtest/gtest.h>
#include "markdown.hpp"
#include <fstream>
#include <sstream>

TEST(Parser, Simple_Heading) {
    std::string input = "# Hello";
    auto result = parse_document(input.c_str(), input.length());
    ASSERT_FALSE(result.empty());
    EXPECT_EQ(result[0].type, HEADING_1);
    EXPECT_EQ(result[0].textblock, "Hello");
}

TEST(Parser, Parse_All_Headings) {
    std::string input = R"(# Heading 1
## Heading 2
### Heading 3
#### Heading 4
##### Heading 5
###### Heading 6)";

    node_type expected_types[] = {HEADING_1, HEADING_2, HEADING_3, HEADING_4, 
        HEADING_5, HEADING_6};

    auto result = parse_document(input.c_str(), input.length());
    ASSERT_FALSE(result.empty());

    for (long unsigned int i = 0; i < result.size(); i++) {
        std::string expected = "Heading " + std::to_string(i + 1);
        ASSERT_EQ(result[i].textblock, expected);
        ASSERT_EQ(result[i].type, expected_types[i]);
    }
}

TEST(Parser, Parse_Code_Block_Single_Line) {
    std::string input = "~~~\nSome text here\n~~~";
    auto result = parse_document(input.c_str(), input.length());
    ASSERT_FALSE(result.empty());
    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].textblock, "Some text here");
    ASSERT_EQ(result[0].type, CODE_BLOCK);
    ASSERT_EQ(result[0].starts_new_line, true);
}

TEST(Parser, Parse_Code_Block_Multi_Line) {
    std::string input = R"(~~~
This is a multi line
code block
~~~)";
    auto result = parse_document(input.c_str(), input.length());
    ASSERT_FALSE(result.empty());
    for (long unsigned int i = 0; i < result.size(); i++) {
        std::cout << i << ": type=" << result[i].type << " text='" << result[i].textblock << "'" << std::endl;
    }
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(result[0].textblock, "This is a multi line");
    ASSERT_EQ(result[1].textblock, "code block");
    ASSERT_EQ(result[0].starts_new_line, true);
    ASSERT_EQ(result[1].starts_new_line, true);
    ASSERT_EQ(result[0].depth, 4);
}

TEST(Parser, Parse_Bold_Text) {
    std::string input = "This is **bold** text.";
    auto result = parse_document(input.c_str(), input.length());
    ASSERT_FALSE(result.empty());
    for (long unsigned int i = 0; i < result.size(); i++) {
        std::cout << i << ": type=" << result[i].type << " text='" << result[i].textblock << "'" << std::endl;
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0].textblock, "This is ");
    ASSERT_EQ(result[1].is_bold, true);
    ASSERT_EQ(result[1].textblock, "bold");
    ASSERT_EQ(result[2].is_bold, false);
    ASSERT_EQ(result[2].textblock, " text.");
}

TEST(Parser, Parse_Emphasized_Text) {
    std::string input = "This is *emphasized* text.";
    auto result = parse_document(input.c_str(), input.length());
    ASSERT_FALSE(result.empty());
    for (long unsigned int i = 0; i < result.size(); i++) {
        std::cout << i << ": type=" << result[i].type << " text='" << result[i].textblock << "'" << std::endl;
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0].textblock, "This is ");
    ASSERT_EQ(result[1].is_italic, true);
    ASSERT_EQ(result[1].textblock, "emphasized");
    ASSERT_EQ(result[2].is_italic, false);
    ASSERT_EQ(result[2].textblock, " text.");
}

TEST(Parser, Parse_Bold_And_Italic_Text) {
    std::string input = "This is ***strong emphasized*** text.";
    auto result = parse_document(input.c_str(), input.length());
    ASSERT_FALSE(result.empty());
    for (long unsigned int i = 0; i < result.size(); i++) {
        std::cout << i << ": type=" << result[i].type << " text='" << result[i].textblock << "'" << std::endl;
    }
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0].textblock, "This is ");
    ASSERT_EQ(result[1].is_italic, true);
    ASSERT_EQ(result[1].is_bold, true);
    ASSERT_EQ(result[1].textblock, "strong emphasized");
    ASSERT_EQ(result[2].is_italic, false);
    ASSERT_EQ(result[2].is_bold, false);
    ASSERT_EQ(result[2].textblock, " text.");
}

TEST(Parser, Parse_URL) {
    std::string input = "This is a [link](https://some.url)";
    auto result = parse_document(input.c_str(), input.length());
    ASSERT_FALSE(result.empty());
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(result[0].textblock, "This is a ");
    ASSERT_EQ(result[1].textblock, "link");
    ASSERT_EQ(result[1].url, "https://some.url");
    ASSERT_EQ(result[1].type, LINK);
}

TEST(Parser, Parse_Unordered_List) {
    std::string input = R"(Unordered list:
- item 1
- item 2
- item 3)";

    node_type expected_types[] = {TEXT, LIST_ITEM_UNORDERED, TEXT, LIST_ITEM_UNORDERED, 
        TEXT, LIST_ITEM_UNORDERED, TEXT};
    std::string expected_strings[] = {" ", " ", "item 1", " ", "item 2", " ", "item 3"};

    auto result = parse_document(input.c_str(), input.length());
    ASSERT_FALSE(result.empty());
    ASSERT_EQ(result.size(), 7);

    ASSERT_EQ(result[0].textblock, "Unordered list:");
    ASSERT_EQ(result[0].type, TEXT);

    for (long unsigned int i = 1; i < result.size(); i++) {
        ASSERT_EQ(result[i].type, expected_types[i]);
        if (result[i].type == TEXT) {
            std::cout << i << ": type" << result[i].type << " text='" << result[i].textblock << "'" << std::endl;
            ASSERT_EQ(result[i].textblock, expected_strings[i]);
        }
    }
}

TEST(Parser, Parse_Nested_Unordered_Lists) {
    std::string input = R"(- item 1
- item 2
- item 3
    - item a
    - item b
- item 4)";

    auto result = parse_document(input.c_str(), input.length());
    ASSERT_FALSE(result.empty());
    ASSERT_EQ(result.size(), 12);

    for (long unsigned int i = 0; i < result.size(); i++)   {
        if(result[i].textblock.empty()) continue;
        if (result[i].textblock == "item 1" || result[i].textblock == "item 2" || 
            result[i].textblock == "item 3" || result[i].textblock == "item 4") {
                ASSERT_EQ(result[i].depth, 4);
            }
        if (result[i].textblock == "item a" || result[i].textblock == "item b") {
                ASSERT_EQ(result[i].depth, 8);
        }
    }
}

TEST(Parser, Parse_Ordered_List) {
    std::string input = R"(1. item 1
2. item 2
3. item 3)";

    node_type expected_types[] = {LIST_ITEM_ORDERED, TEXT, LIST_ITEM_ORDERED, 
            TEXT, LIST_ITEM_ORDERED, TEXT};

    int item_counter = 0;
    int string_counter = 0;

    auto result = parse_document(input.c_str(), input.length());
    ASSERT_FALSE(result.empty());
    ASSERT_EQ(result.size(), 6);
    for (long unsigned int i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].type, expected_types[i]);
        if (result[i].type == LIST_ITEM_ORDERED) {
            item_counter++;
            ASSERT_EQ(result[i].ordered_item_number, item_counter);
        }
        if (!result[i].textblock.empty()) {
            string_counter++;
            std::string expected = "item " + std::to_string(string_counter);
            ASSERT_EQ(result[i].textblock, expected);
        }
    }
}

TEST(Parser, Parse_Nested_Ordered_List) {
    std::string input = R"(List of items:
1. item 1
2. item 2
    1. sub item a
    2. sub item b
3. item 3)";

    node_type expected_types[] = {TEXT, LIST_ITEM_ORDERED, TEXT, LIST_ITEM_ORDERED, TEXT, LIST_ITEM_ORDERED, TEXT, 
        LIST_ITEM_ORDERED, TEXT, LIST_ITEM_ORDERED, TEXT};

    int main_list_counter = 0;
    int sub_list_counter = 0;

    auto result = parse_document(input.c_str(), input.length());
    ASSERT_FALSE(result.empty());
    ASSERT_EQ(result.size(), 11);

    for (long unsigned int i = 0; i < result.size(); i++) {
        ASSERT_EQ(result[i].type, expected_types[i]);
        if ((result[i].textblock == "item 1") || (result[i].textblock == "item 2") || (result[i].textblock == "item 3")) {
            ASSERT_EQ(result[i].depth, 4);
        }
        if ((result[i].type == LIST_ITEM_ORDERED) && (result[i].depth == 4)) {
            main_list_counter++;
            ASSERT_EQ(result[i].ordered_item_number, main_list_counter);
        }

        if ((result[i].textblock == "sub item a") || (result[i].textblock == "sub item b")) {
            ASSERT_EQ(result[i].depth, 8);
        }

        if ((result[i].type == LIST_ITEM_ORDERED) && (result[i].depth == 8)) {
            sub_list_counter++;
            ASSERT_EQ(result[i].ordered_item_number, sub_list_counter);
        }  
    }
}

TEST(Parser, Parse_Code_Block_Inline) {
    std::string input = R"(This is an `inline` code block.)";
    auto result = parse_document(input.c_str(), input.length());

    ASSERT_FALSE(result.empty());
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0].textblock, "This is an ");
    ASSERT_EQ(result[1].textblock, "inline");
    ASSERT_EQ(result[1].is_inline, true);
    ASSERT_EQ(result[1].type, INLINE_CODE);
    ASSERT_EQ(result[2].type, TEXT);
    ASSERT_EQ(result[2].textblock, " code block.");
}

TEST(Parser, Parse_Image_Block) {
    std::string input = "This is an ![image](https://some.image.url)";
    auto result = parse_document(input.c_str(), input.length());

    ASSERT_FALSE(result.empty());
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(result[0].textblock, "This is an ");
    ASSERT_EQ(result[0].type, TEXT);
    ASSERT_EQ(result[0].is_inline, true);
    ASSERT_EQ(result[1].url, "https://some.image.url");
    ASSERT_EQ(result[1].type, IMAGE);
}

TEST(Parser, Parse_Thematic_Break) {
    std::string input = R"(First string

***

2nd string.

---

3rd string

___

4th string)";
    auto result = parse_document(input.c_str(), input.length());
    
    ASSERT_FALSE(result.empty());
    ASSERT_EQ(result.size(), 7);

    for (long unsigned int i = 0; i < result.size(); i++) {
        if(result[i].textblock.empty()) {
            ASSERT_EQ(result[i].type, HORIZONTAL_RULE);
        }
        if (!result[i].textblock.empty()) {
            ASSERT_EQ(result[i].type, TEXT);
        }
    }
}

TEST(Parser, Parse_HTML_Code_Inline) {
    std::string input = "This is <h1>HTML PARSING!</h1>.";
    auto result = parse_document(input.c_str(), input.length());

    std::string expected_strings[] = {"This is ", "<h1>", "HTML PARSING!", "</h1>", "."};

    ASSERT_FALSE(result.empty());
    ASSERT_EQ(result.size(), 5);

    for (unsigned long int i = 0; i < result.size(); i++) {
        if (!result[i].textblock.empty()) {
            ASSERT_EQ(result[i].textblock, expected_strings[i]);
        }
        if (result[i].textblock == "<h1>" || result[i].textblock == "</h1>") {
            ASSERT_EQ(result[i].type, INLINE_HTML);
            ASSERT_EQ(result[i].is_inline, true);
        }
    } 
}

TEST(Parser, Parse_HTML_Code_Block) {
    std::string input = R"(HTML block:

<div>
Some text here
</div>

Some more text)";

    std::string expected_strings[] = {"HTML block:", "<div>", "Some text here", "</div>", "Some more text"};

    auto result = parse_document(input.c_str(), input.length());

    ASSERT_FALSE(result.empty());

    for (unsigned long int i = 0; i < result.size(); i++) {
        if (!result[i].textblock.empty()) {
            std::cout << "Iteration: " << i + 1 << ", text: " << result[i].textblock << std::endl;
        }
    }

    ASSERT_EQ(result.size(), 5);

    for (unsigned long int i = 0; i < result.size(); i++) {
        if (!result[i].textblock.empty()) {
            ASSERT_EQ(result[i].textblock, expected_strings[i]);
        }
        if (result[i].textblock == "<div>" || result[i].textblock == "</div>") {
            ASSERT_EQ(result[i].type, HTML_BLOCK);
        }
        if (result[i].textblock == "<div>" || result[i].textblock == "</div>" || result[i].textblock == "Some text here") {
            ASSERT_EQ(result[i].depth, 4);
        } else {
            ASSERT_EQ(result[i].depth, 0);
        }
    }
}

TEST(Parser, Parse_Soft_Breaks) {
    std::string input = R"(First line
Second line)";

    auto result = parse_document(input.c_str(), input.length());
    ASSERT_FALSE(result.empty());
    ASSERT_EQ(result.size(), 3);

    ASSERT_EQ(result[0].textblock, "First line");
    ASSERT_EQ(result[1].type, SOFTBREAK);
    ASSERT_EQ(result[2].textblock, "Second line");
}

TEST(Parser, Parse_Line_Break) {
    std::string input = "First line  \nSecond line";
    auto result = parse_document(input.c_str(), input.length());
    ASSERT_FALSE(result.empty());
    ASSERT_EQ(result.size(), 3);

    ASSERT_EQ(result[0].textblock, "First line");
    ASSERT_EQ(result[1].type, LINEBREAK);
    ASSERT_EQ(result[2].textblock, "Second line");
}

TEST(Parser, Integration_Parse_Complete_File) {
    std::string path = std::string(TEST_DATA_DIR) + "/integration_test.md";
    std::ifstream file(path);
    ASSERT_EQ(file.is_open(), true);

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string input = ss.str();
    ASSERT_FALSE(input.empty());

    auto result = parse_document(input.c_str(), input.length());
    ASSERT_FALSE(result.empty());

    int heading_counter = 0;
    int ordered_type_counter = 0;
    int unordered_type_counter = 0;

    int soft_break_counter = 0;
    int hard_break_counter = 0;

    for (unsigned long int i = 0; i < result.size(); i++) {
        if (result[i].textblock == "nested item a") {
            ASSERT_EQ(result[i].depth, 8);
        }
        if ((result[i].type == HEADING_2) || (result[i].type == HEADING_3)) {
            heading_counter++;
        }

        if (result[i].textblock == "bold and italic") {
            ASSERT_EQ(result[i].is_bold, true);
            ASSERT_EQ(result[i].is_italic, true);
            ASSERT_EQ(result[i].is_inline, true);
        }

        if ((result[i].type == LIST_ITEM_ORDERED)) {
            ordered_type_counter++;
        }

        if ((result[i].type == LIST_ITEM_UNORDERED)) {
            unordered_type_counter++;
        }

        if (result[i].type == SOFTBREAK) {
            soft_break_counter++;
        }
        if (result[i].type == LINEBREAK) {
            hard_break_counter++;
        }
    }

    ASSERT_EQ(ordered_type_counter, 5);
    ASSERT_EQ(unordered_type_counter, 5);
    ASSERT_EQ(soft_break_counter, 1);
    ASSERT_EQ(hard_break_counter, 1);
}
