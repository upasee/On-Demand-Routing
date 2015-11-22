#include "unp.h"

dg_rreq_from_odr(int sockfd, SA *pcliaddr, socklen_t clilen)
{
    int n, dest_port;
    socklen_t len;
    void *mesg = (void *)malloc(MAXLINE);
    char ip_addr[30];
    len = clilen;

//    for(;;) {
        msg_recv(sockfd, mesg, ip_addr, &dest_port);
        printf("Server at vm serv received a request from vm cli\n");
        printf("Message in server from client is %s\n",mesg);
//        msg_send();
//    }

}

int main(int argc, char **argv) {
    int sockfd;
    struct sockaddr_un servaddr, cliaddr;

    sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
    unlink(SERV_PATH);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, SERV_PATH);

    Bind(sockfd, (SA *)&servaddr, sizeof(servaddr));

    dg_rreq_from_odr(sockfd, (SA *)&cliaddr, sizeof(cliaddr));
}
