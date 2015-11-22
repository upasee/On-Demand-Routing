#include <netdb.h>
#include "hw_addrs.h"
#include "unp.h"
#include "msg.h"

void main(int argc, char **argv) {
    int fd, sockfd, temp, n, recv_port, forced_disc;
    struct timeval t;
    struct sockaddr_un cliaddr;
    char serv_node[10], cli_node[10], file_path[40], sendline[1024], recvline[1024], send_ip[20], recv_ip[20];
    struct hostent *host;
    fd_set rset;
    FD_ZERO(&rset);

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

    printf("Before getting the cli_node \n");
    get_client_node(cli_node);
    printf("Client node is %s\n", cli_node);
    
    while(1)
    {
        forced_disc = 0;
        strcpy(serv_node,"");
        printf("Choose a VM from vm1 to vm10 as a server node:\n");
        scanf("%s", serv_node);
        if(strcmp(serv_node,"exit") == 0) {
            break;
        }

        if((host = gethostbyname(serv_node)) == NULL) {
            printf("Invalid input\n");
            continue;
        }

        if(strncmp(serv_node,"vm",2) != 0) {
            printf("Invalid input\n");
            continue;
        }

        printf("The vm selected is %s\n", serv_node);
        strcpy(send_ip,"");
        Inet_ntop(AF_INET, (void*)host->h_addr, send_ip, 20);
        printf("IP of the selected host is %s\n", send_ip);

        strcpy(sendline,"time");        

send_message:
        printf("Client at node %s sending request to server at %s\n", cli_node, serv_node);
        printf("Preparing to send message \n");
        msg_send(sockfd, send_ip, SERV_PORT_NO, sendline, 0);

        FD_SET(sockfd, &rset);
        t.tv_sec = 8;
        t.tv_usec = 0;
        Select(sockfd+1, &rset, NULL, NULL, &t);

        if(FD_ISSET(sockfd, &rset)) {
            msg_recv(sockfd, recvline, recv_ip, &recv_port);
            printf("Client at node %s: received from %s %s\n", cli_node,serv_node, recvline);
        }
        else {
            if(forced_disc == 0) {
                forced_disc = 1;
                printf("Client at node %s: timeout on response from %s\n", cli_node, serv_node);
                goto send_message;
            }
            else {
                printf("Forced discovery unsuccessful \n");
                continue;
            }
        }
    }

    unlink(cliaddr.sun_path);
    exit(0);
}