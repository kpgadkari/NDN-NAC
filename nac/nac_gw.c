#include "nac_helper.h"

#define CCNPORT 9695

static char *hex = "0123456789abcdef";
//the common namespace
const char *common = "ccnx:/ndn/boradcast";

struct user_info{
    char address[20];
    char port[10];
    struct ccn_charbuf *name_prefix;
    struct user_info *next;
};

struct mydata{
    struct user_info *users_list;
    struct prefixes *broadcast_list;
    struct prefixes *prefix_list;
    int prefix_num;
    struct ccn *h;
};

void print_prefixes(struct mydata *md){
    printf("print prefixes:\n");
    struct prefixes *plist = md->broadcast_list;
    while( plist != NULL ){//to the tail, won't add redunant prefix
        printf("<%s>\n", plist->name);
        plist = plist->next;
    }
}

int add_newprefix(struct mydata *md, char *new){
    struct prefixes *plist = md->broadcast_list;
    struct prefixes *last = NULL;

    if(plist == NULL){
        plist = (struct prefixes *)malloc(sizeof (struct prefixes));
        strcpy(plist->name, new);
        plist->next = NULL;
	md->broadcast_list = plist;
    }else{
        while(plist != NULL){
            if(strcmp(plist->name, new) == 0){
                printf("duplicate prefix found: %s\n", new);
                return -1;
	    }
            last = plist;
            plist = plist->next;
        }
        last->next = (struct prefixes *)malloc(sizeof (struct prefixes));
        strcpy(last->next->name, new);
        last->next->next = NULL;
    }
    print_prefixes(md);
    return 0;
}

int read_config_file(char *filename, struct mydata *md){
    FILE *cfg;
    char buf[1024];
    char *cp;
    int count = 0, len, res;
    //const char *seps = " \t\n";
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

        if(strlen(buf) > 0){
            printf("namespace: <%s>\n", buf);
            //save them into mydata
            res = add_newprefix(md, buf);//add new prefix into the list
            if( res == 0)
                md->prefix_num ++;
        }

        count++;
    }
    fclose(cfg);
    return count;
}

int compare_bufs(unsigned char *one, int l1, unsigned char *two, int l2){
    int x;
    if(l1 != l2){
        return -1;
    }
    for(x = 0; x< l1; x++){
        if(one[x] != two[x])
        return -1;
    }
    return 0;
}

int compare_users(struct user_info *one, struct user_info *two){
    if((compare_bufs(one->name_prefix->buf, one->name_prefix->length, two->name_prefix->buf, two->name_prefix->length) == 0) &&		    
            (strcmp(one->address, two->address) == 0) &&
            (strcmp(one->port, two->port) == 0)){
        return 0;
    }
    return -1;

}

static void usage(const char *progname)
{
    fprintf(stderr,
            "%s <-a gw_ip> <-p gw_port> <-f config_file>\n"
            "\n"
            "\t-h displays this help information\n"
            "\t-a your IP\n"
            "\t-p your Port\n"
            "\t-f change the default config file name"
            "\t./gw.conf is read by default if no config file is specified and the node starts as a server\n"
            , progname);
    exit(1);
}

struct user_info *get_user_info(char *addr, char *port, char *name){
    int res;
    struct user_info *user = (struct user_info *)malloc(sizeof (struct user_info));
    user->next = NULL;

    strcpy(user->address, addr);
    strcpy(user->port, port);
    
    struct ccn_charbuf *prefix = ccn_charbuf_create();
    prefix->length = 0;
    res = ccn_name_from_uri(prefix, name);
    //printf("name %s has length of %d\n", name, prefix->length);
    if(res < 0){
        printf("cannot convert prefix into uri\n");
        //exit(1);
        free(user);
        return NULL;
    }
    user->name_prefix = prefix;
    return user;
}

void free_user_info(struct user_info *user){
    ccn_charbuf_destroy(&(user->name_prefix));
    free(user);
}

void print_users(struct mydata *md){
    printf("print users:\n");
    struct user_info *ulist = md->users_list;
    while( ulist != NULL ){//to the tail, won't add redunant prefix
        printf("<%s:%s>\n", ulist->address, ulist->port);
        ulist = ulist->next;
    }
}

int add_newuser(struct mydata *md, struct user_info *new){
    struct user_info *ulist = md->users_list;
    struct user_info *last = NULL;

    if(ulist == NULL){
        ulist = new;
	md->users_list = ulist;
    }else{
        while(ulist != NULL){
            if(compare_users(ulist, new) == 0){
                printf("duplicate user from %s\n", new->address);
                free_user_info(new);//free new and return -1
                return -1;
	    }
            last = ulist;
            ulist = ulist->next;
        }
        last->next = new;
    }
    print_users(md);
/*
    while( ulist != NULL ){//to the tail, won't add redunant prefix
        if(compare_users(ulist, new) == 0){
            printf("duplicate user from %s\n", new->address);
            free_user_info(new);//free new and return -1
            return -1;
        }
        ulist = ulist->next;
    }
    ulist = (struct user_info *)malloc(sizeof (struct user_info));
    ulist->next = NULL;
    md->users_list = ulist;

    strcpy(ulist->address, addr);
    strcpy(ulist->port, port);

    ulist->name_prefix = prefix;
    if(md->users_list == NULL)
        md->users_list = new;
    ulist = new;
*/
    return 0;
}

void setAddrinfo(struct addrinfo *hints){
    memset(hints, 0, sizeof (struct addrinfo));
    hints->ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints->ai_socktype = SOCK_DGRAM;
    hints->ai_flags = AI_PASSIVE; // use my IP
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int add_common_prefix(struct mydata *md, char *addr, char *port){
    struct ccn *h = md->h;
    struct ccn_charbuf *prefix = NULL;
    struct prefixes *plist = md->broadcast_list;
    int res;

    prefix = ccn_charbuf_create();
    while( plist != NULL ){//to the tail, won't add redunant prefix
        prefix->length = 0;
        res = ccn_name_from_uri(prefix, plist->name);
        if(res < 0){
            printf("cannot convert prefix into uri\n");
            return 1;
        }

        add_new_face(h, prefix, addr, port);
        plist = plist->next;
    }
    ccn_charbuf_destroy(&prefix);
}

int act_handler(char * buf, struct mydata *md){//protocol field may be used latter
    char name[MAXNAMELEN];
    char port[MAXPORTLEN];
    char *addr;
    int res;
    int offset = 0;
    struct ccn_charbuf *prefix = NULL;
    struct ccn *h = md->h;

    /////valid ACT msg//////
    offset += sizeof (struct pheader);
    //printf("offset1: %d\n", offset);
    struct pact act;
    memcpy(&act, buf+offset, sizeof (struct pact));
    offset += sizeof (struct pact);

    struct in_addr node_addr;
    memcpy(&node_addr, &(act.addr), sizeof (struct in_addr));

    memcpy(name, buf + offset, act.nameLen);
    name[act.nameLen] = '\0';

    //printf("protocol:%d, port: %d, len: %d,(%s)\n", act.prot, ntohs(act.port), act.nameLen, name);
    //construct FIB to users
    sprintf(port, "%u", ntohs(act.port));

    struct user_info *new = get_user_info(inet_ntoa(node_addr), port, name);

    addr = inet_ntoa(node_addr);
    if(new != NULL){
        res = add_newuser(md, new);//add new user into the list

        if(res == 0){
            prefix = new->name_prefix;

            res = add_new_face(h, prefix, addr, port);//need to add protocol as a parameter here
            if(res < 0){
                printf("cannot add new face\n");
                return 1;//log into file
            }
            //add all common prefixes
            add_common_prefix(md, addr, port);
        }
    }else
        return 2;
    ///////////////////////
    return 0;
}

int add_broadcast_namespaces(char *buf, int offset, struct mydata *md){
    int prefix_num = md->prefix_num, i;
    buf[offset] = prefix_num & 0xff;//1 common namespaces
    offset++;

    struct prefixes *plist = md->broadcast_list;
    while( plist != NULL ){//traverse the list
        char *str = plist->name;
        buf[offset] = strlen(str) & 0xff;
        offset++;
        memcpy(buf+offset, plist->name, strlen(str));
        offset += strlen(str);
        plist = plist->next;
    }
    return offset;
}

int main(int argc, char **argv)
{
    char *gwaddr = "127.0.0.1";
    char *gwport = "39695";
    char *nacfile = "gw.conf";
    int res;
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    p = &hints;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    struct mydata *md;

    while ((res = getopt(argc, argv, "a:p:f:h")) != -1) {
        switch (res){
            case 'a':
                gwaddr = optarg;
                break;
            case 'p':
                gwport = optarg;
                break;
            case 'f':
                nacfile = optarg;
                break;
            case 'h':
            default:
                usage(argv[0]);
        }
    }

    md = (struct mydata*)malloc(sizeof (struct mydata));
    md->users_list = NULL;
    md->prefix_list = NULL;
    md->h = NULL;

    read_config_file(nacfile, md);
    setAddrinfo(p);

    if ((rv = getaddrinfo(NULL, gwport, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("GW listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("GW listener: bind");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "GW listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    //setup connection to the ccndaemon
    md->h = ccn_create();
    res = ccn_connect(md->h, NULL);
    if (res < 0) {
        ccn_perror(md->h, "Cannot connect to ccnd.");
        exit(1);
    }
    printf("GW listener: waiting to recvfrom...\n");

    addr_len = sizeof their_addr;

    while(1){
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
    
        printf("GW: got packet from %s\n",
            inet_ntop(their_addr.ss_family,
                get_in_addr((struct sockaddr *)&their_addr),
                s, sizeof s));
    
        struct pheader header;
        
        memcpy(&header, buf, sizeof (struct pheader));
        if(is_valid_act(&header) == -1){
            printf("recved msg is not valid act msg\n");
            continue;
        }
    
        /////valid ACT msg//////
        if(header.msgType == MSGACT){//since now, only accept MSGACT
            res = act_handler(buf, md);
            //print_users(md);
            int offset = 0;
            memset(buf, 0, MAXBUFLEN);
            struct pheader *nheader = NULL;
            if(res != 0){
                //return error code here
                nheader = get_header(ACTNACK);
                //memcpy(buf, nheader, sizeof (struct pheader));
                offset += sizeof (struct pheader);
    
                buf[offset] = res & 0xff;
                offset++;
                
            }else{
                nheader = get_header(ACTACK);
                //memcpy(buf, nheader, sizeof (struct pheader));//the header need to cpy as the last one
                offset += sizeof (struct pheader);
    
                struct pack *ack = get_pack(gwaddr, CCNPORT, UDP, 1);//the last one should be how many prefixes
                memcpy(buf + offset, ack, sizeof (struct pack));
                offset += sizeof (struct pack);
                free(ack);
    
                //buf[offset] = 1;//for test, just 1 broadcast name
                //offset++;
                //buf[offset] = 0;//option 0, common namespaces
                //offset++;
                
                //need to add common namespaces
                offset = add_broadcast_namespaces(buf, offset, md);
            }
                
            //here is the place to cpy header
            nheader->length = htons(offset);
            memcpy(buf, nheader, sizeof (struct pheader));
    
            free(nheader);
            //need to respond user's request
            if ((numbytes = sendto(sockfd, buf, offset, 0,
                     (struct sockaddr *)&their_addr, addr_len)) == -1) {
                perror("GW: sendto");
                exit(1);
            }
        }
        ///////////////////////
    }
    
    close(sockfd);

    exit(0);
}
