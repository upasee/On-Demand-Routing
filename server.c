#include "unp.h"

#include "unp.h"

struct message{
    int id;
    char ip[15];
    char buff[10];
};


    void
dg_echo(int sockfd, SA *pcliaddr, socklen_t clilen)
{
    int n;
    socklen_t len;
    char mesg[MAXLINE];

    char buff[4096];
    time_t ticks;
    struct timeval *tm;

    for ( ; ; ) {
        len = clilen;
        n = Recvfrom(sockfd, mesg, MAXLINE, 0, pcliaddr, &len);

        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));

        //        Sendto(sockfd, mesg, n, 0, pcliaddr, len);
        Sendto(sockfd, buff, strlen(buff), 0, pcliaddr, len);
    }
}

dg_rreq_from_odr(int sockfd, SA *pcliaddr, socklen_t clilen)
{
    int n;
    socklen_t len;
//    char mesg[MAXLINE];
    void *mesg;
    mesg = (void *)malloc(MAXLINE);
    len = clilen;
    n = Recvfrom(sockfd, mesg, sizeof(struct message), 0, pcliaddr, &len);
    printf("received\n");
    struct message *msg = (struct message *)(mesg);
    printf("Message in server from client : %s\n", msg->buff);
}

int main(int argc, char **argv)
{
    /*    int sockfd;
          struct sockaddr_un servaddr, cliaddr;

          sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
          unlink(UNIXDG_PATH);

          bzero(&servaddr, sizeof(servaddr));
          servaddr.sun_family = AF_LOCAL;
          strcpy(servaddr.sun_path, UNIXDG_PATH);

          Bind(sockfd, (SA *) &servaddr, sizeof(servaddr));

          dg_echo(sockfd, (SA *) &cliaddr, sizeof(cliaddr));

     */

    int sockfd;
    struct sockaddr_un servaddr, cliaddr;

    sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
    unlink("/tmp/nargis_serv");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, "/tmp/nargis_serv");

    Bind(sockfd, (SA *)&servaddr, sizeof(servaddr));

    dg_rreq_from_odr(sockfd, (SA *)&cliaddr, sizeof(cliaddr));
}
