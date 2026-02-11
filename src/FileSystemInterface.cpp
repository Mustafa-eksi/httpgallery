#pragma once
#include "FileCache.hpp"
#include "Logging.cpp"
#include "LookupTables.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

std::string get_mime_type(std::string name)
{
    auto dotpos = name.rfind('.');
    if (dotpos == std::string::npos)
        return "text/html";
    std::string ext = name.substr(dotpos + 1);
    if (mime_types.find(ext) == mime_types.end())
        return "text/plain";
    return mime_types.at(ext);
}

const char *BASE64_ALPHABET
    = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
std::string base64_hash(std::string data)
{
    std::string output;
    for (auto c : data) {
        output += BASE64_ALPHABET[c % 64];
    }
    return output;
}

std::string string_format(const std::string &format) { return format; }

template <typename T, typename... Args>
std::string string_format(const std::string &format, T one_arg, Args... args)
{
    auto format_pos = format.find("{}");
    if (format_pos == std::string::npos)
        return format;
    std::string new_format = format.substr(0, format_pos) + one_arg
        + format.substr(format_pos + 2);
    return string_format(new_format, args...);
}

std::string read_binary_to_string(const std::string path,
                                  uintmax_t range_start = 0,
                                  uintmax_t range_end   = 0)
{
    if (range_end < range_start)
        return "error: range";
    if (range_end == 0)
        range_end = std::filesystem::file_size(path);

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cout << "ERROR!: " << path << std::endl;
        return "Error";
    }
    file.seekg(range_start);
    std::string strbuff(range_end - range_start, '\0');
    if (!file.read(&strbuff[0], range_end - range_start)) {
        strbuff.clear();
        strbuff.shrink_to_fit();
        return "Error";
    }
    return strbuff;
}

/**
 * @brief File LRU cache.
 */
class FileStorage {
    FileCache<std::string, std::string> cache;
    Logger &logger;

public:
    FileStorage(size_t cache_size, Logger &l)
        : cache(cache_size)
        , logger(l)
    {
    }
    /**
     * @brief Read the file either from the cache or disk.
     *
     * @param path Path to the file that is requested.
     * @param range_start Start of the content range of the file.
     * @param range_end End of the content range of the file. 0 means read the
     * full file.
     * @return Raw file content
     */
    std::string read(const std::string path, uintmax_t range_start = 0,
                     uintmax_t range_end = 0)
    {
        auto cache_entry_name = path;
        auto opt_val          = cache.get(cache_entry_name);
        if (opt_val) {
            auto file_entry = opt_val.value();
            auto r0         = file_entry.range.first;
            auto r1         = file_entry.range.second;
            logger.changeMetric("Partial Cache Hit", 1);
            // r0 p0 p1 r1
            // If Cached value includes the part we want:
            if (r0 <= range_start && range_end <= r1) {
                return file_entry.data.substr(
                    range_start - r0, (range_end - r0) - (range_start - r0));
            }
            // r0 p0 r1 p1
            if (r0 <= range_start && range_end < r1) {
                std::string strbuff
                    = read_binary_to_string(path, r1, range_end);
                std::string new_data
                    = file_entry.data.substr(range_start - r0) + strbuff;
                cache.put(cache_entry_name, new_data,
                          std::make_pair(r0, range_end));
                return new_data;
            }
            // p0 r0 p1 r1
            if (range_start <= r0 && range_end <= r1) {
                std::string strbuff
                    = read_binary_to_string(path, range_start, r0);
                std::string new_data
                    = strbuff + file_entry.data.substr(0, range_end - r0);
                cache.put(cache_entry_name, new_data,
                          std::make_pair(range_start, r1));
                return new_data;
            }
            // p0 r0 r1 p1
            if (range_start <= r0 && r1 <= range_end) {
                std::string left_part
                    = read_binary_to_string(path, range_start, r0);
                std::string right_part
                    = read_binary_to_string(path, r1, range_end);
                std::string new_data = left_part + file_entry.data + right_part;
                cache.put(cache_entry_name, new_data,
                          std::make_pair(range_start, range_end));
                return new_data;
            }
            // TODO: Cache non-intersecting ranges
        }
        logger.changeMetric("Cache Miss", 1);
        std::string strbuff
            = read_binary_to_string(path, range_start, range_end);
        cache.put(cache_entry_name, strbuff,
                  std::make_pair(range_start, range_end));
        return strbuff;
    }
};

const auto dir_icon_link   = "/?icon=directory";
const auto video_icon_link = "/?icon=video";
const auto text_icon_link  = "/?icon=text";

const auto buttonTemplate
    = "<button class=\"item\" onclick=\"window.location.href='{}'\">"
      "<img class=\"item-thumbnail\" src=\"{}\">"
      "<span class=\"item-name\" href=\"{}\">{}</span>"
      "</button>";
const auto listviewButton
    = "<button class=\"item-list-view\" onclick=\"window.location.href='{}'\">"
      "<span class=\"item-name\" href=\"{}\">{}</span>"
      "</button>";
std::string list_contents(std::string current_address, std::string path,
                          std::string queries = "", bool list_view = false)
{
    std::string output = "";
    for (const auto &entry : std::filesystem::directory_iterator(path)) {
        try {
            if (!entry.is_regular_file() && !entry.is_directory())
                continue;
            if (current_address.back() != '/')
                current_address += '/';

            auto filename = entry.path().filename().string();
            auto filepath = current_address + filename + queries;
            auto iconpath = filepath;

            if (!queries.empty()) {
                iconpath += "&icon=1";
            } else {
                iconpath += "?icon=1";
            }

            if (list_view) {
                output += string_format(listviewButton, filepath, filepath,
                                        filename);
                continue;
            }

            if (get_mime_type(filename).starts_with("image")) {
                output += string_format(buttonTemplate, filepath, filepath,
                                        filepath, filename);
            } else if (get_mime_type(filename).starts_with("video")) {
                output += string_format(buttonTemplate, filepath,
                                        filepath + "?icon=video", filepath,
                                        filename);
            } else {
                output += string_format(buttonTemplate, filepath,
                                        entry.is_directory() ? dir_icon_link
                                                             : text_icon_link,
                                        filepath, filename);
            }
        } catch (std::exception &e) {
            std::cout << "Error: " << entry.path().c_str() << e.what()
                      << std::endl;
        }
    }
    return output;
}
