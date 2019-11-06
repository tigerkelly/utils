/*
 * rtdengine.h
 *
 * Description:
 *  Created on: Nov 25, 2017
 *      Author: Kelly Wiles
 */

#ifndef INCS_RTDENGINE_H_
#define INCS_RTDENGINE_H_

#include <netinet/in.h>

#define DFLT_LISTEN_PORT 	7587
#define DFLT_FREE_BLOCKS	1000

#define MAX_TABLE_NAME_SIZE	32
#define MAX_FIELD_NAME_SIZE	32
#define MAX_VAR_ARRAY_SIZE	64

#define RTDCMD_UDP_SIZE		128
#define RTDCMD_JSON_SIZE	1400
#define MAX_DBS_ALLOWED		32

#define DFLT_TABLE_COUNT	256
#define DFLT_FIELD_COUNT	1024

typedef struct _rtdId {
	short tid;
	short fid;
	short idx;
	short dummy;		// added for packing
} RtdId;

typedef struct _rtdEngCfg {
	char rtdListen[32];
	short rtdPort;
	short rtdJsonPort;
	int rtdProcesses;
	int rtdBlocks;
	int rtdSyncTime;
	int rtdSyncFlag;
} RtdEngCfg;

typedef enum {
	RTD_CMD_GET,
	RTD_CMD_SET,
	RTD_CMD_ADD,
	RTD_CMD_SUB,
	RTD_GET_INFO,
	RTD_TABLE_ID,
	RTD_FIELD_ID,
	RTD_GET_DB,
	RTD_CHECK_UP
} RtdCmdType;

typedef enum {
	RTD_VARCHAR,
	RTD_VARSHORT,
	RTD_VARINT,
	RTD_VARLONG,
	RTD_VARSTRING,
	RTD_VARCHAR_ARRAY,
	RTD_VARSHORT_ARRAY,
	RTD_VARINT_ARRAY,
	RTD_VARLONG_ARRAY,
	RTD_VARSTRING_ARRAY,
	RTD_VAREND
} RtdVarType;

typedef struct _rtdInfo {
	RtdId id;
	RtdVarType vType;
	int arraySize;
	char tblName[MAX_TABLE_NAME_SIZE];
	char fldName[MAX_FIELD_NAME_SIZE];
} RtdInfo;

typedef enum {
	JSON = 1,
	RTDCMD,
	WEBSOCK
} RequestType;

typedef struct _rtdBlk {
	int idx;
	RequestType typ;
	char data[RTDCMD_UDP_SIZE];
} RtdBlk;

typedef enum {
	RTD_STATUS_RECVD,
	RTD_STATUS_OK,
	RTD_STATUS_ERR
} RtdStatus;

/*
 * If vType is an array then the index offset in the memory file
 * is in the lv field.
 */
typedef struct _rtdCmd {
	RtdId id;
	RtdCmdType cmd;
	RtdVarType vType;
	int vLen;
	int status;
	union {
		unsigned char cv;
		unsigned short sv;
		unsigned int iv;
#if (__SIZEOF_LONG__ == 4)
		unsigned long lv;
#else
		unsigned int lv;
#endif
		unsigned char av[MAX_VAR_ARRAY_SIZE];
	} var;
	struct sockaddr_in from;
} __attribute__((packed)) RtdCmd;

typedef struct _RtdField {		// must be a multiple of 4
	char fieldName[MAX_FIELD_NAME_SIZE];
	RtdVarType vType;
	int vLen;
	union {
		unsigned char cv;
		unsigned short sv;
		unsigned int iv;
#if (__SIZEOF_LONG__ == 4)
		unsigned long lv;
#else
        unsigned int lv;
#endif
		unsigned char av[MAX_VAR_ARRAY_SIZE];
	} var;
} __attribute__((packed)) RtdField;

typedef struct _rtdTableList {
	char tblName[32];
	int fldCount;
} RtdTableList;

typedef struct _rtdDB {
	int tfd;
	char *addr;
	char tblName[MAX_TABLE_NAME_SIZE];
} RtdDB;

// The *next array on a 64bit system is 288 bytes in size,
// cause on 64bit systems pointers are 8 bytes long.
typedef struct _dbNode {
	short idx;
	short isEnd;
	short inUse;
	short useCount;
	struct _dbNode *next[36];
} DbNode;

typedef struct _dbTrieTree {
	DbNode *root;
} DbTrieTree;

DbTrieTree *dbInit();
int dbInsert(DbTrieTree *trie, char *ascii, int value);
DbNode *dbFindEnd(DbTrieTree *trie, char *ascii);
int dbLookup(DbTrieTree *trie, char *ip);
int dbNumEntries(DbTrieTree *trie);


#endif /* INCS_RTDENGINE_H_ */
