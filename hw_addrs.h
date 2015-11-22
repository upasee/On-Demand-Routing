/* Our own header for the programs that need hardware address info. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#define	IF_NAME		16	/* same as IFNAMSIZ    in <net/if.h> */
#define	IF_HADDR	 6	/* same as IFHWADDRLEN in <net/if.h> */

#define	IP_ALIAS  	 1	/* hwa_addr is an alias */

struct hw_list *hwl_head;

struct hwa_info {
  char    if_name[IF_NAME];	/* interface name, null terminated */
  char    if_haddr[IF_HADDR];	/* hardware address */
  int     if_index;		/* interface index */
  short   ip_alias;		/* 1 if hwa_addr is an alias IP address */
  struct  sockaddr  *ip_addr;	/* IP address */
  struct  hwa_info  *hwa_next;	/* next of these structures */
};

struct hw_list {
	int index;
	unsigned int hw_addr[6];
	struct hw_list *hw_next;
};

/* function prototypes */
struct hwa_info	*get_hw_addrs();
struct hwa_info	*Get_hw_addrs();
void	free_hwa_info(struct hwa_info *);
void prhwaddrs();
void get_hw_list();
void get_client_node(char *cli_node);
