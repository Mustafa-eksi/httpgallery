#ifndef HTTPGALLERY_FILESYSTEM
#define HTTPGALLERY_FILESYSTEM

#include "FileCache.hpp"
#include "Logging.hpp"
#include "LookupTables.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

std::string get_mime_type(std::string name);

std::string base64_hash(std::string data);

std::string base64_decode(std::string data);

inline std::string string_format(const std::string &format);

inline std::string string_format(const std::string &format) { return format; }

template <typename T, typename... Args>
inline std::string string_format(const std::string &format, T one_arg,
                                 Args... args)
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
                                  uintmax_t range_end   = 0);
/**
 * @brief File LRU cache.
 */
class FileStorage {
    FileCache<std::string, std::string> cache;
    Logger &logger;

public:
    FileStorage(size_t cache_size, Logger &l);
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
                     uintmax_t range_end = 0);
};

std::string list_contents(std::string current_address, std::string path,
                          std::string queries = "", bool list_view = false);

#endif // HTTPGALLERY_FILESYSTEM
