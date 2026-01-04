#include "HttpResponseBuilder.cpp"
#include "EmbeddedResources.hpp"

static const unsigned char HTTPGALLERY_SSL_CACHE_ID[] = "HttpGallery";
const int HTTPGALLERY_SSL_CACHE_SIZE = 1024;
const int HTTPGALLERY_SSL_TIMEOUT = 3600;

typedef enum PageType {
    DirectoryPage,
    FileDataPage,
    IconData,
} PageType;

class Server {
    BIO *ssl_socket;
    int socketfd;
    struct sockaddr_in server_address;
    socklen_t address_length;
    std::vector<std::thread> threads;
    std::string path;
    std::string htmltemplate_list, htmltemplate_icon, htmltemplate_error;
    bool shouldClose = false, https;
    Logger &logger;

// OpenSSL
    SSL_CTX *ctx = NULL;
    BIO *acceptor_bio;

public:
    Server(Logger& logr, std::string p, size_t port, std::string cert_path, std::string pkey_path);
    Server(Logger& logr, std::string p=".", size_t port=8000, int backlog=100);
    ~Server();
    PageType choosePageType(HttpMessage httpmsg);
    std::string generateContent(HttpMessage msg);

    void respondClientHttps(SSL* ssl_handle, HttpMessage msg, std::mutex* m);
    void serveClientHttps(SSL *ssl_handle);
    void startHttps();

    void respondClient(int client_socket, HttpMessage msg, std::mutex* m);
    void serveClient(int client_socket);
    void start();
};
