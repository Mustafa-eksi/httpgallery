#include "Http.hpp"

HttpMessageType to_http_message_type(std::string s) {
    if(s == "GET") return GET;
    if(s == "POST") return POST;
    return INVALID;
}

std::string html_decode(std::string path) {
    size_t pos = 0;
    while ((pos = path.find('%', pos)) != std::string::npos) {
        // FIXME: takes 2 char encoded string into consideration
        path.replace(pos, 3, HtmlEncodeTable.at(path.substr(pos, 3)));
    }
    return path;
}

HttpMessage::HttpMessage() {}

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
        this->address = html_decode(line.substr(0, line.find(" ")));
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

// https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Headers/Range
std::optional<std::pair<uintmax_t, uintmax_t>> HttpMessage::getRange(uintmax_t full_size) {
    if (!headers.contains("Range")) return std::nullopt;
    // Example structure (note that there are a few more possibilities)
    // FIXME: Make this work for multiple ranges
    // Range: <unit>=<range-start>-<range-end>
    std::string header_val = headers["Range"];
    auto unit_sep = header_val.find('=');
    if (unit_sep == std::string::npos) return std::nullopt;
    std::string units = header_val.substr(0, unit_sep);
    // currently only unit type that is specified in the standards is bytes
    if (units != "bytes") return std::nullopt;
    std::string after_units = header_val.substr(unit_sep+1);

    uintmax_t range_start=0, range_end=0;
    auto separator_pos = after_units.find('-');
    if (separator_pos == std::string::npos) {
        std::cout << "Error: html request not fits the standards "
            << after_units << std::endl;
        return std::nullopt;
    }

    if (separator_pos == 0) {
        range_start = 0;
        // TODO: take last <range-end> bytes.
    } else {
        // TODO: factor this out.
        std::string range_start_str = after_units.substr(0, separator_pos);
        try {
            range_start = std::stoi(range_start_str);
        } catch (std::invalid_argument& e) {
            std::cout << "Error: HttpMessage::getRange() Stoi Invalid Argument: "
                << range_start_str << std::endl;
            return std::nullopt;
        } catch (std::out_of_range& e) {
            std::cout << "Error: HttpMessage::getRange() Stoi Out of Range: "
                << range_start_str << std::endl;
            return std::nullopt;
        }
    }
    if (separator_pos == after_units.length()-1) {
        range_end = full_size;
    } else {
        std::string range_end_str = after_units.substr(separator_pos+1);
        try {
            range_end = std::stoi(range_end_str);
        } catch (std::invalid_argument& e) {
            std::cout << "Error: HttpMessage::getRange() Stoi Invalid Argument: "
                << range_end_str << std::endl;
            return std::nullopt;
        } catch (std::out_of_range& e) {
            std::cout << "Error: HttpMessage::getRange() Stoi Out of Range: "
                << range_end_str << std::endl;
            return std::nullopt;
        }
    }
    if (range_start > range_end) {
        // This should've actually be a 416 status response but imgur send the
        // full file with 200 instead. So...
        range_start = 0;
        range_end = full_size;
    }
    if (range_end-range_start > full_size || range_start >= full_size)
        return std::nullopt;
    if (range_end > full_size) {
        range_end = full_size;
    }
    return std::make_pair(range_start, range_end);
}

void HttpMessage::print() {
    std::cout << "Printing Message" << std::endl;
    std::cout << "Type = " << this->type << std::endl;
    std::cout << "Address = " << this->address << std::endl;
    std::cout << "Protocol_version = " << this->protocol_version << std::endl;
}

