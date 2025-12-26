typedef enum PageType {
    DirectoryPage,
    FileDataPage,
} PageType;

class Server {
    int socketfd;
    struct sockaddr_in server_address;
    socklen_t address_length;
    std::vector<std::thread> threads;
    std::string path;
    std::string htmltemplate_list, htmltemplate_icon;

public:
    Server(std::string p=".", size_t port=8000, int backlog=3);
    ~Server();
    PageType choosePageType(HttpMessage httpmsg);
    std::string generateContent(HttpMessage msg);
    void serveClient(int client_socket);
    void start();
};
