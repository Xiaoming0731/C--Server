// #include <stdio.h>
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

//     errif(bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1, "socket bind error");

//     errif(listen(sockfd, SOMAXCONN) == -1, "socket listen error");
    
//     struct sockaddr_in clnt_addr;
//     socklen_t clnt_addr_len = sizeof(clnt_addr);
//     bzero(&clnt_addr, sizeof(clnt_addr));

//     int clnt_sockfd = accept(sockfd, (sockaddr*)&clnt_addr, &clnt_addr_len);
//     errif(clnt_sockfd == -1, "socket accept error");

//     printf("new client fd %d! IP: %s Port: %d\n", clnt_sockfd, inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));
//     while (true) {
//         char buf[1024];
//         bzero(&buf, sizeof(buf));
//         ssize_t read_bytes = read(clnt_sockfd, buf, sizeof(buf));
//         if(read_bytes > 0){
//             printf("message from client fd %d: %s\n", clnt_sockfd, buf);
//             write(clnt_sockfd, buf, sizeof(buf));
//         } else if(read_bytes == 0){
//             printf("client fd %d disconnected\n", clnt_sockfd);
//             close(clnt_sockfd);
//             break;
//         } else if(read_bytes == -1){
//             close(clnt_sockfd);
//             errif(true, "socket read error");
//         }
//     }
//     close(sockfd);
//     return 0;
// }


#include "util.h"
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main()
{
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    errif(sockfd==-1,"socket create error");

    struct sockaddr_in serv_addr;
    bzero(&serv_addr,sizeof(serv_addr));

    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    serv_addr.sin_port=htons(8888);

    errif(bind(sockfd,(sockaddr*)&serv_addr,sizeof(serv_addr))==-1,"socket bind error");
    errif(listen(sockfd,SOMAXCONN)==-1,"socket listen error");

    struct sockaddr_in client_addr;
    socklen_t client_addr_len=sizeof(client_addr);
    bzero(&client_addr,sizeof(client_addr));

    int client_fd=accept(sockfd,(sockaddr*)&client_addr,&client_addr_len);
    errif(client_fd==-1,"socket accpet error");

    printf("new client fd %d! IP:%s Port:%d\n",client_fd,inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));

    while(1)
    {
        char buf[1024];
        bzero(&buf,sizeof(buf));
        ssize_t read_bytes=read(client_fd,buf,sizeof(buf));
        if(read_bytes>0)
        {
            printf("message from fd %d:%s\n",client_fd,buf);
            write(client_fd,buf,sizeof(buf));
        }
        else if(read_bytes==0)
        {
            printf("client fd %d disconnected!",client_fd);
            close(client_fd);
            break;
        }
        else 
        {
            close(client_fd);
            errif(1,"socket read error");
        }
    }
    close(sockfd);
    return 0;
}