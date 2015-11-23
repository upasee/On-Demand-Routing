#include "odr.h"
#include "msg.h"

void add_port_path_pair(int port, char *file_path) {
    struct port_path_list *pp_new = malloc(sizeof(struct port_path_list));
    pp_new->port = port;
    strcpy(pp_new->path,file_path);
    pp_new->pp_next = NULL;

    if(pp_head != NULL) {
        struct port_path_list *pp_temp = pp_head;
        while(pp_temp->pp_next != NULL) {
            pp_temp = pp_temp->pp_next;
        }
        pp_temp->pp_next = pp_new;
    }
    else {
        pp_head = pp_new;
    }
}

struct routing_table * check_routing_table(char *ip) {
    int vm = get_vm_num(ip);
    if(table[vm-1].index != 0)
    {
        return &table[vm-1];
    }
    return NULL;
}

int get_port_from_file(char *file_path) {
    struct port_path_list *pp_temp = pp_head;
    int port=-1;
    while(pp_temp != NULL) {
        if(strcmp(pp_temp->path,file_path) == 0) {
            port = pp_temp->port;
            break; 
        }
        pp_temp = pp_temp->pp_next;
    }
    if(port == -1) {
        if(eph_port > PORT_MAX)
            eph_port = PORT_MIN;
        port = eph_port;
        eph_port++;
        add_port_path_pair(port, file_path);
    }
    return port;
}

void get_file_path_from_sock(int sockfd, char *file_path) {
    struct sockaddr_un cliaddr;
    int clilen = sizeof(cliaddr);
    Getsockname(sockfd, (struct sockaddr *)&cliaddr, &clilen);
    strcpy(file_path, cliaddr.sun_path);
}

void get_hw_list() {
    struct hwa_info *hwa, *hwahead;
    struct sockaddr *sa;
    struct hw_list *new_hwl;
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
                new_hwl = malloc(sizeof(struct hw_list));
                new_hwl->index = hwa->if_index;
                new_hwl->hw_next = NULL;
                printf("result index is %d\n", new_hwl->index);
                ptr = hwa->if_haddr;
                i = IF_HADDR;
                do {
                    new_hwl->hw_addr[j++] = (*ptr++ & 0xff);
                } while (--i > 0);
            }

            if(hwl_head != NULL) {
                struct hw_list *hwl = hwl_head;
                while(hwl->hw_next != NULL) {
                    hwl = hwl->hw_next;
                }
                hwl->hw_next = new_hwl;
            }
            else {
                hwl_head = new_hwl;
            }
        }
        else if(strcmp(hwa->if_name, "eth0") == 0)
        {
            struct sockaddr *ip = hwa->ip_addr;
            my_ip = Sock_ntop_host(ip, sizeof(*ip));
            printf("ipaddr of eth0 %s\n", my_ip);
        }
    }

    free_hwa_info(hwahead);
}

int send_payload(struct payload *pay) {
    int s;
    s = socket(AF_PACKET, SOCK_RAW, htons(39356));

    printf("socket for payload created\n");

    struct sockaddr_ll socket_address;
    void *buffer = (void *)malloc(ETH_FRAME_LEN);
    unsigned char *etherhead = buffer;
    unsigned char *data = buffer + 14;
    struct ethhdr *eh = (struct ethhdr *)etherhead;
    int send_result = 0;
    int vm_no = get_vm_num(pay->dest_ip);
    struct routing_table *entry = check_routing_table(pay->dest_ip);
    unsigned char dest_mac[6];
    memcpy(dest_mac, entry->next_hop, ETH_ALEN);

    socket_address.sll_family = PF_PACKET;
    socket_address.sll_protocol = htons(39356);
    socket_address.sll_hatype   = ARPHRD_ETHER;
    socket_address.sll_ifindex  = entry->index;
    socket_address.sll_hatype   = ARPHRD_ETHER;
    socket_address.sll_pkttype  = PACKET_OUTGOING;
    socket_address.sll_halen    = ETH_ALEN;

    socket_address.sll_addr[0]  = entry->next_hop[0];
    socket_address.sll_addr[1]  = entry->next_hop[1];
    socket_address.sll_addr[2]  = entry->next_hop[2];
    socket_address.sll_addr[3]  = entry->next_hop[3];
    socket_address.sll_addr[4]  = entry->next_hop[4];
    socket_address.sll_addr[5]  = entry->next_hop[5];
    socket_address.sll_addr[6]  = 0x00;/*not used*/
    socket_address.sll_addr[7]  = 0x00;/*not used*/

    eh->h_proto = htons(39356);

    memcpy((void*)buffer, (void*)dest_mac, ETH_ALEN);

    struct hw_list *entry_mac = (struct hw_list *)malloc(sizeof(struct hw_list));
    get_mac_from_interface(entry->index, entry_mac);

    unsigned char src_mac[6];
    int i;
    for (i =0; i<6; i++)
        src_mac[i] = entry_mac->hw_addr[i];

    socket_address.sll_ifindex  = entry->index;

    memcpy((void*)(buffer+ETH_ALEN), (void*)src_mac, ETH_ALEN);
    memcpy(data, pay, sizeof(struct payload));

    struct payload *msg1 = (struct payload *)data;

    printf("data being sent is %s\n", msg1->buff);

    send_result = sendto(s, buffer, ETH_FRAME_LEN, 0, (struct sockaddr*)&socket_address, sizeof(socket_address));

    printf("after sending, send return: %d\n", send_result);
    return 0;
}

int send_rrep(struct rreq_packet *pkt) {
    int s;
    s = socket(AF_PACKET, SOCK_RAW, htons(39356));

    printf("socket for rrep created\n");

    struct sockaddr_ll socket_address;
    void* buffer = (void *)malloc(ETH_FRAME_LEN);
    unsigned char * etherhead = buffer;
    unsigned char * data = buffer + 14;
    struct ethhdr *eh = (struct ethhdr *)etherhead;
    int send_result = 0;
    int vm_no = get_vm_num(pkt->dest_ip);
    struct routing_table *entry = check_routing_table(pkt->dest_ip);
    unsigned char dest_mac[6];
    memcpy(dest_mac, entry->next_hop, ETH_ALEN);

    socket_address.sll_family = PF_PACKET; 
    socket_address.sll_protocol = htons(39356);
    socket_address.sll_hatype   = ARPHRD_ETHER;
    socket_address.sll_ifindex  = entry->index;
    socket_address.sll_hatype   = ARPHRD_ETHER;
    socket_address.sll_pkttype  = PACKET_OUTGOING;
    socket_address.sll_halen    = ETH_ALEN;

    socket_address.sll_addr[0]  = entry->next_hop[0];
    socket_address.sll_addr[1]  = entry->next_hop[1];
    socket_address.sll_addr[2]  = entry->next_hop[2];
    socket_address.sll_addr[3]  = entry->next_hop[3];
    socket_address.sll_addr[4]  = entry->next_hop[4];
    socket_address.sll_addr[5]  = entry->next_hop[5];
    socket_address.sll_addr[6]  = 0x00;/*not used*/
    socket_address.sll_addr[7]  = 0x00;/*not used*/

    eh->h_proto = htons(39356);

    memcpy((void*)buffer, (void*)dest_mac, ETH_ALEN);

    struct hw_list *entry_mac = (struct hw_list *)malloc(sizeof(struct hw_list));
    get_mac_from_interface(entry->index, entry_mac);

    send_to_interface(s, entry->index, entry_mac->hw_addr, socket_address, buffer, pkt);

}

void get_dg_rreq(int sockfd, struct rreq_packet *pkt) {
    struct sockaddr_un cliaddr;
    int n;
    socklen_t len;
    void *buff1 = malloc(4096);
    char file_path[40];
    struct send_pack *msg1 = malloc(sizeof(struct send_pack));

    len = sizeof(cliaddr);
    printf("ODR: Receiving from client\n");
    n = Recvfrom(sockfd, msg1, MAXLINE, 0, (SA *)&cliaddr, &len);

    get_file_path_from_sock(sockfd, file_path);
    get_port_from_file(file_path);

    printf("message came from client: %s\n", msg1->mesg);
    strcpy(pkt->dest_ip, msg1->ip_addr);
    strcpy(pkt->src_ip, my_ip);
    strcpy(pkt->buff, msg1->mesg);
    pkt->id = 1;
    pkt->hop_count = 1;
}

int  create_client_socket() {
    int sockfd;
    struct sockaddr_un odraddr;

    sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
    unlink(ODR_PATH);

    bzero(&odraddr, sizeof(odraddr));
    odraddr.sun_family = AF_LOCAL;
    strcpy(odraddr.sun_path, ODR_PATH);

    Bind(sockfd, (SA *) &odraddr, sizeof(odraddr));
    return sockfd;
}

void rreq_to_server(struct recv_pack *rpack) {
    int sockfd;
    struct sockaddr_un servaddr, cliaddr;

    sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
    bzero(&cliaddr, sizeof(cliaddr));
    cliaddr.sun_family = AF_LOCAL;
    strcpy(cliaddr.sun_path, tmpnam(NULL));

    Bind(sockfd, (SA *)&cliaddr, sizeof(cliaddr));

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, SERV_PATH);

    printf("msg ip %s and msg %s\n", rpack->ip_addr, rpack->mesg);
    Sendto(sockfd, rpack, sizeof(struct recv_pack), 0, (SA *)&servaddr, sizeof(servaddr));

}

int send_to_interface(int s, int index, unsigned int mac[6], struct sockaddr_ll socket_address, void * buffer, struct rreq_packet *pkt) {
    printf("in ");
    void *data = buffer+14;

    unsigned char src_mac[6];
    int i;
    for (i =0; i<6; i++)
        src_mac[i] = mac[i];

    socket_address.sll_ifindex  = index;

    memcpy((void*)(buffer+ETH_ALEN), (void*)src_mac, ETH_ALEN);
    memcpy(data, pkt, sizeof(struct rreq_packet));

    struct rreq_packet *msg1 = (struct rreq_packet *)data;

    printf("data being sent is %s\n", msg1->buff);

    int send_result;
    send_result = sendto(s, buffer, ETH_FRAME_LEN, 0, (struct sockaddr*)&socket_address, sizeof(socket_address));

    printf("after sending, send return: %d\n", send_result);
    return 0;

}

int send_rreq(struct rreq_packet *pkt) {
    printf("in send\n");
    int s;
    s = socket(AF_PACKET, SOCK_RAW, htons(SERV_PORT_NO));

    printf("socket created!\n");

    struct sockaddr_ll socket_address;
    void* buffer = (void*)malloc(ETH_FRAME_LEN);
    unsigned char* etherhead = buffer;
    unsigned char* data = buffer + 14;
    struct ethhdr *eh = (struct ethhdr *)etherhead;
    int send_result = 0;

    struct hw_list *node = hwl_head;  

    unsigned char dest_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    socket_address.sll_family   = PF_PACKET;
    socket_address.sll_protocol = htons(SERV_PORT_NO);
    socket_address.sll_hatype   = ARPHRD_ETHER;
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

    eh->h_proto = htons(SERV_PORT_NO);
    
    while(node!= NULL)
    {
        printf("Eth index %d\n ", node->index);
        send_to_interface(s, node->index, node->hw_addr, socket_address, buffer, pkt);
        node = node->hw_next;
    }

    return 0;
}

int create_odr_socket() {
    int s;
    
    s = socket(AF_PACKET, SOCK_RAW, htons(SERV_PORT_NO));
    return s;
}

void get_mac_from_interface(int index, struct hw_list *entry) {
    struct hw_list *node = result_head;
    while(node)
    {
        if (index == node->index)
        {
            memcpy(entry, node, sizeof(struct hw_list));
            return;
        }
        node = node->hw_next;
    }
}

int check_mac_are_equal(char *mac1, char *mac2) {
    int i=0;
    for(i=0; i<6; i++)
    {
        if(mac1[1] != mac2[i])
            return -1;
    }
    return 0;
}

int get_vm_num(char *ip) {
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

void convert_mac_to_string(char mac[6]) {
    int i =6;
    char *ptr = mac;
    do {
        printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
    } while (--i > 0);
    printf("\n");
}

int add_to_routing_table(int vm_no, struct rreq_packet *msgrcv, int index, void *buffer) {
    strcpy(table[vm_no-1].ip, msgrcv->src_ip);
    table[vm_no-1].index = index;
    table[vm_no-1].hop_count = msgrcv->hop_count;
    table[vm_no-1].last_broadcast = msgrcv->id;
    memcpy((void *)table[vm_no-1].next_hop, (void *)buffer+ETH_ALEN, ETH_ALEN); 
    return 0;
}

int update_routing_table(int vm_no, struct rreq_packet *msgrcv, int index, void *buffer) {
    table[vm_no-1].index = index;
    table[vm_no-1].hop_count = msgrcv->hop_count;
    table[vm_no-1].last_broadcast = msgrcv->id;
    memcpy((void *)table[vm_no-1].next_hop, (void *)buffer+ETH_ALEN, ETH_ALEN); 
    return 0;
}

void print_routing_table() {
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

int handle_payload(struct payload *pay, void *buffer, struct sockaddr_ll socket_address) {
    printf("IN HANDLE PAYLOAD\n");
    printf("interface is %d\n", socket_address.sll_ifindex);
    int index = socket_address.sll_ifindex;
    struct payload *msgrcv = (struct payload *)(buffer+14);
    struct routing_table *table_entry = check_routing_table(msgrcv->src_ip);

    int vm_no = get_vm_num(msgrcv->src_ip);
    if (strcmp(my_ip, msgrcv->dest_ip) == 0)
    {
        printf("Payload for me\n");
        printf("send to server\n");

        struct recv_pack *rpack = malloc(sizeof(struct recv_pack));
        strcpy(rpack->mesg, pay->buff);
        strcpy(rpack->ip_addr, pay->dest_ip);
        rreq_to_server(rpack);
    }

    else
    {
        printf("not for me\n");
        pay->hop_count = pay->hop_count  +1;
        send_payload(pay);
    }
    print_routing_table();
}

int handle_rrep(struct rreq_packet *pkt, void *buffer, struct sockaddr_ll socket_address) {
    printf("IN HANDLE RREP\n");
    printf("interface is %d\n", socket_address.sll_ifindex);
    int index = socket_address.sll_ifindex;
    struct rreq_packet *msgrcv = (struct rreq_packet *)(buffer+14);
    struct routing_table *table_entry = check_routing_table(msgrcv->src_ip);

    int vm_no = get_vm_num(msgrcv->src_ip);
    if (table_entry == NULL)
    {
        add_to_routing_table(vm_no, msgrcv, index, buffer);
    }
    else
    {
        if(msgrcv->hop_count < table_entry->hop_count)
        {
            update_routing_table(vm_no, msgrcv, index, buffer);
        }
    }
    if (strcmp(my_ip, msgrcv->dest_ip) == 0)
    {
        printf("RREp for me\n");

        struct payload *pay = (struct payload *)malloc(sizeof(struct payload));
        pay->id = 0;
        memcpy(pay->src_ip, msgrcv->dest_ip, 15);
        memcpy(pay->dest_ip, msgrcv->src_ip, 15);
        pay->hop_count = 1;
        pay->payload = 5;
        strcpy(pay->buff, "time");

        send_payload(pay);

    }
    else
    {
        printf("not for me\n");
        pkt->hop_count = pkt->hop_count  +1;
        send_rrep(pkt);
    }
    print_routing_table();

}

int handle_rreq(struct rreq_packet *pkt, void *buffer, struct sockaddr_ll socket_address) {
    printf("interface is %d\n", socket_address.sll_ifindex);
    int index = socket_address.sll_ifindex;
    struct rreq_packet *msgrcv = (struct rreq_packet *)(buffer+14);
    printf("Data received %s\n", msgrcv->buff);
    printf("Dst IP received %s\n", msgrcv->dest_ip);
    printf("my ip %s\n", my_ip);

    if (strcmp(my_ip, msgrcv->src_ip) == 0)
        return;

    int vm_no = get_vm_num(msgrcv->src_ip);

    struct routing_table *table_entry = check_routing_table(msgrcv->src_ip);

    if(table_entry == NULL)
    {
        add_to_routing_table(vm_no, msgrcv, index, buffer);
    }
    else
    {
        if (msgrcv->id == table_entry->last_broadcast)
        {
            if(msgrcv->hop_count < table_entry->hop_count)
            {
                update_routing_table(vm_no, msgrcv, index, buffer);
            }
            else if(msgrcv->hop_count == table_entry->hop_count)
            {
                unsigned char src_mac[6];
                memcpy(src_mac, buffer+ETH_ALEN, ETH_ALEN);
                if(check_mac_are_equal(src_mac, table_entry->next_hop) == 0)
                {   
                    update_routing_table(vm_no, msgrcv, index, buffer);
                    goto printing;
                }
                else
                    return;
            }
            else if (msgrcv->hop_count > table_entry->hop_count)
                return;
        }
        else if (msgrcv->id > table_entry->last_broadcast)
        {
            if (msgrcv->hop_count > table_entry->hop_count)
            {
                table[vm_no-1].last_broadcast = msgrcv->id;
            }
            else if (msgrcv->hop_count < table_entry->hop_count)
            {
                update_routing_table(vm_no, msgrcv, index, buffer);
            }
            else if (msgrcv->hop_count == table_entry->hop_count)
            {
                unsigned char src_mac[6];
                memcpy(src_mac, buffer+ETH_ALEN, ETH_ALEN);
                if(check_mac_are_equal(src_mac, table_entry->next_hop) == 0)
                {
                    table[vm_no-1].last_broadcast = msgrcv->id;
                }
                else if (check_mac_are_equal(src_mac, table_entry->next_hop) != 0)
                {
                    update_routing_table(vm_no, msgrcv, index, buffer);
                }
            }
        }
        else if(msgrcv->id < table_entry->last_broadcast)
            return;

    }

    memcpy(pkt, msgrcv, sizeof(struct rreq_packet));
    printf("before comparing ip\n");
    if (strcmp(my_ip, msgrcv->dest_ip) == 0)
    {
        printf("meant for me\n");
        memcpy(pkt, msgrcv, sizeof(struct rreq_packet));
        printf("ip is %s, msg is %s\n", pkt->dest_ip, pkt->buff);

        struct rreq_packet *rrep = (struct rreq_packet *)malloc(sizeof(struct rreq_packet));
        rrep->id = 0;
        memcpy(rrep->src_ip, msgrcv->dest_ip, 15);
        memcpy(rrep->dest_ip, msgrcv->src_ip, 15);
        rrep->hop_count = 1;
        strcpy(rrep->buff, "got it");
        send_rrep(rrep);

    }
    else
    {
        printf("not meant for me\n");
        pkt->hop_count = pkt->hop_count +1;
        send_rreq(pkt);
    }    
    printf("after comparing ip\n");

printing:
    print_routing_table();

}


int receive_rreq(int s, struct rreq_packet *pkt) {
    printf("in receive\n");
    void* buffer = (void*)malloc(ETH_FRAME_LEN);
    int length = 0;
    printf("blocking on receive\n");
    struct sockaddr_ll socket_address;
    int size = sizeof(struct sockaddr_ll);

    length = Recvfrom(s, buffer, ETH_FRAME_LEN, 0, (struct sockaddr*)&socket_address, &size);
    printf("interface is %d\n", socket_address.sll_ifindex);
    int index = socket_address.sll_ifindex;
    struct rreq_packet *msgrcv = (struct rreq_packet *)(buffer+14);
    printf("Data received %s\n", msgrcv->buff);
    printf("Dst IP received %s\n", msgrcv->dest_ip);
    printf("my ip %s\n", my_ip);

    if (strcmp(my_ip, msgrcv->src_ip) == 0)
        return;

    int vm_no = get_vm_num(msgrcv->src_ip);

    struct routing_table *table_entry = check_routing_table(msgrcv->src_ip);

    if(table_entry == NULL) {
        add_to_routing_table(vm_no, msgrcv, index, buffer);
    }
    else {
        if (msgrcv->id == table_entry->last_broadcast)
        {
            if(msgrcv->hop_count < table_entry->hop_count)
            {
                update_routing_table(vm_no, msgrcv, index, buffer);
            }
            else if(msgrcv->hop_count == table_entry->hop_count)
            {
                unsigned char src_mac[6];
                memcpy(src_mac, buffer+ETH_ALEN, ETH_ALEN);
                if(check_mac_are_equal(src_mac, table_entry->next_hop) == 0)
                {   
                    update_routing_table(vm_no, msgrcv, index, buffer);
                    goto printing;
                }
                else
                    return;
            }
            else if (msgrcv->hop_count > table_entry->hop_count)
                return;
        }
        else if (msgrcv->id > table_entry->last_broadcast)
        {
            if (msgrcv->hop_count > table_entry->hop_count)
            {
                table[vm_no-1].last_broadcast = msgrcv->id;
            }
            else if (msgrcv->hop_count < table_entry->hop_count)
            {
                update_routing_table(vm_no, msgrcv, index, buffer);
            }
            else if (msgrcv->hop_count == table_entry->hop_count)
            {
                unsigned char src_mac[6];
                memcpy(src_mac, buffer+ETH_ALEN, ETH_ALEN);
                if(check_mac_are_equal(src_mac, table_entry->next_hop) == 0)
                {
                    table[vm_no-1].last_broadcast = msgrcv->id;
                }
                else if (check_mac_are_equal(src_mac, table_entry->next_hop) != 0)
                {
                    update_routing_table(vm_no, msgrcv, index, buffer);
                }
            }
        }
        else if(msgrcv->id < table_entry->last_broadcast)
            return;

    }

    memcpy(pkt, msgrcv, sizeof(struct rreq_packet));
    printf("before comparing ip\n");
    if (strcmp(my_ip, msgrcv->dest_ip) == 0)
    {
        printf("meant for me\n");
        memcpy(pkt, msgrcv, sizeof(struct rreq_packet));
        printf("ip is %s, msg is %s\n", pkt->dest_ip, pkt->buff);

        printf("send to server\n");

        struct recv_pack *rpack1 = malloc(sizeof(struct recv_pack));
        strcpy(rpack1->mesg, pkt->buff);
        strcpy(rpack1->ip_addr, pkt->dest_ip);

        rreq_to_server(rpack1);      
    }
    else
    {
        printf("not meant for me\n");
        pkt->hop_count = pkt->hop_count +1;
        send_rreq(pkt);
    }    
    printf("after comparing ip\n");

    printing:
    print_routing_table();


    memcpy(pkt, msgrcv, sizeof(struct rreq_packet));
    printf("before comparing ip\n");
    if (strcmp(my_ip, msgrcv->dest_ip) == 0)
    {
        printf("meant for me\n");
        memcpy(pkt, msgrcv, sizeof(struct rreq_packet));
        printf("ip is %s, msg is %s\n", pkt->dest_ip, pkt->buff);

        printf("send to server\n");

        struct recv_pack *rpack2 = malloc(sizeof(struct recv_pack));
        strcpy(rpack2->mesg, pkt->buff);
        strcpy(rpack2->ip_addr, pkt->dest_ip);

        rreq_to_server(rpack2);      
    }
    else
    {
        printf("not meant for me\n");
        pkt->hop_count = pkt->hop_count +1;
        send_rreq(pkt);
    }    
    printf("after comparing ip\n");
    print_routing_table();

}

int main(int argc, char **argv) {
    struct hwa_info *hw;
    hw = get_hw_addrs();
    prhwaddrs();

    get_hw_list();
    
    table = (struct routing_table *)malloc(10 * sizeof(struct routing_table));
    memset(table, 0, 10*sizeof(struct routing_table));

    int sockfd;
    sockfd = create_client_socket();

    int sockfd_odr;
    sockfd_odr = create_odr_socket();

    add_port_path_pair(SERV_PORT_NO, SERV_PATH);

    fd_set rset;

    FD_ZERO(&rset);

    FD_SET(sockfd, &rset);
    FD_SET(sockfd_odr, &rset);

    int max = max(sockfd, sockfd_odr);

    while(1) {

        Select(max+1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(sockfd, &rset))
        {
            printf("Client select \n");
            struct rreq_packet *pkt = (struct rreq_packet *)malloc(sizeof(struct rreq_packet));
            get_dg_rreq(sockfd, pkt);
            send_rreq(pkt);
        }
        else if(FD_ISSET(sockfd_odr, &rset))
        {
            void* buffer = (void*)malloc(ETH_FRAME_LEN);
            int length = 0;
            printf("blocking on receive\n");
            struct sockaddr_ll socket_address;
            int size = sizeof(struct sockaddr_ll);
            length = Recvfrom(sockfd_odr, buffer, ETH_FRAME_LEN, 0, (struct sockaddr*)&socket_address, &size);
            struct payload *pay = (struct payload *)(buffer + 14);
            if (pay->payload == 5)
            {
                handle_payload(pay, buffer, socket_address);
            }
            else
            {
                struct rreq_packet *pkt = (struct rreq_packet *)(buffer+14);
                if (pkt->id != 0)
                    handle_rreq(pkt, buffer, socket_address);
                else
                    handle_rrep(pkt, buffer, socket_address);
            }

        }
    }
}
