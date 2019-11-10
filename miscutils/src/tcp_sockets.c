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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#include "miscutils.h"

#define MAXPENDING	10

/*
 * This function tcpConnect is used to connect to a server over TCP.
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
int tcpConnect(const char *server, int port) {
	int sock = -1;

//    int counter = 0;
    while (1) {
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (sock < 0) {
            pErr("socket failed.\n");
            return -1;
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
        servAddr.sin_port = htons(port);    // Server port
        if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
            if (errno != ECONNREFUSED) {
                pErr("connection failed. errno: (%d) %s\n", errno, strerror(errno));
                sleep(2);
            } else {
//                counter++;
//                if ((counter % 15) == 0)
//                    pOut("Waiting on connection to %s at port %d  errno %d\n", server, port, errno);
                close(sock);
                sleep(1);
            }
        } else {
            break;
        }
    }

	return sock;
}

/*
 * This function tcpTimeoutConnect is used to connect to a server over TCP.
 *
 *   server = The dotted notation IP address, does NOT handle domain name look ups.
 *   port = port number to connect to.
 *   timeout = Timeout in seconds.
 *
 *   returns -1 if socket create fails.
 *           -2 if inet_pton fails. Because IP address is invalid address.
 *           -3 if inet_pton fails. Because IP address is NOT in dotted notation form.
 *           -4 Connect failed.
 *           -5 Timed out
 *           -6 getsockopt returned an error.
 *           else a valid socket descriptor is returned.
 */
int tcpTimeoutConnect(const char *server, int port, int timeout) {
	int sock = -1;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sock < 0) {
		pErr("socket failed.\n");
		return -1;
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
	servAddr.sin_port = htons(port);    // Server port

	long ctrls = fcntl(sock, F_GETFL, NULL);
	ctrls |= O_NONBLOCK;
	fcntl(sock, F_SETFL, ctrls);

	int r = connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr));

	if (r == -1 && (errno != EINPROGRESS)) {
		pErr("connection failed: %s\n", strerror(errno));
		return -4;
	}

	fd_set fdset;
	struct timeval tv;

	FD_ZERO(&fdset);
	FD_SET(sock, &fdset);
	tv.tv_sec = timeout;
	tv.tv_usec = 0;

	r = select(sock + 1, NULL, &fdset, NULL, &tv);
	if (r == 0) {
		return -5;
	} else if ( r == 1) {
		int so_error = 0;
		socklen_t len = sizeof(so_error);

		getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);

		if (so_error != 0) {
			pErr("Connection failed, getsockopt() error: %d %s\n",
					 so_error, strerror(so_error));
			return -6;
		}
	}

	return sock;
}

/*
 * This function tcpServer creates a listening socket.
 *
 *   port = Port number to listen on for connection requests.
 *   maxPending = Max number of pending connections.
 *
 *   returns -1 if socket create failed.
 *           -2 setsockopt failed.
 *           -3 if bind fails.
 *           else a valid socket descriptor is returned.
 */
int tcpServer(int port, int maxPending) {
	int sock = -1;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sock < 0) {
		pErr("socket failed.\n");
		return -1;
	}

    int num = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &num, sizeof(int)) == -1 ) {
        pErr("Error setting socket options: %d\n", errno);
        return -2;
    }

	struct sockaddr_in servAddr;                  // Local address
	memset(&servAddr, 0, sizeof(servAddr));       // Zero out structure
	servAddr.sin_family = AF_INET;                // IPv4 address family
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming interface
	servAddr.sin_port = htons(port);              // Local port

	if (bind(sock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0) {
		pErr("bind failed.\n");
		return -3;
	}

	if (maxPending <= 0)
		listen(sock, MAXPENDING);
	else
		listen(sock, maxPending);

	return sock;
}

/*
 * This function tcpServerSubnet creates a listening socket for the subnet given.
 * For systems with multiple interfaces or aliased interfaces. Aliased interfaces can have multiple subnets.
 *
 *   port = Port number to listen on for connection requests.
 *   maxPending = Max number of pending connections.
 *   subnet = IP address, in dotted notation, of the subnet to use. Does NOT handle domain name lookups.
 *
 *   returns -1 if socket create failed.
 *           -2 if bind fails.
 *           -3 invalid or NULL subnet given.
 *           else a valid socket descriptor is returned.
 */
int tcpServerSubnet(int port, int maxPending, const char *subnet) {
	int sock = -1;

	if (subnet == NULL || isIPv4Address(subnet) == 0) {
		pErr("Given subnet is null or invalid.\n");
		return -3;
	}

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sock < 0) {
		pErr("socket failed.\n");
		return -1;
	}

	int num = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &num, sizeof(int)) == -1 ) {
        pErr("Error setting socket options: %d\n", errno);
        return -2;
    }

	struct sockaddr_in servAddr;                  // Local address
	memset(&servAddr, 0, sizeof(servAddr));       // Zero out structure
	servAddr.sin_family = AF_INET;                // IPv4 address family
	servAddr.sin_addr.s_addr = inet_addr(subnet); // only this subnet
	servAddr.sin_port = htons(port);              // Local port

	if (bind(sock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0) {
		pErr("bind failed.\n");
		return -2;
	}

	if (maxPending <= 0)
		listen(sock, MAXPENDING);
	else
		listen(sock, maxPending);

	return sock;
}

/*
 * This function tcpAccept blocks waiting for connection requests.
 *
 *   sock = Socket descriptor.
 *   clientAddr = Remote hosts IP address after accept.
 *
 *   returns -1 on accept failed.
 *           else returns a valid socket descriptor for the new connection.
 */
int tcpAccept(int sock, struct sockaddr *clientAddr) {
	socklen_t addrLen = sizeof(struct sockaddr);
	int clientSock = accept(sock, clientAddr, &addrLen);

	if (clientSock < 0) {
		pErr("accept failed.\n   %s\n", strerror(errno));
		return -1;
	}

	return clientSock;
}

/*
 * This function tcpSend sends a NULL terminated string to remote host.
 * If number of bytes sent does not equal msg length then the send buffer size
 * has been reached.  Interface can not send data fast enough or a number of
 * other reasons.
 *
 *   sock = Socket descriptor.
 *   msg = NULL terminated string to send.
 *
 *   returns -1 on error
 *           else Number of bytes sent.
 */
int tcpSend(int sock, const char *msg) {
	int len = strlen(msg);

	int r = send(sock, msg, len, 0);

	if (r != len) {
//		pErr("Could only send %d bytes of message length %d\n", r, len);
		// Let programmer decide if this is an error.
		// They may want to resend the data not sent.
		// This is normally caused by the send socket buffer being full.  This
		// is not an error but a warning that interface is backing up for some reason.
	}

	return r;
}

/*
 * This function tcpRecSend sends a NULL terminated string to remote host.
 * If number of bytes sent does not equal msg length then the send buffer size
 * has been reached.  Interface can not send data fast enough or a number of
 * other reasons.
 * Prefixes the msg packet with an 8 byte header record.
 *
 *   sock = Socket descriptor.
 *   msg = NULL terminated string to send.
 *
 *   returns -1 on error
 *           else Number of bytes sent.
 */
int tcpRecSend(int sock, const char *msg) {
	int len = strlen(msg);
	char cmd[8];

	strcpy(cmd, "NKX1");
	uint32_t nl = htonl(len);
	cmd[4] = (char)((nl >> 24) & 0xff);
	cmd[5] = (char)((nl >> 16) & 0xff);
	cmd[6] = (char)((nl >> 8) & 0xff);
	cmd[7] = (char)(nl & 0xff);

//	pOut("cmd: %c %c %c %c 0x%02x 0x%02x 0x%02x 0x%02x, %d\n'%s'\n",
//			cmd[0], cmd[1], cmd[2], cmd[3],
//			cmd[4], cmd[5], cmd[6], cmd[7], len, msg);

	send(sock, cmd, 8, 0);

	int r = send(sock, msg, len, 0);

	if (r != len) {
//		pErr("Could only send %d bytes of message length %d\n", r, len);
		// Let programmer decide if this is an error.
		// They may want to resend the data not sent.
		// This is normally caused by the send socket buffer being full.  This
		// is not an error but a warning that interface is backing up for some reason.
	}

	return r;
}

/*
 * This function tcpNumSend sends sends N number of bytes of data.
 * If number of bytes sent does not equal numBytes, then the send buffer size
 * has been reached.  Interface can not send data fast enough or a number of
 * other reasons.
 *
 *   sock = Socket descriptor.
 *   msg = NULL terminated string to send.
 *   numBytes = number of bytes to send.
 *
 *   returns -1 on error
 *           else Number of bytes sent.
 */
int tcpNumSend(int sock, const char *msg, int numBytes) {
	int r = send(sock, msg, numBytes, 0);

	if (r != numBytes) {
		pErr("Could only send %d bytes of message length %d\n", r, numBytes);
		// Let programmer decide if this is an error.
		// Them may want to resend the data not sent.
		// This is normally caused by the send socket buffer being full.  This
		// is not an error but a warning that interface is backing up for some reason.
	}

	return r;
}

/*
 * This function tcpRecv receives a block of bytes from remote host.
 *
 *   sock = Socket descriptor.
 *   buf = Buffer to place received bytes.
 *   buf_len = Max size of buffer.
 *
 *   returns -1 on error
 *           else Number of bytes received.
 */
int tcpRecv(int sock, char *buf, int buf_len) {
	int r = recv(sock, buf, buf_len, 0);

	return r;
}

/*
 * This function tcpRecvString receives a block of bytes from remote host.
 * This string is null terminated.
 *
 *   sock = Socket descriptor.
 *   buf = Buffer to place received bytes.
 *   buf_len = Max size of buffer.
 *
 *   returns -1 on error
 *           else Number of bytes received.
 */
int tcpRecvString(int sock, char *buf, int buf_len) {
	int r = recv(sock, buf, buf_len, 0);

	if (r < buf_len)
		buf[r] = '\0';

	return r;
}

/*
 * This function tcpRecvJson receives a JSON block from remote host.
 * This string is null terminated.
 *
 *   sock = Socket descriptor.
 *   json = Buffer to place received JSON block into.
 *   json_len = Max size of json buffer.
 *
 *   returns -1 on error
 *           else Number of bytes received.
 */
int tcpRecvJson(int sock, char *json, int json_len) {
	int totalBytes = 0;
	char buf[2];

	int n = 0;
	int start = 0;
	while (1) {
		int r = recv(sock, buf, 1, 0);
		if (r < 0)
			return r;

		if (buf[0] == '{') {
			start = 1;
		}

		if (start == 1) {
			json[n++] = buf[0];
		}

		if (n >= json_len) {
			pErr("JSON ending '}' not found in %d bytes.\n", json_len);
			return -1;
		}

		if (buf[0] == '}') {
			json[n] = '\0';
			break;
		}
	}

	return totalBytes;
}

void tcpClose(int sock) {
	close(sock);		// ignore any errors.
}

/*
 * This function isIPv4Address validates the format of an IPV4 address in dotted notation format.
 * Does NOT check to see if the address exists.
 *
 *   addr = address to validate.
 *
 *   return 0 if address is NOT invalid.
 *          1 if address is valid.
 */
int isIPv4Address(const char *addr) {
	char octal[4];
	int octalCnt = 1;
	int cnt = 0;

	if (strcmp(addr, "127.0.0.1") == 0)		// localhost address is OK.
		return 0;

	memset(octal, 0, sizeof(octal));

	int len = strlen(addr);
	for (int i = 0; i < len; i++, addr++) {
		if (*addr == '.' || *addr == '\0') {
			int n = atoi(octal);
			switch (octalCnt) {
			case 1:
				if (n < 1 || n > 223) {	// 224 and up is multicast.
					// pErr("1: %d %s\n", n, octal);
					return 0;
				}
				break;
			case 2:
				if (n < 0 || n > 254) {
					// pErr("2: %d\n", n);
					return 0;
				}
				break;
			case 3:
				if (n < 0 || n > 254) {
					// pErr("3: %d\n", n);
					return 0;
				}
				break;
			case 4:
				if (n < 1 || n > 254) {
					// pErr("4: %d\n", n);
					return 0;
				}
				break;
			}
			memset(octal, 0, sizeof(octal));
			octalCnt++;
			cnt = 0;
		} else {
			if (cnt < sizeof(octal)) {
				octal[cnt++] = *addr;
			}
		}
	}

	if (octalCnt != 4) {
		// pErr("octalCnt = %d\n", octalCnt);
		return 0;
	} else
		return 1;		// valid IPv4 address
}
