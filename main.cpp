#include <iostream>
#include <climits>
#include <thread>
#include <chrono>
#include <vector>
#include <unordered_map>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

typedef enum HttpMessageType {
    INVALID, GET, POST,
} HttpMessageType;

HttpMessageType to_http_message_type(std::string s) {
    if(s == "GET") return GET;
    if(s == "POST") return POST;
    return INVALID;
}

class HttpMessage {
    HttpMessageType type;
    std::string address;
    std::string protocol_version;
    std::unordered_map<std::string, std::string> headers;
public:
    HttpMessage(std::string message) {
        std::string s = message;
        std::string line = s;
        size_t new_line = message.find('\n');
        if (new_line != std::string::npos)
            line = line.substr(0, new_line);
        
        if (line.find(" ") != std::string::npos) {
            std::string typestr = line.substr(0, line.find(" "));
            this->type = to_http_message_type(typestr);
            line = line.substr(line.find(" ")+1);        
        }

        if (line.find(" ") != std::string::npos) {
            this->address = line.substr(0, line.find(" "));
            line = line.substr(line.find(" ")+1);        
        }
        this->protocol_version = line;
        s = s.substr(new_line+1);

        while (s.find('\n') != std::string::npos) {
            line = s;
            size_t new_line_pos = s.find('\n');
            if (new_line_pos != std::string::npos)
                line = line.substr(0, new_line_pos);
            else
                new_line_pos = line.length()-1;
            size_t delim = line.find(":");
            if (delim != std::string::npos)
                this->headers[line.substr(0, delim)] = line.substr(delim+1);
            if (new_line_pos+1 > s.length()-1) break;
            //std::cout << "\"" << s <<  "\"" << std::endl;
            s = s.substr(new_line_pos+1);
        }
    }
    static std::string OK() {
        std::string content = "Hello World!\n";
        return "HTTP/1.1 200 OK\nContent-Length: "+std::to_string(content.length())+"\nContent-Type: text/plain\n\n"+content;
    }
    void print() {
        std::cout << "Printing Message" << std::endl;
        std::cout << "Type = " << this->type << std::endl;
        std::cout << "Address = " << this->address << std::endl;
        std::cout << "Protocol_version = " << this->protocol_version << std::endl;
    }
};

class Server {
    int socketfd;
    struct sockaddr_in server_address;
    socklen_t address_length;
    std::vector<std::thread> threads;
public:
    Server(size_t port=8000, int backlog=3) {
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
            size_t msg_length = recv(client_socket, NULL, INT_MAX, (MSG_PEEK | MSG_TRUNC));
            if (msg_length < 1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                continue;
            }
            char *message_buffer = (char*)malloc(msg_length*sizeof(char));
            recv(client_socket, message_buffer, msg_length, 0);
            std::string message(message_buffer);
            //HttpMessage httpmsg(message);
            std::string ok_message = HttpMessage::OK();
            send(client_socket, ok_message.c_str(), ok_message.length(), 0);
        }
    }
    void start() {
        while (true) {
            int client_socket = accept(socketfd, (struct sockaddr*)&server_address, &address_length);
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

int main() {
    Server *server = new Server(); 
    server->start();
    return 0;
}
