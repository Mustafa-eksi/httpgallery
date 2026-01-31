#include <cstdint>
#include <string.h>
#include <string>
#include <unordered_map>
#include <zlib.h>

/**
 * @brief Builds Http responses with chainable methods.
 */
class HttpResponseBuilder {
    // TODO: Maybe support other protocol versions
    std::string protocol_version                         = "HTTP/1.1";
    int status                                           = 200;
    std::unordered_map<std::string, std::string> headers = {
        { "Accept-Ranges", "bytes" },
        { "Keep-Alive", "timeout=5, max=200" },
        { "Connection", "Keep-Alive" },
    };
    std::string content;

public:
    ~HttpResponseBuilder();
    /**
     * @brief Sets the status.
     */
    HttpResponseBuilder Status(int s);
    /**
     * @brief Sets the content range.
     *
     * Content ranges must be in the range of 0..file_size.
     */
    HttpResponseBuilder ContentRange(uintmax_t range_start,
                                     uintmax_t range_end);
    /**
     * @brief Sets the content length.
     */
    HttpResponseBuilder ContentLength(uintmax_t content_length);
    /**
     * @brief Sets the content type.
     */
    HttpResponseBuilder ContentType(std::string mime_type);
    /**
     * @brief Sets the content and the content length.
     */
    HttpResponseBuilder Content(std::string &content);
    /**
     * @brief Compresses the content with gzip.
     */
    HttpResponseBuilder CompressContent(std::string encoding);
    /**
     * @brief Sets custom header.
     */
    HttpResponseBuilder SetHeader(std::string header, std::string value);
    /**
     * @brief Generate a nice error page.
     *
     * @param page_template Error page template, this template must include two
     * parameters ('{}').
     * @param status Error status code.
     */
    HttpResponseBuilder ErrorPage(std::string page_template, int status);
    /**
     * @brief builds the http response.
     */
    std::string build();
};
