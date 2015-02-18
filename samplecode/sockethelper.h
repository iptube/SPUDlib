
#ifndef SOCKETHELPER_H
#define SOCKETHELPER_H

#include <sockaddr_util.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>

int createLocalUDPSocket(int ai_family,
                         const struct sockaddr *localIp,
                         uint16_t port);

void sendPacket(int sockHandle,
                const uint8_t *buf,
                int bufLen,
                const struct sockaddr *dstAddr,
                bool useRelay,
                uint8_t ttl);


#endif
