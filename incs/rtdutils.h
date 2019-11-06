/*
 * rtdutils.h
 *
 * Description:
 *  Created on: Nov 26, 2017
 *      Author: Kelly Wiles
 */

#ifndef INCS_RTSUTILS_H_
#define INCS_RTSUTILS_H_

#include "rtdengine.h"

#include <sys/types.h>

#ifndef TRIE_NULL
#define TRIE_NULL ((void *) 0)
#endif

// The *next array on a 64bit system is 288 bytes in size,
// cause on 64bit systems pointers are 8 bytes long.
typedef struct _idNode {
	short idx;
	short isEnd;
	short inUse;
	short useCount;
	struct _idNode *next[36];
} IdNode;

typedef struct _idTrieTree {
	IdNode *root;
} IdTrieTree;

typedef struct _rtdConn {
	int udpSock;
	int udpJsonSock;
	int localPort;
	int toLen;
	struct sockaddr_in to;
	int jsonToLen;
	struct sockaddr_in jsonTo;
} RtdConn;

RtdConn *rtdClient(char *rtdIP, int rtdPort, int milliSecs);
int rtdCmdSend(RtdConn *rtdConn, RtdCmd *rtdCmd);
int rtdCmdRecv(RtdConn *rtdConn, RtdCmd *rtdCmd);
int rtdGetInfo(RtdConn *rtdConn, int (*callBack)(RtdInfo *rtdInfo));
int rtdIsUp(RtdConn * rtdConn);
int rtdRecvTimeout(RtdConn *rtdConn, int milliSecs);

RtdVarType rtdTypeStr2Enum(char *sType);

RtdCmd *rtdGetField(RtdConn *rtdConn, RtdId *rtdId, RtdCmd *rtdCmd);
RtdCmd *rtdSetField(RtdConn *rtdConn, RtdId *rtdId, RtdCmd *rtdCmd);
RtdCmd *rtdAddField(RtdConn *rtdConn, RtdId *rtdId, RtdCmd *rtdCmd);
RtdCmd *rtdSubField(RtdConn *rtdConn, RtdId *rtdId, RtdCmd *rtdCmd);
unsigned char rtdGetFieldChar(RtdCmd *rtdCmd);
unsigned short rtdGetFieldShort(RtdCmd *rtdCmd);
unsigned int rtdGetFieldInt(RtdCmd *rtdCmd);
unsigned int rtdGetFieldLong(RtdCmd *rtdCmd);
char *rtdGetFieldArray(RtdCmd *rtdCmd);


int rtdSetChar(RtdConn *rtdConn, RtdId *rtdId, unsigned char v);
int rtdSetShort(RtdConn *rtdConn, RtdId *rtdId, unsigned short v);
int rtdSetInt(RtdConn *rtdConn, RtdId *rtdId, unsigned int v);
int rtdSetLong(RtdConn *rtdConn, RtdId *rtdId, unsigned long v);
int rtdSetString(RtdConn *rtdConn, RtdId *rtdId, unsigned char *v);
int rtdSetNString(RtdConn *rtdConn, RtdId *rtdId, unsigned char *v, int len);

int rtdGetChar(RtdConn *rtdConn, RtdId *rtdId, unsigned char *v);
int rtdGetShort(RtdConn *rtdConn, RtdId *rtdId, unsigned short *v);
int rtdGetInt(RtdConn *rtdConn, RtdId *rtdId, unsigned int *v);
int rtdGetLong(RtdConn *rtdConn, RtdId *rtdId, unsigned long *v);
int rtdGetString(RtdConn *rtdConn, RtdId *rtdId, unsigned char *v);
int rtdGetNString(RtdConn *rtdConn, RtdId *rtdId, unsigned char *v, int len);

int rtdAddChar(RtdConn *rtdConn, RtdId *rtdId, unsigned char v);
int rtdAddShort(RtdConn *rtdConn, RtdId *rtdId, unsigned short v);
int rtdAddInt(RtdConn *rtdConn, RtdId *rtdId, unsigned int v);
int rtdAddLong(RtdConn *rtdConn, RtdId *rtdId, unsigned long v);

int rtdSubChar(RtdConn *rtdConn, RtdId *rtdId, unsigned char v);
int rtdSubShort(RtdConn *rtdConn, RtdId *rtdId, unsigned short v);
int rtdSubInt(RtdConn *rtdConn, RtdId *rtdId, unsigned int v);
int rtdSubLong(RtdConn *rtdConn, RtdId *rtdId, unsigned long v);

int rtdGetTableId(char *tblName);
int rtdGetFieldId(char *fldName);
int rtdGetFieldInfo(char *tblName, char *fldName, RtdField *rtdField);
char *rtdTable2Str(int tid, char *buf);
char *rtdField2Str(int tid, int fid, char *buf);

RtdId *rtdGetId(char *tblName, char *fldName, int index, RtdId *rtdId);
int rtdGetDBInfo(RtdConn *rtdConn, char *iniDBData, int len);

int rtdJson(RtdConn *rtdConn, char *json, char *buf, int len);

void rtdPrintCmd(RtdCmd *rtdCmd);

#endif /* INCS_RTSUTILS_H_ */
