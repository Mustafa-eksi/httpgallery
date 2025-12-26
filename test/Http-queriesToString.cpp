#include <iostream>
#include <cassert>

#include <optional>
#include <unordered_map>
#include <cstdio>
#include <string>
#include "../src/Http.cpp"

int main() {
    { // Test 1
        HttpMessage msg("GET /?world=42&half-life=3 HTTP/1.1\r\n");
        HttpMessage msg_copy("GET /"+msg.queriesToString()+" HTTP/1.1\r\n");
        assert(msg.queries == msg_copy.queries);
    }
    std::cout << "Test 1 passed" << std::endl;

    { // Test 2
        HttpMessage msg("GET / HTTP/1.1\r\n");
        HttpMessage msg_copy("GET /"+msg.queriesToString()+" HTTP/1.1\r\n");
        assert(msg.queries == msg_copy.queries && msg.queriesToString().empty());
    }
    std::cout << "Test 2 passed" << std::endl;

    return 0;
}
