#include <sys/socket.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>

size_t stoi(char* str) {
    size_t output = 0;
    for (; str; str++) {
        if (str[0] < '0' || str[0] > '9') return output;
        output *= 10;
        output += (size_t)(str[0]-'0');
    }
    return output;
}

int main(int argc, char *argv[]) {
    (void)argv;
    if (argc < 2) {
        printf("USAGE: ./stress-test <number-of-requests>\n");
        return -1;
    }
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in connection_addr;
    connection_addr.sin_family = AF_INET;
    connection_addr.sin_port = htons(8000);
    if (inet_pton(AF_INET, "127.0.0.1", &connection_addr.sin_addr) != 1) {
        printf("inet_pton error\n");
        return -1;
    }
    if (connect(socketfd, (const struct sockaddr*)&connection_addr,
                sizeof(connection_addr)) != 0) {
        printf("Couldn't connect to httpgallery\n");
        return -1;
    }
    int flags = fcntl(socketfd, F_GETFL, 0);
    if (fcntl(socketfd, F_SETFL, flags | O_NONBLOCK) != 0) {
        printf("fcntl error\n");
        return -1;
    }
    size_t number_of_req = stoi(argv[1]);
    printf("Number of requests: %ld\n", number_of_req);
    const char* req_text = "GET / HTTP/1.1\r\n\r\n";
    for (size_t i = 0; i < number_of_req; i++) {
        send(socketfd, req_text, strlen(req_text), 0);
    }
    while (true) {}
    printf("Completed!\n");
    return 0;
}
