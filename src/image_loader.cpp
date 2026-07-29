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
#include <algorithm>
#include <curl/curl.h>
#include <functional>
#include <iostream>
#include <string>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

// image_loader constructor creates the thread pool on class instantiation.
image_loader::image_loader(const std::string &document_path) : stop_(false) {
    std::cerr << "Initializing worker pool and setting base path\n";
    // set up document base path
    if (!document_path.empty()) {
        std::filesystem::path p(document_path);

        // need to convert to absolute path
        base_path = std::filesystem::absolute(p).parent_path();
    } else {
// set the base path to the home directory of the current user
#if defined(_WIN32)
        const char *home = std::getenv("USERPROFILE");
        if (home) {
            std::filesystem::path p(home);
            base_path = p;
        }
#else
        // set path to current working directory
        const char *pwd = std::getenv("PWD");
        if (pwd) {
            std::filesystem::path p(pwd);
            base_path = p;
        } 
#endif
    }

    std::cerr << "Base path set to: " << base_path << "\n";

    // initialize global curl context
    curl_global_init(CURL_GLOBAL_DEFAULT);

    for (int i = 0; i < 4; i++) {
        workers_[i] = std::thread([this] { fetch_worker(); });
        std::cerr << "Initalized worker thread " << i << "\n";
    }
}

// ~image_loader destructor joins all threads and ends them safely before
// closing
image_loader::~image_loader() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }
    // notify all threads
    cv_.notify_all();

    // join all threads
    for (auto &thread : workers_) {
        thread.join();
    }

    // clean up all resources
    clear_resources();

    // close the global curl context
    curl_global_cleanup();
}

// get_proxy checks if there are any proxy variables based on prefix so we can
// explicitly add them to curl.
std::string get_proxy(std::string prefix) {
    std::string complete = prefix + "_proxy";

    std::transform(complete.begin(), complete.end(), complete.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    const char *val = std::getenv(complete.c_str());
    std::string proxy = val ? val : "";

    if (proxy == "") {
        std::transform(complete.begin(), complete.end(), complete.begin(),
                       [](unsigned char c) { return std::toupper(c); });
        const char *val = std::getenv(complete.c_str());
        proxy = val ? val : "";
    }
    return proxy;
}

// get_full_url returns a full url to the caller along with its prefix.
std::string get_full_url(const std::filesystem::path base_path,
                         const std::string &url) {
    std::string full_url;

    // convert base path to string

    std::string path = base_path.string();

    // check if the base path has a trailing slash, if it does, pop it
    if (path.ends_with("/")) {
        path.pop_back();
    }

    // assume that file is local if there's no prefix in front
    if (!url.starts_with("http") && (!url.starts_with("file"))) {
        // if the url starts with a slash, treat it like a full path
        if (url.starts_with("/")) {
            full_url = "file://" + url;
        } else {
            full_url = "file://" + path + "/" + url;
        }
    } else {
        full_url = url;
    }
    return full_url;
}

// request_image requests the image from a remote source or a local path.
void image_loader::request_image(const std::string &url) {
    std::string full_url;
    image_state state;

    full_url = get_full_url(this->base_path, url);

    // check if a key with the url already exists in our dictionary
    // if it doesn't exist, add it and call the downloader
    {
        // lock mutex during checks and possible pushes to the queue
        std::unique_lock<std::mutex> lock(mutex_);
        state = image_loader::get_state(full_url);

        if (state == NOT_FOUND) {
            image_loader::insert_entry(full_url);
            std::cerr << "Requesting image with url " << full_url << "\n";
            work_queue_.push(full_url);
        }
    }
    cv_.notify_one();
}

// get_state checks the state of an upload for a provided url.
image_state image_loader::get_state(const std::string &url) {
    // check if the key exists
    if (cache_.contains(url)) {
        return cache_[url].state;
    } else {
        return NOT_FOUND;
    }
}

// insert_entry inserts the new entry to the k-v store for later fetching.
image_state image_loader::insert_entry(const std::string &url) {
    cache_[url] = image_entry{
        .state = CREATED,
        .raw_bytes = {},
        .width = 0,
        .height = 0,
        .texture_id = 0,
        .error_message = {},
    };
    return CREATED;
}

// fetch_worker runs the background thread for downloading images
void image_loader::fetch_worker() {
    std::string url;

    while (true) {
        image_entry entry;
        entry.state = LOADING;

        {
            // lock mutex and check if there's any work
            std::unique_lock<std::mutex> lock(mutex_);

            // wait for the condition
            cv_.wait(lock, [this] { return !work_queue_.empty() || stop_; });

            if (stop_ && work_queue_.empty()) {
                return;
            }

            // get the next download url from the queue
            url = move(work_queue_.front());
            work_queue_.pop();
        }

        // initialize curl context
        CURL *handle = curl_easy_init();

        if (handle == NULL) {
            // curl context was not initialized
            // lock mutex and set image loading to failed
            std::unique_lock<std::mutex> lock(mutex_);
            cache_[url].state = FAILED;
            cache_[url].error_message = "Curl context failed to initialize.";
            continue;
        }

        // check if there are any proxy variables defined
        std::string http_proxy = get_proxy("http");
        std::string https_proxy = get_proxy("https");

        // set proxy
        if (http_proxy != "") {
            curl_easy_setopt(handle, CURLOPT_PROXY, http_proxy.c_str());
        }
        if (https_proxy != "") {
            curl_easy_setopt(handle, CURLOPT_PROXY, https_proxy.c_str());
        }

        // set user agent
        curl_easy_setopt(handle, CURLOPT_USERAGENT, "Barkdown/1.0");

        curl_easy_setopt(handle, CURLOPT_URL, url.c_str());

        // add write callback function
        // and buffer information where the data should be stored
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, &entry.raw_bytes);

        // handle redirects
        curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);

        // attempt to fetch image
        CURLcode success = curl_easy_perform(handle);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (success == CURLE_OK) {
                cache_[url].state = DOWNLOAD_COMPLETE;
                cache_[url].raw_bytes = std::move(entry.raw_bytes);
            } else {
                cache_[url].state = FAILED;
                cache_[url].error_message = curl_easy_strerror(success);
            }
        }

        curl_easy_cleanup(handle);
    }
}

// write_callback is the curl callback function that returns the number of bytes
// stored per call.
size_t write_callback(char *ptr, size_t size, size_t nmemb, void *clientptr) {
    auto *buffer = static_cast<std::vector<unsigned char> *>(clientptr);
    buffer->insert(buffer->end(), ptr, ptr + size * nmemb);
    return size * nmemb;
}

// get_entry returns the pointer to the image object in cache to the caller
// based on the provided url. If the object does not exist the function returns
// a null pointer back.
std::optional<image_entry> image_loader::get_entry(const std::string &url) {
    std::string full_url = get_full_url(this->base_path, url);
    {
        // lock mutex
        std::unique_lock<std::mutex> lock(mutex_);
        auto it = cache_.find(full_url);
        if (it != cache_.end()) {
            return it->second;
        } else {
            return std::nullopt;
        }
    }
}

// process_uploads is the main thread call to upload downloaded images to
// OpenGL.
void image_loader::process_uploads() {
    int height, width, channels;
    unsigned char *buffer;

    std::unordered_map<std::string, std::vector<unsigned char>>
        needs_processing;

    {
        // lock mutex so we can collect all images that are in DOWNLOAD_COMPLETE
        // status
        std::unique_lock<std::mutex> lock(mutex_);
        for (auto it = cache_.begin(); it != cache_.end(); ++it) {
            if (it->second.state == DOWNLOAD_COMPLETE) {
                needs_processing[it->first] = std::move(it->second.raw_bytes);
                it->second.state = PROCESSING;
            }
        }
    }

    for (auto it = needs_processing.begin(); it != needs_processing.end();
         ++it) {
        // decode the image
        const std::string &url = it->first;
        auto &raw = it->second;

        buffer = stbi_load_from_memory(raw.data(), raw.size(), &width, &height,
                                       &channels, 4);

        if (buffer == NULL) {
            std::string error_msg = stbi_failure_reason();
            std::cerr << "Error processing file: " << error_msg << "\n";
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cache_[it->first].state = FAILED;
                cache_[it->first].error_message = error_msg;
            }
            continue;
        }

        GLuint texture_id;

        // load the texture
        glGenTextures(1, &texture_id);

        // bind the texture to the created name
        glBindTexture(GL_TEXTURE_2D, texture_id);

        // add texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // generate a texture
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, buffer);

        {
            // lock mutex
            std::unique_lock<std::mutex> lock(mutex_);
            cache_[url].state = UPLOADED;
            cache_[url].height = height;
            cache_[url].width = width;
            cache_[url].texture_id = texture_id;
            std::cerr << "Image with source url " << url
                      << " processed, height: " << cache_[url].height
                      << ", width: " << cache_[url].width
                      << ", size: " << raw.size()
                      << ", state: " << cache_[url].state << "\n";
        }

        // free buffer
        stbi_image_free(buffer);
    }
}

// set_path sets the base path for the currently opened document
void image_loader::set_path(const std::string &document_path) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        std::filesystem::path p(document_path);
        base_path = p.parent_path();
    }

    invalidate_cache();
}

// invalidate_cache invalidates the current image cache so that we don't leak
// memory. Also removes the cache in the GPU.
void image_loader::invalidate_cache() {
    std::unique_lock<std::mutex> lock(mutex_);

    for (auto &[url, entry] : cache_) {
        if (entry.texture_id != 0) {
            glDeleteTextures(1, &entry.texture_id);
            entry.texture_id = 0;
        }
    }

    // clear all cache and empty queue
    cache_.clear();
    std::queue<std::string> empty_queue;
    std::swap(work_queue_, empty_queue);
}

// clear_resources calls the private method invalidate_cache() to safely clean
// up any resources before OpenGL context is destroyed.
void image_loader::clear_resources() { invalidate_cache(); }
