
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

/*
 * This function udpServer creates a UDP socket.
 *
 *   server = IP address or domain name.
 *   port = Port number to listen on for connection requests.
 *
 *   returns -1 if socket create failed.
 *           -2 setsockopt failed.
 *           -3 invalid IP address string.
 *           -4 inet_pton failed.
 *           -5 if bind fails.
 *           else a valid socket descriptor is returned.
 */
int udpServer(const char *server, int port) {
    int sock;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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

	if (server == NULL) {
		servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	} else {
		int rtnVal = inet_pton(AF_INET, server, &servAddr.sin_addr.s_addr);

		if (rtnVal == 0) {
			pErr("inet_pton() failed, invalid address string.\n");
			return -3;
		} else if (rtnVal < 0) {
			pErr("inet_pton() failed.\n");
			return -4;
		}
	}

	if (bind(sock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0) {
		pErr("bind failed.\n");
		return -5;
	}

    return sock;
}

/*
 * This function udpClientBind creates a UDP socket to be used by client
 *
 *   port = Port number to bind
 *
 *   returns -1 if socket create failed.
 *           -2 setsockopt failed.
 *           -3 if bind fails.
 *           else a valid socket descriptor is returned.
 */
int udpClientBind(int port, const char *client) {
    int sock;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		pErr("socket failed.\n");
		return -1;
	}

	struct sockaddr_in si_me;

	// zero out the structure
	memset((char *) &si_me, 0, sizeof(si_me));

	si_me.sin_family = AF_INET;
	si_me.sin_port = htons(port);
	if (client == NULL) {
		si_me.sin_addr.s_addr = htonl(INADDR_ANY);
	} else {
		int rtnVal = inet_pton(AF_INET, client, &si_me.sin_addr.s_addr);

		if (rtnVal == 0) {
			pErr("inet_pton() failed, invalid address string.\n");
			return -2;
		} else if (rtnVal < 0) {
			pErr("inet_pton() failed.\n");
			return -3;
		}
	}

	//bind socket to port
	if( bind(sock, (struct sockaddr*)&si_me, sizeof(si_me) ) == -1) {
		pErr("UDP client bind failed.\n");
		return -1;
	}

    return sock;
}

/*
 * This function udpClient creates a UDP socket to be used by client
 *
 *   port = Port number to bind
 *
 *   returns -1 if socket create failed.
 *           -2 setsockopt failed.
 *           -3 if bind fails.
 *           else a valid socket descriptor is returned.
 */
int udpClient (int port) {
    int sock;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		pErr("socket failed.\n");
		return -1;
	}

    return sock;
}

int udpSetTimeout(int sock, struct timeval *tv) {

	return setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, tv, sizeof(struct timeval));
}

int udpSend(int sock, const char *msg, int msgLen, struct sockaddr_in *remoteAddr, int remoteAddrLen) {

    int r;

    r = sendto(sock, msg, msgLen, 0, (struct sockaddr *)remoteAddr, remoteAddrLen);
    if (r < 0 || r < msgLen) {
        char errBuf[MAX_STR_LEN];
        strerror_r(errno, errBuf, sizeof(errBuf));
        pErr("msgLen: %d\n", msgLen);
        pErr("Error in sendto. errCode = %d, errno = %d, errStr = %s\n", r, errno, errBuf);
    }

	return r;
}

void getRemoteAddr (const char *remoteAddrStr, int remotePort, struct sockaddr_in *remoteAddr) {

    memset(remoteAddr, 0, sizeof(struct sockaddr_in)); 
    remoteAddr->sin_family = AF_INET;
    remoteAddr->sin_addr.s_addr = inet_addr(remoteAddrStr);
    remoteAddr->sin_port = htons(remotePort);

    return;
}

int udpRecv (int sock, char *buf, int bufLen, struct sockaddr_in *remote, socklen_t *remoteAddrLen) {
    int r;
    if (buf == NULL) {
       pErr("Input buffer must not be null\n");
       return -1;
    }

    memset(buf, 0, bufLen);

    if (remote && remoteAddrLen) {
        *remoteAddrLen = sizeof(struct sockaddr_in);
        memset(remote, 0, sizeof(struct sockaddr_in));
        r = recvfrom(sock, buf, bufLen, 0, (struct sockaddr *) remote, remoteAddrLen);
    } else {
        struct sockaddr_in tmp;
        socklen_t tmpLen = sizeof(struct sockaddr_in);
        r = recvfrom(sock, buf, bufLen, 0, (struct sockaddr *) &tmp, &tmpLen);
    }

    if (r <= 0) {
       char tmpErr[100];
       strerror_r(errno, tmpErr, sizeof(tmpErr) -1);
       pErr("Received 0 bytes. err=%s\n", tmpErr);
    }

    return r;
}

void udpClose(int sock) {
	close(sock);
	sock = -1;
    return;
}

