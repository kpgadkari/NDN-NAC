#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#include <ccn/ccn.h>
#include <ccn/uri.h>
#include <ccn/face_mgmt.h>
#include <ccn/reg_mgmt.h>
#include <ccn/charbuf.h>


#define CCN_DHCP_REQUEST "ccnx:/local/dhcp/REQUEST"
#define CCN_DHCP_URI "ccnx:/local/dhcp"
#define CCN_DHCP_CONFIG "ccn_dhcp_server.conf"
#define CCN_DHCP_CONFIG_CLIENT "ccn_dhcp_client.conf"  
#define CCN_DHCP_HOSTNAME "dhcp_users"  
#define CCN_DHCP_ADDR "224.0.23.170"
#define CCN_DHCP_PORT "59695"
//#define CCN_DHCP_LIFETIME ((~0U) >> 1) don't use this... bad
#define CCN_DHCP_LIFETIME (1)
#define CCN_DHCP_MCASTTTL (-1)

#define MAXBUFLEN 250
#define MAXPORTLEN 50
#define MAXNAMELEN 255

#define MSGACT 0
#define ACTACK 1
#define ACTNACK 2

#define TCP 6
#define UDP 17

typedef char * string;


struct ccn_dhcp_entry {
    struct ccn_charbuf *name_prefix;
    const char address[20];
    const char port[10];
    struct ccn_charbuf *store;
    struct ccn_dhcp_entry *next;
};

struct pheader{//packet header
    uint8_t ver;
    uint8_t msgType;
    uint16_t length;
}__attribute__((__packed__));

struct pact{//active msg
    uint32_t addr;
    uint16_t port;
    uint8_t prot;//protocol, 6 for TCP and 17 for UDP
    uint8_t nameLen;
}__attribute__((__packed__));

struct pack{//active ACK msg
    uint16_t port;
    uint8_t prot;//protocol, 6 for TCP and 17 for UDP
    uint8_t number;
}__attribute__((__packed__));

extern string Concat (string s1, string s2);

int join_dhcp_group(struct ccn *h);

int add_new_face(struct ccn *h, struct ccn_charbuf *prefix, const char *address, const char *port);

int ccn_dhcp_content_parse(const unsigned char *p, size_t size, struct ccn_dhcp_entry *tail);

void ccn_dhcp_content_destroy(struct ccn_dhcp_entry *head);

int ccnb_append_dhcp_content(struct ccn_charbuf *c, int count, const struct ccn_dhcp_entry *head);

struct pheader *get_header(uint8_t type);

struct pact *get_pact(char *myip, int iport, int len, int protocol);

struct pack *get_pack(int port, int protocol, int number);

int is_valid_act(struct pheader *header);

int is_valid_response(struct pheader *header);

