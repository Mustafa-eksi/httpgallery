#include "EmbeddedResources.hpp"
#include "FileSystemInterface.hpp"
#include "Server.hpp"

#ifndef HTTPGALLERY_RES_DIR
#define HTTPGALLERY_RES_DIR "./res"
#endif

typedef enum PageType {
    DirectoryPage,
    FileDataPage,
    IconData,
} PageType;

/**
 * @brief Includes inner workings of the http server
 */
class FileServer : public Server {
#ifndef HTTPGALLERY_EMBED_RESOURCES
    std::string directory_icon_data, video_icon_data, text_icon_data;
#endif
    FileStorage file_storage;
    std::string htmltemplate_list, htmltemplate_icon, htmltemplate_error, path;
    bool https, cache_files, has_thumbnailer;

public:
    /**
     * @brief Initialize a http server with https support.
     * @param logr Reference to the Logger.
     * @param conf Configuration object that is used for permission system.
     */
    FileServer(Logger &logr, Configuration &&conf);

    /**
     * @brief Returns appropriate page type based on http request.
     * @param msg Http request.
     * @return Returns the page type.
     */
    PageType choosePageType(HttpMessage msg);

    /**
     * @brief Generates video thumbnail.
     * @param filepath path to the video file.
     * @return Returns raw png data if succeeds, std::nullopt otherwise.
     */
    std::optional<std::string> generateVideoThumbnail(std::string filepath);

    /**
     * @brief Generates http response according to msg.
     * @param msg Http request.
     * @return Html data.
     */
    std::string handleGetRequest(HttpMessage msg);

    /**
     * @brief This function handles PUT requests.
     *
     * @return Returns the response.
     */
    std::string handlePutRequest(HttpMessage msg);

    /**
     * @brief This function handles DELETE requests.
     *
     * @return Returns the response.
     */
    std::string handleDeleteRequest(HttpMessage msg);
};
