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

#include "image_loader.hpp"
#include <gtest/gtest.h>

TEST(URLResolverTest, Handle_Remote_URLs) {
    std::filesystem::path fake_base = "/home/user";
    EXPECT_EQ(get_full_url(fake_base, "https://wikipedia.org/image.png"),
              "https://wikipedia.org/image.png");

    EXPECT_EQ(get_full_url(fake_base, "http://fake.url/images/tech.jpg"),
              "http://fake.url/images/tech.jpg");
}

TEST(URLResolverTest, Handle_Explicit_File_URLs) {
    std::filesystem::path fake_base = "/var/www";
    EXPECT_EQ(get_full_url(fake_base, "file:///var/www/images/foo.jpg"),
              "file:///var/www/images/foo.jpg");
}

TEST(URLResolverTest, Handle_Absolute_Local_Paths) {
    std::filesystem::path fake_base = "/some/path";
    EXPECT_EQ(get_full_url(fake_base, "/var/www/images/foo.jpg"),
              "file:///var/www/images/foo.jpg");
}

TEST(URLResolverTest, Empty_Base_Path) {
    std::filesystem::path fake_base = "";
    EXPECT_EQ(get_full_url(fake_base, "/var/www/images/foo.jpg"),
              "file:///var/www/images/foo.jpg");
}

TEST(URLResolverTest, Empty_Base_Path_With_Relative_Path) {
    std::filesystem::path fake_base = "";
    EXPECT_EQ(
        get_full_url(fake_base, "images/foo.jpg"),
        "file://" +
            (std::filesystem::current_path() / "images/foo.jpg").string());
}

TEST(URLResolverTest, All_Paths_Empty) {
    std::filesystem::path fake_base = "";
    EXPECT_NO_THROW(get_full_url(fake_base, ""));
    EXPECT_EQ(get_full_url(fake_base, ""),
              "file://" + std::filesystem::current_path().string());
}

TEST(URLResolverTest, Handle_Relative_Local_Paths) {
    std::filesystem::path fake_base = "/var/www";

    // test subdirectory
    EXPECT_EQ(get_full_url(fake_base, "images/foo.jpg"),
              "file:///var/www/images/foo.jpg");

    // test same directory
    EXPECT_EQ(get_full_url(fake_base, "bar.gif"), "file:///var/www/bar.gif");
}

TEST(URLResolverTest, Trailing_Slash_On_Base_Path) {
    std::filesystem::path fake_base = "/var/www/";
    EXPECT_EQ(get_full_url(fake_base, "images/foo.jpg"),
              "file:///var/www/images/foo.jpg");
}

TEST(URLResolverTest, Empty_URL) {
    std::filesystem::path fake_base = "/var/www";
    EXPECT_EQ(get_full_url(fake_base, ""), "file:///var/www/");
}

TEST(URLResolverTest, File_Containing_File_In_Name) {
    std::filesystem::path fake_base = "/var/www";
    EXPECT_EQ(get_full_url(fake_base, "file_icon.png"),
              "file:///var/www/file_icon.png");
}

TEST(URLResolverTest, File_Containing_http_In_Name) {
    std::filesystem::path fake_base = "/var/www";
    EXPECT_EQ(get_full_url(fake_base, "http_header.jpg"),
              "file:///var/www/http_header.jpg");
}

TEST(URLResolverTest, File_Containing_http_In_Name_But_Is_Remote) {
    std::filesystem::path fake_base = "/home/user";
    EXPECT_EQ(get_full_url(fake_base, "https://foo.bar/http_header.jpg"),
              "https://foo.bar/http_header.jpg");
}

TEST(URLResolverTest, Parent_Directory) {
    std::filesystem::path fake_base = "/var/www";
    EXPECT_EQ(get_full_url(fake_base, "../img/a.png"), "file:///var/img/a.png");
}
