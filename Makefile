CFLAGS=-Wall -Werror -Wextra -Wshadow -ggdb -std=c++23 -fPIC -fprofile-arcs -ftest-coverage --coverage -g
all: main test

test: clean_test compile_test run_test gcov_test

clean_test:
	rm  -f ./test/*.gcov
	rm -rf ./test/out/

compile_test: Http-getRange

Http-getRange: ./test/Http-getRange.cpp ./src/Http.cpp ./src/Http.hpp
	g++ ./test/Http-getRange.cpp -o ./test/Http-getRange $(CFLAGS)

run_test:
	./test/Http-getRange

gcov_test:
	gcov ./test/Http-getRange.cpp
	lcov --capture --directory ./test/ --output-file ./test/coverage.info
	lcov --remove ./test/coverage.info '/usr/*' --output-file ./test/coverage_filtered.info
	genhtml ./test/coverage_filtered.info --output-directory ./test/out

main: ./src/*
	g++ ./src/main.cpp -o main $(CFLAGS)
