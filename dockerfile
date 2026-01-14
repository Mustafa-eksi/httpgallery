FROM alpine:latest

RUN apk add --no-cache \
    build-base \
    openssl \
    openssl-dev

WORKDIR /httpgallery

COPY . .
RUN make clean
RUN make clean_test
RUN make CC=g++ install
EXPOSE 8000

CMD ["httpgallery", "--no-metrics"]

