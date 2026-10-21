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

#include "window.hpp"
#include "file_operations.hpp"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
#include "markdown.hpp"
#include "portable-file-dialogs.h"
#include "render.hpp"
#include "image_loader.hpp"
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <filesystem>
#include <iostream>

// include fonts used for rendering
#include "DejaVuSans-Bold.hpp"
#include "DejaVuSans-BoldOblique.hpp"
#include "DejaVuSans-Oblique.hpp"
#include "DejaVuSans.hpp"
#include "DejaVuSansMono.hpp"

namespace fs = std::filesystem;

// create_window creates the window with provided height, width and title on a
// primary display.
enum errorcodes main_window(const char *window_title, int height, int width,
                            std::string filename) {
    // display window position and size
    int display_index = 0;
    int screen_width, screen_height, window_pos_x, window_pos_y;

    // buffers and messages
    std::string input_buffer;
    std::string msg;

    file_errors return_value;
    fs::path absolute_path;

    // set SDL window parameters
    SDL_Window *main_window;
    SDL_DisplayMode mode;
    SDL_Event event;
    SDL_GLContext gl_context;

    // parsed document
    std::vector<markdown_node> parsed;

    bool is_running = true;

    // initialize global curl context
    global_curl_context context;

    // default font scale
    float font_scale = 2.0f;

    if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO) != 0) {
        std::cerr << "Could not initialize SDL subsystem. SDL error: "
                  << SDL_GetError() << "\n";
        return SDL_INIT_FAILED;
    }

    if (SDL_GetCurrentDisplayMode(display_index, &mode) == 0) {
        screen_width = mode.w;
        screen_height = mode.h;
    } else {
        std::cerr << "Could not get display mode. SDL error" << SDL_GetError()
                  << "\n";
        SDL_Quit();
        return DETERMINE_WINDOW_SIZE_FAILED;
    }

    // Set scaling hint for SDL2 window
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");

    // set GL attributes so that MacOS is happy
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);


    // create a window
    // we'll choose some "happy" coordinates, 1/4 from the left side, 1/3 from
    // the top
    window_pos_x = 0 + (screen_width / 4);
    window_pos_y = 0 + (screen_height / 3);

    main_window = SDL_CreateWindow(
        window_title, window_pos_x, window_pos_y, width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    if (main_window == NULL) {
        std::cerr << "Could not create window. SDL error: " << SDL_GetError()
                  << "\n";
        SDL_Quit();
        return WINDOW_CREATION_FAILED;
    }

    gl_context = SDL_GL_CreateContext(main_window);
    if (gl_context == NULL) {
        std::cerr << "Could not create OpenGL context. SDL error: "
                  << SDL_GetError() << "\n";
        SDL_DestroyWindow(main_window);
        SDL_Quit();
        return GL_CONTEXT_CREATE_FAILED;
    }

    // initialize image loader
    image_loader loader(filename);

    // get actual window size
    int actual_width, actual_height;

    // Initialize ImGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplSDL2_InitForOpenGL(main_window, gl_context);
    ImGui_ImplOpenGL3_Init();

    // initialize fonts
    ImFontConfig cfg;
    cfg.FontDataOwnedByAtlas = false;

    ImFont *fontRegular = io.Fonts->AddFontFromMemoryTTF(
        (void *)_usr_share_fonts_truetype_dejavu_DejaVuSans_ttf,
        _usr_share_fonts_truetype_dejavu_DejaVuSans_ttf_len, 18.0f, &cfg);

    ImFont *fontBold = io.Fonts->AddFontFromMemoryTTF(
        (void *)_usr_share_fonts_truetype_dejavu_DejaVuSans_Bold_ttf,
        _usr_share_fonts_truetype_dejavu_DejaVuSans_Bold_ttf_len, 18.0f, &cfg);

    ImFont *fontItalic = io.Fonts->AddFontFromMemoryTTF(
        (void *)_usr_share_fonts_truetype_dejavu_DejaVuSans_Oblique_ttf,
        _usr_share_fonts_truetype_dejavu_DejaVuSans_Oblique_ttf_len, 18.0f,
        &cfg);

    ImFont *fontBoldItalic = io.Fonts->AddFontFromMemoryTTF(
        (void *)_usr_share_fonts_truetype_dejavu_DejaVuSans_BoldOblique_ttf,
        _usr_share_fonts_truetype_dejavu_DejaVuSans_BoldOblique_ttf_len, 18.0f,
        &cfg);

    ImFont *fontMono = io.Fonts->AddFontFromMemoryTTF(
        (void *)_usr_share_fonts_truetype_dejavu_DejaVuSansMono_ttf,
        _usr_share_fonts_truetype_dejavu_DejaVuSansMono_ttf_len, 18.0f, &cfg);

    // track theme
    bool dark_theme = true;
    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);

    if (!filename.empty()) {
        // try reading the provided file from the commmand line
        return_value = read_file(filename, input_buffer);
        absolute_path = fs::absolute(filename);
        switch (return_value) {
        case FILE_NOT_FOUND: {
            msg = "Error opening: file not found. Path: \n" +
                  absolute_path.string();
            create_ok_modal_dialog("Open File", msg, ICON_INFORMATION);
            break;
        }
        case FILE_READING_FAILED: {
            msg = "Error opening: error reading file on path: \n" +
                  absolute_path.string();
            create_ok_modal_dialog("Open File", msg, ICON_STOP);
            break;
        }
        case FILE_EMPTY: {
            msg = "Empty file provided on path: \n" + absolute_path.string();
            create_ok_modal_dialog("Open File", msg, ICON_INFORMATION);
            break;
        }
        case FILE_WRONG_EXTENSION: {
            msg = "Error opening file: wrong extension of provided file on "
                  "path: \n" +
                  absolute_path.string();
            create_ok_modal_dialog("Open File", msg, ICON_INFORMATION);
            break;
        }
        case FILE_READ_SUCCESSFUL:
            parsed = parse_document(input_buffer.c_str(), input_buffer.size());
            if (parsed.empty()) {
                msg = "Encountered an error during parsing of the MarkDown "
                      "document";
                create_ok_modal_dialog("Error parsing document", msg,
                                       ICON_STOP);
            }
            break;
        default:
            break;
        }
    }

    // Main loop
    while (is_running) {
        SDL_GetWindowSize(main_window, &actual_width, &actual_height);
        while (SDL_PollEvent(&event) != 0) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            if (event.type == SDL_QUIT) {
                is_running = false;
            } else if (event.type == SDL_WINDOWEVENT &&
                       event.window.event == SDL_WINDOWEVENT_CLOSE) {
                is_running = false;
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Main", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
        ImGui::SetWindowFontScale(2.0f);

        if (ImGui::Button("Open File")) {
            auto selection = pfd::open_file("Choose MarkDown Document", ".",
                                            {"Markdown files", "*.md"})
                                 .result();
            if (!selection.empty()) {
                return_value = read_file(selection[0], input_buffer);
                absolute_path = fs::absolute(selection[0]);
                switch (return_value) {
                case FILE_NOT_FOUND: {
                    msg = "Error opening: file not found. Path: \n" +
                          absolute_path.string();
                    create_ok_modal_dialog("Open File", msg, ICON_INFORMATION);
                    break;
                }
                case FILE_READING_FAILED: {
                    msg = "Error opening: error reading file on path: \n" +
                          absolute_path.string();
                    create_ok_modal_dialog("Open File", msg, ICON_STOP);
                    break;
                }
                case FILE_EMPTY: {
                    msg = "Empty file provided on path: \n" +
                          absolute_path.string();
                    create_ok_modal_dialog("Open File", msg, ICON_INFORMATION);
                    break;
                }
                case FILE_WRONG_EXTENSION: {
                    msg = "Error opening file: wrong extension of provided "
                          "file on path: "
                          "\n" +
                          absolute_path.string();
                    create_ok_modal_dialog("Open File", msg, ICON_INFORMATION);
                    break;
                }
                case FILE_READ_SUCCESSFUL:
                    parsed = parse_document(input_buffer.c_str(),
                                            input_buffer.size());
                    loader.set_path(absolute_path);
                    if (parsed.empty()) {
                        msg = "Encountered an error during parsing of the "
                              "MarkDown document";
                        create_ok_modal_dialog("Error parsing document", msg,
                                               ICON_STOP);
                    }
                    break;
                default:
                    break;
                }
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Zoom in")) {
            font_scale = font_scale + 0.2f;
            if (font_scale >= 4.0f) {
                font_scale = 4.0f;
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Zoom out")) {
            font_scale = font_scale - 0.2f;
            if (font_scale <= 0.4f) {
                font_scale = 0.4f;
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Switch Theme")) {
            if (dark_theme) {
                ImGui::StyleColorsLight();
                dark_theme = false;
            } else {
                ImGui::StyleColorsDark();
                ImGuiStyle &style = ImGui::GetStyle();

                style.Colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
                dark_theme = true;
            }
        }

        ImGui::BeginChild("Content", ImVec2(0, 0), false);
        ImGui::SetWindowFontScale(font_scale);

        // process all downloaded iamges
        loader.process_uploads();

        render_document(parsed, fontRegular, fontBold, fontItalic,
                        fontBoldItalic, fontMono, dark_theme, loader,
                        actual_width);

        ImGui::EndChild();
        ImGui::End();

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(main_window);
    }

    // clear all data while the OpenGL context still exists
    loader.clear_resources();

    // closing ImGui specific stuff
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    // close SDL specific stuff
    SDL_DestroyWindow(main_window);
    SDL_Quit();
    return SUCCESSFUL_EXIT;
}

// create_ok_modal_dialog creates a modal dialog with the OK button. Consumes
// the icon information and text to render.
void create_ok_modal_dialog(std::string window_title, std::string msg,
                            enum icon ICON) {
    pfd::icon icon = pfd::icon::info;

    if (ICON == ICON_WARNING) {
        icon = pfd::icon::warning;
    }
    if (ICON == ICON_INFORMATION) {
        icon = pfd::icon::info;
    }

    if (ICON == ICON_STOP) {
        icon = pfd::icon::error;
    }

    pfd::message(window_title, msg, pfd::choice::ok, icon).result();
}
