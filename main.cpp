#include <iostream>
#include <climits>
#include <thread>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <format>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>

static const std::unordered_map<std::string, std::string> mime_types = {
    // Web Essentials (Updated)
    {"html",  "text/html"},
    {"htm",   "text/html"},
    {"css",   "text/css"},
    {"js",    "text/javascript"},
    {"mjs",   "text/javascript"}, // JavaScript modules
    {"json",  "application/json"},
    {"jsonld","application/ld+json"},
    {"php",   "text/x-php"},

    // Images (Updated)
    {"png",   "image/png"},
    {"jpg",   "image/jpeg"},
    {"jpeg",  "image/jpeg"},
    {"gif",   "image/gif"},
    {"svg",   "image/svg+xml"},
    {"ico",   "image/x-icon"},
    {"webp",  "image/webp"},
    {"avif",  "image/avif"}, // Modern high-efficiency format
    {"tiff",  "image/tiff"},
    {"tif",   "image/tiff"},
    {"bmp",   "image/bmp"},

    // Video & Audio
    {"mp4",   "video/mp4"},
    {"webm",  "video/webm"},
    {"ogv",   "video/ogg"},
    {"mov",   "video/quicktime"},
    {"avi",   "video/x-msvideo"},
    {"mp3",   "audio/mpeg"},
    {"wav",   "audio/wav"},
    {"ogg",   "audio/ogg"},
    {"m4a",   "audio/x-m4a"},
    {"aac",   "audio/aac"},

    // Microsoft Office Documents
    {"doc",   "application/msword"},
    {"docx",  "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {"xls",   "application/vnd.ms-excel"},
    {"xlsx",  "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {"ppt",   "application/vnd.ms-powerpoint"},
    {"pptx",  "application/vnd.openxmlformats-officedocument.presentationml.presentation"},

    // Documents & Data
    {"pdf",   "application/pdf"},
    {"txt",   "text/plain"},
    {"xml",   "application/xml"},
    {"csv",   "text/csv"},
    {"rtf",   "application/rtf"},
    {"md",    "text/markdown"},
    {"yaml",  "text/yaml"},
    {"yml",   "text/yaml"},

    // Fonts
    {"woff",  "font/woff"},
    {"woff2", "font/woff2"},
    {"ttf",   "font/ttf"},
    {"otf",   "font/otf"},
    {"eot",   "application/vnd.ms-fontobject"},

    // Archives & Binary
    {"zip",   "application/zip"},
    {"gz",    "application/gzip"},
    {"rar",   "application/vnd.rar"},
    {"7z",    "application/x-7z-compressed"},
    {"tar",   "application/x-tar"},
    {"bin",   "application/octet-stream"},
    {"exe",   "application/octet-stream"},
    {"dll",   "application/octet-stream"}
};

std::string get_mime_type(std::string name) {
    auto dotpos = name.rfind('.');
    if (dotpos == std::string::npos)
        return "text/html";
    std::string ext = name.substr(dotpos+1);
    if (mime_types.find(ext) == mime_types.end()) return "text/plain";
    return mime_types.at(ext);
}

// FIXME: from gemini:
template<typename... Args>
std::string string_format(const std::string& format, Args... args) {
    // Determine size
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; 
    auto size = static_cast<size_t>(size_s);
    
    std::vector<char> buf(size);
    std::snprintf(buf.data(), size, format.c_str(), args...);
    
    return std::string(buf.data(), buf.data() + size - 1);
}

// FIXME: also from gemini:
std::string read_binary_to_string(const std::string& path) {
    // Open in binary mode!
    std::ifstream file(path, std::ios::binary);
    if (!file) return "";

    // Read the entire file into the string
    return std::string((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
}

std::string read_entire_file(std::string path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) {
        std::cout << "Error: File not found: " << path << std::endl;
        return "";
    }
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    rewind(f);
    char *buff = (char*)malloc(length*sizeof(char)+1);
    fread(buff, sizeof(char), length, f);
    buff[length] = '\0';
    fclose(f);
    std::string result = buff;
    free(buff);
    return result;
}

const auto buttonTemplate = "<a href=\"./%s\">%s</a><br/>";
const auto imageTemplate = "<image class=\"thumbnail-image\" src=\"./%s\"/><br/>";
const auto videoTemplate = "<a href=\"./%s\"><image class=\"thumbnail-image\" src=\"./%s\"/></a><br/>";
const auto ffmpegCommand = "ffmpeg -i %s -ss 00:00:10 -vframes 1 thumbnail-%s.jpg";
std::string list_contents(std::string current_address, std::string path) {
    std::string output = "";
	for (const auto & entry : std::filesystem::directory_iterator(path)){
		try {
			if (!entry.is_regular_file() && !entry.is_directory()) continue;
            auto filepath = std::string(entry.path().c_str());
            auto filename = current_address + "/" + std::string(entry.path().filename().c_str());
            if (get_mime_type(filename).starts_with("image")) {
                output += string_format(imageTemplate, filename.c_str());
            } else {
                output += string_format(buttonTemplate, filename.c_str(), filepath.c_str());
            }
		} catch (std::exception& e) {
            std::cout << "Error: " << entry.path().c_str() << e.what() << std::endl;
		}
	}
    return output;
}

typedef enum HttpMessageType {
    INVALID, GET, POST,
} HttpMessageType;

HttpMessageType to_http_message_type(std::string s) {
    if(s == "GET") return GET;
    if(s == "POST") return POST;
    return INVALID;
}

class HttpMessage {
public:
    HttpMessageType type;
    std::string address;
    std::string protocol_version;
    std::unordered_map<std::string, std::string> headers;
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
    static std::string OK(std::string content = "Hello World!\n", std::string contentType="text/html") {
        return "HTTP/1.1 200 OK\nContent-Length: "+std::to_string(content.length())+"\nContent-Type: "+contentType+"\n\n"+content;
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
    std::string path;
    std::string htmltemplate;
public:
    Server(std::string p=".", size_t port=8000, int backlog=3) {
        this->htmltemplate = read_entire_file("template.html");
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
            size_t msg_length = recv(client_socket, NULL, INT_MAX, (MSG_PEEK | MSG_TRUNC));
            if (msg_length < 1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                continue;
            }
            char *message_buffer = (char*)malloc(msg_length*sizeof(char));
            if (!message_buffer) {
                std::cout << "Error: buy more ram ;)" << std::endl;
                return;
            }
            // receive message
            recv(client_socket, message_buffer, msg_length, 0);
            std::string message(message_buffer);
            // parse message
            HttpMessage httpmsg(message);
            std::string lscontent;
            std::string mimetype = get_mime_type(httpmsg.address);
            if (std::filesystem::is_directory(path + httpmsg.address))
                lscontent = list_contents(httpmsg.address, path + httpmsg.address);
            else if (mimetype.starts_with("text"))
                lscontent = read_entire_file(path + httpmsg.address);
            else
                lscontent = read_binary_to_string(path+httpmsg.address);
            std::string content = lscontent;
            if (mimetype == "text/html")
                content = string_format(this->htmltemplate, lscontent.c_str());
            std::string ok_message = HttpMessage::OK(content, mimetype);
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

int main(int argc, char** argv) {
    std::string path = ".";
    if (argc > 1) path = std::string(argv[1]);
    Server *server = new Server(path); 
    server->start();
    return 0;
}
