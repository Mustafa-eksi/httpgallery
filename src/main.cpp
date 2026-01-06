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
#include <signal.h>

// OpenSSL
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "Logging.cpp"
#include "Http.cpp"
#include "FileSystemInterface.cpp"
#include "Server.cpp"

const char HELP_MESSAGE[] =
"Usage: httpgallery <path-to-folder> [options]\n"
"Options:\n"
"   -h | --help\n"
"       Shows this message\n"
#ifndef HTTPGALLERY_NO_OPENSSL
"   -s | --secure <path-to-certificate> <path-to-PrivateKey>\n"
"       Enable HTTPS support (using openssl).\n"
#endif
"   -l | --log-file <path-to-logs-file>\n"
"       Specify log file path to write to. Default is current working\n"
"       directory.\n"
"   -p | --port <number>\n"
"       Use <number> instead of 8000 as the port\n"
"   --silent\n"
"       Do not print the logs to stdout\n"
"   --no-metrics\n"
"       Do not include metrics in the logs (also no .dat files)\n"
;

std::atomic<bool> *shouldClose = NULL;

void exit_handler(int s) {
    (void) s;
    if (shouldClose)
        *shouldClose = true;
}

int main(int argc, char** argv) {
    signal(SIGINT, &exit_handler);
    std::string path = ".";
    if (argc > 1  && !std::string(argv[1]).starts_with("-"))
        path = argv[1];

    std::string logs_path = "./";
    bool secure = false, silent = false, no_metrics = false;
    std::string cert_path, pkey_path;
    int port = 8000;
    for (int i = 1; i < argc; i++) {
        std::string current_arg = argv[i];
        if (current_arg == "-h" || current_arg == "--help") {
            std::cout << HELP_MESSAGE;
            return 0;
        } else if (current_arg == "-s" || current_arg == "--secure") {
#ifndef HTTPGALLERY_NO_OPENSSL
            if (i+2 > argc) {
                std::cout << "\033[1;31mError: Wrong flag usage\033[1;0m" << std::endl;
                std::cout << HELP_MESSAGE;
                return -1;
            }
            cert_path = argv[i+1];
            pkey_path = argv[i+2];
            secure = true;
#else
            std::cout << "\033[1;31mError: HTTPS feature is not available.\033[1;0m" << std::endl;
            return -1;
#endif
        } else if (current_arg == "-l" || current_arg == "--log-file") {
            if (i+1 > argc) {
                std::cout << "\033[1;31mError: Wrong flag usage\033[1;0m" << std::endl;
                std::cout << HELP_MESSAGE;
                return -1;
            }
            logs_path = argv[i+1];
        } else if (current_arg == "-p" || current_arg == "--port") {
            if (i+1 > argc) {
                std::cout << "\033[1;31mError: Wrong flag usage\033[1;0m" << std::endl;
                std::cout << HELP_MESSAGE;
                return -1;
            }
            try {
                port = std::stoi(argv[i+1]);
            } catch(std::out_of_range& e) {
                std::cout << "\033[1;31mError: String to Integer conversion error:\033[1;0m" << argv[i+1] << std::endl;
                return -1;
            } catch(std::invalid_argument& e) {
                std::cout << "\033[1;31mError: String to Integer conversion error:\033[1;0m" << argv[i+1] << std::endl;
                return -1;
            }
        } else if (current_arg == "--silent") {
            silent = true;
        } else if (current_arg == "--no-metrics") {
            no_metrics = true;
        }
    }
    if (!std::filesystem::exists(path)) {
        std::cout << "\033[1;31mError: specified path does not exist\033[1;0m" << std::endl;
        std::cout << HELP_MESSAGE;
        return -2;
    }
    Logger logger = Logger(logs_path+"httpgallery_logs.txt", true, !silent, no_metrics);
    logger.report("INFO", "Starting Server");

    if (secure) {
#ifndef HTTPGALLERY_NO_OPENSSL
        Server server = Server(logger, path, port, cert_path, pkey_path);
        shouldClose = &server.shouldClose;
        server.startHttps();
#endif
    } else {
        Server server = Server(logger, path, port);
        shouldClose = &server.shouldClose;
        server.start();
    }
    return 0;
}
