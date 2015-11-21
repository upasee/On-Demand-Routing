#include "unp.h"
#include "hw_addrs.h"
#include <sys/socket.h>
#include <netinet/if_ether.h>
#include <linux/if_packet.h>


//char mesg[MAXLINE];
void *mesg;
//char buff[ETH_FRAME_LEN];
void *buff;
char *my_ip;
struct routing_table *table;

struct hw_list *result_head;
struct hw_list {
    int index;
    unsigned int hw_addr[6];
    struct hw_list *hw_next;
};

struct message{
    int id;
    char ip[15];
    char buff[10];
};

struct rreq_packet{
    int id;
    char src_ip[15];
    char dest_ip[15];
    char buff[10];
    int hop_count;
};

struct routing_table{
    char ip[15];
    int index;
    char next_hop[6];
    int hop_count;

};

void get_hw_list() {
    struct hwa_info *hwa, *hwahead;
    struct sockaddr *sa;
    struct hw_list *result;
    int i, j, prflag;
    char *ptr;

    for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {
        i=0;
        j=0;
        prflag=0;
        if((strcmp(hwa->if_name,"eth0") != 0) && (strcmp(hwa->if_name,"lo") != 0)) {
            printf("hw name is %s\n\n", hwa->if_name);

            do {
                if (hwa->if_haddr[i] != '\0') {
                    prflag = 1;
                    break;
                }
            } while (++i < IF_HADDR);

            if(prflag == 1) {           
                result = malloc(sizeof(struct hw_list));
                result->index = hwa->if_index;
                result->hw_next = NULL;
                printf("result index is %d\n", result->index);
                ptr = hwa->if_haddr;
                i = IF_HADDR;
                do {
                    result->hw_addr[j++] = (*ptr++ & 0xff);
                } while (--i > 0);
            }

            if(result_head != NULL) {
                struct hw_list *hwl = result_head;
                while(hwl->hw_next != NULL) {
                    hwl = hwl->hw_next;
                }
                hwl->hw_next = result;
            }
            else {
                result_head = result;
            }
        }
        else if(strcmp(hwa->if_name, "eth0") == 0)
        {
            struct sockaddr *ip = hwa->ip_addr;
            my_ip = Sock_ntop_host(ip, sizeof(*ip));
            printf("ipaddr of eth0 %s\n", my_ip);
            //            printf("ipaddr of eth0 %s\n", Sock_ntop_host(ip, sizeof(*ip)));
        }
    }

    free_hwa_info(hwahead);
} 




void get_dg_rreq(int sockfd, SA *pcliaddr, socklen_t clilen, struct rreq_packet *pkt)
{
    int n;
    socklen_t len;
    //    char mesg[MAXLINE];
    //    mesg = (void *)malloc(MAXLINE);
    char buff[4096];
    time_t ticks;
    struct timeval *tm;

    //    for ( ; ; ) {
    len = clilen;
    n = Recvfrom(sockfd, mesg, MAXLINE, 0, pcliaddr, &len);
    struct message *msg =  (struct message *)mesg;
    printf("message ip %s and data %s\n", msg->ip, msg->buff);
    //    struct rreq_packet *pkt = (struct rreq_packet *)malloc(sizeof(struct rreq_packe));
    strcpy(pkt->dest_ip, msg->ip);
    strcpy(pkt->src_ip, my_ip);
    strcpy(pkt->buff, msg->buff);
    pkt->id = msg->id;
    pkt->hop_count = 1;
    //        return mesg;

    //        ticks = time(NULL);
    //        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));

    //        Sendto(sockfd, buff, strlen(buff), 0, pcliaddr, len);
    //    }
}

void send_dg_rreq(FILE *fp, int sockfd, const SA *pservaddr, socklen_t servlen, struct message *msg_srv)
{
    //    char sendline[MAXLINE];
    void *sendline;

    sendline =(void *)malloc(sizeof(struct message));

    //    while (Fgets(sendline, MAXLINE, fp) != NULL) {

    //    strcpy(sendline, buff);
    memcpy(sendline, msg_srv, sizeof(struct message));

    struct message *msg = (struct message *)sendline;
    printf("msg ip %s and msg %s\n", msg->ip, msg->buff);
    Sendto(sockfd, sendline, sizeof(struct message), 0, pservaddr, servlen);

    //        n = Recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);
    //        recvline[n] = 0;

    //        Fputs(recvline, stdout);
    //    }

}


int  rreq_from_client()
{
    int sockfd;
    //    struct sockaddr_un servaddr, cliaddr;
    struct sockaddr_un servaddr;

    sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
    unlink("/tmp/nargis");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, "/tmp/nargis");

    Bind(sockfd, (SA *) &servaddr, sizeof(servaddr));

    //    get_dg_rreq(sockfd, (SA *) &cliaddr, sizeof(cliaddr));
    return sockfd;

}

void rreq_to_server(struct message *msg_srv)
{
    int sockfd;
    struct sockaddr_un servaddr, cliaddr;

    sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
    bzero(&cliaddr, sizeof(cliaddr));
    cliaddr.sun_family = AF_LOCAL;
    strcpy(cliaddr.sun_path, tmpnam(NULL));

    Bind(sockfd, (SA *)&cliaddr, sizeof(cliaddr));

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, "/tmp/nargis_serv");

    send_dg_rreq(stdin, sockfd, (SA *)&servaddr, sizeof(servaddr), msg_srv);
}

int send_to_interface(int s, int index, unsigned int mac[6], struct sockaddr_ll socket_address, void * buffer, struct rreq_packet *pkt)
{

    //    unsigned char* data = buffer + 14;

    void *data = buffer+14;

    unsigned char src_mac[6];
    int i;
    for (i =0; i<6; i++)
        src_mac[i] = mac[i];

    socket_address.sll_ifindex  = index;

    memcpy((void*)(buffer+ETH_ALEN), (void*)src_mac, ETH_ALEN);


    memcpy(data, pkt, sizeof(struct rreq_packet));
    //    memcpy(data, mesg, sizeof(struct message));
    //    strcpy(data, mesg);

    struct rreq_packet *msg1 = (struct rreq_packet *)data;

    printf("data being sent is %s\n", msg1->buff);

    int send_result;
    send_result = sendto(s, buffer, ETH_FRAME_LEN, 0, (struct sockaddr*)&socket_address, sizeof(socket_address));

    printf("after sending, send return: %d\n", send_result);
    return 0;

}

int send_rreq(struct rreq_packet *pkt)
{
    printf("in send\n");
    int s;
    s = socket(AF_PACKET, SOCK_RAW, htons(39356));

    printf("socket created!\n");

    struct sockaddr_ll socket_address;
    void* buffer = (void*)malloc(ETH_FRAME_LEN);
    unsigned char* etherhead = buffer;
    unsigned char* data = buffer + 14;
    struct ethhdr *eh = (struct ethhdr *)etherhead;
    int send_result = 0;

    //    get_hw_list();


    struct hw_list *node = result_head;  


    /*our MAC address*/
    //    unsigned char src_mac[6] = {0x00, 0x0C, 0x29, 0x49, 0x3F, 0x65};

    /*other host MAC address*/
    //    unsigned char dest_mac[6] = {0x00, 0x0C, 0x29, 0xD9, 0x08, 0xF6};
    unsigned char dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    socket_address.sll_family   = PF_PACKET;

    socket_address.sll_protocol = htons(39356);

    //    socket_address.sll_ifindex  = 3;

    socket_address.sll_hatype   = ARPHRD_ETHER;

    //    socket_address.sll_pkttype  = PACKET_OUTGOING;
    socket_address.sll_pkttype  = PACKET_BROADCAST;

    socket_address.sll_halen    = ETH_ALEN; 

    socket_address.sll_addr[0]  = 0xFF;     
    socket_address.sll_addr[1]  = 0xFF;     
    socket_address.sll_addr[2]  = 0xFF;
    socket_address.sll_addr[3]  = 0xFF;
    socket_address.sll_addr[4]  = 0xFF;
    socket_address.sll_addr[5]  = 0xFF;
    socket_address.sll_addr[6]  = 0x00;/*not used*/
    socket_address.sll_addr[7]  = 0x00;/*not used*/


    memcpy((void*)buffer, (void*)dest_mac, ETH_ALEN);
    //    memcpy((void*)(buffer+ETH_ALEN), (void*)src_mac, ETH_ALEN);

    eh->h_proto = htons(39356);

    //    strcpy(data, mesg);
    //    printf("After forming data\n");
    //    printf("data being sent is %s\n", data+8);

    while(node!= NULL)
    {
        printf("Eth index %d\n ", node->index);
        send_to_interface(s, node->index, node->hw_addr, socket_address, buffer, pkt);
        node = node->hw_next;
    }

    //    send_result = sendto(s, buffer, ETH_FRAME_LEN, 0, (struct sockaddr*)&socket_address, sizeof(socket_address));

    return 0;
}

int create_odr_socket()
{
    int s;
    s = socket(AF_PACKET, SOCK_RAW, htons(39356));
    return s;
}

int get_vm_num(char *ip)
{
    struct in_addr ipv4addr;
    Inet_pton(AF_INET, ip, &ipv4addr);

    printf("After inet\n");
    struct hostent *host = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET);
    printf("Host name is %s\n", host->h_name);
    char *num_str = host->h_name;
    char str[4];
    strcpy(str, num_str+2);
    printf("Host no %s\n", str);
    int num = atoi(str);
    printf("Host no %d\n", num);
    return num;
}

void convert_mac_to_string(char mac[6])
{
    int i =6;
    char *ptr = mac;
    do {
        printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
    } while (--i > 0);
    printf("\n");
}


int add_to_routing_table(int vm_no, struct rreq_packet *msgrcv, int index, void *buffer)
{
    strcpy(table[vm_no-1].ip, msgrcv->src_ip);
    table[vm_no-1].index = index;
    table[vm_no-1].hop_count = msgrcv->hop_count;
    memcpy((void *)table[vm_no-1].next_hop, (void *)buffer, ETH_ALEN); 
    return 0;
}

void print_routing_table()
{
    int i;
    printf("src vm name----------src vm ip----------index-------------hop_count--------next hop\n");
    for(i=0; i<10; i++)
    {
        if(table[i].index != 0)
        {
            printf("%d---------------%s-----------------%d-----------------%d-------------",i+1, table[i].ip, table[i].index, table[i].hop_count);
            convert_mac_to_string(table[i].next_hop);
        }
    }
}

int receive_rreq(int s, struct rreq_packet *pkt)
{
    printf("in receive\n");
    //    int s;
    //    s = socket(AF_PACKET, SOCK_RAW, htons(39356));
    //    printf("socket created\n");
    void* buffer = (void*)malloc(ETH_FRAME_LEN);
    int length = 0;
    printf("blocking on receive\n");
    struct sockaddr_ll socket_address;
    int size = sizeof(struct sockaddr_ll);
    //    length = recvfrom(s, buffer, ETH_FRAME_LEN, 0, NULL, NULL);
    length = Recvfrom(s, buffer, ETH_FRAME_LEN, 0, (struct sockaddr*)&socket_address, &size);
    printf("interface is %d\n", socket_address.sll_ifindex);
    int index = socket_address.sll_ifindex;
    struct rreq_packet *msgrcv = (struct rreq_packet *)(buffer+14);
    printf("Data received %s\n", msgrcv->buff);
    printf("Dst IP received %s\n", msgrcv->dest_ip);
    printf("my ip %s\n", my_ip);

    int vm_no = get_vm_num(msgrcv->src_ip);

    add_to_routing_table(vm_no, msgrcv, index, buffer);


    memcpy(pkt, msgrcv, sizeof(struct rreq_packet));
    printf("before comparing ip\n");
    if (strcmp(my_ip, msgrcv->dest_ip) == 0)
    {
        printf("meant for me\n");
        //        strcpy(buff, (char *)buffer + 14);
        memcpy(pkt, msgrcv, sizeof(struct rreq_packet));
        printf("ip is %s, msg is %s\n", pkt->dest_ip, pkt->buff);

        printf("send to server\n");

        struct message *msgsrv = (struct message *)malloc(sizeof(struct message));
        msgsrv->id = pkt->id;
        strcpy(msgsrv->buff, pkt->buff);
        strcpy(msgsrv->ip, pkt->dest_ip);

        rreq_to_server(msgsrv);      
    }
    else
    {
        printf("not meant for me\n");
        //        memcpy(mesg, msgrcv, sizeof(struct message));
        struct rreq_packet *mg = (struct rreq_packet *)mesg;
        printf("ip is %s, msg is %s\n", mg->dest_ip, mg->buff);
        pkt->hop_count = pkt->hop_count +1;
        send_rreq(pkt);
    }    
    printf("after comparing ip\n");

    print_routing_table();

    //    strcpy(buff, (char *)buffer + 14);
}



int main(int argc, char **argv)
{
    struct hwa_info *hw;
    hw = get_hw_addrs();
    prhwaddrs();

    get_hw_list();
    mesg = (void *)malloc(sizeof(struct message));
    buff = (void *)malloc(sizeof(struct message));

    table = (struct routing_table *)malloc(10 * sizeof(struct routing_table));


    printf("before memset\n");
    memset(table, 0, 10*sizeof(struct routing_table));
    printf("after memset\n");

    int sockfd;
    sockfd = rreq_from_client();

    int sockfd_odr;
    sockfd_odr = create_odr_socket();

    fd_set rset;

    FD_ZERO(&rset);

    FD_SET(sockfd, &rset);
    FD_SET(sockfd_odr, &rset);

    int max = max(sockfd, sockfd_odr);

    printf("before select\n");

    Select(max+1, &rset, NULL, NULL, NULL);

    if (FD_ISSET(sockfd, &rset))
    {
        struct sockaddr_un cliaddr;
        struct rreq_packet *pkt = (struct rreq_packet *)malloc(sizeof(struct rreq_packet));
        get_dg_rreq(sockfd, (SA *) &cliaddr, sizeof(cliaddr), pkt);
        send_rreq(pkt);
    }
    else if(FD_ISSET(sockfd_odr, &rset))
    {
        printf("here\n");
        struct rreq_packet *pkt = (struct rreq_packet *)malloc(sizeof(struct rreq_packet));
        receive_rreq(sockfd_odr, pkt);
    }
    /*
       rreq_from_client();
       send_rreq();
       receive_rreq();
       rreq_to_server();
     */
}

