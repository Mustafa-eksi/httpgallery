#include "HttpResponseBuilder.hpp"

/*
 * 
    HttpResponseBuilder status(int s);
    HttpResponseBuilder ContentRange(uintmax_t range_start, uintmax_t range_end);
    HttpResponseBuilder ContentLength(uintmax_t content_length);
    HttpResponseBuilder ContentType(std::string mime_type);
    HttpResponseBuilder Content(std::string content);
    std::string build();
 * */

HttpResponseBuilder HttpResponseBuilder::Status(int s) {
    status = s;
    return *this;
}

HttpResponseBuilder HttpResponseBuilder::ContentRange(uintmax_t range_start, uintmax_t range_end) {
    headers["Content-Range"] = std::to_string(range_start)+"-"+std::to_string(range_end);
    return *this;
}

HttpResponseBuilder HttpResponseBuilder::ContentLength(uintmax_t content_length) {
    headers["Content-Length"] = std::to_string(content_length);
    return *this;
}

HttpResponseBuilder HttpResponseBuilder::ContentType(std::string mime_type) {
    headers["Content-Type"] = mime_type;
    return *this;
}

HttpResponseBuilder HttpResponseBuilder::Content(std::string new_content) {
    this->content = new_content;
    headers["Content-Length"] = std::to_string(content.length());
    return *this;
}

std::string HttpResponseBuilder::build() {
    std::string response = "";
    response += protocol_version + " " + std::to_string(status) + " OK\n";
    for (auto [header, value] : headers) {
        response += header +": "+ value + "\n";
    }
    response += "\n";
    response += content;
    return response;
}
