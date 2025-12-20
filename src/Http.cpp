#include "Http.hpp"

HttpMessageType to_http_message_type(std::string s) {
    if(s == "GET") return GET;
    if(s == "POST") return POST;
    return INVALID;
}

HttpMessage::HttpMessage(std::string message) {
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
std::string HttpMessage::respond(std::string content, std::string contentType, int status, uintmax_t range_start, uintmax_t range_end, uintmax_t filesize) {
    std::string content_range = "";
    if (range_end != 0) {
        content_range = "\nContent-Range: bytes ";
        content_range += std::to_string(range_start)+"-"+std::to_string(range_end)+"/"+std::to_string(filesize);
        
    }
    return "HTTP/1.1 "+std::to_string(status)+" OK\nAccept-Ranges: bytes"+content_range+
        "\nContent-Length: "+std::to_string(content.length())+"\nContent-Type: "+contentType+"\n\n"+content;
}

void HttpMessage::print() {
    std::cout << "Printing Message" << std::endl;
    std::cout << "Type = " << this->type << std::endl;
    std::cout << "Address = " << this->address << std::endl;
    std::cout << "Protocol_version = " << this->protocol_version << std::endl;
}

