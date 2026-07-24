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
#include <GL/gl.h>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// image_state is an enum containing the current state of the image.
enum image_state {
    NOT_FOUND,         // download doesn't exist yet
    CREATED,           // created but not picked up
    LOADING,           // download in progress
    DOWNLOAD_COMPLETE, // download is complete
    PROCESSING,        // image has been downloaded and is in processing phase
    UPLOADED,          // image in process of rendering
    FAILED,            // image loading failed
};

// image_entry structs contains information about the image we want to download,
// based on the data provided in the markdown document.
struct image_entry {
        image_state state;
        std::vector<unsigned char> raw_bytes; // image raw bytes
        int width, height;
        GLuint texture_id;
        std::string error_message;
};

// write_callback is the curl callback function that returns the number of bytes
// stored per call.
size_t write_callback(char *ptr, size_t size, size_t nmemb, void *clientptr);

// image_loader class represents the image loading object. Tracks object state,
// starts remote fetching and processes each of the downloaded images into
// textures.
class image_loader {
    public:
        // Constructor that initializes the thread pool
        image_loader(const std::string &document_path);

        // Class destructor that joins all threads and closes the pool safely
        ~image_loader();

        // request_image requests the image from a remote source or a local
        // path.
        void request_image(const std::string &url);

        // get_entry returns the image information structure based on the
        // provided url.
        std::optional<image_entry> get_entry(const std::string &url);
        // process_uploads is the main thread call to upload downloaded images
        // to OpenGL.
        void process_uploads();

        // set_path sets the base path for the currently opened document
        void set_path(const std::string &base_path);

        // clear_resources calls the private method invalidate_cache() to safely
        // clean up any resources when class destructor is called.
        void clear_resources();

    private:
        // document root directory
        std::filesystem::path base_path;

        // a k-v dictionary that maps each image url with the image data
        std::unordered_map<std::string, image_entry> cache_;
        // mutex to protect cache against race conditions
        std::mutex mutex_;
        // worker threads (max 4 workers running)
        std::thread workers_[4];

        // get_state checks the state of an upload for a provided url.
        image_state get_state(const std::string &url);

        // condition variable to wake workers when work arrives
        std::condition_variable cv_;

        // stop flag
        bool stop_;

        // job queue
        std::queue<std::string> work_queue_;

        // fetch_worker runs the background thread for downloading images
        void fetch_worker();
        // insert_entry inserts the new entry for processing.
        image_state insert_entry(const std::string &url);

        // invalidate_cache invalidates the current image cache so that we don't
        // leak memory. Also removes the cache in the GPU.
        void invalidate_cache();
};

// get_proxy checks if there are any proxy variables so we can add them to curl
// request.
std::string get_proxy(std::string prefix);

// get_full_url returns the full url to the object including its prefix
std::string get_full_url(const std::filesystem::path base_path,
                         const std::string &url);