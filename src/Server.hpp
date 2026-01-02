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
    struct sockaddr_in server_address;
    socklen_t address_length;
    std::vector<std::thread> threads;
    std::string path;
    std::string htmltemplate_list, htmltemplate_icon, htmltemplate_error;
    bool shouldClose = false;
    Logger &logger;

// OpenSSL
    SSL_CTX *ctx = NULL;
    BIO *acceptor_bio;

public:
    Server(Logger& logr, std::string p=".", size_t port=8000, std::string cert_path="chain.pem", std::string pkey_path="pkey.pem");
    ~Server();
    PageType choosePageType(HttpMessage httpmsg);
    std::string generateContent(HttpMessage msg);
    void respondClient(SSL* ssl_handle, HttpMessage msg, std::mutex* m);
    void serveClient(SSL *ssl_handle);
    void start();
};
