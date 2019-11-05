
all: ../../libs/libmiscutils.a

# Set export PROFILE=yes to turn on profiling flags.

CC=cc
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

LDFLAGS=-L/usr/local/lib -lrt -lpthread
CFLAGS=-std=gnu99 -g -Wall -I../../incs -I/usr/local/include
ifeq ($(PROFILE),yes)
	CFLAGS += -pg
	LDFLAGS += -pg
endif

ARC=../../libs/libmiscutils.a

all: $(ARC)

$(ARC): $(OBJS)
	$(AR) -r $(ARC) $(OBJS)

$(OBJS): ../../incs/ini.h ../../incs/strutils.h ../../incs/logutils.h

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS) $(ARC)
