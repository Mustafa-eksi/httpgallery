.PHONY: format main
prefix=/usr
BIN_DIR=$(prefix)/bin
SHARE_DIR=$(prefix)/share
CC=clang++
LIBS=libssl
CFLAGS=-Wall -Werror -Wextra -Wshadow -ggdb -std=c++23 -g \
	   ${shell pkg-config --cflags $(LIBS)}
RELEASE_CFLAGS=-Wall -Werror -Wextra -Wshadow -std=c++23 \
			   ${shell pkg-config --cflags $(LIBS)} -O3 \
			   -DHTTPGALLERY_RES_DIR="\"/usr/share/httpgallery\""
LDFLAGS=$(shell pkg-config --libs $(LIBS)) -lcrypto
ASAN_FLAGS=-fsanitize=address -fno-omit-frame-pointer -O0
TEST_FLAGS=-fPIC -fprofile-arcs -ftest-coverage --coverage
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

main: ./src/*
	$(CC) ./src/main.cpp -o httpgallery $(CFLAGS) $(LDFLAGS)

main_release: ./src/*
	$(CC) $(RELEASE_CFLAGS) ./src/main.cpp -o httpgallery $(LDFLAGS)

asan: ./src/*
	$(CC) ./src/main.cpp -o main_asan $(CFLAGS) $(ASAN_FLAGS) $(LDFLAGS)

check:
	cppcheck . --check-level=exhaustive

format: src/*
	clang-format --style=file:./.clang-format -i src/*

clean:
	rm -f ./main_asan
	rm -f ./httpgallery
	rm -f ./data_graph.png
	rm -f ./metrics.dat
	rm -f ./httpgallery_logs.txt
	rm -f chain.pem
	rm -f pkey.pem

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
