#include "unp.h"
#include "hw_addrs.h"
#include <sys/socket.h>
#include <netinet/if_ether.h>
#include <linux/if_packet.h>

struct message{
    int id;
    char ip[15];
    char buff[10];
};

struct port_path_list {
    int port;
    char path[40];
    struct port_path_list *pp_next;
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
    int last_broadcast;

};

char *my_ip;
struct routing_table *table;
struct port_path_list *pp_head;

int eph_port = 1537;