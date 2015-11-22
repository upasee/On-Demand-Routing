#include "msg.h"

int msg_send(int sockfd, char *ip_addr, int dest_port, char *mesg, int flag) {
    printf("Inside message send \n");
    struct sockaddr_un servaddr;    

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, ODR_PATH);

    void *sendline;
    sendline = (void *)malloc(sizeof(struct send_pack));
    struct send_pack *odr_sendmsg = malloc(sizeof(struct send_pack));
    
    strcpy(odr_sendmsg->ip_addr, ip_addr);
    odr_sendmsg->dest_port = dest_port;
    strncpy(odr_sendmsg->mesg, mesg, strlen(mesg));
    odr_sendmsg->flag = flag;
    memcpy(sendline, &odr_sendmsg, sizeof(struct send_pack));

    printf("Message being sent is %s\n", odr_sendmsg->mesg);
    Sendto(sockfd, odr_sendmsg, sizeof(struct send_pack), 0, (SA *)&servaddr, sizeof(servaddr));
}

int msg_recv(int sockfd, char *mesg, char *ip_addr, int *dest_port) {
    struct sockaddr_un cliaddr;
    int n, clilen;

    bzero(&cliaddr,sizeof(cliaddr));
    cliaddr.sun_family = AF_LOCAL;
    strcpy(cliaddr.sun_path, ODR_PATH);

    struct recv_pack *odr_recvmsg = malloc(sizeof(struct recv_pack));

    clilen = sizeof(cliaddr);
    n = Recvfrom(sockfd, (char *)&odr_recvmsg, sizeof(odr_recvmsg), 0, (SA*)&cliaddr, &clilen);

    printf("Message received is %s\n", odr_recvmsg->mesg);
    strcpy(ip_addr,odr_recvmsg->ip_addr);
    strncpy(mesg,odr_recvmsg->mesg, strlen(mesg));
    *dest_port = odr_recvmsg->dest_port;
}

