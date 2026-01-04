#include <iostream>
#include <climits>
#include <thread>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <format>
#include <set>
#include <cassert>
#include <mutex>
#include <cstring>
#include <cstdio>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <fcntl.h>

// OpenSSL
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "Logging.cpp"
#include "Http.cpp"
#include "FileSystemInterface.cpp"
#include "Server.cpp"

int main(int argc, char** argv) {
    std::string path = ".";
    std::string logs_path = "./";
    if (argc > 1) path = std::string(argv[1]);
    if (!std::filesystem::exists(path)) {
        std::cout << "FATAL ERROR: specified path does not exist" << std::endl;
        return -2;
    }
    if (argc > 2) path = std::string(argv[2]);
    Logger logger = Logger(logs_path+"httpgallery_logs.txt", argc > 2, true);
    logger.info("Starting Server");
    Server *server = new Server(logger, path, 8000);
    server->start();
    return 0;
}
