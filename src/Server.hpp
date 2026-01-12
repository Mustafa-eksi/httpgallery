#include "EmbeddedResources.hpp"
#include "HttpResponseBuilder.cpp"
#include <atomic>
#include <sys/resource.h>

static const unsigned char HTTPGALLERY_SSL_CACHE_ID[] = "HttpGallery";
const int HTTPGALLERY_SSL_CACHE_SIZE                  = 1024;
const int HTTPGALLERY_SSL_TIMEOUT                     = 3600;

typedef enum PageType {
    DirectoryPage,
    FileDataPage,
    IconData,
} PageType;

class Server {
#ifndef HTTPGALLERY_NO_OPENSSL
    BIO *ssl_socket;
    SSL_CTX *ctx = NULL;
#endif
#ifndef HTTPGALLERY_EMBED_RESOURCES
    std::string directory_icon_data, video_icon_data, text_icon_data;
#endif
    int socketfd;
    struct sockaddr_in server_address;
    socklen_t address_length;
    std::vector<std::thread> threads;
    std::string path;
    std::string htmltemplate_list, htmltemplate_icon, htmltemplate_error;
    bool https;
    Logger &logger;

public:
    std::atomic<bool> shouldClose = false;
    Server(Logger &logr, std::string p, size_t port, std::string cert_path,
           std::string pkey_path);
    Server(Logger &logr, std::string p = ".", size_t port = 8000,
           int backlog = 100);
    ~Server();
    PageType choosePageType(HttpMessage httpmsg);
    std::string generateContent(HttpMessage msg);

#ifndef HTTPGALLERY_NO_OPENSSL
    void respondClientHttps(SSL *ssl_handle, HttpMessage msg, std::mutex *m);
    void serveClientHttps(SSL *ssl_handle);
    void startHttps();
#endif

    void respondClient(int client_socket, HttpMessage msg, std::mutex *m);
    void serveClient(int client_socket);
    void start();
};
