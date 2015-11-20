#include "unp.h"
#include <netdb.h>

struct msg{
    char ip[10];
    int dest_port;
    char buff[500];
    int flag;
};

struct message{
    int id;
    char ip[15];
    char buff[10];
};

int msg_send(int sockfd, char *ip, int dest_port, char *buff, int flag)
{
    struct msg *msg1 = (struct msg *)malloc(sizeof(struct msg));
    strncpy(msg1->ip, ip, 10);
    msg1->dest_port = dest_port;
    strncpy(msg1->buff, buff, strlen(buff));
    msg1->flag = flag;

    //write(sockfd, (void *)msg1, sizeof(struct msg)); 
//    sendto(sockfd, (void *)msg1, sizeof(struct msg), 0);
}

int msg_recv(int sockfd, char *buff, char *ip, int *src_port)
{
    struct msg *msg1 = (struct msg *)malloc(sizeof (struct msg));
    int one = read(sockfd, (void *)msg1, sizeof(struct msg));

}

void dg_cli(FILE *fp, int sockfd, const SA *pservaddr, socklen_t servlen)
{
    int n;
//    char sendline[MAXLINE], recvline[MAXLINE + 1];
    void *sendline;
    sendline = (void *)malloc(sizeof(struct message));
/*    struct hostent *host;


    while(1)
    {
        char vm[3];
        printf("Choose a VM as a server node: vm1, vm2, vm3, vm4, vm5, vm6, vm7, vm8, vm9, vm10\n");
        scanf("%s", vm);
        printf("vm selected is %s\n", vm);
//        host = gethostbyname(vm);      
//        ip =  (char *)inet_ntoa(*(struct in_addr*)host->h_addr);;
//        msg_send(sockfd, ip, )
        sendto(sockfd, "Nargis", 7, )      
    }
*/

    while(1)
    {
    char vm[5];
    printf("Choose a VM as a server node: vm1, vm2, vm3, vm4, vm5, vm6, vm7, vm8, vm9, vm10:\n");
    scanf("%s", vm);
//    Fgets(vm, 5, fp);
    printf("The vm selected is %s\n", vm);
    struct hostent *host = gethostbyname(vm);
    char *ip;
    ip = (char *)malloc(15);
    ip = (char *)inet_ntoa(*(struct in_addr*)host->h_addr);
    printf("IP of the selected host is %s\n", ip);
    int size = sizeof(ip);
    struct message msg;
    msg.id = 1;
    strcpy(msg.ip, ip);
    strcpy(msg.buff, "message");
    memcpy(sendline, &msg, sizeof(struct message));

//    while (Fgets(sendline, MAXLINE, fp) != NULL) {
        Sendto(sockfd, sendline, sizeof(struct message), 0, pservaddr, servlen);
    while(1);
//        n = Recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);
//        recvline[n] = 0; 

//        Fputs(recvline, stdout);
    }

    
}

int main(int argc, char **argv)
{
    int sockfd, fd;
    struct sockaddr_un cliaddr, servaddr;

//    char name[] = "/tmp/fileXXXXXX";
//    fd = mkstemp(name);
/*
    sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);

    bzero(&servaddr, sizeof(cliaddr));
    cliaddr.sun_family = AF_LOCAL;
//    strcpy(cliaddr.sun_path, name);
    strcpy(cliaddr.sun_path, tmpnam(NULL));

    Bind(sockfd, (SA *) &cliaddr, sizeof(cliaddr));

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, UNIXDG_PATH);

    dg_cli(stdin, sockfd, (SA *) &servaddr, sizeof(servaddr));
*/

    sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
    bzero(&cliaddr, sizeof(cliaddr));
    cliaddr.sun_family = AF_LOCAL;
    strcpy(cliaddr.sun_path, tmpnam(NULL));

    Bind(sockfd, (SA *)&cliaddr, sizeof(cliaddr));

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, "/tmp/nargis");

    dg_cli(stdin, sockfd, (SA *)&servaddr, sizeof(servaddr));
    exit(0);
}
