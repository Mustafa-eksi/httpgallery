#include "FileServer.hpp"

FileServer::FileServer(Logger &logr, Configuration &&conf)
    : Server(logr, std::move(conf))
    , file_storage(config.configInt("CacheSize"), logr)
{
    https           = config.configBool("UseHttps");
    path            = config.configString("WorkingDirectory");
    cache_files     = config.configBool("CacheFiles");
    has_thumbnailer = config.configBool("HasVideoThumbnailer");

    htmltemplate_list  = read_binary_to_string(HTTPGALLERY_RES_DIR
                                               "/html/template-list-view.html");
    htmltemplate_icon  = read_binary_to_string(HTTPGALLERY_RES_DIR
                                               "/html/template-icon-view.html");
    htmltemplate_error = read_binary_to_string(HTTPGALLERY_RES_DIR
                                               "/html/template-error.html");
#ifndef HTTPGALLERY_EMBED_RESOURCES
    directory_icon_data = read_binary_to_string(HTTPGALLERY_RES_DIR
                                                "/image/directory-icon.png");
    video_icon_data
        = read_binary_to_string(HTTPGALLERY_RES_DIR "/image/video-icon.png");
    text_icon_data
        = read_binary_to_string(HTTPGALLERY_RES_DIR "/image/text-icon.png");
#endif
}

PageType FileServer::choosePageType(HttpMessage msg)
{
    if (msg.queries.contains("icon"))
        return IconData;
    std::string decoded_path = path + msg.address;
    bool is_dir              = std::filesystem::is_directory(decoded_path);
    return is_dir ? DirectoryPage : FileDataPage;
}

std::optional<std::string>
FileServer::generateVideoThumbnail(std::string filepath)
{
    std::string thumbnail_path
        = "/tmp/httpgallery-" + base64_hash(filepath) + ".png";
    if (access(thumbnail_path.c_str(), R_OK) == 0)
        return read_binary_to_string(thumbnail_path);

    std::string command = "ffmpegthumbnailer -i\"" + filepath + "\" -o"
        + thumbnail_path + " -m -a";
    system(command.c_str());
    if (access(thumbnail_path.c_str(), R_OK) == 0)
        return read_binary_to_string(thumbnail_path);
    return std::nullopt;
}

std::string FileServer::handleGetRequest(HttpMessage msg)
{
    PageType pt = this->choosePageType(msg);
    if (path.back() == '/' && path.size() > 1)
        path = path.substr(0, path.length() - 1);

    auto filepath = path + msg.address;
    // This responds with 404 rather than 403 because 403 can leak existence of
    // some files
    if (access(filepath.c_str(), R_OK) != 0
        || (path != "." && !isPathCanonical(filepath)))
        return HttpResponseBuilder().ErrorPage(htmltemplate_error, 404).build();

    auto r = negotiateAuth(msg, filepath, P_READ);
    if (!r.empty())
        return r;

    std::string comp = "";
    if (msg.headers.contains("Accept-Encoding")) {
        comp = msg.headers["Accept-Encoding"];
    }
    if (pt == FileDataPage) {
        uintmax_t filesize = std::filesystem::file_size(filepath);
        auto range_opt     = msg.getRange(filesize);
        if (!range_opt.has_value()) {
            return HttpResponseBuilder()
                .ErrorPage(htmltemplate_error, 416)
                .SetHeader("Content-Range",
                           "bytes */" + std::to_string(filesize))
                .build();
        }
        auto [range_start, range_end] = range_opt.value();
        int status = (range_end - range_start) == filesize ? 200 : 206;
        std::string mimetype = get_mime_type(msg.address);
        std::string file_content;

        logger.changeMetric("File Data Sent", filesize);
        if (msg.type == GET) {
            if (cache_files)
                file_content
                    = file_storage.read(filepath, range_start, range_end);
            else
                file_content
                    = read_binary_to_string(filepath, range_start, range_end);
        } else if (msg.type == HEAD) {
            file_content = "";
        }

        return HttpResponseBuilder()
            .Status(status)
            .ContentType(mimetype)
            .ContentRange(range_start, range_end)
            .Content(file_content)
            .CompressContent(comp)
            .build();
    } else if (pt == DirectoryPage) {
        bool list_view = msg.queries.contains("list-view")
            && msg.queries["list-view"] == "true";
        std::string dir_page_contents = list_contents(
            msg.address, filepath, msg.queriesToString(), list_view);
        std::string current_path = msg.address.substr(
            0, msg.address.length() - 1); // exclude last char

        auto slash_pos = current_path.rfind('/');
        std::string up = "/";
        if (slash_pos != std::string::npos)
            up = current_path.substr(0, slash_pos + 1);
        std::string final_content = string_format(
            list_view ? this->htmltemplate_list : this->htmltemplate_icon,
            filepath.c_str(), up, dir_page_contents.c_str());
        uintmax_t content_length = final_content.length();
        if (msg.type == HEAD)
            final_content = "";
        return HttpResponseBuilder()
            .Status(200)
            .ContentType("text/html; charset=utf-8")
            .Content(final_content)
            .ContentLength(content_length) // Same as one in above
            .CompressContent(comp)
            .build();
    } else if (pt == IconData) {
        if (msg.queries["icon"] == "video") {
            std::string image_data;
            if (has_thumbnailer) {
                auto thumb_opt = generateVideoThumbnail(filepath);
                if (thumb_opt.has_value())
                    image_data = thumb_opt.value();
                else
                    logger.report("ERROR", "Thumbnailer failed");
            }

            if (image_data.empty())
                image_data = std::string(video_icon_data.begin(),
                                         video_icon_data.end());
            return HttpResponseBuilder()
                .Status(200)
                .ContentType("image/png")
                .Content(image_data)
                .CompressContent(comp)
                .build();
        } else if (msg.queries["icon"] == "directory") {
            std::string image_data(directory_icon_data.begin(),
                                   directory_icon_data.end());
            return HttpResponseBuilder()
                .Status(200)
                .ContentType("image/png")
                .Content(image_data)
                .CompressContent(comp)
                .build();
        } else {
            std::string image_data(text_icon_data.begin(),
                                   text_icon_data.end());
            return HttpResponseBuilder()
                .Status(200)
                .ContentType("image/png")
                .Content(image_data)
                .CompressContent(comp)
                .build();
        }
    } else {
        logger.report("ERROR", "Invalid page type");
    }
    return "";
}

std::string FileServer::handlePutRequest(HttpMessage msg)
{
    if (msg.type != PUT)
        return HttpResponseBuilder().ErrorPage(htmltemplate_error, 404).build();

    // FIXME: setting path to "." enables non-canonical paths
    auto filepath_   = std::filesystem::path(path + msg.address);
    auto parent_path = filepath_.parent_path().string();
    auto filepath    = filepath_.string();

    // This responds with 404 rather than 403 because 403 can leak existence of
    // some files
    if (access(parent_path.c_str(), R_OK) != 0)
        return HttpResponseBuilder().ErrorPage(htmltemplate_error, 404).build();

    auto r = negotiateAuth(msg, parent_path, P_WRITE);
    if (!r.empty())
        return r;

    std::ofstream f(filepath);
    f << msg.content;
    f.close();

    return HttpResponseBuilder().Status(204).build();
}

std::string FileServer::handleDeleteRequest(HttpMessage msg)
{
    if (msg.type != DELETE || !std::filesystem::exists(path + msg.address))
        return HttpResponseBuilder().ErrorPage(htmltemplate_error, 404).build();
    auto filepath_   = std::filesystem::canonical(path + msg.address);
    auto parent_path = filepath_.parent_path().string();
    auto filepath    = filepath_.string();

    // This responds with 404 rather than 403 because 403 can leak existence of
    // some files
    if (access(parent_path.c_str(), R_OK) != 0)
        return HttpResponseBuilder().ErrorPage(htmltemplate_error, 404).build();
    auto r = negotiateAuth(msg, filepath, P_DELETE);
    if (!r.empty())
        return r;

    if (std::remove(filepath.c_str()) != 0)
        return HttpResponseBuilder().ErrorPage(htmltemplate_error, 404).build();
    else
        return HttpResponseBuilder().Status(204).build();
}
