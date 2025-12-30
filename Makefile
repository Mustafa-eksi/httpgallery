CFLAGS=-Wall -Werror -Wextra -Wshadow -ggdb -std=c++23 -g
ASAN_FLAGS=-fsanitize=address -fno-omit-frame-pointer -O0
TEST_FLAGS=-fPIC -fprofile-arcs -ftest-coverage --coverage
all: main

test: clean_test compile_test run_test

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

compile_test: Http-getRange Http-HttpMessage Http-queriesToString Http-HtmlDecode

Http-getRange: ./test/Http-getRange.cpp ./src/Http.cpp ./src/Http.hpp
	g++ ./test/Http-getRange.cpp -o ./test/Http-getRange $(CFLAGS) $(TEST_FLAGS)

Http-HttpMessage: ./test/Http-HttpMessage.cpp ./src/Http.cpp ./src/Http.hpp
	g++ ./test/Http-HttpMessage.cpp -o ./test/Http-HttpMessage $(CFLAGS) $(TEST_FLAGS)

Http-queriesToString: ./test/Http-queriesToString.cpp ./src/Http.cpp ./src/Http.hpp
	g++ ./test/Http-queriesToString.cpp -o ./test/Http-queriesToString $(CFLAGS) $(TEST_FLAGS)

Http-HtmlDecode: ./test/Http-HtmlDecode.cpp ./src/Http.cpp ./src/Http.hpp
	g++ ./test/Http-HtmlDecode.cpp -o ./test/Http-HtmlDecode $(CFLAGS) $(TEST_FLAGS)

run_test:
	./test/Http-getRange
	./test/Http-HttpMessage
	./test/Http-queriesToString
	./test/Http-HtmlDecode

gcov_test:
	lcov --capture --directory ./test/ --output-file ./test/coverage.info
	lcov --remove ./test/coverage.info '/usr/*' --output-file ./test/coverage_filtered.info
	genhtml ./test/coverage_filtered.info --output-directory ./test/out

main: ./src/*
	g++ ./src/main.cpp -o main $(CFLAGS)

asan: ./src/*
	g++ ./src/main.cpp -o main_asan $(CFLAGS) $(ASAN_FLAGS)
