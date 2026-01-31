#include "HttpResponseBuilder.hpp"
#include "FileSystemInterface.cpp"
#include "LookupTables.hpp"

HttpResponseBuilder::~HttpResponseBuilder()
{
    content = "";
    content.shrink_to_fit();
}

HttpResponseBuilder HttpResponseBuilder::Status(int s)
{
    status = s;
    return *this;
}

HttpResponseBuilder HttpResponseBuilder::ContentRange(uintmax_t range_start,
                                                      uintmax_t range_end)
{
    headers["Content-Range"]
        = std::to_string(range_start) + "-" + std::to_string(range_end);
    return *this;
}

HttpResponseBuilder HttpResponseBuilder::ContentLength(uintmax_t content_length)
{
    headers["Content-Length"] = std::to_string(content_length);
    return *this;
}

HttpResponseBuilder HttpResponseBuilder::ContentType(std::string mime_type)
{
    headers["Content-Type"] = mime_type;
    return *this;
}

HttpResponseBuilder HttpResponseBuilder::Content(std::string &new_content)
{
    this->content             = std::move(new_content);
    headers["Content-Length"] = std::to_string(this->content.length());
    return *this;
}

HttpResponseBuilder HttpResponseBuilder::CompressContent(std::string encoding)
{
    // TODO: support brotli
    if (encoding.find("gzip") == std::string::npos
        && encoding.find("*") == std::string::npos) {
        return *this;
    }
    if (this->content.empty())
        return *this;
    auto destLen = compressBound(this->content.length());
    std::string destBuffer(destLen, '\0');
    z_stream_s zs = {};
    zs.next_in    = (unsigned char *)this->content.data();
    zs.avail_in   = this->content.length();
    zs.next_out   = (unsigned char *)destBuffer.data();
    zs.avail_out  = destLen;
    deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8,
                 Z_DEFAULT_STRATEGY);
    deflate(&zs, Z_FINISH);
    deflateEnd(&zs);

    // destBuffer might be leaking
    return this->Content(destBuffer).SetHeader("Content-Encoding", "gzip");
}

HttpResponseBuilder HttpResponseBuilder::SetHeader(std::string header,
                                                   std::string value)
{
    this->headers[header] = value;
    return *this;
}

HttpResponseBuilder HttpResponseBuilder::ErrorPage(std::string page_template,
                                                   int error_code)
{
    std::string error_page
        = string_format(page_template, std::to_string(error_code),
                        HTTP_STATUS_MESSAGES.at(error_code));
    return this->Status(error_code)
        .ContentType("text/html; charset=utf-8")
        .Content(error_page);
}

std::string HttpResponseBuilder::build()
{
    std::string response = "";
    response += protocol_version + " " + std::to_string(status) + " "
        + HTTP_STATUS_MESSAGES.at(status) + "\n";
    for (auto [header, value] : headers) {
        response += header + ": " + value + "\n";
    }
    response += "\n";
    response += content;
    content.clear();
    content.shrink_to_fit();
    return response;
}
