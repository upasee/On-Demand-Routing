#include "unp.h"

#define SERV_PORT_NO	5526
#define SERV_PATH	"/tmp/nargis"
#define ODR_PATH	"/tmp/upasi"

struct send_pack {
    char ip_addr[20];
    char mesg[1024];
    int dest_port;
    int flag;
};

struct recv_pack{
	char ip_addr[10];
	int dest_port;
	char mesg[500];
};

int msg_send(int sockfd, char *ip_addr, int dest_port, char *mesg, int flag);
int msg_recv(int sockfd, char *mesg, char *ip_addr, int *dest_port);