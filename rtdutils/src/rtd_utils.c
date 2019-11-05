/*
 * rtd_utils.c
 *
 * Description: Helper functions for the RTD Engine.
 *  Created on: Nov 26, 2017
 *      Author: Kelly Wiles
 *   Copyright: Network Kinetix
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ini.h"
#include "logutils.h"
#include "miscutils.h"
#include "rtdengine.h"
#include "rtdutils.h"
#include "strutils.h"

IniFile *_rtdDB = NULL;
char iniDBData[31 * 1024];	// can not go over UDP limit of 64K

IdTrieTree *_lookupTree = NULL;

int _rtdInitFlag = 0;
static void _convert2Host(RtdCmd * rtdCmd);
static void _convert2Network(RtdCmd * rtdCmd);

extern IdTrieTree *ittInit();
extern int ittInsert(IdTrieTree *trie, char *ascii, int value);
extern int ittLookup(IdTrieTree *trie, char *ip);

/* This function rtdClient sets up and creates the UDP socket.
 *
 * localPort = Local port number to bind receive to.
 * rtdIP = IP address of rtdengine program.
 * rtdPort = Port rtdengine is listening on.
 * milliSecs = timeout value for receiving data in milliseconds, 0 = no timeout
 *
 * Returns NULL on error else RtdConn pointer.
 */
RtdConn *rtdClient(char *rtdIP, int rtdPort, int milliSecs) {

	RtdConn *rtdConn = (RtdConn *)calloc(1, sizeof(RtdConn));

	if (rtdConn == NULL) {
		printf("Can not allocate RtdConn.\n");
		return NULL;
	}

	memset((char *) &rtdConn->to, 0, sizeof(rtdConn->to));
	rtdConn->to.sin_family = AF_INET;
	rtdConn->to.sin_port = htons(rtdPort);

	memset((char *) &rtdConn->jsonTo, 0, sizeof(rtdConn->jsonTo));
	rtdConn->jsonTo.sin_family = AF_INET;
	rtdConn->jsonTo.sin_port = htons(rtdPort+1);

	rtdConn->toLen = sizeof(struct sockaddr_in);
	rtdConn->jsonToLen = sizeof(struct sockaddr_in);

	if (inet_aton(rtdIP, &rtdConn->to.sin_addr) == 0) {
		printf("inet_aton() failed %s\n", rtdIP);
		free(rtdConn);
		return NULL;
	}

	if (inet_aton(rtdIP, &rtdConn->jsonTo.sin_addr) == 0) {
		printf("inet_aton() failed %s\n", rtdIP);
		free(rtdConn);
		return NULL;
	}

	rtdConn->udpSock = udpClientBind(0, NULL);

	if (rtdConn->udpSock < 0) {
		printf("Failed binding socket local port.\n");
		free(rtdConn);
		return NULL;
	}

	rtdConn->udpJsonSock = udpClientBind(0, NULL);

	if (rtdConn->udpSock < 0) {
		printf("Failed binding socket local JSON port.\n");
		free(rtdConn);
		return NULL;
	}

	if (milliSecs > 0) {
		struct timeval t;
		t.tv_sec = (milliSecs / 1000);
		t.tv_usec = (milliSecs % 1000) * 1000;

		if (udpSetTimeout(rtdConn->udpSock, &t) < 0) {
			printf("Failed setting recv timeout.\n");
			free(rtdConn);
			return NULL;
		}

		if (udpSetTimeout(rtdConn->udpJsonSock, &t) < 0) {
			printf("Failed setting JSON recv timeout.\n");
			free(rtdConn);
			return NULL;
		}
	}

	int r = rtdIsUp(rtdConn);
	if (r < 0) {
		if (r == -1)
			printf("Could not send UDP packet.\n");
		else
			printf("RTD Engine failed to answer back. (timed out)\n");
		free(rtdConn);
		return NULL;
	}

	_rtdInitFlag = 1;

	iniDBData[0] = '\0';
	rtdGetDBInfo(rtdConn, iniDBData, sizeof(iniDBData));

	if (iniDBData[0] == '\0') {
		printf("No tables in RTD Engine.\n");
		free(rtdConn);
		return NULL;
	}

	_rtdDB = iniCreateBuf("_iniDB.ini", iniDBData, 1024);

	// Build lookup tree to find index by name of table or field.

	_lookupTree = ittInit();

	int maxKeys = iniGetKeyMax(_rtdDB);

	int n = 0;
	Section *secs = iniGetSectionNames(_rtdDB);

	for (int i = 0; i < maxKeys; i++, secs++) {
		if (secs->inUse == 1) {
			ittInsert(_lookupTree, secs->secName, n);
			n++;

//			printf("secName: %s\n", secs->secName);

			KV *kv = iniGetSectionKeys(_rtdDB, secs->secName);
			for (int z = 0; z < maxKeys; z++, kv++) {
				if (kv->inUse == 1) {
					ittInsert(_lookupTree, kv->key, z);
				}
			}
		}
	}

    printf("Leaveing rtdConn: %p\n", rtdConn);

	return rtdConn;
}

/* This function rtdCmdSend sends command to rtdengine.
 *
 * rtdConn = returned from rtdCreate()
 * rtdCmd = the operation to perform.
 *
 * Returns < 0 on error else number of byte transmitted.
 */
int rtdCmdSend(RtdConn *rtdConn, RtdCmd *rtdCmd) {

	if (_rtdInitFlag == 0) {
		printf("Must call rtdClient first.\n");
		return -1;
	}

	rtdCmd->status = 0;
	int r = udpSend(rtdConn->udpSock, (char *)rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);

	return r;
}

/* This function rtdCmdRecv reads returning value from rtdengine.
 *
 * rtdConn = returned from rtdCreate()
 * rtdCmd = the operation to perform.
 */
int rtdCmdRecv(RtdConn *rtdConn, RtdCmd *rtdCmd) {

	if (_rtdInitFlag == 0) {
		printf("Must call rtdClient first.\n");
		return -1;
	}

	int r = udpRecv(rtdConn->udpSock, (char *)rtdCmd, sizeof(RtdCmd), NULL, NULL);

	return r;
}

int rtdRecvTimeout(RtdConn *rtdConn, int milliSecs) {

	if (_rtdInitFlag == 0) {
		printf("Must call rtdClient first.\n");
		return -1;
	}

	if (rtdConn != NULL && milliSecs > 0) {
		struct timeval t;
		t.tv_sec = (milliSecs / 1000);
		t.tv_usec = (milliSecs % 1000) * 1000;
		if (udpSetTimeout(rtdConn->udpSock, &t) < 0) {
			printf("Setting recv timeout failed.\n");
			free(rtdConn);
			return -1;
		}
	}

	return 0;
}

/* This function _rtdRecvInfo is private to file.
 *
 */
//int _rtdRecvInfo(RtdConn *rtdConn, RtdInfo *rtdInfo) {
//
//	if (_rtdInitFlag == 0) {
//		printf("Must call rtdClient first.\n");
//		return -1;
//	}
//
//	int r = udpRecv(rtdConn->udpSock, (char *)rtdInfo, sizeof(RtdInfo), NULL, NULL);
//
//	return r;
//}

/* This function _rtdRecvDBInfo is private to file.
 *
 */
int _rtdRecvDBInfo(RtdConn *rtdConn, char *rtdDBInfo, int len) {

	if (_rtdInitFlag == 0) {
		printf("Must call rtdClient first.\n");
		return -1;
	}

	int r = udpRecv(rtdConn->udpSock, rtdDBInfo, len, NULL, NULL);

//	pOut("r: %d\n", r);

	return r;
}

static void _convert2Host(RtdCmd * rtdCmd) {

    // convert back to host order.
	rtdCmd->cmd = ntohl(rtdCmd->cmd);
	rtdCmd->id.tid = ntohs(rtdCmd->id.tid);
	rtdCmd->id.fid = ntohs(rtdCmd->id.fid);
	rtdCmd->id.idx = ntohs(rtdCmd->id.idx);
	//  printf("offset(status): %d\n", offsetof(RtdCmd, status));
	rtdCmd->status = ntohl(rtdCmd->status);
	rtdCmd->vType = ntohl(rtdCmd->vType);
	rtdCmd->vLen = ntohl(rtdCmd->vLen);

    switch(rtdCmd->vType) {
    case RTD_VARSHORT:
    case RTD_VARSHORT_ARRAY:
        rtdCmd->var.sv = ntohs(rtdCmd->var.sv);
        break;
    case RTD_VARINT:
    case RTD_VARINT_ARRAY:
        rtdCmd->var.iv = ntohl(rtdCmd->var.iv);
        break;
    case RTD_VARLONG:
    case RTD_VARLONG_ARRAY:
        rtdCmd->var.lv = ntohl(rtdCmd->var.lv);
        break;
    case RTD_VARCHAR:
    case RTD_VARCHAR_ARRAY:
    case RTD_VARSTRING_ARRAY:
    case RTD_VARSTRING:
    case RTD_VAREND:
        break;
    }
}

static void _convert2Network(RtdCmd * rtdCmd) {

    // convert back to host order.
	rtdCmd->cmd = htonl(rtdCmd->cmd);
	rtdCmd->id.tid = htons(rtdCmd->id.tid);
	rtdCmd->id.fid = htons(rtdCmd->id.fid);
	rtdCmd->id.idx = htons(rtdCmd->id.idx);
	rtdCmd->status = htonl(rtdCmd->status);
	rtdCmd->vType = htonl(rtdCmd->vType);
	rtdCmd->vLen = htonl(rtdCmd->vLen);

    switch(rtdCmd->vType) {
    case RTD_VARSHORT:
    case RTD_VARSHORT_ARRAY:
        rtdCmd->var.sv = htons(rtdCmd->var.sv);
        break;
    case RTD_VARINT:
    case RTD_VARINT_ARRAY:
        rtdCmd->var.iv = htonl(rtdCmd->var.iv);
        break;
    case RTD_VARLONG:
    case RTD_VARLONG_ARRAY:
        rtdCmd->var.lv = htonl(rtdCmd->var.lv);
        break;
    case RTD_VARCHAR:
    case RTD_VARCHAR_ARRAY:
    case RTD_VARSTRING_ARRAY:
    case RTD_VARSTRING:
    case RTD_VAREND:
        break;
    }
}

/*
 * Returns 1 if RTD Engine is up.
 */
int rtdIsUp(RtdConn *rtdConn) {
	RtdCmd rtdCmd;

	if (rtdConn == NULL || rtdConn->udpSock <= 0)
		return -1;

	rtdCmd.cmd = RTD_CHECK_UP;
	rtdCmd.id.tid = 0;
	rtdCmd.id.fid = 0;
	rtdCmd.id.idx = 0;
	rtdCmd.status = 0;

    _convert2Network(&rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		return -1;
	}

	r = udpRecv(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		return -2;
	}

    _convert2Host(&rtdCmd);

	return rtdCmd.status;
}

RtdCmd *rtdGetField(RtdConn *rtdConn, RtdId *rtdId, RtdCmd *rtdCmd) {

	if (rtdCmd == NULL || rtdConn == NULL || rtdConn->udpSock <= 0) {
		rtdCmd->status = -1;
		return rtdCmd;
	}

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0) {
		rtdCmd->status = -1;
		return rtdCmd;
	}

	rtdCmd->cmd = RTD_CMD_GET;
	rtdCmd->id.tid = rtdId->tid;
	rtdCmd->id.fid = rtdId->fid;
	rtdCmd->id.idx = rtdId->idx;
	rtdCmd->status = 0;
	rtdCmd->vLen = 0;

    _convert2Network(rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		rtdCmd->status = -1;
		return rtdCmd;
	}

	r = udpRecv(rtdConn->udpSock, (char *)rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		rtdCmd->status = -1;
		return rtdCmd;
	}

    _convert2Host(rtdCmd);

	return rtdCmd;
}

RtdCmd *rtdAddField(RtdConn *rtdConn, RtdId *rtdId, RtdCmd *rtdCmd) {

	if (rtdCmd == NULL || rtdConn == NULL || rtdConn->udpSock <= 0) {
		rtdCmd->status = -1;
		return rtdCmd;
	}

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0) {
		rtdCmd->status = -1;
		return rtdCmd;
	}

	rtdCmd->cmd = RTD_CMD_ADD;
	rtdCmd->id.tid = rtdId->tid;
	rtdCmd->id.fid = rtdId->fid;
	rtdCmd->id.idx = rtdId->idx;
	rtdCmd->status = 0;
	rtdCmd->vLen = 0;

    _convert2Network(rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		rtdCmd->status = -1;
		return rtdCmd;
	}

	r = udpRecv(rtdConn->udpSock, (char *)rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		rtdCmd->status = -1;
		return rtdCmd;
	}

    _convert2Host(rtdCmd);

	return rtdCmd;
}

RtdCmd *rtdSubField(RtdConn *rtdConn, RtdId *rtdId, RtdCmd *rtdCmd) {

	if (rtdCmd == NULL || rtdConn == NULL || rtdConn->udpSock <= 0) {
		rtdCmd->status = -1;
		return rtdCmd;
	}

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0) {
		rtdCmd->status = -1;
		return rtdCmd;
	}

	rtdCmd->cmd = RTD_CMD_SUB;
	rtdCmd->id.tid = rtdId->tid;
	rtdCmd->id.fid = rtdId->fid;
	rtdCmd->id.idx = rtdId->idx;
	rtdCmd->status = 0;
	rtdCmd->vLen = 0;

    _convert2Network(rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		rtdCmd->status = -1;
		return rtdCmd;
	}

	r = udpRecv(rtdConn->udpSock, (char *)rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		rtdCmd->status = -1;
		return rtdCmd;
	}

    _convert2Host(rtdCmd);

	return rtdCmd;
}

RtdCmd *rtdSetField(RtdConn *rtdConn, RtdId *rtdId, RtdCmd *rtdCmd) {
	if (rtdConn == NULL || rtdConn->udpSock <= 0) {
		return NULL;
	}

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0) {
		return NULL;
	}

	rtdCmd->cmd = RTD_CMD_SET;
	rtdCmd->id.tid = rtdId->tid;
	rtdCmd->id.fid = rtdId->fid;
	rtdCmd->id.idx = rtdId->idx;
	rtdCmd->status = 0;
	if (rtdCmd->vType != RTD_VARSTRING && rtdCmd->vType != RTD_VARSTRING_ARRAY)
		rtdCmd->vLen = 0;

    _convert2Network(rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		rtdCmd->status = -1;
		return rtdCmd;
	}

	r = udpRecv(rtdConn->udpSock, (char *)rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		rtdCmd->status = -1;
		return rtdCmd;
	}

    _convert2Host(rtdCmd);

	return rtdCmd;
}

int rtdSetChar(RtdConn *rtdConn, RtdId *rtdId, unsigned char v) {
	RtdCmd rtdCmd;

	if (rtdConn == NULL || rtdConn->udpSock <= 0)
		return -1;

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0)
		return -1;

	rtdCmd.cmd = RTD_CMD_SET;
	rtdCmd.id.tid = rtdId->tid;
	rtdCmd.id.fid = rtdId->fid;
	rtdCmd.id.idx = rtdId->idx;
	rtdCmd.status = 0;
	rtdCmd.vType = RTD_VARCHAR;
	rtdCmd.vLen = 0;
	rtdCmd.var.cv = v;

    _convert2Network(&rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		return r;
	}

	r = udpRecv(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		return r;
	}

    _convert2Host(&rtdCmd);

	return 0;
}

int rtdSetShort(RtdConn *rtdConn, RtdId *rtdId, unsigned short v) {
	RtdCmd rtdCmd;

	if (rtdConn == NULL || rtdConn->udpSock <= 0)
		return -1;

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0)
		return -1;

	rtdCmd.cmd = RTD_CMD_SET;
	rtdCmd.id.tid = rtdId->tid;
	rtdCmd.id.fid = rtdId->fid;
	rtdCmd.id.idx = rtdId->idx;
	rtdCmd.status = 0;
	rtdCmd.vType = RTD_VARCHAR;
	rtdCmd.vLen = 0;
	rtdCmd.var.sv = v;

    _convert2Network(&rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		return r;
	}

	r = udpRecv(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		return r;
	}

    _convert2Host(&rtdCmd);

	return 0;
}

int rtdSetInt(RtdConn *rtdConn, RtdId *rtdId, unsigned int v) {
	RtdCmd rtdCmd;

	if (rtdConn == NULL || rtdConn->udpSock <= 0)
		return -1;

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0)
		return -1;

	rtdCmd.cmd = RTD_CMD_SET;
	rtdCmd.id.tid = rtdId->tid;
	rtdCmd.id.fid = rtdId->fid;
	rtdCmd.id.idx = rtdId->idx;
	rtdCmd.status = 0;
	rtdCmd.vType = RTD_VARINT;
	rtdCmd.vLen = 0;
	rtdCmd.var.iv = v;

    _convert2Network(&rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		return r;
	}

	r = udpRecv(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		return r;
	}

    _convert2Host(&rtdCmd);

	return 0;
}

int rtdSetLong(RtdConn *rtdConn, RtdId *rtdId, unsigned long v) {
	RtdCmd rtdCmd;

	if (rtdConn == NULL || rtdConn->udpSock <= 0)
		return -1;

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0)
		return -1;

	rtdCmd.cmd = RTD_CMD_SET;
	rtdCmd.id.tid = rtdId->tid;
	rtdCmd.id.fid = rtdId->fid;
	rtdCmd.id.idx = rtdId->idx;
	rtdCmd.status = 0;
	rtdCmd.vType = RTD_VARLONG;
	rtdCmd.vLen = 0;
	rtdCmd.var.lv = v;

    _convert2Network(&rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		return r;
	}

	r = udpRecv(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		return r;
	}

    _convert2Host(&rtdCmd);

	return 0;
}

int rtdSetString(RtdConn *rtdConn, RtdId *rtdId, unsigned char *v) {
	RtdCmd rtdCmd;

	if (rtdConn == NULL || rtdConn->udpSock <= 0)
		return -1;

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0)
		return -1;

	rtdCmd.cmd = RTD_CMD_SET;
	rtdCmd.id.tid = rtdId->tid;
	rtdCmd.id.fid = rtdId->fid;
	rtdCmd.id.idx = rtdId->idx;
	rtdCmd.status = 0;
	rtdCmd.vType = RTD_VARSTRING;
	rtdCmd.vLen = strlen((char *)v);
	strcpy((char *)rtdCmd.var.av, (char *)v);

    _convert2Network(&rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		return r;
	}

	r = udpRecv(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		return r;
	}

    _convert2Host(&rtdCmd);

	return 0;
}

int rtdSetNString(RtdConn *rtdConn, RtdId *rtdId, unsigned char *v, int len) {
	RtdCmd rtdCmd;

	if (rtdConn == NULL || rtdConn->udpSock <= 0)
		return -1;

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0)
		return -1;

	rtdCmd.cmd = RTD_CMD_SET;
	rtdCmd.id.tid = rtdId->tid;
	rtdCmd.id.fid = rtdId->fid;
	rtdCmd.id.idx = rtdId->idx;
	rtdCmd.status = 0;
	rtdCmd.vType = RTD_VARSTRING;
	rtdCmd.vLen = len;
	strcpy((char *)rtdCmd.var.av, (char *)v);

    _convert2Network(&rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		return r;
	}

	r = udpRecv(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		return r;
	}

    _convert2Host(&rtdCmd);

	return 0;
}

int rtdGetChar(RtdConn *rtdConn, RtdId *rtdId, unsigned char *v) {
	RtdCmd rtdCmd;

	if (rtdConn == NULL || rtdConn->udpSock <= 0)
		return -1;

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0)
		return -1;

	rtdCmd.cmd = RTD_CMD_GET;
	rtdCmd.id.tid = rtdId->tid;
	rtdCmd.id.fid = rtdId->fid;
	rtdCmd.id.idx = rtdId->idx;
	rtdCmd.status = 0;
	rtdCmd.vType = RTD_VARCHAR;
	rtdCmd.vLen = 0;

    _convert2Network(&rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		return r;
	}

	r = udpRecv(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		return r;
	}

    _convert2Host(&rtdCmd);

	*v = rtdCmd.var.cv;

	return 0;
}

int rtdGetShort(RtdConn *rtdConn, RtdId *rtdId, unsigned short *v) {
	RtdCmd rtdCmd;

	if (rtdConn == NULL || rtdConn->udpSock <= 0)
		return -1;

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0)
		return -1;

	rtdCmd.cmd = RTD_CMD_GET;
	rtdCmd.id.tid = rtdId->tid;
	rtdCmd.id.fid = rtdId->fid;
	rtdCmd.id.idx = rtdId->idx;
	rtdCmd.status = 0;
	rtdCmd.vType = RTD_VARCHAR;
	rtdCmd.vLen = 0;

    _convert2Network(&rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		return r;
	}

	r = udpRecv(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		return r;
	}

    _convert2Host(&rtdCmd);

	*v = rtdCmd.var.sv;

	return 0;
}

int rtdGetInt(RtdConn *rtdConn, RtdId *rtdId, unsigned int *v) {
	RtdCmd rtdCmd;

	if (rtdConn == NULL || rtdConn->udpSock <= 0)
		return -1;

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0)
		return -1;

	rtdCmd.cmd = RTD_CMD_GET;
	rtdCmd.id.tid = rtdId->tid;
	rtdCmd.id.fid = rtdId->fid;
	rtdCmd.id.idx = rtdId->idx;
	rtdCmd.status = 0;
	rtdCmd.vType = RTD_VARINT;
	rtdCmd.vLen = 0;

    _convert2Network(&rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		return r;
	}

	r = udpRecv(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		return r;
	}

    _convert2Host(&rtdCmd);

	*v = rtdCmd.var.iv;

	return 0;
}

int rtdGetLong(RtdConn *rtdConn, RtdId *rtdId, unsigned long *v) {
	RtdCmd rtdCmd;

	if (rtdConn == NULL || rtdConn->udpSock <= 0)
		return -1;

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0)
		return -1;

	rtdCmd.cmd = RTD_CMD_GET;
	rtdCmd.id.tid = rtdId->tid;
	rtdCmd.id.fid = rtdId->fid;
	rtdCmd.id.idx = rtdId->idx;
	rtdCmd.status = 0;
	rtdCmd.vType = RTD_VARLONG;
	rtdCmd.vLen = 0;

    _convert2Network(&rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		return r;
	}

	r = udpRecv(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		return r;
	}

    _convert2Host(&rtdCmd);

	*v = rtdCmd.var.lv;

	return 0;
}

int rtdGetString(RtdConn *rtdConn, RtdId *rtdId, unsigned char *v) {
	RtdCmd rtdCmd;

	if (rtdConn == NULL || rtdConn->udpSock <= 0)
		return -1;

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0)
		return -1;

	rtdCmd.cmd = RTD_CMD_GET;
	rtdCmd.id.tid = rtdId->tid;
	rtdCmd.id.fid = rtdId->fid;
	rtdCmd.id.idx = rtdId->idx;
	rtdCmd.status = 0;
	rtdCmd.vType = RTD_VARSTRING;
	rtdCmd.vLen = 0;

    _convert2Network(&rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		return r;
	}

	r = udpRecv(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		return r;
	}

    _convert2Host(&rtdCmd);

	// The pointer to v needs to be zero set by the caller if they
	// want null terminated string.
	memcpy((char *)v, (char *)rtdCmd.var.av, rtdCmd.vLen);

	return 0;
}

int rtdGetNString(RtdConn *rtdConn, RtdId *rtdId, unsigned char *v, int len) {
	RtdCmd rtdCmd;

	if (rtdConn == NULL || rtdConn->udpSock <= 0)
		return -1;

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0)
		return -1;

	rtdCmd.cmd = RTD_CMD_SET;
	rtdCmd.id.tid = rtdId->tid;
	rtdCmd.id.fid = rtdId->fid;
	rtdCmd.id.idx = rtdId->idx;
	rtdCmd.status = 0;
	rtdCmd.vType = RTD_VARSTRING;
	rtdCmd.vLen = 0;

    _convert2Network(&rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		return r;
	}

	r = udpRecv(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		return r;
	}

    _convert2Host(&rtdCmd);

	// The pointer to v needs to be zero set by the caller if they
	// want null terminated string.
	memcpy((char *)v, (char *)rtdCmd.var.av, (len < rtdCmd.vLen) ? len : rtdCmd.vLen);

	return 0;
}

int rtdAddChar(RtdConn *rtdConn, RtdId *rtdId, unsigned char v) {
	RtdCmd rtdCmd;

	if (rtdConn == NULL || rtdConn->udpSock <= 0)
		return -1;

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0)
		return -1;

	rtdCmd.cmd = RTD_CMD_ADD;
	rtdCmd.id.tid = rtdId->tid;
	rtdCmd.id.fid = rtdId->fid;
	rtdCmd.id.idx = rtdId->idx;
	rtdCmd.status = 0;
	rtdCmd.vType = RTD_VARCHAR;
	rtdCmd.vLen = 0;
	rtdCmd.var.cv = v;

    _convert2Network(&rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		return r;
	}

	r = udpRecv(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		return r;
	}

	return 0;
}

int rtdAddShort(RtdConn *rtdConn, RtdId *rtdId, unsigned short v) {
	RtdCmd rtdCmd;

	if (rtdConn == NULL || rtdConn->udpSock <= 0)
		return -1;

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0)
		return -1;

	rtdCmd.cmd = RTD_CMD_ADD;
	rtdCmd.id.tid = rtdId->tid;
	rtdCmd.id.fid = rtdId->fid;
	rtdCmd.id.idx = rtdId->idx;
	rtdCmd.status = 0;
	rtdCmd.vType = RTD_VARCHAR;
	rtdCmd.vLen = 0;
	rtdCmd.var.sv = v;

    _convert2Network(&rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		return r;
	}

	r = udpRecv(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		return r;
	}

	return 0;
}

int rtdAddInt(RtdConn *rtdConn, RtdId *rtdId, unsigned int v) {
	RtdCmd rtdCmd;

	if (rtdConn == NULL || rtdConn->udpSock <= 0)
		return -1;

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0)
		return -1;

	rtdCmd.cmd = RTD_CMD_ADD;
	rtdCmd.id.tid = rtdId->tid;
	rtdCmd.id.fid = rtdId->fid;
	rtdCmd.id.idx = rtdId->idx;
	rtdCmd.status = 0;
	rtdCmd.vType = RTD_VARINT;
	rtdCmd.vLen = 0;
	rtdCmd.var.iv = v;

    _convert2Network(&rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		return r;
	}

	r = udpRecv(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		return r;
	}

	return 0;
}

int rtdAddLong(RtdConn *rtdConn, RtdId *rtdId, unsigned long v) {
	RtdCmd rtdCmd;

	if (rtdConn == NULL || rtdConn->udpSock <= 0)
		return -1;

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0)
		return -1;

	rtdCmd.cmd = RTD_CMD_ADD;
	rtdCmd.id.tid = rtdId->tid;
	rtdCmd.id.fid = rtdId->fid;
	rtdCmd.id.idx = rtdId->idx;
	rtdCmd.status = 0;
	rtdCmd.vType = RTD_VARLONG;
	rtdCmd.vLen = 0;
	rtdCmd.var.lv = v;

    _convert2Network(&rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		return r;
	}

	r = udpRecv(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		return r;
	}

	return 0;
}

int rtdSubChar(RtdConn *rtdConn, RtdId *rtdId, unsigned char v) {
	RtdCmd rtdCmd;

	if (rtdConn == NULL || rtdConn->udpSock <= 0)
		return -1;

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0)
		return -1;

	rtdCmd.cmd = RTD_CMD_SUB;
	rtdCmd.id.tid = rtdId->tid;
	rtdCmd.id.fid = rtdId->fid;
	rtdCmd.id.idx = rtdId->idx;
	rtdCmd.status = 0;
	rtdCmd.vType = RTD_VARCHAR;
	rtdCmd.vLen = 0;
	rtdCmd.var.cv = v;

    _convert2Network(&rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		return r;
	}

	r = udpRecv(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		return r;
	}

	return 0;
}

int rtdSubShort(RtdConn *rtdConn, RtdId *rtdId, unsigned short v) {
	RtdCmd rtdCmd;

	if (rtdConn == NULL || rtdConn->udpSock <= 0)
		return -1;

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0)
		return -1;

	rtdCmd.cmd = RTD_CMD_SUB;
	rtdCmd.id.tid = rtdId->tid;
	rtdCmd.id.fid = rtdId->fid;
	rtdCmd.id.idx = rtdId->idx;
	rtdCmd.status = 0;
	rtdCmd.vType = RTD_VARCHAR;
	rtdCmd.vLen = 0;
	rtdCmd.var.sv = v;

    _convert2Network(&rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		return r;
	}

	r = udpRecv(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		return r;
	}

	return 0;
}

int rtdSubInt(RtdConn *rtdConn, RtdId *rtdId, unsigned int v) {
	RtdCmd rtdCmd;

	if (rtdConn == NULL || rtdConn->udpSock <= 0)
		return -1;

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0)
		return -1;

	rtdCmd.cmd = RTD_CMD_SUB;
	rtdCmd.id.tid = rtdId->tid;
	rtdCmd.id.fid = rtdId->fid;
	rtdCmd.id.idx = rtdId->idx;
	rtdCmd.status = 0;
	rtdCmd.vType = RTD_VARINT;
	rtdCmd.vLen = 0;
	rtdCmd.var.iv = v;

    _convert2Network(&rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		return r;
	}

	r = udpRecv(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		return r;
	}

	return 0;
}

int rtdSubLong(RtdConn *rtdConn, RtdId *rtdId, unsigned long v) {
	RtdCmd rtdCmd;

	if (rtdConn == NULL || rtdConn->udpSock <= 0)
		return -1;

	if (rtdId == NULL || rtdId->tid < 0 || rtdId->fid < 0 || rtdId->idx < 0)
		return -1;

	rtdCmd.cmd = RTD_CMD_SUB;
	rtdCmd.id.tid = rtdId->tid;
	rtdCmd.id.fid = rtdId->fid;
	rtdCmd.id.idx = rtdId->idx;
	rtdCmd.status = 0;
	rtdCmd.vType = RTD_VARLONG;
	rtdCmd.vLen = 0;
	rtdCmd.var.lv = v;

    _convert2Network(&rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), &(rtdConn->to), rtdConn->toLen);
	if (r < 0) {
		return r;
	}

	r = udpRecv(rtdConn->udpSock, (char *)&rtdCmd, sizeof(RtdCmd), NULL, NULL);
	if (r < 0) {
		return r;
	}

	return 0;
}

/*
 * Builds a RtdId of the table and field.
 *
 * Caller must free the RtdId pointer when done.
 */
RtdId *rtdGetId(char *tblName, char *fldName, int index, RtdId *rtdId) {

//	RtdId *rtdId = (RtdId *)calloc(1, sizeof(RtdId));

	rtdId->tid = ittLookup(_lookupTree, tblName);
	if (rtdId->tid < 0) {
		free(rtdId);
		return NULL;
	}
	rtdId->fid = ittLookup(_lookupTree, fldName);
	if (rtdId->fid < 0) {
		free(rtdId);
		return NULL;
	}

	rtdId->idx = index;

	return rtdId;
}

/* Find the first table with the given name and
 * returns table id only.
 */
int rtdGetTableId(char *tblName) {
	return ittLookup(_lookupTree, tblName);
}

/*
 * Find the first field of the name given and
 * returns both table id and field id.
 */
int rtdGetFieldId(char *fldName) {
	return ittLookup(_lookupTree, fldName);
}

int rtdGetFieldInfo(char *tblName, char *fldName, RtdField *rtdField) {

	char *v = iniGetValue(_rtdDB, tblName, fldName);
	if (v != NULL) {
		printf("v: %s\n", v);
	}

	return 0;
}

int rtdGetDBInfo(RtdConn *rtdConn, char *iniDBData, int len) {
	RtdCmd rtdCmd;

	if (_rtdInitFlag == 0) {
		printf("Must call rtdClient first.\n");
		return -1;
	}

	rtdCmd.status = 0;
	rtdCmd.cmd = RTD_GET_DB;

    _convert2Network(&rtdCmd);

	int r = udpSend(rtdConn->udpSock, (char *) &rtdCmd, sizeof(RtdCmd), &rtdConn->to, rtdConn->toLen);

	if (r < 0) {
		printf("Failed to send RTD_GET_DB request.\n");
		return -1;
	}

	r = _rtdRecvDBInfo(rtdConn, iniDBData, len);

	return r;
}

char *rtdTable2Str(int tid, char *buf) {
	int maxKeys = iniGetKeyMax(_rtdDB);

	Section *secs = iniGetSectionNames(_rtdDB);

	*buf = '\0';

	for (int i = 0; i < maxKeys; i++, secs++) {
		if (secs->inUse == 1) {
			if (i == tid) {
				strcpy(buf, secs->secName);
				break;
			}
		}
	}

	return buf;
}

char *rtdField2Str(int tid, int fid, char *buf) {
	int maxKeys = iniGetKeyMax(_rtdDB);

	Section *secs = iniGetSectionNames(_rtdDB);

	*buf = '\0';

	for (int i = 0; i < maxKeys; i++, secs++) {
		if (secs->inUse == 1) {
			if (i == tid) {
				KV *kv = iniGetSectionKeys(_rtdDB, secs->secName);
				for (int z = 0; z < maxKeys; z++, kv++) {
					if (kv->inUse == 1) {
						if (z == fid) {
							strcpy(buf, kv->key);
							break;
						}
					}
				}
			}
		}
	}

	return buf;
}

int rtdJson(RtdConn *rtdConn, char *json, char *buf, int len) {
	if (rtdConn == NULL || rtdConn->udpSock <= 0)
		return -1;

	int r = udpSend(rtdConn->udpJsonSock, json, strlen(json), &rtdConn->jsonTo, rtdConn->jsonToLen);

	if (r < 0) {
		printf("send failed. %d\n", r);
		return r;
	}

	r = udpRecv(rtdConn->udpJsonSock, buf, len, NULL, NULL);
	if (r < 0) {
		printf("recv failed. %d\n", r);
		return r;
	}

	return 0;
}

RtdVarType rtdTypeStr2Enum(char *sType) {
	RtdVarType vType = RTD_VAREND;

	if (strcasecmp(sType, "RTD_VARCHAR") == 0)
		vType = RTD_VARCHAR;
	else if (strcasecmp(sType, "RTD_VARSHORT") == 0)
		vType = RTD_VARSHORT;
	else if (strcasecmp(sType, "RTD_VARINT") == 0)
		vType = RTD_VARINT;
	else if (strcasecmp(sType, "RTD_VARLONG") == 0)
		vType = RTD_VARLONG;
	else if (strcasecmp(sType, "RTD_VARSTRING") == 0)
		vType = RTD_VARSTRING;
	else if (strcasecmp(sType, "RTD_VARCHAR_ARRAY") == 0)
		vType = RTD_VARCHAR_ARRAY;
	else if (strcasecmp(sType, "RTD_VARSHORT_ARRAY") == 0)
		vType = RTD_VARSHORT_ARRAY;
	else if (strcasecmp(sType, "RTD_VARINT_ARRAY") == 0)
		vType = RTD_VARINT_ARRAY;
	else if (strcasecmp(sType, "RTD_VARLONG_ARRAY") == 0)
		vType = RTD_VARLONG_ARRAY;
	else if (strcasecmp(sType, "RTD_VARSTRING_ARRAY") == 0)
		vType = RTD_VARSTRING_ARRAY;

	return vType;
}
