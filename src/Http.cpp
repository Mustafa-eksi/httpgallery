#include "Http.hpp"
#include <stdio.h>

HttpMessageType to_http_message_type(std::string s)
{
    if (s == "GET")
        return GET;
    if (s == "POST")
        return POST;
    return INVALID;
}

// FIXME: write testing for it
std::string html_decode(std::string path)
{
    size_t pos = 0;
    while ((pos = path.find('%', pos)) != std::string::npos) {
        if (pos + 3 > path.length()) {
            pos++;
            continue;
        }
        auto before    = path.substr(0, pos);
        auto after     = path.substr(pos + 3);
        char utf8_part = UTF8_ERROR_CHAR;
        try {
            utf8_part = (char)std::stoi(path.substr(pos + 1, 2), nullptr, 16);
        } catch (std::invalid_argument &e) {
            pos++;
            continue;
        }
        before += utf8_part;
        path = before + after;
        pos++;
    }
    return path;
}

std::string trim_left(std::string s)
{
    while (s.length() > 0 && s.front() == ' ')
        s = s.substr(1);
    return s;
}

HttpMessage::HttpMessage() { }

HttpMessage::HttpMessage(std::string message)
{
    this->type       = INVALID;
    std::string s    = message;
    std::string line = s;
    auto new_line    = message.find('\r');
    if (new_line == std::string::npos)
        return;
    line = line.substr(0, new_line);

    if (line.find(" ") == std::string::npos)
        return;
    std::string typestr = line.substr(0, line.find(" "));
    this->type          = to_http_message_type(typestr);
    line                = line.substr(line.find(" ") + 1);
    this->address       = html_decode(line.substr(0, line.find(" ")));
    // FIXME: there can be question marks in the file name
    auto query_delim = this->address.rfind('?');
    if (query_delim != std::string::npos) {
        std::string query_string = this->address.substr(query_delim + 1);
        this->address            = this->address.substr(0, query_delim);

        while (!query_string.empty()) {
            auto and_pos   = query_string.find('&');
            auto equal_pos = query_string.find('=');
            if (equal_pos == std::string::npos)
                break;
            auto key_str = query_string.substr(0, equal_pos);
            auto val_str
                = query_string.substr(equal_pos + 1, and_pos - equal_pos - 1);
            this->queries[key_str] = val_str;
            if (and_pos == std::string::npos
                || query_string.length() - 1 < and_pos + 1)
                break;
            query_string = query_string.substr(and_pos + 1);
        }
    }

    if (line.find(" ") == std::string::npos)
        return;
    line                   = line.substr(line.find(" ") + 1);
    this->protocol_version = line;
    s                      = s.substr(new_line + 2);

    while (s.length() != 0) {
        line                = s;
        size_t new_line_pos = s.find('\r');
        if (new_line_pos != std::string::npos)
            line = line.substr(0, new_line_pos);
        else
            new_line_pos = line.length() - 1;
        size_t delim = line.find(":");

        if (delim != std::string::npos)
            this->headers[line.substr(0, delim)]
                = trim_left(line.substr(delim + 1));

        if (new_line_pos + 2 > s.length() - 1)
            break;
        // std::cout << "\"" << s <<  "\"" << std::endl;
        s = s.substr(new_line_pos + 2);
    }
}

// https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Headers/Range
std::optional<std::pair<uintmax_t, uintmax_t>>
HttpMessage::getRange(uintmax_t full_size)
{
    if (!headers.contains("Range"))
        return std::make_pair(0, full_size);
    // Example structure (note that there are a few more possibilities)
    // FIXME: Make this work for multiple ranges
    // Range: <unit>=<range-start>-<range-end>
    std::string header_val = headers["Range"];
    auto unit_sep          = header_val.find('=');
    if (unit_sep == std::string::npos)
        return std::nullopt;
    std::string units = header_val.substr(0, unit_sep);
    // currently only unit type that is specified in the standards is bytes
    if (units != "bytes")
        return std::nullopt;
    std::string after_units = header_val.substr(unit_sep + 1);

    uintmax_t range_start = 0, range_end = 0;
    auto separator_pos = after_units.find('-');
    if (separator_pos == std::string::npos)
        return std::nullopt;

    if (separator_pos == 0) {
        range_start = 0;
        // TODO: take last <range-end> bytes.
    } else {
        // TODO: factor this out.
        std::string range_start_str = after_units.substr(0, separator_pos);
        try {
            range_start = std::stoi(range_start_str);
        } catch (std::invalid_argument &e) {
            return std::nullopt;
        } catch (std::out_of_range &e) {
            return std::nullopt;
        }
    }
    if (separator_pos == after_units.length() - 1) {
        range_end = full_size;
    } else {
        printf("'%s'\n", after_units.c_str());
        std::string range_end_str = after_units.substr(separator_pos + 1);
        try {
            range_end = std::stoi(range_end_str);
        } catch (std::invalid_argument &e) {
            return std::nullopt;
        } catch (std::out_of_range &e) {
            return std::nullopt;
        }
    }
    if (range_start > range_end) {
        // This should've actually be a 416 status response but imgur send the
        // full file with 200 instead. So...
        range_start = 0;
        range_end   = full_size;
    }
    if (range_end - range_start > full_size || range_start >= full_size)
        return std::nullopt;
    if (range_end > full_size) {
        range_end = full_size;
    }
    return std::make_pair(range_start, range_end);
}

std::string HttpMessage::queriesToString()
{
    std::string res = "?";
    for (auto [key, val] : this->queries) {
        res += key + "=" + val + "&";
    }
    return res.substr(0, res.length() - 1);
}

void HttpMessage::print()
{
    std::cout << "Printing Message" << std::endl;
    std::cout << "Type = " << this->type << std::endl;
    std::cout << "Address = " << this->address << std::endl;
    std::cout << "Protocol_version = " << this->protocol_version << std::endl;
    for (auto [header, val] : this->headers) {
        std::cout << header << ": " << val << std::endl;
    }
}
