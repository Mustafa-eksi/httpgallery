#include <iostream>
#include <cassert>

#include <optional>
#include <unordered_map>
#include <cstdio>
#include <string>
#include "../src/Http.cpp"

int main() {
    { // Test 1
        HttpMessage msg("");
        assert(msg.type == INVALID);
    }
    std::cout << "Test 1 passed" << std::endl;

    { // Test 2
        HttpMessage msg("\n");
        assert(msg.type == INVALID);
    }
    std::cout << "Test 2 passed" << std::endl;

    { // Test 3
        HttpMessage msg(" \n");
        assert(msg.type == INVALID);
    }
    std::cout << "Test 3 passed" << std::endl;

    { // Test 4
        HttpMessage msg("GET \n");
        assert(msg.type == GET);
    }
    std::cout << "Test 4 passed" << std::endl;

    { // Test 5
        HttpMessage msg("GET /\n");
        assert(msg.type == GET && msg.address == "/");
    }
    std::cout << "Test 5 passed" << std::endl;

    { // Test 6
        HttpMessage msg("GET / HTTP/1.1\n");
        assert(msg.type == GET && msg.address == "/" && msg.protocol_version == "HTTP/1.1");
    }
    std::cout << "Test 6 passed" << std::endl;

    { // Test 7
        HttpMessage msg("GET / HTTP/2\n");
        assert(msg.type == GET && msg.address == "/" && msg.protocol_version == "HTTP/2");
    }
    std::cout << "Test 7 passed" << std::endl;

    { // Test 8
        HttpMessage msg("GET / HTTP/1.1\nContent-Type: text/html");
        assert(msg.type == GET && msg.address == "/" && msg.protocol_version == "HTTP/1.1"
                && msg.headers["Content-Type"] == "text/html");
    }
    std::cout << "Test 8 passed" << std::endl;

    { // Test 9
        HttpMessage msg("GET / HTTP/1.1\nContent-Type: text/html\n");
        assert(msg.type == GET && msg.address == "/" && msg.protocol_version == "HTTP/1.1"
                && msg.headers["Content-Type"] == "text/html");
    }
    std::cout << "Test 9 passed" << std::endl;

    { // Test 10
        HttpMessage msg("GET / HTTP/1.1\nContent-Type: text/html\nContent-Length:0");
        assert(msg.type == GET && msg.address == "/" && msg.protocol_version == "HTTP/1.1"
                && msg.headers["Content-Type"] == "text/html" && msg.headers["Content-Length"] == "0");
    }
    std::cout << "Test 10 passed" << std::endl;

    { // Test 11
        HttpMessage msg("GET / HTTP/1.1\nContent-Type: text/html\nContent-Length:0\n\n");
        assert(msg.type == GET && msg.address == "/" && msg.protocol_version == "HTTP/1.1"
                && msg.headers["Content-Type"] == "text/html" && msg.headers["Content-Length"] == "0");
    }
    std::cout << "Test 11 passed" << std::endl;

    { // Test 12
        HttpMessage msg("GET /hello%20world HTTP/1.1\n");
        assert(msg.type == GET && msg.address == "/hello world" && msg.protocol_version == "HTTP/1.1");
    }
    std::cout << "Test 12 passed" << std::endl;
    return 0;
}
