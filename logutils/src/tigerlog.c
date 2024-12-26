
/*
 * Copyright (c) 2024 Richard Kelly Wiles (rkwiles@twc.com)
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

/* Functions used with the TigerLog program. */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#include "logutils.h"
#include "tigerlog.h"

#define MAX_MSG_SIZE	(32 * 1024)
#define MAX_BUF_SIZE	(60 * 1024)

unsigned short _port;
char _logMsg[MAX_BUF_SIZE];

int tigerLogSocket() {
	int sock;

	struct servent *sp = getservbyname("tigerlog", "udp");
    if (sp == NULL) {
		return -1;
    }
    _port = sp->s_port;

	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        return -2;
    }

	return sock;
}

int tigerLogNew(int sock, char *logName) {
	sprintf(_logMsg, "NewLog:/var/log:%s", logName);

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));

    // Server address information
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = _port;
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
		return -1;
    }

	if (sendto(sock, (const char *)_logMsg, strlen(_logMsg), 0,
               (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("sendto failed");
		return -2;
    }

	return 0;
}

int tigerLogMsg(int sock, char *logName, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);

	char msg[MAX_MSG_SIZE + 1];

	vsprintf(msg, fmt, ap);

	sprintf(_logMsg, "LogMsg~%s~%s", logName, msg);

	va_end(ap);

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));

    // Server address information
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = _port;
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
		return -1;
    }

	if (sendto(sock, (const char *)_logMsg, strlen(_logMsg), 0,
               (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("sendto failed");
		return -2;
    }

	return 0;
}
