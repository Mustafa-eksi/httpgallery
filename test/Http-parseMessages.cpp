#include "../src/Http.cpp"
#include <iostream>
#include <vector>
#include <cassert>

bool is_same(std::vector<HttpMessage> a, std::vector<HttpMessage> b) {
    if (a.size() != b.size()) {
        goto fail;
    }
    for (size_t i = 0; i < a.size(); i++)
        if (a[i] != b[i]) {
            goto fail;
        }
    return true;
fail:
    std::cout << std::endl;
    std::cout << "A: " << std::endl;
    for (auto m : a)
        m.print();
    std::cout << std::endl;
    std::cout << "B: " << std::endl;
    for (auto m : b)
        m.print();
    return false;
}

int main() {
    { // Test 1
        auto messages = parseMessages("GET / HTTP/1.1\r\n\r\nGET / HTTP/1.1\r\n\r\n");
        std::vector<HttpMessage> expected = {
            HttpMessage("GET / HTTP/1.1\r\n"),
            HttpMessage("GET / HTTP/1.1\r\n"),
        };
        assert(is_same(messages, expected));
    }
    std::cout << "Test 1 Passed" << std::endl;

    { // Test 2
        auto messages = parseMessages("PUT / HTTP/1.1\r\nHello World\r\n\r\nGET / HTTP/1.1\r\n\r\n");
        std::vector<HttpMessage> expected = {
            HttpMessage("PUT / HTTP/1.1\r\nHello World"),
            HttpMessage("GET / HTTP/1.1\r\n"),
        };
        assert(is_same(messages, expected));
    }
    std::cout << "Test 2 Passed" << std::endl;
    return 0;
}
