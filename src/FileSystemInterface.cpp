#include "LookupTables.hpp"

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
    ;
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

std::string read_entire_file(std::string path)
{
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) {
        std::cout << "Error: File not found: " << path << std::endl;
        return "";
    }
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);
    char *buff = (char *)malloc(length * sizeof(char) + 1);
    if (!buff) {
        std::cout << "Error: Memory allocation failed" << std::endl;
        fclose(f);
        return "";
    }
    fread(buff, sizeof(char), length, f);
    buff[length] = '\0';
    fclose(f);
    std::string result = buff;
    free(buff);
    return result;
}

const auto dir_icon_link   = "/?icon=directory";
const auto video_icon_link = "/?icon=video";
const auto text_icon_link  = "/?icon=text";

const auto buttonTemplate = "<div class=\"item\">"
                            "<img class=\"item-icon\" src=\"{}\">"
                            "<a class=\"item-name\" href=\"{}\">{}</a>"
                            "</div>";
const auto imageTemplate  = "<div class=\"item\">"
                            "<img class=\"item-thumbnail\" src=\"{}\">"
                            "<a class=\"item-name\" href=\"{}\">{}</a>"
                            "</div>";
const auto videoTemplate  = "<div class=\"item\">"
                            "<img class=\"item-icon\" src=\"{}\">"
                            "<a class=\"item-name\" href=\"{}\">{}</a>"
                            "</div>";
const auto ffmpegCommand
    = "ffmpeg -i {} -ss 00:00:10 -vframes 1 thumbnail-{}.jpg";
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
            if (get_mime_type(filename).starts_with("image") && !list_view) {
                output += string_format(imageTemplate, filepath, filepath,
                                        filename);
            } else if (get_mime_type(filename).starts_with("video")
                       && entry.file_size() < 5e+7 && !list_view) {
                output += string_format(videoTemplate, video_icon_link,
                                        filepath, filename);
            } else {
                output += string_format(buttonTemplate,
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
