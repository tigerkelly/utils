
all: ../../libs/libipcutils.a

# Set export PROFILE=yes to turn on profiling flags.

CC=cc
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

LDFLAGS=-L/usr/local/lib -lrt -lpthread
CFLAGS=-std=gnu99 -g -Wall -I../../incs 
ifeq ($(PROFILE),yes)
	CFLAGS += -pg
	LDFLAGS += -pg
endif

ARC=../../libs/libipcutils.a

all: $(ARC)

$(ARC): $(OBJS)
	$(AR) -r $(ARC) $(OBJS)

$(OBJS): ../../incs/ipcutils.h

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS) $(ARC)
