#include "common.h"

#define DHCPACK 5
#define DHCPINFORM 8
#define INTERFACENAME 255
#define DHO_NDN_GATEWAY 246
#define DHO_NDN_NAMESPACE 247

#define MAXRETRYTIMES 3
#define TIMEOUT 6
#define RESULTFILE "nac.conf"

u_int32_t my_xid;//transaction id

struct ndn_data{
	char domain[255];
	struct in_addr gw_addr;
	uint16_t gw_port;
	char name[255];
};

void usage(char *program){
	printf("usage:%s <-a your_ip> <-s server_ip> [-h display_help_info]\n", program);
	exit(1);
}

void save_para(struct ndn_data *mydata){
	//printf("domain:%s; gw:<%s:%d>;name:%s\n", mydata->domain, inet_ntoa(mydata->gw_addr), ntohs(mydata->gw_port), mydata->name);
	FILE *fp = NULL;
	fp = fopen(RESULTFILE, "w");
	if(fp == NULL){
		printf("Error in opening file %s\n", RESULTFILE);
	}

	fprintf(fp, "ndn_gateway %s:%d\n", inet_ntoa(mydata->gw_addr), ntohs(mydata->gw_port));
	fprintf(fp, "ndn_namespace ccnx:/%s/%s\n", mydata->domain, mydata->name);

	fclose(fp);
}

int parse_options(struct dhcp_packet *packet){
	int flag = -1;//successful?
	int flag_gateway = 0;//indicating whether we get the NDN option
	int flag_name = 0;//indicating whether we get the domain option
	unsigned char options[DHCP_MAX_OPTION_LEN];
	struct ndn_data inform_reply;
	memcpy(options, packet->options, DHCP_MAX_OPTION_LEN);

	//I only care 246 and Domain name
	//the first 4 is the magic cookie
	int index = 4;
	unsigned code, l;//l is the length of the value
	while(index < DHCP_MAX_OPTION_LEN - 1){//the last one is the END(255)
		code = options[index];
		if(code == DHO_END)//this is the end
			break;

		index ++;
		l = options[index];
		index ++;

		if(code == DHO_NDN_NAMESPACE){//namespace
			char *name = inform_reply.name;
			memcpy(name, options + index, l);
			name[l] = '\0';
			//printf("domain: %s\n", domain);
			flag_name = 1;
		}

		if(code == DHO_NDN_GATEWAY){//ndn, now the format must be (address type(1), address(4~), port(2), name(variable))
			unsigned char ndn[255];
			int offset = 0;
			memcpy(ndn, options + index, l);
			unsigned addr_type = ndn[offset];
			offset ++;

			if(addr_type == 1){//v4 addr 
				struct in_addr *gw_addr = &(inform_reply.gw_addr);
				memcpy(gw_addr, ndn + offset, 4);//addr				
				//printf("gw addr %s\n", inet_ntoa(gw_addr));
				offset += 4;

				uint16_t gw_port;
				memcpy(&gw_port, ndn + offset, 2);//port
				//printf("gw port %d\n", ntohs(gw_port));
				inform_reply.gw_port = gw_port;
				offset += 2;

				char *domain = inform_reply.domain;
				memcpy(domain, ndn + offset, l - offset);
				domain[l - offset] = '\0';
				//printf("name is %s\n", name);

				flag_gateway = 1;
			}else if(addr_type == 2){//v6 addr
				//update latter
			}else{
				printf("unknown addr type\n");
				break;
			}
		}

		index = index + l;//after the last option
	}

	if(flag_gateway == 1 && flag_name == 1){
		//save the parameters in a file
		save_para(&inform_reply);
		flag = 0;
	}

	return flag;
}

//this func is only for packet not more than MAX
int is_valid_reply(struct dhcp_packet *packet, int len){
	//if the transaction ID is wrong, or it is not the reply, it is invalid
	if(packet->xid != my_xid || packet->op != BOOTREPLY)
		return -1;

	//if the msgType is not ACK, it is invalid
	unsigned char options[DHCP_MAX_OPTION_LEN];
	memcpy(options, packet->options, DHCP_MAX_OPTION_LEN);

	//parse options here
	int opt_len = len - DHCP_FIXED_NON_UDP;
	//the first 4 is the magic cookie
	int index = 4;
	u_int8_t code, l;
	while(index < opt_len - 1){//the last one is the END(255)
		code = options[index];
		if(code == DHO_END)//this is the end
			return -1;

		index ++;
		l = options[index];
		index ++;

		if(code == DHO_DHCP_MESSAGE_TYPE && options[index] == DHCPACK)
			return 0;//this is the valid msg

		index = index + l;//after the last option
	}
	return -1;
}

void init_packet(struct dhcp_packet *packet){
	memset(packet, 0, sizeof (struct dhcp_packet));
}

void set_sock_opt(int sockfd){
	int broadcast = 1, reuse = 1;
	// this call is what allows broadcast packets to be sent:
	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast,
		sizeof broadcast) == -1) {
		perror("setsockopt (SO_BROADCAST)");
		exit(1);
	}

	// this call is what allows reuse port:
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse,
		sizeof reuse) == -1) {
		perror("setsockopt (SO_REUSEADDR)");
		exit(1);
	}

	//bind to the port, only for v4 now
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(CLTPORT);
	sin.sin_addr.s_addr = INADDR_ANY;
	if( -1 == bind(sockfd, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) ){
		printf("bind error\n");
		exit(1);
	}
}

void make_common_parts(struct dhcp_packet *packet, char *myip){
	int rv;
	//get the hardware address by ip
	char face_name[INTERFACENAME];
	get_face_name(face_name, myip);
	struct hardware myhw;
	get_hw_addr(face_name, &myhw);

	//assign value
	packet->op = BOOTREQUEST;
	packet->htype = myhw.hbuf[0]; //assuming it is a ethernet card
	packet->hlen = myhw.hlen - 1;//assuming it is ethernet card
	packet->hops = 0;
	packet->xid = my_xid;
	packet->secs = 0;
	packet->flags = 0;//assuming this interface can use unicast way, but we need another mechanism to test, and then set it
	memset(&(packet->ciaddr), 0, sizeof (packet->ciaddr));
	
	//assign my current ip
	struct in_addr ciaddr;
	if((rv = inet_aton(myip, &ciaddr)) == 0){
		printf("cannot convert your ip");
		exit(1);
	}
	packet->ciaddr = ciaddr;

	//to us, those parts are 0
	memset(&(packet->yiaddr), 0, sizeof (packet->yiaddr));
	memset(&(packet->siaddr), 0, sizeof (packet->siaddr));
	memset(&(packet->giaddr), 0, sizeof (packet->giaddr));

	//cpy the hardware address
	memset(&(packet->chaddr), 0, sizeof (packet->chaddr));
	if(myhw.hlen > 0)
		memcpy(&(packet->chaddr), &(myhw.hbuf[1]), (unsigned)(myhw.hlen - 1));
}

int make_options(struct dhcp_packet *packet){
	int offset = 0;
	//construct options here
	unsigned char options[DHCP_MAX_OPTION_LEN];
	
	//cpy the magic cookie(start of options)
	memcpy(options, DHCP_OPTIONS_COOKIE, 4);
	offset += 4;

	//option 53, msg Type, we must contain this one
	struct option_header oh;
	oh.code = DHO_DHCP_MESSAGE_TYPE;
	oh.len = 1;
	memcpy(options + offset, &oh, sizeof (struct option_header));
	offset += sizeof (struct option_header);
	options[offset] = DHCPINFORM; 
	offset ++;

	//option 55, request list
	memset(&oh, 0, sizeof (struct option_header));
	oh.code = DHO_DHCP_PARAMETER_REQUEST_LIST;
	oh.len = 2;//right now, we only need domain name and the 246
	memcpy(options + offset, &oh, sizeof (struct option_header));
	offset += sizeof (struct option_header);
	options[offset] = DHO_NDN_NAMESPACE;//this one should be the NDN option
	offset ++;
	options[offset] = DHO_NDN_GATEWAY;
	offset ++;

	//the end of options
	options[offset] = DHO_END;
	offset ++;
	memcpy(packet->options, options, offset);
	
	return offset;
}

int main(int argc, char *argv[])
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv, retry = 0;
	int numbytes;
	char *myip = NULL, *srv = NULL;
	char buf[MAXBUFLEN];

	while ((rv = getopt(argc, argv, "a:s:h")) != -1) {
        	switch (rv){
	            case 'a':
        	        myip = optarg;
                	break;
	            case 's':
        	        srv = optarg;
                	break;
	            case 'h':
        	    default:
                	usage(argv[0]);
	        }
   	 }
	if (argc != 5 || myip == NULL || srv == NULL) {
		usage(argv[0]);
	}

	//generate the transaction no.
	my_xid = get_random();

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(srv, SERVERPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: failed to bind socket\n");
		return 2;
	}

	//let the socket can listening on 68, and broadcast
	set_sock_opt(sockfd);

	//create the dhcp packet structure
	struct dhcp_packet out_packet;
	init_packet(&out_packet);

	//make the common parts, such as xid, ciadr et al.
	make_common_parts(&out_packet, myip);
	
	//make the options part, but right now only support domain name and private 246
	int offset = make_options(&out_packet);

	//DHCP_FIXED_NON_UDP includes all the field from op to the end of file, offset is the length of options
	int msg_len = DHCP_FIXED_NON_UDP + offset;
redo:
	memset(buf, 0, MAXBUFLEN);
	memcpy(buf, &out_packet, msg_len);
	//send packet
	if ((numbytes = sendto(sockfd, buf, msg_len, 0, p->ai_addr, p->ai_addrlen)) == -1) {
		perror("inform: sendto");
		close(sockfd);
		exit(1);
	}

	//read from remote server, maybe we need to get its addr for the latter use
	struct sockaddr_storage their_addr;
	socklen_t addr_len = sizeof their_addr;
wait:
	if(readable_time(sockfd, TIMEOUT) == 0){//need to add retry
		printf("timeout, try again\n");
		if(retry < MAXRETRYTIMES){
			retry ++;
			goto redo;
		}
	}else{
	   	memset(buf, 0, MAXBUFLEN);
		if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN, 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
			perror("inform: recvfrom");
			close(sockfd);
			exit(1);
		}
		struct dhcp_packet in_packet;
		init_packet(&in_packet);
		memcpy(&in_packet, buf, numbytes);

		//check if this message is the right ACK, on the same port, dhclient may send out request and get ack or offer
		if((rv = is_valid_reply(&in_packet, numbytes)) == 0){
			rv = parse_options(&in_packet);
			if(rv == -1)
				goto wait;
		}
		else{
			printf("this msg is not valid\n");//need to wait the valid one
			goto wait;
		}
	}
	
out:
	freeaddrinfo(servinfo);
	close(sockfd);

	return 0;
}
