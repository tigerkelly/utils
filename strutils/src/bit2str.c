/*
 * Copyright (c) 2018 Richard Kelly Wiles (rkwiles@twc.com)
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
 * bit2str.c
 *
 * Description:
 *  Created on: Jul 19, 2018
 *      Author: Kelly Wiles
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "miscutils.h"

static char *tcpFlagNames[] = {
	"FIN_FLAG",
	"SYN_FLAG",
	"RESET_FLAG",
	"PUSH_FLAG",
	"ACK_FLAG",
	"URGENT_FLAG",
	"ECHO_FLAG",
	"CWR_FLAG",
	"NONCE_FLAG"
};

static char *tlsFlagNames[] = {
	"HANDSHAKE_FLAG",
	"ENCRYPTED_ALERT_FLAG",
	"APP_DATA_FLAG",
	"CYPHER_SPEC_FLAG"
};

// Used to lookup the text string of the sip_t enum
static char *sipMethodNames[] = {
	"UNKNOWN",
	"INVITE",
	"BYE",
	"REGISTER",
	"CANCEL",
	"ACK",
	"OPTIONS",
	"SUBSCRIBE",
	"NOTIFY",
	"PUBLISH",
	"REFER",
	"INFO",
	"UPDATE",
	"PRACK",
	"MESSAGE"
};

/*
 * Returns a pointer to static pointer array
 */
char **getSipNames() {
	return sipMethodNames;
}

char **getTcpNames() {
	return tcpFlagNames;
}

char **getTlsNames() {
	return tlsFlagNames;
}

/*
 * This function converts an integer into a string of the first
 * letter of each SIP method type present.
 */
char *sipBitString(uint32_t n, char *t) {

    int cnt = 0;
    char ptmp[17];
    memset(ptmp, 0, sizeof(ptmp));
    char **sipNames = getSipNames();
    for (int i = 15; i >= 0; i--) {
        if (isBit(n, i))
            ptmp[cnt++] = sipNames[i][0];
        else
            ptmp[cnt++] = '.';
    }
    ptmp[cnt] = '\0';
    strcpy(t, ptmp);
    return t;
}

/*
 * This function converts an integer into a string of the first
 * letter of each TCP flag type present.
 */
char *tcpBitString(uint32_t n, char *t) {

    int cnt = 0;
    char ptmp[17];
    memset(ptmp, 0, sizeof(ptmp));
    char **tcpNames = getTcpNames();
    for (int i = 8; i >= 0; i--) {
        if (isBit(n, i))
            ptmp[cnt++] = tcpNames[i][0];
        else
            ptmp[cnt++] = '.';
    }
    ptmp[cnt] = '\0';
    strcpy(t, ptmp);
    return t;
}

char *tcpBitNString(uint32_t n, char *t, int nBits) {

    int cnt = 0;
    char ptmp[17];
    memset(ptmp, 0, sizeof(ptmp));
    char **tcpNames = getTcpNames();
    for (int i = nBits; i >= 0; i--) {
        if (isBit(n, i))
            ptmp[cnt++] = tcpNames[i][0];
        else
            ptmp[cnt++] = '.';
    }
    ptmp[cnt] = '\0';
    strcpy(t, ptmp);
    return t;
}

/*
 * This function converts an integer into a string of the first
 * letter of each TLS flag type present.
 */
char *tlsBitString(uint32_t n, char *t) {

    int cnt = 0;
    char ptmp[17];
    memset(ptmp, 0, sizeof(ptmp));
    for (int i = 3; i >= 0; i--) {
        if (isBit(n, i))
            ptmp[cnt++] = tlsFlagNames[i][0];
        else
            ptmp[cnt++] = '.';
    }
    ptmp[cnt] = '\0';
    strcpy(t, ptmp);
    return t;
}

