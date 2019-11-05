
all: ../../libs/librtdutils.a

# Set export PROFILE=yes to turn on profiling flags.

CC=cc
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

LDFLAGS=-L/usr/local/lib -L../../libs -llogutils -lmiscutils -lrt -lpthread
CFLAGS=-std=gnu99 -g -Wall -I../../incs -I/usr/local/include
ifeq ($(PROFILE),yes)
	CFLAGS += -pg
	LDFLAGS += -pg
endif

ARC=../../libs/librtdutils.a

all: $(ARC)

$(ARC): $(OBJS)
	$(AR) -r $(ARC) $(OBJS)

$(OBJS): ../../incs/ini.h ../../incs/rtdutils.h ../../incs/strutils.h ../../incs/miscutils.h

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS) $(ARC)
