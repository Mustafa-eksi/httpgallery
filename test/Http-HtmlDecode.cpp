#include <iostream>
#include <cassert>

#include <optional>
#include <unordered_map>
#include <cstdio>
#include <string>
#include "../src/Http.cpp"

int main() {
    { // Test 1
        assert(html_decode("ayl%C4%B1k") == "aylık");
    }
    std::cout << "Test 1 passed" << std::endl;

    { // Test 2
        assert(html_decode("The Quick%20Brown Fox Jumps Over The Lazy Dog") == "The Quick Brown Fox Jumps Over The Lazy Dog");
    }
    std::cout << "Test 2 passed" << std::endl;

    { // Test 3
        assert(html_decode("%ZZ") == "%ZZ");
    }
    std::cout << "Test 3 passed" << std::endl;

    { // Test 4
        assert(html_decode("%2599") == "%99");
    }
    std::cout << "Test 4 passed" << std::endl;

    { // Test 5
        assert(html_decode("%1") == "%1");
    }
    std::cout << "Test 5 passed" << std::endl;
    return 0;
}
