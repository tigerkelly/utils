
all: ../../libs/libini.a

# Set export PROFILE=yes to turn on profiling flags.

CC=cc
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

LDFLAGS=-L/usr/local/lib -lpq -lrt -lpthread
CFLAGS=-std=gnu99 -g -Wall -I../../incs -I/usr/local/include -I/usr/lib/gcc/arm-linux-gnueabihf/6/include
ifeq ($(PROFILE),yes)
	CFLAGS += -pg
	LDFLAGS += -pg
endif

ARC=../../libs/libini.a

all: $(ARC)

$(ARC): $(OBJS)
	$(AR) -r $(ARC) $(OBJS)

$(OBJS): ini.c ../../incs/ini.h ../../incs/strutils.h

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS) $(ARC)
