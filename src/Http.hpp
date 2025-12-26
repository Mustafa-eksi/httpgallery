#include "LookupTables.hpp"

typedef enum HttpMessageType {
    INVALID, GET, POST, HEAD
} HttpMessageType;

HttpMessageType to_http_message_type(std::string s);
std::string html_decode(std::string path);
std::string trim_left(std::string s);

class HttpMessage {
public:
    HttpMessageType type;
    std::string address;
    std::string protocol_version;
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> queries;

    HttpMessage();
    HttpMessage(std::string message);
    std::optional<std::pair<uintmax_t, uintmax_t>> getRange(uintmax_t full_size);
    void print();
};
