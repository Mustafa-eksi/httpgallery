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

std::set<char> chars_to_escape = {
    '!',  // history expansion
    '"',  // shell syntax
    '#',  // comment start / zsh wildcards
    '$',  // shell syntax
    '&',  // shell syntax
    '\'', // shell syntax (escaped for C++)
    '(',  // ksh extended globs / zsh wildcards
    ')',  // shell syntax
    '*',  // sh wildcard
    ',',  // brace expansion
    ';',  // shell syntax
    '<',  // shell syntax
    '=',  // assignment / zsh PATH lookup
    '>',  // shell syntax
    '?',  // sh wildcard
    '[',  // sh wildcard
    '\\', // shell syntax (escaped backslash for C++)
    ']',  // wildcard/globbing
    '^',  // history expansion / zsh wildcard
    '`',  // shell syntax
    '{',  // brace expansion
    '|',  // shell syntax
    '}',  // brace expansion / zsh logic
    '~'   // home directory expansion
};

unsigned long long get_binary_size(const std::string unescaped_path) {
    auto path = unescaped_path;
    std::string target = "%20";
    std::string replacement = " ";
    size_t pos = 0;

    // Search for the target substring starting from 'pos'
    while ((pos = path.find(target, pos)) != std::string::npos) {
        // replace(starting_index, length_to_remove, new_pathing)
        path.replace(pos, target.length(), replacement);
        
        // Move pos forward by the length of the replacement 
        // to avoid infinite loops or re-scanning
        pos += replacement.length();
    }
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cout << "ERROR!: " << path << std::endl;
        return 0;
    }

    // 1. Move the file pointer to the end of the file
    file.seekg(0, std::ios::end);

    // 2. Get the current position (which is the total size in bytes)
    std::streampos size = file.tellg();

    // 3. Close the file and return the size as a string
    file.close();
    return size;
}

std::string path_escape(std::string path) {
    std::string target = "%20";
    std::string replacement = " ";
    size_t pos = 0;

    // Search for the target substring starting from 'pos'
    while ((pos = path.find(target, pos)) != std::string::npos) {
        // replace(starting_index, length_to_remove, new_pathing)
        path.replace(pos, target.length(), replacement);
        
        // Move pos forward by the length of the replacement 
        // to avoid infinite loops or re-scanning
        pos += replacement.length();
    }
    return path;
}

// FIXME: also from gemini:
std::string read_binary_to_string(const std::string unescaped_path, unsigned long long range_start, unsigned long long range_end) {
    // Open in binary mode!
    if (range_end < range_start) return "error: range";
    auto path = path_escape(unescaped_path);

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cout << "ERROR!: " << path << std::endl;
        return "Error";
    }
    file.seekg(range_start);
    std::string strbuff(range_end-range_start, '\0');
    std::cout << "range_start: " << range_start << ", range_end: " << range_end << std::endl;
    if (!file.read(&strbuff[0], range_end-range_start))
        return "error: read";
    return strbuff;
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

const auto buttonTemplate = 
            "<div class=\"item\">"
                "<a class=\"item-name\" href=\"%s\">%s</a>"
            "</div>";
const auto imageTemplate =
            "<div class=\"item\">"
                "<img class=\"item-thumbnail\" src=\"%s\">"
                "<a class=\"item-name\" href=\"%s\">%s</a>"
            "</div>";
const auto videoTemplate =
            "<div class=\"item\">"
                "<video width=\"100%\" height=\"95%\" class=\"item-thumbnail\" src=\"%s\" controls loop>brrp</video>"
                "<a class=\"item-name\" href=\"%s\">%s</a>"
            "</div>";
const auto ffmpegCommand = "ffmpeg -i %s -ss 00:00:10 -vframes 1 thumbnail-%s.jpg";
std::string list_contents(std::string current_address, std::string path) {
    std::string output = "";
	for (const auto & entry : std::filesystem::directory_iterator(path)){
		try {
			if (!entry.is_regular_file() && !entry.is_directory()) continue;
            if (current_address.back() != '/')
                current_address += '/';
            auto filename = std::string(entry.path().filename());
            auto filepath = current_address + filename;
            if (get_mime_type(filename).starts_with("image")) {
                output += string_format(imageTemplate, filepath.c_str(), filepath.c_str(), filename.c_str());
            } else if (get_mime_type(filename).starts_with("video") && entry.file_size() < 5e+7) {
                output += string_format(videoTemplate, filepath.c_str(), filepath.c_str(), filename.c_str());
            } else {
                output += string_format(buttonTemplate, filepath.c_str(), filename.c_str());
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
    static std::string respond(std::string content = "Hello World!\n", std::string contentType="text/html", int status=200, uintmax_t range_start=0, uintmax_t range_end=0, uintmax_t filesize=0) {
        std::string content_range = "";
        if (range_end != 0) {
            content_range = "\nContent-Range: bytes ";
            content_range += std::to_string(range_start)+"-"+std::to_string(range_end)+"/"+std::to_string(filesize);
            
        }
        return "HTTP/1.1 "+std::to_string(status)+" OK\nAccept-Ranges: bytes"+content_range+
            "\nContent-Length: "+std::to_string(content.length())+"\nContent-Type: "+contentType+"\n\n"+content;
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
            if (httpmsg.address.ends_with("favicon.ico")) return;
            auto is_dir = std::filesystem::is_directory(path + httpmsg.address);
            uintmax_t filesize = 0;
            uintmax_t range_start = 0, range_end = 0;
            if (!is_dir) {
                filesize = std::filesystem::file_size(path_escape(path + httpmsg.address));
                range_end = filesize;
                for (auto [key, val] : httpmsg.headers) {
                    if (key == "Range") {
                        auto important_stuff = val.substr(val.find('=')+1);
                        auto separtorpos = important_stuff.find('-');
                        auto rstart = important_stuff.substr(0, separtorpos);
                        std::cout << "Range parsing" << std::endl;
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
            std::cout << "filesize: " << filesize << std::endl;
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
