
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/sctp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#include "miscutils.h"

#define MAXLISTENING	10

/*
 * This function sctpConnect is used to connect to a server over TCP.
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
int sctpConnect(const char *server, int port) {
	int sock = -1;

	struct sctp_initmsg initmsg;
	struct sctp_event_subscribe events;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

	if (sock < 0) {
		pErr("socket failed.\n");
		return -1;
	}

	/* Specify that a maximum of 5 streams will be available per socket */
	memset( &initmsg, 0, sizeof(initmsg) );
	initmsg.sinit_num_ostreams = 5;
	initmsg.sinit_max_instreams = 5;
	initmsg.sinit_max_attempts = 4;
	setsockopt( sock, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg) );

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

    int counter = 0;
    while (1) {
        if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0) {
            if (errno != ECONNREFUSED) {
                pErr("connection failed. errno: %d\n", errno);
                return -4;
            } else {
                counter++;
                if ((counter % 15) == 0)
                    pOut("Waiting on connection to %s at port %d\n", server, port);
                sleep(1);
            }
        } else {
			/* Enable receipt of SCTP Snd/Rcv Data via sctp_recvmsg */
			memset((void *) &events, 0, sizeof(events));
			events.sctp_data_io_event = 1;
			setsockopt(sock, SOL_SCTP, SCTP_EVENTS, (const void *) &events, sizeof(events));

#if(0)
			/* Read and emit the status of the Socket (optional step) */
			struct sctp_status status;

			int in = sizeof(status);
			getsockopt(sock, SOL_SCTP, SCTP_STATUS, (void *) &status, (socklen_t *) &in);

			printf("assoc id  = %d\n", status.sstat_assoc_id);
			printf("state     = %d\n", status.sstat_state);
			printf("instrms   = %d\n", status.sstat_instrms);
			printf("outstrms  = %d\n", status.sstat_outstrms);
#endif
			break;
        }
    }

	return sock;
}

/*
 * This function stcpTimeoutConnect is used to connect to a server over TCP.
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
int sctpTimeoutConnect(const char *server, int port, int timeout) {
	int sock = -1;

	struct sctp_initmsg initmsg;
	struct sctp_event_subscribe events;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

	if (sock < 0) {
		pErr("socket failed.\n");
		return -1;
	}

	/* Specify that a maximum of 5 streams will be available per socket */
	memset( &initmsg, 0, sizeof(initmsg) );
	initmsg.sinit_num_ostreams = 5;
	initmsg.sinit_max_instreams = 5;
	initmsg.sinit_max_attempts = 4;
	setsockopt( sock, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg) );

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
		} else {
			/* Enable receipt of SCTP Snd/Rcv Data via sctp_recvmsg */
			memset((void *) &events, 0, sizeof(events));
			events.sctp_data_io_event = 1;
			setsockopt(sock, SOL_SCTP, SCTP_EVENTS, (const void *) &events, sizeof(events));

#if(0)
			/* Read and emit the status of the Socket (optional step) */
			struct sctp_status status;

			int in = sizeof(status);
			getsockopt(sock, SOL_SCTP, SCTP_STATUS, (void *) &status, (socklen_t *) &in);

			printf("assoc id  = %d\n", status.sstat_assoc_id);
			printf("state     = %d\n", status.sstat_state);
			printf("instrms   = %d\n", status.sstat_instrms);
			printf("outstrms  = %d\n", status.sstat_outstrms);
#endif
		}
	}

	return sock;
}

/*
 * This function stcpServer creates a listening socket.
 *
 *   port = Port number to listen on for connection requests.
 *   maxPending = Max number of pending connections.
 *
 *   returns -1 if socket create failed.
 *           -2 setsockopt failed.
 *           -3 if bind fails.
 *           else a valid socket descriptor is returned.
 */
int sctpServer(int port, int maxPending) {
	int sock = -1;

	struct sctp_initmsg initmsg;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

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

	/* Specify that a maximum of 5 streams will be available per socket */
	memset( &initmsg, 0, sizeof(initmsg) );
	initmsg.sinit_num_ostreams = 5;
	initmsg.sinit_max_instreams = 5;
	initmsg.sinit_max_attempts = 4;
	setsockopt( sock, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg) );

	if (maxPending <= 0)
		listen(sock, MAXLISTENING);
	else
		listen(sock, maxPending);

	return sock;
}

/*
 * This function sctpServerSubnet creates a listening socket for the subnet given.
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
int sctpServerSubnet(int port, int maxPending, const char *subnet) {
	int sock = -1;

	struct sctp_initmsg initmsg;

	if (subnet == NULL || isIPv4Address(subnet) == 0) {
		pErr("Given subnet is null or invalid.\n");
		return -3;
	}

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);

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

	/* Specify that a maximum of 5 streams will be available per socket */
	memset( &initmsg, 0, sizeof(initmsg) );
	initmsg.sinit_num_ostreams = 5;
	initmsg.sinit_max_instreams = 5;
	initmsg.sinit_max_attempts = 4;
	setsockopt( sock, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg) );

	if (maxPending <= 0)
		listen(sock, MAXLISTENING);
	else
		listen(sock, maxPending);

	return sock;
}

/*
 * This function sctpAccept blocks waiting for connection requests.
 *
 *   sock = Socket descriptor.
 *   clientAddr = Remote hosts IP address after accept.
 *
 *   returns -1 on accept failed.
 *           else returns a valid socket descriptor for the new connection.
 */
int sctpAccept(int sock, struct sockaddr *clientAddr) {
	socklen_t addrLen = sizeof(struct sockaddr);
	int clientSock = accept(sock, clientAddr, &addrLen);

	if (clientSock < 0) {
		pErr("SCTP accept failed.\n");
		return -1;
	}

	return clientSock;
}

/*
 * This function sctpSend sends a NULL terminated string to remote host.
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
int sctpSend(int sock, const char *msg) {
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
 * This function sctpNumSend sends sends N number of bytes of data.
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
int sctpNumSend(int sock, const char *msg, int numBytes) {
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
 * This function stcpRecv receives a block of bytes from remote host.
 *
 *   sock = Socket descriptor.
 *   buf = Buffer to place received bytes.
 *   buf_len = Max size of buffer.
 *
 *   returns -1 on error
 *           else Number of bytes received.
 */
int sctpRecv(int sock, char *buf, int buf_len) {
	int r = recv(sock, buf, buf_len, 0);

	return r;
}

/*
 * This function sctpRecvString receives a block of bytes from remote host.
 * This string is null terminated.
 *
 *   sock = Socket descriptor.
 *   buf = Buffer to place received bytes.
 *   buf_len = Max size of buffer.
 *
 *   returns -1 on error
 *           else Number of bytes received.
 */
int sctpRecvString(int sock, char *buf, int buf_len) {
	int r = recv(sock, buf, buf_len, 0);

	if (r < buf_len)
		buf[r] = '\0';

	return r;
}

void sctpClose(int sock) {
	close(sock);		// ignore any errors.
}
