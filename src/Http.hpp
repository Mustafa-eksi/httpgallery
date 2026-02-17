#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

typedef enum HttpMessageType {
    INVALID,
    GET,
    POST,
    HEAD,
    PUT,
    DELETE
} HttpMessageType;

HttpMessageType to_http_message_type(std::string s);

const char UTF8_ERROR_CHAR = '?';
std::string html_decode(std::string path);
std::string trim_left(std::string s);

/**
 * @brief Http message class for parsing http requests.
 */
class HttpMessage {
public:
    /**
     * @brief Type of the request (eg. GET, POST, HEAD...).
     */
    HttpMessageType type;
    /**
     * @brief Address of the request.
     */
    std::string address;
    /**
     * @brief Http protocol version.
     *
     * Only 1.1 is supported at the moment.
     */
    std::string protocol_version;
    /**
     * @brief Request headers.
     */
    std::unordered_map<std::string, std::string> headers;
    /**
     * @brief Queries in the address. (eg.
     * https://example.com/?this-is-a-query=yes)
     */
    std::unordered_map<std::string, std::string> queries;

    /**
     * @brief Holds request body if it exists.
     */
    std::string content;

    /**
     * @brief Creates empty http message. (Request)
     */
    HttpMessage();
    /**
     * @brief Parses the http request in the string.
     *
     * Queries in the address are stripped and put into queries member. Only
     * queries with values are supported at the moment.
     */
    HttpMessage(std::string message);
    /**
     * @brief Gets the content range of the request if it exists and it is
     * valid.
     *
     * This function may return (0, full_size) in some cases to match browser
     * behaviour.
     *
     * @param full_size File size of the resource requested.
     * @return returns nullopt if the range is invalid and returns the range
     * otherwise.
     */
    std::optional<std::pair<uintmax_t, uintmax_t>>
    getRange(uintmax_t full_size);
    /**
     * @brief Converts queries to string.
     *
     * This function is used for preserving queries while nagivating different
     * pages.
     *
     * @return returns the converted string.
     */
    std::string queriesToString();
    /**
     * @brief Prints the http message to stdout.
     *
     * Use this function only for debug purposes.
     */
    void print();
};

/*
 * @brief Parses multiple HTTP requests that are packed into a single tcp
 * packet.
 * @param unparsed Unparsed string read from the socket.
 * @return Returns a vector containing http messages. If it can't find any, it
 * will return an empty vector.
 */
std::vector<HttpMessage> parseMessages(std::string unparsed);
