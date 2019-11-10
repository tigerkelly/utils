
# Utils library.  General C functions.

These functions are common functions I use a lot, enjoy.

### Building utils library.

I have compiled this library on several different types of OSes but this has only been tested on Raspberry Pi.

These are pretty much simple C code that should compile without any issues.

To build just type **make** and the libraries will be placed in the **libs** directory.


### Description of each library.

Each library directory should have a text file that describes the functions calls within the library.

ini - Set of functions to support .ini type files.  I use this a lot for storing and retriving configuration data for a program or system. 


See file **[libini.txt](https://github.com/tigerkelly/utils/blob/master/ini/src/libini.txt)** for format syntax.

ipcutils - Set of functions or helper functions to help with IPC calls for Semaphores, Message Queues and Shared Memory.

	- mem_utils.c, helper functions for shared memory.
	- msg_utils.c, helper functions for IPC message queues.
	- sem_utils.c, helper functions for semaphores

logUtils - Set of functions or helper functions to help with system logging of program messages.

	- logger.c, helper fcuntions for UNIX system logging.

miscUtils - A deverse set of fucntions to help with many types of tasks.

	- cqueue.c, circular link list functions, very fast.
	- crc32.c, to create a 32 bit CRC value for data given.
	- farmhash.c, The Google FarmHash functions.
	- gqueue.c, a generic FIFO queue.
	- jsmn.c, JSON functions.
	- json_utils.c, helper functions for jsmn.c
	- listoflists.c, used to great linked list of lists.
	- llist.c, simple double linked list functions.
	- llqueue.c, simple double linked FIFO list
	- mcast_siocket.c, helper fucntions to support multicast packets and addresses.
	- sctp_sockets.c, helper functions for the SCTP (Stream Control Transmission Protocol).
	- sllist.c, simple single linked list functions.
	- tcp_sockets.c, helper functions for TCP protocol
	- timefunc.c, helper fucntions to standardize time calls.
	- udp_conn_sockets.c, helper functions for UDP connection state.
	- udp_sockets.c, helper functions for UDP connectionless state.

mmaputils - Set of functions to support mmap system.

	- mmaputils.c, helper functions for mmap system.

rtdutils - Set of functions to support my RTD (Real Time Data) engine. (RTD engine not released yet.)

	- idnode.c, RTD functions and structures.

strutils = Set of functions to support strings.

	- ascii2binary.c, helper fucntions to convert binaries like int and long to ascii string.
	- binary2ascii.c, same as ascii2binary but in reverse.
	- bit2str.c, helper functions to convert bits into strings.
	- copyutils.c, safe copy string functions.
	- ip2str.c, convert an IP address to string and back.
	- ipv42hex.c, convert an IP address in binary form to ascii hex.
	- kqparse.c, parses a string returning tokens but keeping quoted text together.
	- kstrqtok.c, support fucntion for kqparse.c
	- mac2str.c, converts MAC address to a string.
	- parse.c, parse a string into tokens by give string.
	- qparse.c, parse a srting into tokens and keeping quoted string together.
	- replace.c, replace a char with another in a string.
	- split.c, split string based on char given.
	- splitbystr.c, split string based on string given.
	- str_match.c, does strcmp on beginging or end of strings, looking for matches.
	- strqtok.c, support function for parsing string with quoted tokens.
	- tokeniaze.c, tokenizes a string based on char given.
	- trim.c, trims the head or tail of a string removing whitepsaces.
	- ts_utils.c, returns a date/time with milliseconds.
