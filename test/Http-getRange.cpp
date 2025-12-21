#include <iostream>
#include <cassert>

#include <optional>
#include <unordered_map>
#include <cstdio>
#include <string>
#include "../src/Http.cpp"

HttpMessage genHttpMessage(std::string range) {
    HttpMessage m1;
    m1.type = GET;
    m1.address = "/";
    m1.protocol_version = "HTTP/1.1";
    m1.headers["Range"] = range;
    return m1;
}

int main() {
    { // Test 1
        auto msg = genHttpMessage("bytes=0-0");
        auto [start, end] = msg.getRange(100).value();
        assert(start == end && start == 0);
    }
    std::cout << "Test 1 Passed" << std::endl;

    { // Test 2
        auto msg = genHttpMessage("bytes=1-0");
        auto [start, end] = msg.getRange(100).value();
        assert(start == 0 && end == 100);
    }
    std::cout << "Test 2 Passed" << std::endl;

    { // Test 3
        auto msg = genHttpMessage("bytes=0-200");
        auto output = msg.getRange(100);
        assert(!output.has_value());
    }
    std::cout << "Test 3 Passed" << std::endl;

    { // Test 4
        auto msg = genHttpMessage("bytes=200-300");
        auto output = msg.getRange(100);
        assert(!output.has_value());
    }
    std::cout << "Test 4 Passed" << std::endl;

    { // Test 5
        auto msg = genHttpMessage("bytes=0-");
        auto output = msg.getRange(100);
        assert(output.has_value());
    }
    std::cout << "Test 5 Passed" << std::endl;

    { // Test 6
        auto msg = genHttpMessage("bytes=-");
        auto [start, end] = msg.getRange(100).value();
        assert(start == 0 && end == 100);
    }
    std::cout << "Test 6 Passed" << std::endl;

    { // Test 7
        auto msg = genHttpMessage("horses=0-");
        auto output = msg.getRange(100);
        assert(!output.has_value());
    }
    std::cout << "Test 7 Passed" << std::endl;

    { // Test 8
        auto msg = genHttpMessage("bytes=asd-");
        auto output = msg.getRange(100);
        assert(!output.has_value());
    }
    std::cout << "Test 8 Passed" << std::endl;

    { // Test 9
        auto msg = genHttpMessage("bytes=100000000000000000000000000000000-");
        auto output = msg.getRange(100);
        assert(!output.has_value());
    }
    std::cout << "Test 9 Passed" << std::endl;

    { // Test 10
        auto msg = genHttpMessage("bytes=");
        auto output = msg.getRange(100);
        assert(!output.has_value());
    }
    std::cout << "Test 10 Passed" << std::endl;

    { // Test 11
        auto msg = genHttpMessage("bytes=0-10000000000000000000000000000000000000000000");
        auto output = msg.getRange(100);
        assert(!output.has_value());
    }
    std::cout << "Test 11 Passed" << std::endl;

    { // Test 12
        auto msg = genHttpMessage("bytes=-asd");
        auto output = msg.getRange(100);
        assert(!output.has_value());
    }
    std::cout << "Test 12 Passed" << std::endl;

    { // Test 13
        auto msg = genHttpMessage("bytes=99-102");
        auto [start, end] = msg.getRange(100).value();
        assert(start == 99 && end == 100);
    }
    std::cout << "Test 13 Passed" << std::endl;

    return 0;
}
