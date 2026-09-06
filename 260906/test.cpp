#include <iostream>
#include <unistd.h>
#include <string.h>
#include <functional>
#include "src/util.h"
#include "src/Buffer.h"
#include "src/InetAddress.h"
#include "src/Socket.h"
#include "src/ThreadPool.h"
#include <string>

void oneClient(int msgs, int wait){
    Socket *sock = new Socket();
    InetAddress *addr = new InetAddress("127.0.0.1", 1234);
    sock->connect(addr);
    int sockfd = sock->getfd();
    Buffer *sendBuffer = new Buffer();
    Buffer *readBuffer = new Buffer();

    sleep(wait);
    int count = 0;
    const char* msg = "I'm client!";
    size_t msg_len = strlen(msg);

    while(count < msgs){
        // sendBuffer->setBuf("I'm client!");
        sendBuffer->retrieveAll();
        sendBuffer->append(msg, msg_len);

        // sendBuffer->c_str() / sendBuffer->size()
        ssize_t write_bytes = write(sockfd, sendBuffer->peek(), sendBuffer->readableBytes());
        if(write_bytes == -1){
            printf("socket already disconnected, can't write any more!\n");
            break;
        }

        int already_read = 0;
        char buf[1024];
        while(true){
            bzero(&buf, sizeof(buf));
            ssize_t read_bytes = read(sockfd, buf, sizeof(buf));
            if(read_bytes > 0){
                readBuffer->append(buf, read_bytes);
                already_read += read_bytes;
            } else if(read_bytes == 0){
                printf("server disconnected!\n");
                delete readBuffer;
                delete sendBuffer;
                delete addr;
                delete sock;
                return;
            }
            // sendBuffer->size()
            if((size_t)already_read >= sendBuffer->readableBytes()){
                printf("count: %d, message from server: %s\n", count++, readBuffer->peek());
                break;
            }
        }
        // readBuffer->clear();
        readBuffer->retrieveAll();
    }

    delete readBuffer;
    delete sendBuffer;
    delete addr;
    delete sock;
}

int main(int argc, char *argv[]) {
    int threads = 10;
    int msgs = 10;
    int wait = 0;
    int o;
    const char *optstring = "t:m:w:";
    while ((o = getopt(argc, argv, optstring)) != -1) {
        switch (o) {
            case 't':
                threads = std::stoi(optarg);
                break;
            case 'm':
                msgs = std::stoi(optarg);
                break;
            case 'w':
                wait = std::stoi(optarg);
                break;
            case '?':
                printf("error optopt: %c\n", optopt);
                break;
        }
    }
    ThreadPool *poll = new ThreadPool(threads);
    std::function<void()> func = std::bind(oneClient, msgs, wait);
    for(int i = 0; i < threads; ++i){
        poll->add(func);
    }
    delete poll;
    return 0;
}
