#include "Configuration.hpp"
#include "Http.hpp"
#include "HttpResponseBuilder.hpp"
#include "Logging.hpp"

#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/resource.h>
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

class Server {
protected:
    Logger &logger;
    Configuration config;

private:
#ifndef HTTPGALLERY_NO_OPENSSL
    BIO *ssl_socket;
    SSL_CTX *ctx = NULL;
#endif
    int socketfd;
    struct sockaddr_in server_address;
    socklen_t address_length;
    std::vector<std::thread> threads;
    std::string htmltemplate_list, htmltemplate_icon, htmltemplate_error;

public:
    /**
     * @brief Gracefully closes the server when it is set to true.
     */
    std::atomic<bool> shouldClose = false;

    /**
     * @brief Initialize a http server with https support.
     * @param logr Reference to the Logger.
     * @param conf Configuration object that is used for permission system.
     */
    Server(Logger &logr, Configuration &&conf);

    ~Server();

    /**
     * @brief Sanitizes the path against path traversal exploit.
     * @param unsanitized_path Path to sanitize.
     * @return Returns true if the path received is canonical (in its shortest
     * form), false otherwise.
     */
    bool isPathCanonical(std::string unsanitized_path);

    /**
     * @brief Tries to get permission by authenticate request to browser.
     *
     * @param msg Http message that has the Authorization header.
     * @param filepath path to the resource that the client is requesting
     * access.
     * @param pt Permission type that client requests.
     * @return Returns empty string when successful.
     */
    std::string negotiateAuth(HttpMessage msg, std::string filepath,
                              enum PermissionType pt);

    /**
     * @brief This function should handle GET requests.
     * @param msg Http request.
     * @return Html data.
     */
    virtual std::string handleGetRequest(HttpMessage msg) = 0;

    /**
     * @brief This function should handle PUT requests.
     *
     * @return Returns the response.
     */
    virtual std::string handlePutRequest(HttpMessage msg) = 0;

    /**
     * @brief This function should handle DELETE requests.
     *
     * @return Returns the response.
     */
    virtual std::string handleDeleteRequest(HttpMessage msg) = 0;

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
