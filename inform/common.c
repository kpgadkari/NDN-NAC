#include "common.h"

#ifdef __APPLE__&__MATCH__
void get_hw_addr(const char *name, struct hardware *hw) {
	struct ifaddrs * addrs;
	struct ifaddrs * cursor;
	const struct sockaddr_dl * dlAddr;
	
	if (getifaddrs(&addrs) == 0) {
	    cursor = addrs;
	    while (cursor != 0) {
	        if ( (cursor->ifa_addr->sa_family == AF_LINK)
	            && strcmp(name,  cursor->ifa_name)==0 ) {
			//(((const struct sockaddr_dl *) cursor->ifa_addr)->sdl_type == IFT_ETHER) && 
	            dlAddr = (const struct sockaddr_dl *) cursor->ifa_addr;
			if(dlAddr->sdl_type == IFT_ETHER){//check net/if_types.h for details
				hw->hlen = 7;
				hw->hbuf[0] = HTYPE_ETHER;
				memcpy(&hw->hbuf[1], &dlAddr->sdl_data[dlAddr->sdl_nlen], 6);
			}
			if(dlAddr->sdl_type == IFT_FDDI){
				hw->hlen = 17;
				hw->hbuf[0] = HTYPE_FDDI;
				memcpy(&hw->hbuf[1], &dlAddr->sdl_data[dlAddr->sdl_nlen], 16);
			}
	        }
	        cursor = cursor->ifa_next;
	    }
	
	    freeifaddrs(addrs);
	}    
}
#endif

#ifdef linux
void get_hw_addr(const char *name, struct hardware *hw) {
	int sock;
	struct ifreq tmp;
	struct sockaddr *sa;

	if (strlen(name) >= sizeof(tmp.ifr_name)) {
		printf("Device name too long: \"%s\"", name);
	}

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		printf("Can't create socket for \"%s\": %m", name);
	}

	memset(&tmp, 0, sizeof(tmp));
	strcpy(tmp.ifr_name, name);
	if (ioctl(sock, SIOCGIFHWADDR, &tmp) < 0) {
		printf("Error getting hardware address for \"%s\": %m", 
			  name);
	}

	sa = &tmp.ifr_hwaddr;
	switch (sa->sa_family) {
		case ARPHRD_ETHER:
			hw->hlen = 7;
			hw->hbuf[0] = HTYPE_ETHER;
			memcpy(&hw->hbuf[1], sa->sa_data, 6);
			break;
		case ARPHRD_IEEE802:
#ifdef ARPHRD_IEEE802_TR
		case ARPHRD_IEEE802_TR:
#endif /* ARPHRD_IEEE802_TR */
			hw->hlen = 7;
			hw->hbuf[0] = HTYPE_IEEE802;
			memcpy(&hw->hbuf[1], sa->sa_data, 6);
			break;
		case ARPHRD_FDDI:
			hw->hlen = 17;
			hw->hbuf[0] = HTYPE_FDDI;
			memcpy(&hw->hbuf[1], sa->sa_data, 16);
			break;
		default:
			printf("Unsupported device type %ld for \"%s\"",
				  (long int)sa->sa_family, name);
	}

	close(sock);
}
#endif

void get_face_name (char *face_name, char *ip){
	struct ifaddrs *addrs, *iap;
	struct sockaddr_in *sa;
	char buf[32];

	getifaddrs(&addrs);
	for (iap = addrs; iap != NULL; iap = iap->ifa_next) {
		if (iap->ifa_addr && (iap->ifa_flags & IFF_UP) && iap->ifa_addr->sa_family == AF_INET) {
			sa = (struct sockaddr_in *)(iap->ifa_addr);
			inet_ntop(iap->ifa_addr->sa_family, (void *)&(sa->sin_addr), buf, sizeof(buf));
			if (!strcmp(ip, buf)) {
				strcpy(face_name, iap->ifa_name);
			}
		}
	}
	freeifaddrs(addrs);
}

int get_random(){
	srand((int) time(0));
	return random();
}

int readable_time(int fd, int sec){
	fd_set rset;
	struct timeval tv;
    
	FD_ZERO(&rset);
	FD_SET(fd, &rset);
    
	tv.tv_sec = sec;
	tv.tv_usec = 0;

	return (select(fd+1, &rset, NULL, NULL, &tv));
}
