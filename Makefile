CFLAGS=-Wall -Werror -Wextra -Wshadow -ggdb -std=c++23 -g
ASAN_FLAGS=-fsanitize=address -fno-omit-frame-pointer -O0
TEST_FLAGS=-fPIC -fprofile-arcs -ftest-coverage --coverage
all: main

TESTS=Http-getRange Http-HttpMessage Http-queriesToString Http-HtmlDecode

test: clean_test compile_test

clean_test:
	rm  -f ./test/*.gcov
	rm  -f ./test/*.gcda
	rm  -f ./test/*.gcno
	rm  -f ./test/*.info
	rm -rf ./test/out/
	rm -rf ./test/Http-getRange
	rm -rf ./test/Http-HttpMessage
	rm -rf ./test/Http-queriesToString
	rm -rf ./test/Http-HtmlDecode
	rm -f *.gcov
	rm -f *.gcno

compile_test: $(TESTS)

$(TESTS):
	g++  -o ./test/$@ ./test/$@.cpp $(CFLAGS) $(TEST_FLAGS)
	./test/$@

gcov_test:
	lcov --capture --directory ./test/ --output-file ./test/coverage.info
	lcov --remove ./test/coverage.info '/usr/*' --output-file ./test/coverage_filtered.info
	genhtml ./test/coverage_filtered.info --output-directory ./test/out

main: ./src/*
	g++ ./src/main.cpp -o main $(CFLAGS)

asan: ./src/*
	g++ ./src/main.cpp -o main_asan $(CFLAGS) $(ASAN_FLAGS)
