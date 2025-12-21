typedef enum HttpMessageType {
    INVALID, GET, POST,
} HttpMessageType;

HttpMessageType to_http_message_type(std::string s);

class HttpMessage {
public:
    HttpMessageType type;
    std::string address;
    std::string protocol_version;
    std::unordered_map<std::string, std::string> headers;

    HttpMessage(std::string message);
    std::optional<std::pair<uintmax_t, uintmax_t>> getRange(uintmax_t full_size);
    static std::string respond(std::string content = "Hello World!\n", std::string contentType="text/html", int status=200, uintmax_t range_start=0, uintmax_t range_end=0, uintmax_t filesize=0);
    void print();
};
