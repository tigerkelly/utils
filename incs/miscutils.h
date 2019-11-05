
#ifndef _NKXUTILS_H
#define _NKXUTILS_H

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <stdatomic.h>

#include "logutils.h"

#define SOL_SCTP	132
#define MASTER_LISTEN_PORT (9999)
#define MASTER_LISTEN_ADDR ("127.0.0.1")

// USE_SCTP is defined in the Makefiles.
// If USE_SCTP is set to 1 then SCTP else if 0 then TCP
#if USE_SCTP	// Stream Control Transmission Protocol
#define Connect(addr, port) sctpConnect(addr, port)
#define TimeoutConnect(addr, port, timeout) sctpTimeoutConnect(addr, port, timeout)
#define Server(port, pending) sctpServer(port, pending)
#define ServerSubnet(port, pending, subnet) sctpServer(port, pending, subnet)
#define Accept(sock, addr) sctpAccept(sock, addr)
#define Recv(sock, data, len)	sctpRecv(sock, data, len)
#define RecvString(sock, data, len)	sctpRecvString(sock, data, len)
#define Send(sock, txt) sctpSend(sock, txt)
#define NumSend(sock, txt, len) sctpNumSend(sock, txt, len)
#define Close(sock) sctpClose(sock)
#endif

#if USE_TCP		// Transmission Control Protocol
#define Connect(addr, port) tcpConnect(addr, port)
#define TimeoutConnect(addr, port, timeout) tcpTimeoutConnect(addr, port, timeout)
#define Server(port, pending) tcpServer(port, pending)
#define ServerSubnet(port, pending, subnet) tcpServer(port, pending, subnet)
#define Accept(sock, addr) tcpAccept(sock, addr)
#define Recv(sock, data, len)	tcpRecv(sock, data, len)
#define RecvString(sock, data, len)	tcpRecvString(sock, data, len)
#define Send(sock, txt) tcpSend(sock, txt)
#define NumSend(sock, txt, len) tcpNumSend(sock, txt, len)
#define Close(sock) tcpClose(sock)
#endif

#if USE_RCD		// Record Transmission Control Protocol
#define Connect(addr, port) tcpConnect(addr, port)
#define TimeoutConnect(addr, port, timeout) tcpTimeoutConnect(addr, port, timeout)
#define Server(port, pending) tcpServer(port, pending)
#define ServerSubnet(port, pending, subnet) tcpServer(port, pending, subnet)
#define Accept(sock, addr) tcpAccept(sock, addr)
#define Recv(sock, data, len)	recRecv(sock, data, len)
#define IDRecv(sock, id, data, len)	recIDRecv(sock, id, data, len)
#define RecvString(sock, data, len)	recRecvString(sock, data, len)
#define Send(sock, txt) recSend(sock, txt)
#define NumSend(sock, txt, len) recNumSend(sock, txt, len)
#define IDSend(sock, id, txt) recIDSend(sock, id, txt)
#define NumIDSend(sock, id, txt, len) recNumIDSend(sock, id, txt, len)
#define Close(sock) recClose(sock)
#endif

int udpServer (const char *server, int port);
int udpClient (int port);
int udpClientBind(int port, const char *client);
int udpSend(int sock, const char *msg, int msgLen, struct sockaddr_in *remoteAddr, int remoteAddrLen);
void getRemoteAddr (const char *remoteAddrStr, int remotePort, struct sockaddr_in *remoteAddr);
int udpRecv (int sock, char *buf, int bufLen, struct sockaddr_in *remote, socklen_t *remoteAddrLen);
int udpSetTimeout(int sock, struct timeval *tv);
void udpClose(int sock);

/* Connected UDP sockets */

int udpConnClient(const char *client, int cPort, const char *server, int sPort);
int udpConnServer(const char *server, int port);
int udpConnSend(int sock, const char *msg, int len);
int udpConnRecv(int sock, char *buf, int buf_len);
void udpConnClose(int sock);
int udpConnBytesRecvd(int sock);



#define pOut(fmt, ...) \
        do { fprintf(stdout, "INFO: %s [%s](%d): " fmt, __FILE__, \
            __FUNCTION__, __LINE__, ##__VA_ARGS__); fflush(stdout);} while (0)
#define pErr(fmt, ...) \
        do { fprintf(stderr, "ERROR: %s [%s](%d): " fmt, __FILE__, \
            __FUNCTION__, __LINE__, ##__VA_ARGS__); fflush(stderr);} while (0)
#define pBug(fmt, ...) \
        do { fprintf(stdout, "DEBUG: %s [%s](%d): " fmt, __FILE__, \
            __FUNCTION__, __LINE__, ##__VA_ARGS__); fflush(stdout);} while (0)

#define isBit(x, bit) (((x) >> (bit)) & 1)
#define setBit(x, bit) x |= (1 << bit)
#define clearBit(x, bit) x &= (~(1 << bit))

#define Info(txt, ...) \
    do { logInfo("INFO: %s [%s](%d): " txt, __FILE__, \
        __FUNCTION__, __LINE__, ##__VA_ARGS__);} while (0)
#define Err(txt, ...) \
    do { logErr("ERROR: %s [%s](%d): "txt, __FILE__, \
        __FUNCTION__, __LINE__, ##__VA_ARGS__);} while (0)
#define Debug(txt, ...) \
    do { logDebug("DEBUG: %s [%s](%d): " txt, __FILE__, \
    	__FUNCTION__, __LINE__, ##__VA_ARGS__);} while (0)
#define Notice(txt, ...) \
    do { logNotice("NOTICE: %s [%s](%d): " txt, __FILE__, \
        __FUNCTION__, __LINE__, ##__VA_ARGS__);} while (0)
#define Warn(txt, ...) \
    do { logWarn("WARNING: %s [%s](%d): " txt, __FILE__, \
        __FUNCTION__, __LINE__, ##__VA_ARGS__);} while (0)

#define aBug(flg, fmt, ...) \
		do { \
			if (adIsSet(flg)) \
				logDebug("ABUG: %s [%s](%d): " fmt, __FILE__, \
					__FUNCTION__, __LINE__, ##__VA_ARGS__);} while (0)

/* AtomicExchange is used to compare and set p and return 1 on success else 0
 * p pointer to location to test.
 * e pointer to expected value at p.
 * if p equal e then set p to n and return 1 else 0
 * All three arguments are pointers. */
#define AtomicExchange(p, e, n) \
	__atomic_compare_exchange(p, e, n, 1, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE)

// These defines return the new value after operation.
#define AtomicAdd(p, n) 		__atomic_add_fetch(p, n, __ATOMIC_SEQ_CST)
#define AtomicSub(p, n) 		__atomic_sub_fetch(p, n, __ATOMIC_SEQ_CST)

#define AtomicAnd(p, n)			__atomic_and_fetch(p, n, __ATOMIC_SEQ_CST)
#define AtomicXor(p, n)			__atomic_xor_fetch(p, n, __ATOMIC_SEQ_CST)
#define AtomicOr(p, n)			__atomic_or_fetch(p, n, __ATOMIC_SEQ_CST)
#define AtomicNand(p, n)		__atomic_nand_fetch(p, n, __ATOMIC_SEQ_CST)

// These defines return the previous value before the operation.
#define AtomicFetchAdd(p, n) 	__atomic_fetch_add(p, n, __ATOMIC_SEQ_CST)
#define AtomicFetchSub(p, n) 	__atomic_fetch_sub(p, n, __ATOMIC_SEQ_CST)

#define AtomicFetchAnd(p, n)	__atomic_fetch_and(p, n, __ATOMIC_SEQ_CST)
#define AtomicFetchXor(p, n)	__atomic_fetch_xor(p, n, __ATOMIC_SEQ_CST)
#define AtomicFetchOr(p, n)		__atomic_fetch_or(p, n, __ATOMIC_SEQ_CST)
#define AtomicFetchNand(p, n)	__atomic_fetch_nand(p, n, __ATOMIC_SEQ_CST)

// returns the value at type *v.
#define AtomicGet(p)			__atomic_load_n(p, __ATOMIC_SEQ_CST)
// no return value. type *p, type n
#define AtomicSet(p, n)			__atomic_store_n(p, n, __ATOMIC_SEQ_CST)
//  It writes v into p, and returns the previous contents of p.
#define AtomicFetchSet(p, n) 	__atomic_exchange_n(p, n, __ATOMIC_SEQ_CST)

// This performs an atomic test-and-set operation on the byte at *ptr. The byte is
// set to some implementation defined nonzero "set" value and the return value is true if and
// only if the previous contents were "set". It should be only used for operands of type bool or char.
#define AtomicTestSet(p)		__atomic_test_and_set(p, __ATOMIC_SEQ_CST)
// This performs an atomic clear operation on *ptr. After the operation, *ptr contains 0.
// It should be only used for operands of type bool or char and in conjunction with __atomic_test_and_set.
// For other types it may only clear partially. If the type is not bool prefer using __atomic_store.
#define AtomicClear(p)			__atmoic_clear(p, __ATOMIC_SEQ_CST)

typedef struct _treeNode {
    char     *key;
    char     call_type;           // 'V'oice or 'S'MS, so we store both voice calls and text messages in the same table.
    time_t   timestamp;
    struct _treeNode *parent;
    struct _treeNode *left;
    struct _treeNode *right;
} Tree;

#define MAX_BTNAME		32
#define MAX_BTREES		16
#define MAX_KEY_SIZE	96

typedef struct _btree {
	int inUse;
	char btName[MAX_BTNAME + 1];
	pthread_mutex_t btreeLock;
	Tree *root;
} Btree;

#define MAX_LLQNAME		32
#define MAX_LLQUEUES	16

typedef struct _qdata {
    unsigned int length;
	unsigned char *data;
	struct _qdata *next;
} Qdata;

typedef struct _Queue {
	int inUse;
    int itemCount;
	char llqName[MAX_LLQNAME + 1];
	pthread_mutex_t listLock;
	pthread_cond_t listCond;
	Qdata *head;
	Qdata *tail;
} Queue;

#define MAX_LLNAME		32
#define MAX_LLISTS	    16

typedef struct _ldata {
    int length;
	char *data;
	struct _ldata *next;
} Ldata;

typedef struct _LList {
	int inUse;
    int itemCount;
    char *listStr;
	char llName[MAX_LLNAME + 1];
	pthread_mutex_t llock;
	pthread_cond_t lcond;
	Ldata *head;
	Ldata *tail;
} LList;

typedef struct _SLList {
	int inUse;
    int itemCount;
    char *listStr;
	char llName[MAX_LLNAME + 1];
	pthread_mutex_t sortLock;
	pthread_cond_t sortCond;
	Ldata *head;
	Ldata *tail;
} SLList;

#define MAX_NUM		32
#define MAX_CALLID	64
#define MAX_KEY		(MAX_NUM + MAX_CALLID)

typedef struct _lol_ {
	char aNum[MAX_NUM + 1];
	char bNum[MAX_NUM + 1];
	char callType;
	time_t timestamp;
	struct _lol_ *lolNext;
} Lol;

typedef struct _mlist_ {
	char key[MAX_NUM + MAX_CALLID + 1];
	time_t timestamp;
	struct _mlist_ *mNext;
	struct _mlist_ *mPrev;
	Lol *lol;
} MList;

typedef struct _mcastinfo {
	char ifInterface[64];	// interface IP address or 127.0.0.1 for localhost.
	char mAddress[64];		// multicast address in the range of 224.0.0.0 and 224.0.0.255
	int mPort;				// port number above 1024
	int mSock;				// multicast socket number;
} McastInfo;

int tcpConnect(const char *server, int port);
int tcpTimeoutConnect(const char *server, int port, int timeout);
int tcpServer(int port, int maxPending);
int tcpServerSubnet(int port, int maxPending, const char *subnet);
int tcpAccept(int sock, struct sockaddr *clientAddr);
int tcpSend(int sock, const char *msg);
int tcpRecSend(int sock, const char *msg);
int tcpNumSend(int sock, const char *msg, int numBytes);
int tcpRecv(int sock, char *buf, int buf_len);
int tcpRecvJson(int sock, char *json, int json_len);
int tcpRecvString(int sock, char *buf, int buf_len);
int isIPv4Address(const char *addr);
void tcpClose(int sock);

int mcastServer(McastInfo *mi);
int mcastJoin(McastInfo *mi);
int mcastSend(McastInfo *mi, const char *msg);
int mcastRecSend(McastInfo *mi, const char *msg);
int mcastNumSend(McastInfo *mi, const char *msg, int numBytes);
int mcastRecv(McastInfo *mi, char *buf, int buf_len);
int mcastRecvJson(McastInfo *mi, char *json, int json_len);
int mcastRecvString(McastInfo *mi, char *buf, int buf_len);
void mcastClose(McastInfo *mi);

int sctpConnect(const char *server, int port);
int sctpTimeoutConnect(const char *server, int port, int timeout);
int sctpServer(int port, int maxPending);
int sctpServerSubnet(int port, int maxPending, const char *subnet);
int sctpAccept(int sock, struct sockaddr *clientAddr);
int sctpSend(int sock, const char *msg);
int sctpNumSend(int sock, const char *msg, int numBytes);
int sctpRecv(int sock, char *buf, int buf_len);
int sctpRecvJson(int sock, char *json, int json_len);
int sctpRecvString(int sock, char *buf, int buf_len);
void sctpClose(int sock);

typedef struct _fdata {
	int needsFreeing;
	int length;
	unsigned char *data;
} FData;

int llqInit(int queueCnt);
int llqCreate(const char *llqName);
int llqAdd(int queNum, const unsigned char *data, int length);
int llqAddString(int queNum, unsigned char *data);
int llqRemove(int queNum, FData *fdata, int block);
int llqCount(int queNum);
char *llqQueName(int queNum);
int llqQueNum(char *llqName);
int llqDestory(int queNum);
void llqFree(FData *fdata);

int llInit(int queueCnt);
int llCreate(const char *llName);
int llAdd(int llNum, const char *data, int length);
int llAddUnique(int llNum, const char * data, int length);
int llAddString(int llNum, const char *data);
int llAddUniqueString(int llNum, const char * data);
int llFind(int llNum, const char *findStr);
char *llRemove(int llNum, int *length, int block);
int llCount(int llNum);
char *llList(int llNum);
int llDestroy(int llNum);
char *llGetName(int llNum);

int sllInit(int queueCnt);
int sllCreate(const char *sllName);
int sllAdd(int sllNum, const char *data, int length);
int sllAddUnique(int sllNum, const char * data, int length);
int sllAddString(int sllNum, const char *data);
int sllAddUniqueString(int sllNum, const char * data);
int sllFind(int sllNum, const char *findStr);
char *sllRemove(int sllNum, int *length, int block);
int sllCount(int sllNum);
char *sllList(int sllNum);
int sllDestroy(int sllNum);
char *sllGetName(int sllNUm);

int lolInit();
MList *lolGetList();
int lolMainListCount();
int lolAdd(char *aNum, char *bNum, char *callId, char callType);
void lolRemoveExpired(int seconds);
void lolPrint();

#define MAX_CQNAME	32
#define MAX_CQUEUES	16

#define CQ_NONBLOCK	0
#define CQ_BLOCK	1

typedef union _items_ {
	unsigned char c;
	unsigned short s;
	unsigned int i;
	unsigned long l;
	char *p;
} ItemType;

typedef struct _CQueue {
	volatile int cqItemCount;
	char cqName[MAX_CQNAME + 1];
	void **buffer;
	int blkSize;
	int arrSize;
	int growth;
	pthread_mutex_t cqLock;
	pthread_cond_t cqCond;
	int queIn;
	int queOut;
	ItemType array[0];
} CQueue;

int cqInit(int cqueueCnt);
int cqCreate(const char *cqName, int arrSize);
int cqCreateDynamic(const char *cqName, int arrSize, int growth);
int cqGrow(int cqNum, int growth);
int cqSetGrowth(int cqNum, int growth);
int cqSetBuffer(int cqNum, void *buffer, int blkSize);
int cqAdd(int cqNum, ItemType *value);
int cqRemove(int cqNum, ItemType *value, int block);
int cqRemoveTimed(int cqNum, ItemType *value, int timeout);
int cqDestroy(int queNum);
int cqCount(int queNum);
int cqArrSize(int queNum);
char *cqGetName(int queNum);
int cqGetNum(char *cqName);

// process initiation utilities 
#define MAX_STR_LEN       (255)
#define MAX_MP_NAME       (32)
#define IN_SN_PORT        (9992)

#define DEF_MP_SZ         (512)  /* MB */
#define DEF_MBUFF_SZ      (1560*2) /* byte, Ethernet MTU */
#define HEAD_ROOM_SZ      (512)  /* byte */ 
#define TAIL_ROOM_SZ      (28)   /* byte */
#define MBUF_SIGN         ("12ABCDEFGHIJKLMNOPQRSTUVWXYZ")
#define DEF_MP_COUNT      (10)   /* Number of memory pool */

typedef enum _timeType {
	TIME_VAL,
	TIME_SPEC,
	TIME_SECONDS
} TimeType;

unsigned long timeTimestamp(void *timeStruct, TimeType timeType);
unsigned int timeEpcoh(void * timeStruct, TimeType timeType);
void timeFunc(char *name, int flag, struct timespec *st);
void timeFuncNano(char *name, int flag, struct timespec *st);
unsigned long timeNano(int flag, struct timespec *st);
double timeCPU(int flag, clock_t *st);
long getTimeDiff (const struct timeval *pEndTime, const struct timeval *pBeginTime); 

void milliSleep(int milliSeconds);

uint32_t crc32(const void *buf, size_t size);
uint32_t crc8(unsigned char *data, size_t len);

int jsonGet(const char *jsonData, const char *name, char *buf);
int jsonGetInt(const char *jsonData, const char *name, int *intValue);
int jsonGetLong(const char *jsonData, const char *name, long *longValue);
int jsonGetLongLong(const char *jb, const char *name, unsigned long long *longLongValue);
int jsonFind(const char *jsonData, const char *name);
int jsonObjGet(const char *jsonData, const char *name, char *buf);
void jsonReset(char *jb);
char *jsonBuf(char *jb);
char *jsonAdd(char *jb, const char *name, const char *value);
char *jsonAddChar(char *jb, const char *name, char charValue);
char *jsonAddUChar(char *jb, const char *name, unsigned char charValue);
char *jsonAddShort(char *jb, const char *name, short shortValue);
char *jsonAddUShort(char *jb, const char *name, unsigned short shortValue);
char *jsonAddInt(char *jb, const char *name, int intValue);
char *jsonAddUInt(char *jb, const char *name, unsigned int intValue);
char *jsonAddIp(char *jb, const char *name, uint32_t intValue);
char *jsonAddLong(char *jb, const char *name, long longValue);
char *jsonAddULong(char *jb, const char *name, unsigned long longValue);
char *jsonAddFloat(char *jb, const char *name, float floatValue, int percision);
char *jsonAddLongLong(char *jb, const char *name, unsigned long long longLongValue);

#endif
