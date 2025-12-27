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

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "Http.cpp"
#include "FileSystemInterface.cpp"
#include "Server.cpp"

int main(int argc, char** argv) {
    std::string path = ".";
    if (argc > 1) path = std::string(argv[1]);
    Server *server = new Server(path, 8000, 100); 
    server->start();
    return 0;
}
