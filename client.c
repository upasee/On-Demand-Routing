#include "unp.h"
#include <netdb.h>
#include "hw_addrs.h"

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

void send_request(int sockfd, const SA *pservaddr, socklen_t servlen)
{
    int n;
    char serv_node[10], cli_node[10];
    char  *serv_ip = malloc(20*sizeof(char));
    struct message msg;

    printf("Before getting the cli_node \n");
    get_client_node(cli_node);
    printf("Client node is %s\n", cli_node);

    while(1)
    {
        strcpy(serv_node,"");
        printf("Choose a VM from vm1 to vm10 as a server node:\n");
        scanf("%s", serv_node);
        printf("The vm selected is %s\n", serv_node);
        struct hostent *host = gethostbyname(serv_node);
        strcpy(serv_ip,"");
        Inet_ntop(AF_INET, (void*)host->h_addr, serv_ip, 20);
        printf("IP of the selected host is %s\n", serv_ip);
        msg.id = 1;
        strcpy(msg.ip, serv_ip);
        strcpy(msg.buff, "message");
        printf("Client at node %s sending request to server at %s\n", cli_node, serv_node);
        Sendto(sockfd, "hello", sizeof("hello"), 0, pservaddr, servlen);
    }
}

int main(int argc, char **argv)
{
    char file_path[40];
    int fd, sockfd, temp;
    struct sockaddr_un cliaddr, servaddr;

    strcpy(file_path,"/tmp/upasi_XXXXXX");
    fd = mkstemp(file_path);

    if(fd < 0) {
        printf("Could not create a new file \n");
        exit(-1);       
    }

    sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);

    unlink(file_path);

    bzero(&cliaddr,sizeof(cliaddr));
    cliaddr.sun_family = AF_LOCAL;
    strcpy(cliaddr.sun_path, file_path);
    Bind(sockfd, (struct sockaddr *)&cliaddr, SUN_LEN(&cliaddr));

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, "/tmp/nargis");

    printf("Before sending request \n");
    send_request(sockfd, (SA *)&servaddr, sizeof(servaddr));

    exit(0);
}
