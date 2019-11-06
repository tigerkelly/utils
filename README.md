
# Utils library.  General C functions.

These functions are common functions I use a lot, enjoy.

### Building utils library.

I have compiled these are several different type of OSes but this has only been tested on Raspberry Pi.

These are pretty much simple C code that should compile without any issues.

To build just type **make** and the libraries will be placed in the **libs** directory.


### Description of each library.

Each library directory shoudl have a text file that describes the functions calls within the libnrary.

ini - Set of functions to support .ini type files.  I use this a lot for storing and retriving configuration data for a program or system. 


See file **[libini.txt](https://github.com/tigerkelly/utils/blob/master/ini/src/libini.txt)** for format syntax.

ipcutils - Set of functions or helper functions to help with IPC calls for Semaphores, Message Queues and Shared Memory.

logUtils - Set of functions or helper functions to help with system logging of program messages.

miscUtils - Set deverse set of fucntions to help with many types of tasks.

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
