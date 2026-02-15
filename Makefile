.PHONY: format main
prefix=/usr
BIN_DIR=$(prefix)/bin
SHARE_DIR=$(prefix)/share
CC=clang++
LIBS=libssl zlib
DEBUG_CFLAGS=-Wall -Werror -Wextra -Wshadow -ggdb -std=c++23 -g \
	   ${shell pkg-config --cflags $(LIBS)}
UNOPTIMIZED_CFLAGS=-Wall -Werror -Wextra -Wshadow -std=c++23 \
			   ${shell pkg-config --cflags $(LIBS)} \
			   -DHTTPGALLERY_RES_DIR="\"/usr/share/httpgallery\""
RELEASE_CFLAGS=-Wall -Werror -Wextra -Wshadow -std=c++23 \
			   ${shell pkg-config --cflags $(LIBS)} -O3 \
			   -DHTTPGALLERY_RES_DIR="\"/usr/share/httpgallery\""
LDFLAGS=$(shell pkg-config --libs $(LIBS)) -lcrypto
ASAN_FLAGS=-fsanitize=address -fno-omit-frame-pointer -O0
TEST_FLAGS=-fPIC -fprofile-arcs -ftest-coverage --coverage
COMPILE_MODE=DEBUG
ifeq ($(COMPILE_MODE), RELEASE)
	CFLAGS=$(RELEASE_CFLAGS)
else
	CFLAGS=$(DEBUG_CFLAGS)
	UNOPTIMIZED_CFLAGS=$(DEBUG_CFLAGS)
endif
all: format main

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
	$(CC)  -o ./test/$@ ./test/$@.cpp $(CFLAGS) $(TEST_FLAGS)
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

SRCS=Configuration Http HttpResponseBuilder Logging Server main FileSystemInterface
OBJS = $(SRCS:%=./build/%.oxx)

build:
	mkdir -p build

./build/%.oxx: ./src/%.cpp ./src/%.hpp
	$(CC) $(UNOPTIMIZED_CFLAGS) -c $< -o $@

./build/main.oxx: ./src/main.cpp
	$(CC) $(UNOPTIMIZED_CFLAGS) -c $< -o $@

main: build $(OBJS)
	$(CC) -o httpgallery $(CFLAGS) $(LDFLAGS) $(OBJS)

main_release: ./src/*
	$(CC) $(RELEASE_CFLAGS) -o httpgallery $(LDFLAGS) $(OBJS)

asan: ./src/*
	$(CC) ./src/main.cpp -o main_asan $(CFLAGS) $(ASAN_FLAGS) $(LDFLAGS)

check:
	cppcheck . --check-level=exhaustive

format: src/*
	clang-format --style=file:./.clang-format -i src/*

clean: clean_test
	rm -f ./main_asan
	rm -f ./httpgallery
	rm -f ./data_graph.png
	rm -f ./metrics.dat
	rm -f ./httpgallery_logs.txt
	rm -f chain.pem
	rm -f pkey.pem
	rm -f build/*

install: ./src/* main_release
	mkdir -p $(BIN_DIR)
	mkdir -p $(SHARE_DIR)
	install httpgallery $(BIN_DIR)/
	install -d $(SHARE_DIR)/httpgallery
	cp -r ./res/* $(SHARE_DIR)/httpgallery

uninstall:
	rm $(BIN_DIR)/httpgallery
	rm -rf $(SHARE_DIR)/httpgallery

docker_build:
	docker build . -t httpgallery

docker_run: docker_build
	docker run -p 8000:8000 httpgallery

generate_docs:
	doxygen Doxyfile

stress_test:
	gcc -Wall -Wextra -Wshadow ./test/stress-test.c -o ./test/stress-test $(LDFLAGS)
	# Current limit is 10000, we're bounded by linux thread limit
	./test/stress-test 1000
