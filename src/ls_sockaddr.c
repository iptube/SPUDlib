#include "ls_sockaddr.h"

LS_API size_t ls_sockaddr_get_length(const struct sockaddr *addr)
{
    switch (addr->sa_family) {
    case AF_INET:
        return sizeof(struct sockaddr_in);
    case AF_INET6:
        return sizeof(struct sockaddr_in6);
    default:
        assert(false);
        return -1;
    }
}
