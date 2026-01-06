.PHONY: chain main
LIBS=libssl
CFLAGS=-Wall -Werror -Wextra -Wshadow -ggdb -std=c++23 -g ${shell pkg-config --cflags $(LIBS)}
LDFLAGS=$(shell pkg-config --libs $(LIBS)) -lcrypto
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

plot: metrics.dat
	gnuplot -c plotMetrics.gpi metrics.dat

chain: chain.pem
pkey.pem:
	openssl genpkey -algorithm rsa -out pkey.pem -pkeyopt rsa_keygen_bits:2048
chain.pem: pkey.pem
	openssl req -x509 -new -key pkey.pem -days 36500 -subj '/CN=localhost' -out chain.pem

main: ./src/*
	g++ ./src/main.cpp -o httpgallery $(CFLAGS) $(LDFLAGS)

asan: ./src/*
	g++ ./src/main.cpp -o main_asan $(CFLAGS) $(ASAN_FLAGS) $(LDFLAGS)

clean:
	rm -f ./main_asan
	rm -f ./httpgallery
	rm -f ./data_graph.png
	rm -f ./metrics.dat
	rm -f ./httpgallery_logs.txt
	rm -f chain.pem
	rm -f pkey.pem

