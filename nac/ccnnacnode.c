#include "nac_helper.h"

#define GWPORT "9695"
#define CONFIGFILE "/etc/nac.conf"

struct ndngw{
    char addr[20];
    char port[10];
    struct prefixes *namespace;
    struct ndngw *next;
};

struct mydata{
    struct ndngw *gws;
    char namespace[255];
    //struct prefixes namespace;
    char *myip;
    char *myport;
    struct ccn *h;
};

static void usage(const char *progname)
{
    fprintf(stderr,
            "%s <-f config_file>\n"
            "\n"
            "\t-h displays this help information\n"
            "\t-a your IP\n"
            "\t-p your Port\n"
            "\t-f change the default config file name"
            "\t./etc/nac.conf is read by default if no config file is specified and the node starts as a server\n"
            , progname);
    exit(1);
}

int construct_fib(char *common, struct mydata *md){
    //construct FIB to gw
    struct ccn *h = md->h;
    struct ccn_charbuf *prefix = ccn_charbuf_create();
    int res = ccn_name_from_uri(prefix, common);
    if(res < 0){
        printf("cannot convert prefix into uri\n");
        return 1;
    }

    res = add_new_face(h, prefix, md->gws->addr, GWPORT);
    if(res < 0){
        printf("cannot add new face\n");
        return 2;
    }
    ccn_charbuf_destroy(&prefix);
    return 0;
}

int parse_name(char *buf, int offset, struct mydata *md){
    char common[255];
    int names = buf[offset];//number of namespaces
    offset ++;
    int i = 0;
 
    while(i<names){
        int len = buf[offset];//len of current namespace
        offset ++;
        //common = (char *)malloc(len+1);
        memcpy(common, buf + offset, len);
        offset += len;
        common[len] = '\0';
        //printf("common prefix<%s><%u>\n", common, len);
        construct_fib(common, md);
        i++;
    }

    //construct_fib("ccnx:/", md);
    return 0;
}

/*
int parse_opt0(char *buf, int offset, struct mydata *md){
    char common[255];
    int names = buf[offset];//number of namespaces
    offset ++;
    int i = 0;
 
    while(i<names){
        int len = buf[offset];//len of current namespace
        offset ++;
        //common = (char *)malloc(len+1);
        memcpy(common, buf + offset, len);
        offset += len;
        common[len] = '\0';
        printf("common prefix<%s><%u>\n", common, len);
        //construct_fib(common, md);
        i++;
    }

    construct_fib("ccnx:/", md);
    return 0;
}
*/
struct ndngw *addgw(char *addr, char *port, struct mydata *md){
    //find the place that can add gw
    struct ndngw *pos = md->gws;
    if(pos == NULL){
        pos = (struct ndngw *)malloc(sizeof (struct ndngw));
        pos->next = NULL;
        md->gws = pos;
    }else{
        while(pos->next != NULL)
            pos = pos->next;
        pos->next = (struct ndngw *)malloc(sizeof (struct ndngw));
        pos = pos->next;
        pos->next = NULL;
    }

    strcpy(pos->addr, addr);
    strcpy(pos->port, port);

    //printf("<%s><%s>\n", pos->addr, pos->port);
    return pos;
}

void add_prefix(struct ndngw *p, char *name){
    struct prefixes *pos = p -> namespace;
    if(pos == NULL){
        pos = (struct prefixes *)malloc(sizeof (struct prefixes));
        pos->next = NULL;
        p->namespace = pos;
    }else{
        while(pos->next != NULL)
            pos = pos->next;
        pos->next = (struct prefixes *)malloc(sizeof (struct prefixes));
        pos = pos->next;
        pos->next = NULL;
    }
    strcpy(pos->name, name);
}

void construct_gw(char *gw, struct mydata *md){
    struct ndngw *pos = md->gws;
    while(pos != NULL){
        if(strcmp(pos->addr, gw) == 0){

            struct prefixes *list = pos->namespace;
            while(list != NULL){
                //printf("<%s>\n", list->name);
                construct_fib(list->name, md);
                list = list->next;
            }

            pos = pos->next;
        }
    }
}

void print_gws(struct mydata *md){
    struct ndngw *pos = md->gws;
    while(pos != NULL){
        printf("--<%s><%s>--\n", pos->addr, pos->port);

        struct prefixes *list = pos->namespace;
        while(list != NULL){
            printf("<%s>\n", list->name);
            list = list->next;
        }

        pos = pos->next;
    }
}

int read_config(char * filename, struct mydata *md){
    FILE *cfg;
    char *type, *last, *cp, *namespace, *gw;
    int len, i;
    i = 0;
    int count = 0;
    char buf[1024];
    const char *seps = " \t\n";
    cfg = fopen(filename, "r");
    if(cfg == NULL){
        fprintf(stderr, "opening file %s failed\n", filename);
	exit(1);
    }

    while (fgets((char*)buf, sizeof (buf), cfg)){
        len = strlen(buf);
	if(buf[0] == '#' || len == 0)//skip all lines starts with a #
	    continue;

	if(buf[len -1] == '\n')
	    buf[len - 1] = '\0';

	cp = index(buf, '#');//skip comments
	if(cp != NULL)
	    *cp = '\0';

        type = strtok_r(buf, seps, &last);
	if(type == NULL)//blank line
	    continue;
	if(strcmp(type, "ndn_namespace") != 0 && strcmp(type, "ndn_gateway") != 0){
	    printf("cannot recognize %s\n", type);
	    exit(1);
        }

	for (i=0; type[i]; i++)
   	    type[i] = tolower(type[i]);

	if(strcmp(type, "ndn_namespace") == 0){
	    //only support one name for each user
	    if((namespace = strtok_r(NULL, seps, &last)) != NULL){
                char* tmp = strcpy(md->namespace, namespace);
                if(strlen(tmp) > 0)
                    printf("%s\n", md->namespace);
            }
	}
	else if(strcmp(type, "ndn_gateway") == 0){
	    //printf("gw: <%s>\n", last);
            int count = 0;
            struct ndngw *pos;
	    while((gw = strtok_r(NULL, seps, &last)) != NULL){
                if(count == 0){
                    char *ip, *port;
                    ip = strtok_r(gw,":",&port);
                    if(ip != NULL){
                        pos = addgw(ip, port, md);
                    }
                    //printf("%s %s\n", pos->addr, pos->port);
                }else{
                    //printf("gwName:%s\n", gw); 
                    //add the gw's prefix
                    add_prefix(pos, gw); 
                }
                count ++;
            }
/*
            if((gw = strtok_r(NULL, seps, &last)) != NULL){
                char *ip, *port;
                ip = strtok_r(gw, ":", &port);
                if(ip != NULL){
                    addgw(ip, port, md);
                }
            }
*/
	}
	else{
	    printf("cannot  recognize %s\n", type);
            return -1;
        }

	count ++;
    }
    fclose(cfg);
    return count;
}

int readable_timeo(int fd, int sec){
    fd_set rset;
    struct timeval tv;
    
    FD_ZERO(&rset);
    FD_SET(fd, &rset);
    
    tv.tv_sec = sec;
    tv.tv_usec = 0;

    return (select(fd+1, &rset, NULL, NULL, &tv));
}

void setCltAddrinfo(struct addrinfo *hints){
    memset(hints, 0, sizeof (struct addrinfo));
    hints->ai_family = AF_UNSPEC;
    hints->ai_socktype = SOCK_DGRAM;
}

int main(int argc, char *argv[])
{
    char *gwaddr = "127.0.0.1";
    char *gwport = "39695";
    char *myip = "127.0.0.1";
    char *myport = "9695";
    char *nacfile = CONFIGFILE;
    int sockfd, res;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct pheader *header = NULL;
    struct mydata *md = (struct mydata *)malloc(sizeof (struct mydata));
    
    while ((res = getopt(argc, argv, "a:p:t:f:h")) != -1) {
        switch (res){
            case 'a':
                myip = optarg;
                break;
            case 'p':
                myport = optarg;
                break;
            case 'f':
                nacfile = optarg;
                break;
            case 'h':
            default:
                usage(argv[0]);
        }
    }
    md->gws = NULL;
    md->myport = myport;
    md->myip = myip;

    //setup connection to the ccndaemon
    md->h = ccn_create();
    struct ccn *h = md->h;
    res = ccn_connect(h, NULL);
    if (res < 0) {
        ccn_perror(h, "Cannot connect to ccnd.");
        exit(1);
    }

    //read config
    read_config(nacfile, md);
    print_gws(md);

    //for now, just try the first gw
    gwaddr = md->gws->addr;
    gwport = md->gws->port;
    //printf("dest:<%s:%s>\n", gwaddr,gwport);

    //set the addrinfo struct
    p = &hints;
    setCltAddrinfo(p);

    if ((rv = getaddrinfo(gwaddr, gwport, &hints, &servinfo)) != 0) {
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

    //header 0
    header = get_header(MSGACT);

    //content
    struct pact *act = NULL;
    act = get_pact(md->myip, atoi(md->myport), strlen(md->namespace), UDP);

    //create buffer and save info here
    char *buf = (char *)malloc(MAXBUFLEN);
    int offset = 0;
    //memcpy(buf, header, sizeof (struct pheader));//need to do this after add all the data
    offset += sizeof (struct pheader);
    memcpy(buf+offset, act, sizeof (struct pact));
    offset += sizeof (struct pact);
    memcpy(buf+offset, md->namespace, strlen(md->namespace));
    offset += strlen(md->namespace);

    //cpy the header
    header->length = htons(offset);
    memcpy(buf, header, sizeof (struct pheader));

    if ((numbytes = sendto(sockfd, buf, offset, 0,
             p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }
    //printf("ccnnacnode: sent %d bytes to %s\n", numbytes, gwaddr);

    //receive gw's response
    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof their_addr;
    memset(buf, 0, MAXBUFLEN);

    if(readable_timeo(sockfd, 3) == 0){
        printf("socket timeout, try again\n");
    }else{
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        //parse response of ACT here
        int offset = 0;
        memset(header, 0, sizeof (struct pheader));
        memcpy(header, buf, sizeof (struct pheader));
        offset += sizeof (struct pheader);
        if(is_valid_response(header) == -1){
            printf("recved msg is not valid response of ACT\n");
            goto out;
        }

        if(header->msgType == ACTNACK){
            printf("error code: <%u>\n", buf[offset]);
        }else if (header->msgType == ACTACK){
            //printf("add the common FIBs, print out them in a file\n");
            ///parse options
            //int options = buf[offset];//number of options
            //offset ++;

            struct pack ack_msg;
            memcpy(&ack_msg, buf + offset, sizeof (struct pack));
            offset += sizeof (struct pack);

            struct in_addr gw;
            memcpy(&gw, &(ack_msg.addr), sizeof (struct in_addr));
            //printf("addr:%s, port:%d, protocol %d, name num %d\n", inet_ntoa(gw), ntohs(ack_msg.port), ack_msg.prot, ack_msg.number);

            //int num = buf[offset];//option code
            //offset ++;
            //char *common = NULL;
/*
            if(opt_code == 0){
                parse_opt0(buf, offset, md);
            }
*/
            parse_name(buf, offset, md);
            construct_gw(inet_ntoa(gw), md);
        }
    }

    freeaddrinfo(servinfo);

out:
    free(buf);
    free(header);
    free(act);

    close(sockfd);

    return 0;
}
