
class HttpResponseBuilder {
    // TODO: Maybe support other protocol versions
    std::string protocol_version = "HTTP/1.1";
    int status = 200;
    std::unordered_map<std::string, std::string> headers = {
        {"Accept-Ranges", "bytes"},
    };
    std::string content;
public:
    ~HttpResponseBuilder();
    HttpResponseBuilder Status(int s);
    HttpResponseBuilder ContentRange(uintmax_t range_start, uintmax_t range_end);
    HttpResponseBuilder ContentLength(uintmax_t content_length);
    HttpResponseBuilder ContentType(std::string mime_type);
    HttpResponseBuilder Content(std::string& content);
    HttpResponseBuilder SetHeader(std::string header, std::string value);
    std::string build();
};
