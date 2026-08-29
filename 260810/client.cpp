// #include <iostream>
// #include <sys/socket.h>
// #include <arpa/inet.h>
// #include <string.h>
// #include <unistd.h>
// #include "util.h"


// int main() {
//     int sockfd = socket(AF_INET, SOCK_STREAM, 0);
//     errif(sockfd == -1, "socket create error");

//     struct sockaddr_in serv_addr;
//     bzero(&serv_addr, sizeof(serv_addr));
//     serv_addr.sin_family = AF_INET;
//     serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
//     serv_addr.sin_port = htons(8888);

//     errif(connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1, "socket connect error");
    
//     while(true){
//         char buf[1024];
//         bzero(&buf, sizeof(buf));
//         scanf("%s", buf);
//         ssize_t write_bytes = write(sockfd, buf, strlen(buf));
//         if(write_bytes == -1){
//             printf("socket already disconnected, can't write any more!\n");
//             break;
//         }
//         bzero(&buf, sizeof(buf));
//         ssize_t read_bytes = read(sockfd, buf, sizeof(buf));
//         if(read_bytes > 0){
//             printf("message from server: %s\n", buf);
//         }else if(read_bytes == 0){
//             printf("server socket disconnected!\n");
//             break;
//         }else if(read_bytes == -1){
//             close(sockfd);
//             errif(true, "socket read error");
//         }
//     }
//     close(sockfd);
//     return 0;
// }



#include <stdio.h>
#include "util.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main()
{
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    errif(sockfd==-1,"socket create error");

    struct sockaddr_in serv_addr;
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    serv_addr.sin_port=htons(8888);

    errif(connect(sockfd,(sockaddr*)&serv_addr,sizeof(serv_addr))==-1,"socket connect error");

    while(1)
    {
        char buf[1024];
        bzero(&buf,sizeof(buf));
        fgets(buf,sizeof(buf),stdin);
        ssize_t write_bytes=write(sockfd,buf,sizeof(buf));
        if(write_bytes==-1)
        {
            printf("socket already disconected,can't read any more!\n");
            break;
        }
        bzero(&buf,sizeof(buf));
        ssize_t read_bytes=read(sockfd,buf,sizeof(buf));
        if(read_bytes>0)
        {
            printf("message from server: %s\n",buf);
        }
        else if(read_bytes==0)
        {
            printf("server socket disconnected!");
            break;
        }
        else 
        {
            close(sockfd);
            errif(1,"socket read error");
        }
    }
    close(sockfd);
    return 0;
}