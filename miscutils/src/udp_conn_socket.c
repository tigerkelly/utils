/*
 * Copyright (c) 2015 Richard Kelly Wiles (rkwiles@twc.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#include "miscutils.h"

#define MAXPENDING	10

/*
 * This function udpConnect is used to connect to a server over UDP.
 * This function waits forever.
 * If connect() gets an ECONNREFUSED error it will sleep 1 second and try again.
 *
 *   server = The dotted notation IP address, does NOT handle domain name look ups.
 *   port = port number to connect to.
 *
 *   returns -1 if socket create fails.
 *           -2 if inet_pton fails. Because IP address is invalid address.
 *           -3 if inet_pton fails. Because IP address is NOT in dotted notation form.
 *           -4 Connect failed.
 *           else a valid socket descriptor is returned.
 */
int udpConnClient(const char *client, int cPort, const char *server, int sPort) {
	int sock = -1;

    int counter = 0;
    while (1) {
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (sock < 0) {
            pErr("socket failed.\n");
            return -1;
        }

        // If client port is provided, bind this to socket 
        if (cPort) {
            struct sockaddr_in cliAddr;            // Client address
            memset(&cliAddr, 0, sizeof(cliAddr));
            cliAddr.sin_family = AF_INET;
            if (client == NULL) {
                cliAddr.sin_addr.s_addr = htonl(INADDR_ANY);
            } else {
                int rtnVal = inet_pton(AF_INET, client, &cliAddr.sin_addr.s_addr);

                if (rtnVal == 0) {
                    pErr("inet_pton() failed, invalid address string.\n");
                    return -2;
                } else if (rtnVal < 0) {
                    pErr("inet_pton() failed.\n");
                    return -3;
                }
            }
            cliAddr.sin_port = htons(cPort);    // Client port
            if (0 != bind(sock, (struct sockaddr *)&cliAddr, sizeof(cliAddr))) {
                pErr("udp bind  failed.\n");
                close(sock);
                return -1;
            }
        }

        struct sockaddr_in servAddr;            // Server address
        memset(&servAddr, 0, sizeof(servAddr)); // Zero out structure
        servAddr.sin_family = AF_INET;          // IPv4 address family

        int rtnVal = inet_pton(AF_INET, server, &servAddr.sin_addr.s_addr);

        if (rtnVal == 0) {
            pErr("inet_pton() failed, invalid address string.\n");
            return -2;
        } else if (rtnVal < 0) {
            pErr("inet_pton() failed.\n");
            return -3;
        }
        servAddr.sin_port = htons(sPort);    // Server port
        if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
            if (errno != ECONNREFUSED) {
                pErr("connection failed. errno: (%d) %s\n", errno, strerror(errno));
                sleep(2);
            } else {
                counter++;
                if ((counter % 15) == 0)
                    pOut("Waiting on connection to %s at port %d  errno %d\n", server, sPort, errno);
                close(sock);
                sleep(1);
            }
        } else {
            break;
        }
    }

	return sock;
}

int udpConnServer(const char *server, int port) {
	int sock = -1;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sock < 0) {
        pErr("socket failed.\n");
        return -1;
    }

    struct sockaddr_in servAddr;            // Server address
    memset(&servAddr, 0, sizeof(servAddr)); // Zero out structure
    servAddr.sin_family = AF_INET;          // IPv4 address family
 
    if (server == NULL) {
        servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        int rtnVal = inet_pton(AF_INET, server, &servAddr.sin_addr.s_addr);

        if (rtnVal == 0) {
            pErr("inet_pton() failed, invalid address string.\n");
            return -2;
        } else if (rtnVal < 0) {
            pErr("inet_pton() failed.\n");
            return -3;
        }
    }
    servAddr.sin_port = htons(port);    // Server port
    if (0 != bind(sock, (struct sockaddr *)&servAddr, sizeof(servAddr))) {
        pErr("udp bind failed.\n");
        close(sock);
        return -4;
    }
	return sock;
}


int udpConnSend(int sock, const char *msg, int len) {
	int r = send(sock, msg, len, 0);
    if (r < 0) {
        perror("send failed");
    }
    return r;
}

/*
 * This function udpRecv receives a block of bytes from remote host.
 *
 *   sock = Socket descriptor.
 *   buf = Buffer to place received bytes.
 *   buf_len = Max size of buffer.
 *
 *   returns -1 on error
 *           else Number of bytes received.
 */
int udpConnRecv(int sock, char *buf, int buf_len) {
	int r = recv(sock, buf, buf_len, 0);
	return r;
}


void udpConnClose(int sock) {
	close(sock);		// ignore any errors.
}


int udpConnBytesRecvd(int sock) {
    int number;
    if (ioctl(sock, FIONREAD, &number) < 0) {
        perror ("ioctl - FIONREAD");
        return -1;
    }
    return number;
}

#if 0
int udpConnGetRemoteAddr(int sock, char *remIP, unsigned int *remPort) {
    struct sockaddr_in      addr;
    socklen_t               len = 0;

    len = sizeof(addr);
    memset(&addr, 0, len);

    if (getpeername(sock, (struct sockaddr *)&addr, &len) != 0) {
        perror("udpConnGetRemoteAddr error:");
        return (-1);
    }
    *remPort = ntohs(addr.sin_port);
    inet_ntop(AF_INET, (const void *)&addr.sin_addr.s_addr, remIP, INET_ADDRSTRLEN); 
    return 0;
}
#endif
