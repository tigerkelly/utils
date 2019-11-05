
all: ../../libs/libstrutils.a

# Set export PROFILE=yes to turn on profiling flags.

CC=cc
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

LDFLAGS=-lpcap L/usr/local/lib -lpq -lrt -lpthread
CFLAGS=-std=gnu99 -g -Wall -I../../incs -I/usr/local/include -I/usr/local/include/nopoll
ifeq ($(PROFILE),yes)
	CFLAGS += -pg
	LDFLAGS += -pg
endif

ARC=../../libs/libstrutils.a

all: $(ARC)

$(ARC): $(OBJS)
	$(AR) -r $(ARC) $(OBJS)

$(OBJS): ../../incs/strutils.h

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS) $(ARC)
