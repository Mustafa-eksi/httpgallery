#include "Server.hpp"


Server::Server(Logger& logr, std::string p, size_t port, std::string cert_path, std::string pkey_path) : logger(logr) {
    this->https = true;
    this->htmltemplate_list = read_binary_to_string("./res/html/template-list-view.html");
    this->htmltemplate_icon = read_binary_to_string("./res/html/template-icon-view.html");
    this->htmltemplate_error = read_binary_to_string("./res/html/template-error.html");
    this->path = p;
    
    // Setting up OpenSSL
    ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        logger.error("Failed to create server ssl context");
        return;
    }

    if (!SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION)) {
        SSL_CTX_free(ctx);
        ERR_print_errors_fp(stderr);
        logger.error("Failed to set the minimum TLS version");
        return;
    }
    long opts = SSL_OP_IGNORE_UNEXPECTED_EOF | SSL_OP_NO_RENEGOTIATION |
                SSL_OP_SERVER_PREFERENCE;
    SSL_CTX_set_options(ctx, opts);

    if (SSL_CTX_use_certificate_chain_file(ctx, cert_path.c_str()) <= 0) {
        SSL_CTX_free(ctx);
        ERR_print_errors_fp(stderr);
        logger.error("Failed to load the certificate");
        return;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, pkey_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
        SSL_CTX_free(ctx);
        ERR_print_errors_fp(stderr);
        logger.error("Failed to load the private key");
        return;
    }

    SSL_CTX_set_session_id_context(ctx, HTTPGALLERY_SSL_CACHE_ID, sizeof(HTTPGALLERY_SSL_CACHE_ID));
    SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_SERVER);
    SSL_CTX_sess_set_cache_size(ctx, HTTPGALLERY_SSL_CACHE_SIZE);
    SSL_CTX_set_timeout(ctx, HTTPGALLERY_SSL_TIMEOUT);

    // Disable authentication of the client
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

    // FIXME: this is a hack, change this later
    auto portstr = std::to_string(port);
    this->ssl_socket = BIO_new_accept(portstr.c_str());
    if (!this->ssl_socket) {
        SSL_CTX_free(ctx);
        ERR_print_errors_fp(stderr);
        logger.error("Failed to create a ssl socket");
        return;
    }

    BIO_set_bind_mode(ssl_socket, BIO_BIND_REUSEADDR);
    // First BIO_do_accept call doesn't accept a connection. It initiates the
    // BIO acceptor.
    if (BIO_do_accept(this->ssl_socket) <= 0) {
        SSL_CTX_free(ctx);
        ERR_print_errors_fp(stderr);
        logger.error("Failed to set up bio acceptor socket");
        return;
    }
}

Server::Server(Logger& logr, std::string p, size_t port, int backlog) : logger(logr) {
    this->https = false;
    this->htmltemplate_list = read_binary_to_string("./res/html/template-list-view.html");
    this->htmltemplate_icon = read_binary_to_string("./res/html/template-icon-view.html");
    this->htmltemplate_error = read_binary_to_string("./res/html/template-error.html");
    this->path = p;
    this->socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd == -1) {
        logger.error("Socket");
        return;
    }
    // TODO: this might cause problems
    int temp = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &temp, sizeof(int)) == -1) {
        logger.error("setsockopt");
        return;
    }

    int status = fcntl(socketfd, F_SETFL, fcntl(socketfd, F_GETFL, 0) | O_NONBLOCK);
    if (status == -1){
        logger.error("fcntl failed");
        return;
    }

    server_address = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = (struct in_addr) {
            .s_addr = INADDR_ANY,
        },
        .sin_zero = 0
    };
    address_length = sizeof(server_address);
    if (bind(socketfd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        logger.error("bind");
        return;
    }
    if (listen(socketfd, backlog) < 0) {
        logger.error("listen");
        return;
    }
}

PageType Server::choosePageType(HttpMessage msg) {
    if (msg.queries.contains("icon")) return IconData;
    std::string decoded_path = path + msg.address;
    bool is_dir = std::filesystem::is_directory(decoded_path);
    return is_dir ? DirectoryPage : FileDataPage;
}

std::string Server::generateContent(HttpMessage msg) {
    PageType pt = this->choosePageType(msg);
    if (path.back() == '/') path = path.substr(0, path.length()-1);
    auto filepath = path+msg.address;
    if (pt == FileDataPage) {
        if (!std::filesystem::exists(filepath)) {
            std::string error_page = string_format(this->htmltemplate_error, "404 Not Found");
            return HttpResponseBuilder()
                .Status(404)
                .ContentType("text/html; charset=utf-8")
                .Content(error_page)
                .build();
        }
        uintmax_t filesize = std::filesystem::file_size(filepath);
        auto range_opt = msg.getRange(filesize);
        if (!range_opt.has_value()) {
            std::string error_page = string_format(this->htmltemplate_error, "416 Range Not Satisfiable");
            return HttpResponseBuilder()
                .Status(416)
                .SetHeader("Content-Range", "bytes */"+std::to_string(filesize))
                .Content(error_page)
                .build();
        }
        auto [range_start, range_end] = range_opt.value();
        int status = range_end-range_start == filesize ? 200 : 206;
        std::string mimetype = get_mime_type(msg.address);
        std::string file_content;

        if (msg.type == GET)
            file_content = read_binary_to_string(filepath, range_start, range_end);
        else if (msg.type == HEAD)
            file_content = "";

        return HttpResponseBuilder()
            .Status(status)
            .ContentType(mimetype)
            .ContentRange(range_start, range_end)
            .Content(file_content)
            .ContentLength(filesize) // Content already sets it but we override for HEAD
            .build();
    } else if (pt == DirectoryPage) {
        bool list_view = msg.queries.contains("list-view") && msg.queries["list-view"] == "true";
        std::string dir_page_contents = list_contents(msg.address, filepath, msg.queriesToString(), list_view);
        std::string current_path = msg.address.substr(0, msg.address.length()-1); // exclude last char
        
        auto slash_pos = current_path.rfind('/');
        std::string up = "/";
        if (slash_pos != std::string::npos)
            up = current_path.substr(0, slash_pos+1);
        std::string final_content = string_format(list_view ? this->htmltemplate_list : this->htmltemplate_icon,
                                filepath.c_str(), up, dir_page_contents.c_str());
        uintmax_t content_length = final_content.length();
        if (msg.type == HEAD)
            final_content = "";
        return HttpResponseBuilder()
            .Status(200)
            .ContentType("text/html; charset=utf-8")
            .Content(final_content)
            .ContentLength(content_length) // Same as one in above
            .build();
    } else if (pt == IconData) {
        if (msg.queries["icon"] == "video") {
            std::string image_data(video_icon_data.begin(), video_icon_data.end());
            return HttpResponseBuilder()
                .Status(200)
                .ContentType("image/png")
                .Content(image_data)
                .build();
        } else if (msg.queries["icon"] == "directory") {
            std::string image_data(directory_icon_data.begin(), directory_icon_data.end());
            return HttpResponseBuilder()
                .Status(200)
                .ContentType("image/png")
                .Content(image_data)
                .build();
        } else {
            std::string image_data(text_icon_data.begin(), text_icon_data.end());
            return HttpResponseBuilder()
                .Status(200)
                .ContentType("image/png")
                .Content(image_data)
                .build();
        }
    } else {
        logger.error("Invalid page type");
    }
    return "";
}

void Server::respondClientHttps(SSL* ssl_handle, HttpMessage msg, std::mutex* m) {
    std::string response;
    if (msg.address.ends_with("favicon.ico")) {
        std::string image_data(directory_icon_data.begin(), directory_icon_data.end());
        response = HttpResponseBuilder()
            .Status(200)
            .ContentType("image/png")
            .Content(image_data)
            .build();
    } else {
        response = generateContent(msg);
    }
    m->lock();
    if (SSL_write(ssl_handle, response.c_str(), response.length()) <= 0) {
        logger.error("Failed to write to ssl socket");
    }
    m->unlock();
    response.clear();
    response.shrink_to_fit();
}

void Server::serveClientHttps(SSL *ssl_handle) {
    std::vector<std::thread> client_threads;
    std::mutex m;

    while (!this->shouldClose) {
        char message_buffer[8192] = {'\0'};
        // receive message
        SSL_read(ssl_handle, message_buffer, sizeof(message_buffer));
        std::string message(message_buffer);
        // parse message
        HttpMessage httpmsg = HttpMessage(message);
        message = "";
        message.shrink_to_fit();
        if (httpmsg.type == INVALID) continue;

        //logger.info("responding client: " + httpmsg.address);
        client_threads.push_back(std::thread(&Server::respondClientHttps, this, ssl_handle, httpmsg, &m));
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

void Server::startHttps() {
    if (this->ctx == NULL) {
        logger.error("SSL context is NULL, exiting");
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
            logger.error("Failed to retrieve the client socket");
            continue;
        }

        SSL *ssl_handle = SSL_new(ctx);
        if (!ssl_handle) {
            BIO_free(client_socket);
            ERR_print_errors_fp(stderr);
            logger.error("Failed to create new ssl handle");
            continue;
        }
        
        // We write client_socket twice since it is both where we will read
        // from and write to.
        SSL_set_bio(ssl_handle, client_socket, client_socket);

        if (SSL_accept(ssl_handle) <= 0) {
            BIO_free(client_socket);
            ERR_print_errors_fp(stderr);
            logger.error("Failed to make a TLS handshake");
            continue;
        }

        logger.info("Successfull handshake");

        threads.push_back(std::thread(&Server::serveClientHttps, this, ssl_handle));
    }
}

void Server::respondClient(int client_socket, HttpMessage msg, std::mutex* m) {
    std::string response;
    if (msg.address.ends_with("favicon.ico")) {
        std::string image_data(directory_icon_data.begin(), directory_icon_data.end());
        response = HttpResponseBuilder()
            .Status(200)
            .ContentType("image/png")
            .Content(image_data)
            .build();
    } else {
        response = generateContent(msg);
    }
    m->lock();
    send(client_socket, response.c_str(), response.length(), 0);
    m->unlock();
    response.clear();
    response.shrink_to_fit();
}

void Server::serveClient(int client_socket) {
    std::vector<std::thread> client_threads;
    std::mutex m;
    int timeout = 0;
    while (!this->shouldClose) {
        intmax_t msg_length = recv(client_socket, NULL, INT_MAX, (MSG_PEEK | MSG_TRUNC));
        if (msg_length < 1) {
            if (timeout > 5) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            timeout++;
            continue;
        }
        timeout = 0;
        char *message_buffer = (char*)malloc(msg_length*sizeof(char)+1);
        memset(message_buffer, '\0', msg_length*sizeof(char)+1);
        if (!message_buffer) {
            // FIXME: make logger variadic like string_format
            logger.error("failed to allocate " + std::to_string(msg_length) + " bytes");
            return;
        }
        // receive message
        recv(client_socket, message_buffer, msg_length, 0);
        std::string message(message_buffer);
        free(message_buffer);
        // parse message
        HttpMessage httpmsg = HttpMessage(message);
        message = "";
        message.shrink_to_fit();
        if (httpmsg.type == INVALID) continue;

        //logger.info("responding client: " + httpmsg.address);
        client_threads.push_back(std::thread(&Server::respondClient, this, client_socket, httpmsg, &m));
    }
    for (auto &t : client_threads)
        t.join();
    client_threads.clear();
    client_threads.shrink_to_fit();
    close(client_socket);
    return;
}

void Server::start() {
    while (!this->shouldClose) {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int client_socket = accept(socketfd, (struct sockaddr*)&clientAddr, &clientLen);
        if (client_socket < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            continue;
        }
        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);

        // FIXME: make this optional (opt-in)
        // Simple check: Does the IP start with "192.168."?
        std::string ipStr = clientIp;
        if (!ipStr.starts_with("10.42.") && ipStr != "127.0.0.1") {
            close(client_socket); // Reject connection
            continue;
        }
        if (client_socket < 0) {
            logger.error("serveClient->accept");
            return;
        }
        threads.push_back(std::thread(&Server::serveClient, this, client_socket));
    }
}

Server::~Server() {
    if (https)
        SSL_CTX_free(ctx);
    else
        close(socketfd);
}
