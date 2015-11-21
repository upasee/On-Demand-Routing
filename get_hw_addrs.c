#include <errno.h>		/* error numbers */
#include <sys/ioctl.h>          /* ioctls */
#include <net/if.h>             /* generic interface structures */
#include <sys/socket.h>
#include <arpa/inet.h>
#include "hw_addrs.h"
#include <netdb.h>

void get_host_name(char *serv_ip, char * cli_node) {
	struct in_addr ipv4addr;
	struct hostent *host;

	Inet_pton(AF_INET, serv_ip, &ipv4addr);

//	printf("IP address is %s\n",serv_ip);
	host = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET);
//	printf("Host name is %s\n", host2->h_name);
	strcpy(cli_node,host->h_name);
}

void get_client_node(char *cli_node) {

	printf("Entered get_client_node\n");	
	int result=0; 
	struct hwa_info	*hwa, *hwahead;
	struct sockaddr	*sa;
	char ip_addr[30];

	for (hwahead = hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {
		if(strcmp(hwa->if_name,"eth0") == 0) {
			strcpy(ip_addr,"");
			if ((sa = hwa->ip_addr) != NULL) {
				strcpy(ip_addr,Sock_ntop_host(sa, sizeof(*sa)));
			}
			struct in_addr ipv4addr;
			Inet_pton(AF_INET, ip_addr, &ipv4addr);
			get_host_name(ip_addr, cli_node);
		}
	}

	free_hwa_info(hwahead);
}

struct hwa_info *
get_hw_addrs()
{
	struct hwa_info	*hwa, *hwahead, **hwapnext;
	int		sockfd, len, lastlen, alias, nInterfaces, i;
	char		*ptr, *buf, lastname[IF_NAME], *cptr;
	struct ifconf	ifc;
	struct ifreq	*ifr, *item, ifrcopy;
	struct sockaddr	*sinptr;

	sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

	lastlen = 0;
	len = 100 * sizeof(struct ifreq);	/* initial buffer size guess */
	for ( ; ; ) {
		buf = (char *) malloc(len);
		ifc.ifc_len = len;
		ifc.ifc_buf = buf;
		if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0) {
			if (errno != EINVAL || lastlen != 0)
				err_sys("ioctl error");
		} else {
			if (ifc.ifc_len == lastlen)
				break;		/* success, len has not changed */
				lastlen = ifc.ifc_len;
		}
		len += 10 * sizeof(struct ifreq);	/* increment */
		free(buf);
	}

	hwahead = NULL;
	hwapnext = &hwahead;
	lastname[0] = 0;

	ifr = ifc.ifc_req;
	nInterfaces = ifc.ifc_len / sizeof(struct ifreq);
	for(i = 0; i < nInterfaces; i++)  {
		item = &ifr[i];
		alias = 0; 
		hwa = (struct hwa_info *) Calloc(1, sizeof(struct hwa_info));
		memcpy(hwa->if_name, item->ifr_name, IF_NAME);		/* interface name */
		hwa->if_name[IF_NAME-1] = '\0';
				/* start to check if alias address */
		if ( (cptr = (char *) strchr(item->ifr_name, ':')) != NULL)
			*cptr = 0;		/* replace colon will null */
			if (strncmp(lastname, item->ifr_name, IF_NAME) == 0) {
				alias = IP_ALIAS;
			}
			memcpy(lastname, item->ifr_name, IF_NAME);
			ifrcopy = *item;
		*hwapnext = hwa;		/* prev points to this new one */
		hwapnext = &hwa->hwa_next;	/* pointer to next one goes here */

		hwa->ip_alias = alias;		/* alias IP address flag: 0 if no; 1 if yes */
			sinptr = &item->ifr_addr;
			hwa->ip_addr = (struct sockaddr *) Calloc(1, sizeof(struct sockaddr));
	        memcpy(hwa->ip_addr, sinptr, sizeof(struct sockaddr));	/* IP address */
			if (ioctl(sockfd, SIOCGIFHWADDR, &ifrcopy) < 0)
                          perror("SIOCGIFHWADDR");	/* get hw address */
				memcpy(hwa->if_haddr, ifrcopy.ifr_hwaddr.sa_data, IF_HADDR);
			if (ioctl(sockfd, SIOCGIFINDEX, &ifrcopy) < 0)
                          perror("SIOCGIFINDEX");	/* get interface index */
				memcpy(&hwa->if_index, &ifrcopy.ifr_ifindex, sizeof(int));
		}
		free(buf);
	return(hwahead);	/* pointer to first structure in linked list */
	}

	void
	free_hwa_info(struct hwa_info *hwahead)
	{
		struct hwa_info	*hwa, *hwanext;

		for (hwa = hwahead; hwa != NULL; hwa = hwanext) {
			free(hwa->ip_addr);
		hwanext = hwa->hwa_next;	/* can't fetch hwa_next after free() */
		free(hwa);			/* the hwa_info{} itself */
		}
	}
/* end free_hwa_info */

	struct hwa_info *
	Get_hw_addrs()
	{
		struct hwa_info	*hwa;

		if ( (hwa = get_hw_addrs()) == NULL)
			err_quit("get_hw_addrs error");
		return(hwa);
	}

