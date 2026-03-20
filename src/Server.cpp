#include "Server.hpp"

#ifndef HTTPGALLERY_NO_OPENSSL
Server::Server(Logger &logr, Configuration &&conf)
    : logger(logr)
    , config(std::move(conf))
{
    if (config.configBool("UseHttps")) {
        // Setting up OpenSSL
        ctx = SSL_CTX_new(TLS_server_method());
        if (!ctx) {
            ERR_print_errors_fp(stderr);
            logger.report("ERROR", "Failed to create server ssl context");
            return;
        }

        if (!SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION)) {
            SSL_CTX_free(ctx);
            ERR_print_errors_fp(stderr);
            logger.report("ERROR", "Failed to set the minimum TLS version");
            return;
        }
        long opts = SSL_OP_IGNORE_UNEXPECTED_EOF | SSL_OP_NO_RENEGOTIATION
            | SSL_OP_CIPHER_SERVER_PREFERENCE;
        SSL_CTX_set_options(ctx, opts);

        if (SSL_CTX_use_certificate_chain_file(
                ctx, config.configString("CertificationPath").c_str())
            <= 0) {
            SSL_CTX_free(ctx);
            ERR_print_errors_fp(stderr);
            logger.report("ERROR", "Failed to load the certificate");
            return;
        }

        if (SSL_CTX_use_PrivateKey_file(
                ctx, config.configString("PrivateKeyPath").c_str(),
                SSL_FILETYPE_PEM)
            <= 0) {
            SSL_CTX_free(ctx);
            ERR_print_errors_fp(stderr);
            logger.report("ERROR", "Failed to load the private key");
            return;
        }

        SSL_CTX_set_session_id_context(ctx, HTTPGALLERY_SSL_CACHE_ID,
                                       sizeof(HTTPGALLERY_SSL_CACHE_ID));
        SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_SERVER);
        SSL_CTX_sess_set_cache_size(ctx, HTTPGALLERY_SSL_CACHE_SIZE);
        SSL_CTX_set_timeout(ctx, HTTPGALLERY_SSL_TIMEOUT);

        // Disable authentication of the client
        SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

        // FIXME: this is a hack, change this later
        auto portstr     = std::to_string(config.configInt("Port"));
        this->ssl_socket = BIO_new_accept(portstr.c_str());
        if (!this->ssl_socket) {
            SSL_CTX_free(ctx);
            ERR_print_errors_fp(stderr);
            logger.report("ERROR", "Failed to create a ssl socket");
            return;
        }

        BIO_set_bind_mode(ssl_socket, BIO_BIND_REUSEADDR);
        // First BIO_do_accept call doesn't accept a connection. It initiates
        // the BIO acceptor.
        if (BIO_do_accept(this->ssl_socket) <= 0) {
            SSL_CTX_free(ctx);
            ERR_print_errors_fp(stderr);
            logger.report("ERROR", "Failed to set up bio acceptor socket");
            return;
        }
    } else {
        this->socketfd = socket(AF_INET, SOCK_STREAM, 0);
        if (socketfd == -1) {
            logger.report("ERROR", "Socket");
            return;
        }
        // TODO: this might cause problems
        int temp = 1;
        if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(int))
            == -1) {
            logger.report("ERROR", "setsockopt");
            return;
        }

        int status = fcntl(socketfd, F_SETFL,
                           fcntl(socketfd, F_GETFL, 0) | O_NONBLOCK);
        if (status == -1) {
            logger.report("ERROR", "fcntl failed");
            return;
        }

        server_address = (struct sockaddr_in){
            .sin_family = AF_INET,
            .sin_port = htons(config.configInt("Port")),
            .sin_addr = (struct in_addr) {
                .s_addr = htonl(INADDR_ANY),
            },
            .sin_zero = {}
        };
        address_length = sizeof(server_address);
        if (bind(socketfd, (struct sockaddr *)&server_address,
                 sizeof(server_address))
            < 0) {
            logger.report("ERROR", "bind");
            return;
        }
        if (listen(socketfd, config.configInt("Backlog")) < 0) {
            logger.report("ERROR", "listen");
            return;
        }
    }
}

void Server::respondClientHttps(SSL *ssl_handle, HttpMessage msg, std::mutex *m)
{
    std::string response;
    switch (msg.type) {
    case GET:
        response = handleGetRequest(msg);
        break;
    case PUT:
        response = handlePutRequest(msg);
        break;
    case DELETE:
        response = handleDeleteRequest(msg);
        break;
    default:
        logger.report("ERROR", "Operation not supported");
        break;
    }
    m->lock();
    if (SSL_write(ssl_handle, response.c_str(), response.length()) <= 0) {
        logger.report("ERROR", "Failed to write to ssl socket");
    }
    m->unlock();
    response.clear();
    response.shrink_to_fit();
}

void Server::serveClientHttps(SSL *ssl_handle)
{
    std::vector<std::thread> client_threads;
    std::mutex m;

    while (!this->shouldClose) {
        char message_buffer[8192] = { '\0' };
        // receive message
        SSL_read(ssl_handle, message_buffer, sizeof(message_buffer));
        std::string message(message_buffer);
        // parse message
        HttpMessage httpmsg = HttpMessage(message);
        message             = "";
        message.shrink_to_fit();
        if (httpmsg.type == INVALID)
            continue;

        // logger.info("responding client: " + httpmsg.address);
        client_threads.push_back(std::thread(&Server::respondClientHttps, this,
                                             ssl_handle, httpmsg, &m));
    }
    for (auto &t : client_threads)
        t.join();
    client_threads.clear();
    client_threads.shrink_to_fit();
    if (ssl_handle) {
        SSL_shutdown(ssl_handle);
        SSL_free(ssl_handle);
    }
    return;
}

void Server::startHttps()
{
    if (this->ctx == NULL) {
        logger.report("ERROR", "SSL context is NULL, exiting");
        return;
    }
    while (!this->shouldClose) {
        // Clears ssl error stack
        ERR_clear_error();

        if (BIO_do_accept(ssl_socket) <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            continue;
        }

        BIO *client_socket = BIO_pop(ssl_socket);
        if (!client_socket) {
            ERR_print_errors_fp(stderr);
            logger.report("ERROR", "Failed to retrieve the client socket");
            continue;
        }

        SSL *ssl_handle = SSL_new(ctx);
        if (!ssl_handle) {
            BIO_free(client_socket);
            ERR_print_errors_fp(stderr);
            logger.report("ERROR", "Failed to create new ssl handle");
            continue;
        }

        // We write client_socket twice since it is both where we will read
        // from and write to.
        SSL_set_bio(ssl_handle, client_socket, client_socket);

        if (SSL_accept(ssl_handle) <= 0) {
            BIO_free(client_socket);
            // ERR_print_errors_fp(stderr);
            // logger.report("ERROR", "Failed to make a TLS handshake");
            continue;
        }

        threads.push_back(
            std::thread(&Server::serveClientHttps, this, ssl_handle));
    }
}
#endif // HTTPGALLERY_NO_OPENSSL

bool Server::isPathCanonical(std::string unsanitized_path)
{
    try {
        std::string canon = std::filesystem::canonical(unsanitized_path);
        if (canon.back() != '/')
            canon += "/";
        std::string absol = std::filesystem::absolute(unsanitized_path);
        if (absol.back() != '/')
            absol += "/";
        return canon == absol;
    } catch (std::filesystem::filesystem_error &e) {
        return false;
    }
}

std::string Server::negotiateAuth(HttpMessage msg, std::string filepath,
                                  enum PermissionType pt)
{
    if (config.askPermission(filepath, "guest", pt))
        return "";
    if (!msg.headers.contains("Authorization"))
        return HttpResponseBuilder()
            .ErrorPage(htmltemplate_error, 401)
            .SetHeader("WWW-Authenticate", "Basic realm=\"Protected\"")
            .build();

    auto auth  = msg.headers["Authorization"];
    auto delim = auth.find(" ");
    if (delim == std::string::npos)
        return HttpResponseBuilder().ErrorPage(htmltemplate_error, 401).build();
    // TODO: support more alternative authentication methods.
    if (auth.substr(0, delim) != "Basic")
        return HttpResponseBuilder().ErrorPage(htmltemplate_error, 401).build();
    auto userpass = auth.substr(delim + 1);
    // FIXME: control the permission of the user
    auto auth_res = config.authenticate(userpass);
    if (!auth_res.first)
        return HttpResponseBuilder()
            .SetHeader("WWW-Authenticate", "Basic realm=\"Protected\"")
            .ErrorPage(htmltemplate_error, 401)
            .build();
    if (config.askPermission(filepath, auth_res.second, pt))
        return "";
    else
        return HttpResponseBuilder()
            .ErrorPage(htmltemplate_error, 401)
            .SetHeader("WWW-Authenticate", "Basic realm=\"Protected\"")
            .build();
}

void Server::respondClient(int client_socket, HttpMessage msg, std::mutex *m)
{
    std::string response;
    switch (msg.type) {
    case GET:
        response = handleGetRequest(msg);
        break;
    case PUT:
        response = handlePutRequest(msg);
        break;
    case DELETE:
        response = handleDeleteRequest(msg);
        break;
    default:
        logger.report("ERROR", "Operation not supported");
        break;
    }
    m->lock();
    if (send(client_socket, response.c_str(), response.length(), MSG_NOSIGNAL)
        == -1) {
        if (errno == EMSGSIZE) {
            auto errmsg = HttpResponseBuilder()
                              .ErrorPage(htmltemplate_error, 413)
                              .build();
            send(client_socket, errmsg.c_str(), errmsg.length(), MSG_NOSIGNAL);
        } else if (errno == EPIPE) {
            // TODO: close connection
        }
    }
    logger.changeMetric("HTTP Responses", 1);
    m->unlock();
    response.clear();
    response.shrink_to_fit();
}

void Server::serveClient(int client_socket)
{
    std::vector<std::thread> client_threads;
    std::mutex m;
    while (!this->shouldClose) {
        intmax_t msg_length
            = recv(client_socket, NULL, INT_MAX, (MSG_PEEK | MSG_TRUNC));
        if (msg_length < 1) {
            break;
        }
        char *message_buffer = (char *)malloc(msg_length * sizeof(char) + 1);
        if (!message_buffer) {
            logger.report("ERROR", "Memory allocation failed");
            return;
        }
        memset(message_buffer, '\0', msg_length * sizeof(char) + 1);
        if (!message_buffer) {
            // FIXME: make logger variadic like string_format
            logger.report("ERROR",
                          "failed to allocate " + std::to_string(msg_length)
                              + " bytes");
            return;
        }
        // receive message
        recv(client_socket, message_buffer, msg_length, 0);
        std::string message(message_buffer);
        free(message_buffer);
        // parse message
        std::vector<HttpMessage> parsed_messages = parseMessages(message);
        for (auto &httpmsg : parsed_messages) {
            if (httpmsg.address.empty()) {
                auto response = HttpResponseBuilder()
                                    .ErrorPage(htmltemplate_error, 404)
                                    .build();
                send(client_socket, response.c_str(), response.length(),
                     MSG_NOSIGNAL);
                continue;
            }
            message = "";
            message.shrink_to_fit();
            if (httpmsg.type == INVALID)
                continue;
            client_threads.push_back(std::thread(&Server::respondClient, this,
                                                 client_socket, httpmsg, &m));
        }
    }
    for (auto &t : client_threads) {
        t.join();
    }
    logger.changeMetric("Connection Count", -1);
    client_threads.clear();
    client_threads.shrink_to_fit();
    close(client_socket);
    return;
}

void Server::start()
{
    while (!this->shouldClose) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int client_socket
            = accept(socketfd, (struct sockaddr *)&clientAddr, &clientLen);
        if (client_socket <= 0) {
            struct rusage resource_usage;
            if (getrusage(RUSAGE_SELF, &resource_usage) == 0)
                logger.setMetric("Total Memory Usage (KB)",
                                 resource_usage.ru_maxrss);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            continue;
        }
        logger.changeMetric("Connection Count", 1);
        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);

        // FIXME: make this optional (opt-in)
        // Simple check: Does the IP start with "192.168."?
        // std::string ipStr = clientIp;
        // if (!ipStr.starts_with("10.42.") && ipStr != "127.0.0.1") {
        //     close(client_socket); // Reject connection
        //     continue;
        // }
        if (client_socket < 0) {
            logger.report("ERROR", "serveClient->accept");
            return;
        }
        threads.push_back(
            std::thread(&Server::serveClient, this, client_socket));
    }
    for (auto &t : threads)
        t.join();
    logger.report("INFO", "Server is shutting down");
}

Server::~Server()
{
    if (config.configBool("UseHttps"))
        SSL_CTX_free(ctx);
    else
        close(socketfd);
}
