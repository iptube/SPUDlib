#include <bitstring.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>

#include "spudlib.h"
#include "tube.h"
#include "iphelper.h"
#include "sockethelper.h"


#define MYPORT 1402    // the port users will be connecting to
#define MAXBUFLEN 2048
#define MAX_LISTEN_SOCKETS 10

#define UNUSED(x) (void)(x)

int sockfd;

// TODO: replace this with a hashtable
#define MAX_CLIENTS 128
bitstr_t bit_decl(bs_clients, MAX_CLIENTS);
tube_t clients[MAX_CLIENTS];

typedef struct _context_t {
    int num;
    size_t count;
} context_t;

context_t *new_context(int num) {
    context_t *c = malloc(sizeof(context_t));
    c->num = num;
    c->count = 0;
    return c;
}

tube_t* tube_unused()
{
    int n = -1;
    bit_ffc(bs_clients, MAX_CLIENTS, &n);
    if (n == -1) {
        return NULL;
    }
    bit_set(bs_clients, n);
    return &clients[n];
}

tube_t* tube_match(spud_flags_id_t *flags_id) {
    spud_flags_id_t search;
    memcpy(&search, flags_id, sizeof(search));
    search.octet[0] &= SPUD_FLAGS_EXCLUDE_MASK;

    // table scan.  I said, replace this with a hashtable.
    for (int i=0; i<MAX_CLIENTS; i++) {
        if (bit_test(bs_clients, i)) {
            if (memcmp(&search, &clients[i].id, sizeof(search)) == 0) {
                return &clients[i];
            }
        }
    }
    return NULL;
}

void teardown()
{
  close(sockfd);
  printf("Quitting...\n");
  exit(0);
}

static int socketListen() {
    struct sockaddr_storage their_addr;
    uint8_t buf[MAXBUFLEN];
    char idStr[SPUD_ID_STRING_SIZE+1];
    socklen_t addr_len;
    int numbytes;
    tube_t *tube;
    spud_message_t sMsg;

    addr_len = sizeof their_addr;

    while(1) {
        addr_len = sizeof(their_addr);
        if ((numbytes = recvfrom(sockfd, buf,
                                 MAXBUFLEN , 0,
                                 (struct sockaddr *)&their_addr,
                                 &addr_len)) == -1) {
            perror("recvfrom (data)");
            return 1;
        }

        if (!spud_cast(buf, numbytes, &sMsg)) {
            // it's an attack.  Move along.
            continue;
        }

        tube = tube_match(&sMsg.header->flags_id);
        if (!tube) {
            // get started
            tube = tube_unused();
            if (!tube) {
                // full.
                // TODO: send back error
                continue;
            }
            printf("Spud ID: %s(%d) OPEN\n",
                   spud_idToString(idStr,
                                   sizeof(idStr),
                                   &sMsg.header->flags_id),
                   ((context_t*)tube->data)->num);

        }
        tube_recv(tube, &sMsg, (struct sockaddr *)&their_addr);
    }
    return 0;
}

static void read_cb(tube_t *tube,
                    const uint8_t *data,
                    ssize_t length,
                    const struct sockaddr* addr)
{
    UNUSED(addr);
    tube_data(tube, (uint8_t*)data, length);
    ((context_t*)tube->data)->count++;
}

static void close_cb(tube_t *tube,
                     const struct sockaddr* addr)
{
    context_t *c = (context_t*)tube->data;
    char idStr[SPUD_ID_STRING_SIZE+1];
    UNUSED(addr);
    bit_clear(bs_clients, c->num);
    printf("Spud ID: %s(%d) CLOSED: %zd data packets\n",
           spud_idToString(idStr,
                           sizeof(idStr),
                           &tube->id),
           c->num,
           c->count);
}

int main(void)
{
    struct sockaddr_in6 servaddr;

    signal(SIGINT, teardown);

    memset(clients, 0, MAX_CLIENTS*sizeof(tube_t));
    bit_nclear(bs_clients, 0, MAX_CLIENTS-1);

    sockfd = socket(PF_INET6, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }
    sockaddr_initAsIPv6Any(&servaddr, MYPORT);
    if (bind(sockfd, (struct sockaddr*)&servaddr, servaddr.sin6_len) != 0) {
        perror("bind");
        return 1;
    }

    for (int i=0; i<MAX_CLIENTS; i++) {
        tube_init(&clients[i], sockfd);
        clients[i].data = new_context(i);
        clients[i].data_cb = read_cb;
        clients[i].close_cb = close_cb;
    }

    return socketListen();
}
