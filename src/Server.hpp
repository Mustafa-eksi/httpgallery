#include "EmbeddedResources.hpp"
#include "FileSystemInterface.cpp"
#include "Http.cpp"
#include "HttpResponseBuilder.cpp"
#include "Logging.cpp"
#include <atomic>
#include <errno.h>
#include <sys/resource.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#ifndef HTTPGALLERY_NO_OPENSSL
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

static const unsigned char HTTPGALLERY_SSL_CACHE_ID[] = "HttpGallery";
const int HTTPGALLERY_SSL_CACHE_SIZE                  = 1024;
const int HTTPGALLERY_SSL_TIMEOUT                     = 3600;
#endif

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
class Server {
#ifndef HTTPGALLERY_NO_OPENSSL
    BIO *ssl_socket;
    SSL_CTX *ctx = NULL;
#endif
#ifndef HTTPGALLERY_EMBED_RESOURCES
    std::string directory_icon_data, video_icon_data, text_icon_data;
#endif
    FileStorage file_storage;
    int socketfd;
    struct sockaddr_in server_address;
    socklen_t address_length;
    std::vector<std::thread> threads;
    std::string path;
    std::string htmltemplate_list, htmltemplate_icon, htmltemplate_error;
    bool https, cache_files, has_thumbnailer;
    Logger &logger;

public:
    /**
     * @brief Gracefully closes the server when it is set to true.
     */
    std::atomic<bool> shouldClose = false;
    /**
     * @brief Initialize a http server with https support.
     * @param logr Reference to the Logger.
     * @param p Path to serve.
     * @param cert_path Path to certificate chain file.
     * @param pkey_path Path to private key file.
     * @param caching Enables server side file caching.
     * @param cache_size Sets file cache size (effective only when caching is
     * true).
     * @param thumbnailer Enables video thumbnailing (requires
     * ffmpegthumbnailer)
     */
    Server(Logger &logr, std::string p, size_t port, std::string cert_path,
           std::string pkey_path, bool caching = true, size_t cache_size = 100,
           bool thumbnailer = false);
    /**
     * @brief Initialize a http server with https support.
     * @param logr Reference to the Logger.
     * @param p Path to serve.
     * @param port port number
     * @param backlog sets socket's backlog (how many requests can there be in
     * line to be processed)
     * @param caching Enables server side file caching.
     * @param cache_size Sets file cache size (effective only when caching is
     * true).
     * @param thumbnailer Enables video thumbnailing (requires
     * ffmpegthumbnailer)
     */
    Server(Logger &logr, std::string p = ".", size_t port = 8000,
           int backlog = 100, bool caching = true, size_t cache_size = 100,
           bool thumbnailer = false);
    ~Server();
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
     * @brief Sanitizes the path against path traversal exploit.
     * @param unsanitized_path Path to sanitize.
     * @return Returns true if the path received is canonical (in its shortest
     * form), false otherwise.
     */
    bool isPathCanonical(std::string unsanitized_path);
    /**
     * @brief Generates http response according to msg.
     * @param msg Http request.
     * @return Html data.
     */
    std::string generateContent(HttpMessage msg);

#ifndef HTTPGALLERY_NO_OPENSSL
    /**
     * @brief Calls generateContent and sends the result to client.
     *
     * This function exists to enable us to parallize the content generation
     * process.
     *
     * @param ssl_handle SSL handle for the client. (analogous to client_socket
     * in non-https counterparts)
     * @param msg Http request
     * @param m mutex for ssl_handle
     */
    void respondClientHttps(SSL *ssl_handle, HttpMessage msg, std::mutex *m);
    /**
     * @brief Reads and parses incoming https requests.
     * @param ssl_handle SSL handle to client connection.
     */
    void serveClientHttps(SSL *ssl_handle);
    /**
     * @brief Starts https server.
     */
    void startHttps();
#endif

    /**
     * @brief Calls generateContent and sends the result to client.
     *
     * This function exists to enable us to parallize the content generation
     * process.
     *
     * @param client_socket Socket file descriptor for the client.
     * @param msg Http request
     * @param m mutex for client_socket
     */
    void respondClient(int client_socket, HttpMessage msg, std::mutex *m);
    /**
     * @brief Reads and parses incoming http requests.
     * @param client_socket File descriptor to client connection.
     */
    void serveClient(int client_socket);
    /**
     * @brief Starts http server.
     */
    void start();
};
