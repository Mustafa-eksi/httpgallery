#include <iostream>
#include <climits>
#include <thread>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <format>
#include <set>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "Http.cpp"
#include "FileSystemInterface.cpp"

class Server {
    int socketfd;
    struct sockaddr_in server_address;
    socklen_t address_length;
    std::vector<std::thread> threads;
    std::string path;
    std::string htmltemplate;
public:
    Server(std::string p=".", size_t port=8000, int backlog=3) {
        this->htmltemplate = read_entire_file("./res/html/template.html");
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
    void serveClient(int client_socket) {
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
            if (httpmsg.address.ends_with("favicon.ico")) return;
            auto is_dir = std::filesystem::is_directory(path + httpmsg.address);
            uintmax_t filesize = 0;
            uintmax_t range_start = 0, range_end = 0;
            if (!is_dir) {
                filesize = std::filesystem::file_size(html_decode(path + httpmsg.address));
                range_end = filesize;
                for (auto [key, val] : httpmsg.headers) {
                    if (key == "Range") {
                        auto important_stuff = val.substr(val.find('=')+1);
                        auto separtorpos = important_stuff.find('-');
                        auto rstart = important_stuff.substr(0, separtorpos);
                        std::cout << important_stuff << std::endl;
                        std::cout << rstart << std::endl;
                        range_start = std::stoi(rstart);
                        if (val.size()-1 > separtorpos+1) {
                            auto rend = important_stuff.substr(separtorpos+1);
                        }
                    }
                }
                const uintmax_t file_limit = 50000000;
                if (range_end-range_start > file_limit) {
                    range_end = range_start + file_limit;
                }
            }
            std::string lscontent;

            std::string mimetype = get_mime_type(httpmsg.address);
            // send different things for different request mime type
            if (is_dir) {
                lscontent = list_contents(httpmsg.address, path + httpmsg.address);
                mimetype = "text/html";
            } else if (mimetype.starts_with("text")) {
                lscontent = read_binary_to_string(path + httpmsg.address, range_start, range_end);
            } else {
                lscontent = read_binary_to_string(path+httpmsg.address, range_start, range_end);
            }

            std::string content = lscontent;
            if (mimetype == "text/html")
                content = string_format(this->htmltemplate, (path+httpmsg.address).c_str(), lscontent.c_str());
            std::string ok_message = HttpMessage::respond(content, mimetype, range_end ? 206 : 200, range_start, range_end, filesize);


            send(client_socket, ok_message.c_str(), ok_message.length(), 0);
        }
    }
    void start() {
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
    ~Server() {
        close(socketfd);
    }
};

int main(int argc, char** argv) {
    std::string path = ".";
    if (argc > 1) path = std::string(argv[1]);
    Server *server = new Server(path); 
    server->start();
    return 0;
}
