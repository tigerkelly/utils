
all: ../../libs/libtemplate.a

# Set export PROFILE=yes to turn on profiling flags.

CC=cc
SRCS=$(wildcard *.c)
HDRS=$(wildcard *.h)
OBJS=$(SRCS:.c=.o)

LDFLAGS=-lpcap -L/usr/local/lib -lpq -lrt -lnopoll -lpthread
CFLAGS=-std=gnu99 -g -Wall -I. -I../../incs -I/usr/local/include -I/usr/local/include/nopoll
ifeq ($(PROFILE),yes)
	CFLAGS += -pg
	LDFLAGS += -pg
endif

ARC=../../libs/libtemplate.a

all: $(ARC)

$(ARC): $(OBJS)
	$(AR) -r $(ARC) $(OBJS)

$(OBJS): ../../incs/ini.h ../../incs/ipcutils.h ../../incs/strutils.h $(HDRS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS) $(ARC)
