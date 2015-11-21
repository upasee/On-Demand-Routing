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

//char mesg[MAXLINE];
void *mesg;
//char buff[ETH_FRAME_LEN];
void *buff;
char *my_ip;
struct routing_table *table;

struct hw_list *hwl_head;
