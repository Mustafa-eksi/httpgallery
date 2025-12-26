#include "Server.hpp"
#include "HttpResponseBuilder.cpp"

Server::Server(std::string p, size_t port, int backlog) {
    this->htmltemplate = read_binary_to_string("./res/html/template.html");
    this->path = p;
    this->socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == -1) {
        std::cout << "Error: socket" << std::endl;
        return;
    }
    // TODO: this might cause problems
    int temp = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(int)) == -1) {
        std::cout << "Error: setsockopt" << std::endl;
        return;
    }
    server_address = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = (struct in_addr) {
            .s_addr = INADDR_ANY,
        },
        .sin_zero = 0
    };
    address_length = sizeof(server_address);
    if (bind(socketfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        std::cout << "Error: bind" << std::endl;
        return;
    }
    if (listen(socketfd, backlog) < 0) {
        std::cout << "Error: listen" << std::endl;
        return;
    }
}

PageType Server::choosePageType(HttpMessage msg) {
    std::string decoded_path = path + msg.address;
    bool is_dir = std::filesystem::is_directory(decoded_path);
    return is_dir ? DirectoryPage : FileDataPage;
}

std::string Server::generateContent(HttpMessage msg) {
    PageType pt = this->choosePageType(msg);
    if (path.back() == '/') path = path.substr(0, path.length()-1);
    auto filepath = path+msg.address;
    if (pt == FileDataPage) {
        if (!std::filesystem::exists(filepath))
            return HttpResponseBuilder()
                .Status(404)
                .build();
        uintmax_t filesize = std::filesystem::file_size(filepath);
        auto range_opt = msg.getRange(filesize);
        if (!range_opt.has_value()) {
            return HttpResponseBuilder()
                .Status(416)
                .SetHeader("Content-Range", "bytes */"+std::to_string(filesize))
                .build();
        }
        auto [range_start, range_end] = range_opt.value();
        int status = range_end-range_start == filesize ? 200 : 206;
        std::string mimetype = get_mime_type(msg.address);
        std::string file_content;
        if (msg.type == GET)
            file_content = read_binary_to_string(filepath, range_start, range_end);
        else if (msg.type == HEAD)
            file_content = "";
        return HttpResponseBuilder()
            .Status(status)
            .ContentType(mimetype)
            .ContentRange(range_start, range_end)
            .Content(file_content)
            .ContentLength(filesize) // Content already sets it but we override for HEAD
            .build();
    } else if (pt == DirectoryPage) {
        std::string dir_page_contents = list_contents(msg.address, filepath, msg.queriesToString());
        std::string final_content = string_format(this->htmltemplate,
                                filepath.c_str(), dir_page_contents.c_str());
        uintmax_t content_length = final_content.length();
        if (msg.type == HEAD)
            final_content = "";
        return HttpResponseBuilder()
            .Status(200)
            .ContentType("text/html")
            .Content(final_content)
            .ContentLength(content_length) // Same as one in above
            .build();
    } else {
        std::cout << "Error: Invalid page type" << std::endl;
    }
    return "";
}

void Server::serveClient(int client_socket) {
    while (true) {
        intmax_t msg_length = recv(client_socket, NULL, INT_MAX, (MSG_PEEK | MSG_TRUNC));
        if (msg_length < 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            continue;
        }
        char *message_buffer = (char*)malloc(msg_length*sizeof(char));
        if (!message_buffer) {
            std::cout << "Error: failed to allocate " << msg_length << " bytes" << std::endl;
            return;
        }
        // receive message
        recv(client_socket, message_buffer, msg_length, 0);
        std::string message(message_buffer);
        // parse message
        HttpMessage httpmsg(message);
        // FIXME: ignores favicon requests
        if (httpmsg.address.ends_with("favicon.ico")) return;
        
        std::string response = generateContent(httpmsg);
        send(client_socket, response.c_str(), response.length(), 0);
    }
}

void Server::start() {
    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int client_socket = accept(socketfd, (struct sockaddr*)&clientAddr, &clientLen);
        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);

        // Simple check: Does the IP start with "192.168."?
        std::string ipStr(clientIp);
        if (!ipStr.starts_with("10.42.") && ipStr != "127.0.0.1") {
            close(client_socket); // Reject connection
        }
        if (client_socket < 0) {
            std::cout << "Error: serveClient->accept" << std::endl;
            return;
        }
        threads.push_back(std::thread(&Server::serveClient, this, client_socket));
    }
}

Server::~Server() {
    close(socketfd);
}
