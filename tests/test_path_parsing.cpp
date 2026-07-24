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
#include "image_loader.hpp"

TEST(URLResolverTest, Handle_Remote_URLs)   {
    std::filesystem::path fake_base = "/home/user";
    EXPECT_EQ(get_full_url(fake_base, "https://wikipedia.org/image.png"), 
        "https://wikipedia.org/image.png");

    EXPECT_EQ(get_full_url(fake_base, "http://fake.url/images/tech.jpg"), 
        "http://fake.url/images/tech.jpg");
}

TEST(URLResolverTest, Handle_Explicit_File_URLs)    {
    std::filesystem::path fake_base = "/var/www";
    EXPECT_EQ(get_full_url(fake_base, "file:///var/www/images/foo.jpg"), 
        "file:///var/www/images/foo.jpg");
}

TEST(URLResolverTest, Handle_Absolute_Local_Paths)  {
    std::filesystem::path fake_base = "/var/www";
    EXPECT_EQ(get_full_url(fake_base, "images/foo.jpg"), 
        "file:///var/www/images/foo.jpg");
}

TEST(URLResolverTest, Handle_Relative_Local_Paths)  {
    std::filesystem::path fake_base = "/var/www";

    // test subdirectory
    EXPECT_EQ(get_full_url(fake_base, "images/foo.jpg"), 
        "file:///var/www/images/foo.jpg");

    // test same directory
    EXPECT_EQ(get_full_url(fake_base, "bar.gif"), 
        "file:///var/www/bar.gif");
}

TEST(URLResolverTest, Trailing_Slash_On_Base_Path)  {
    std::filesystem::path fake_base = "/var/www/";
    EXPECT_EQ(get_full_url(fake_base, "images/foo.jpg"), 
        "file:///var/www/images/foo.jpg");
}

TEST(URLResolverTest, Empty_URL)    {
    std::filesystem::path fake_base = "/var/www";
    EXPECT_EQ(get_full_url(fake_base, ""), "file:///var/www/");
}
