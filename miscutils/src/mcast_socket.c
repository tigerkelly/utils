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

/*
 * This function mcastJoin joins a multicast address to read packets from.
 *
 *   ifInterface = An network interface capable of multicast. localhost (127.0.0.1) is one.
 */
int mcastJoin(McastInfo *mi) {
//	struct sockaddr_in localSock;
	struct ip_mreq group;
	mi->mSock = -1;

	mi->mSock = socket(AF_INET, SOCK_DGRAM, 0);
	if (mi->mSock < 0) {
		pOut("Error opening socket.\n");
		perror(__FILE__);
		return -1;
	}

	int reuse = 1;
	if(setsockopt(mi->mSock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
		pOut("Error setting SO_REUSEADDR.\n");
		perror(__FILE__);
		close(mi->mSock);
		mi->mSock = -1;
		return -1;
	}

	 /* Join the multicast group mAddress on the local ifInterface */
	 /* interface. Note that this IP_ADD_MEMBERSHIP option must be */
	 /* called for each local interface over which the multicast */
	 /* datagrams are to be received. */
	 group.imr_multiaddr.s_addr = inet_addr(mi->mAddress);
	 group.imr_interface.s_addr = inet_addr(mi->ifInterface);
	 if(setsockopt(mi->mSock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0) {
		 pOut("Error joining multicast group.\n");
		 perror(__FILE__);
		 close(mi->mSock);
		 mi->mSock = -1;
		 return -1;
	 }

	return mi->mSock;
}

/*
 * This function mcastServer creates a Multicast listener socket.
 *
 *   ifInterface = An network interface capable of multicast. localhost is one.
 *   mAddress = A multicast address in the range of 224.0.0.0 to 239.255.255.255
 *   mPort = port number to use.
 */
int mcastServer(McastInfo *mi) {
	struct in_addr localInterface;
	struct sockaddr_in groupSock;

	mi->mSock = socket(AF_INET, SOCK_DGRAM, 0);
	if (mi->mSock < 0) {
		pOut("Error creating socket.\n");
		perror(__FILE__);
		return -1;
	}

	memset((char *) &groupSock, 0, sizeof(groupSock));
	groupSock.sin_family = AF_INET;
	groupSock.sin_addr.s_addr = inet_addr(mi->mAddress);
	groupSock.sin_port = htons(mi->mPort);

	localInterface.s_addr = inet_addr(mi->ifInterface);
	if(setsockopt(mi->mSock, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface)) < 0) {
		pOut("Error setting enabling multicast on the interface.\n");
		perror(__FILE__);
		close(mi->mSock);
		mi->mSock = -1;
		return -1;
	}

	int num = 1;
    if (setsockopt(mi->mSock, SOL_SOCKET, SO_REUSEADDR, &num, sizeof(int)) == -1 ) {
        pErr("Error setting socket options: %d\n", errno);
        return -2;
    }

	if (bind(mi->mSock,(struct sockaddr *) &groupSock,sizeof(groupSock)) < 0) {
		pOut("Error binding to port.\n");
	    perror(__FILE__);
	    close(mi->mSock);
	    mi->mSock = -1;
	    return -1;
     }

	return mi->mSock;
}

int mcastSend(McastInfo *mi, const char *msg) {
	struct sockaddr_in groupSock;

	memset((char *) &groupSock, 0, sizeof(groupSock));
	groupSock.sin_family = AF_INET;
	groupSock.sin_addr.s_addr = inet_addr(mi->mAddress);
	groupSock.sin_port = htons(mi->mPort);

	size_t len = strlen(msg);

	int r = sendto(mi->mSock, msg, len, 0, (struct sockaddr*)&groupSock, sizeof(groupSock));

	if (r != len) {
//		pErr("Could only send %d bytes of message length %d\n", r, len);
		// Let programmer decide if this is an error.
		// They may want to resend the data not sent.
		// This is normally caused by the send socket buffer being full.  This
		// is not an error but a warning that interface is backing up for some reason.
	}

	return r;
}

int mcastNumSend(McastInfo *mi, const char *msg, int numBytes) {
	struct sockaddr_in groupSock;

	memset((char *) &groupSock, 0, sizeof(groupSock));
	groupSock.sin_family = AF_INET;
	groupSock.sin_addr.s_addr = inet_addr(mi->mAddress);
	groupSock.sin_port = htons(mi->mPort);

	int r = sendto(mi->mSock, msg, numBytes, 0, (struct sockaddr*)&groupSock, sizeof(groupSock));

	if (r != numBytes) {
		pErr("Could only send %d bytes of message length %d\n", r, numBytes);
		// Let programmer decide if this is an error.
		// Them may want to resend the data not sent.
		// This is normally caused by the send socket buffer being full.  This
		// is not an error but a warning that interface is backing up for some reason.
	}

	return r;
}

int mcastRecv(McastInfo *mi, char *buf, int buf_len) {
	struct sockaddr_in addr;
	socklen_t addrlen = 0;

	memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_ANY); /* N.B.: differs from sender */
    addr.sin_port=htons(mi->mPort);

    addrlen = sizeof(addr);

    int r = recvfrom(mi->mSock, buf, buf_len, 0, (struct sockaddr *) &addr, &addrlen);

    return r;
}

int mcastRecvJson(McastInfo *mi, char *json, int json_len) {
	struct sockaddr_in addr;
	socklen_t addrlen = 0;

	memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_ANY); /* N.B.: differs from sender */
    addr.sin_port=htons(mi->mPort);

    addrlen = sizeof(addr);

    int r = recvfrom(mi->mSock, json, json_len, 0, (struct sockaddr *) &addr, &addrlen);

    return r;
}

int mcastRecvString(McastInfo *mi, char *buf, int buf_len) {
	struct sockaddr_in addr;
	socklen_t addrlen = 0;

	memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_ANY); /* N.B.: differs from sender */
    addr.sin_port=htons(mi->mPort);

    addrlen = sizeof(addr);

    int r = recvfrom(mi->mSock, buf, buf_len, 0, (struct sockaddr *) &addr, &addrlen);

    if (r >= 0)
    	buf[r] ='\0';

    return r;
}

void mcastClose(McastInfo *mi) {
	close(mi->mSock);
	mi->mSock = -1;
}
