CC     := gcc
CFLAGS := -Wall -Werror 

SRCS   := mfscli.c \
	mfsserver.c 

OBJS   := ${SRCS:c=o}
PROGS  := ${SRCS:.c=}

.PHONY: all
all: ${PROGS}

libmfs.so: libmfs.o udp.o
	${CC} -shared -Wl,-soname,libmfs.so -o libmfs.so libmfs.o udp.o -lc

libmfs: libmfs.so
	ln -sf libmfs.so libmfs

mfscli: mfscli.o libmfs
	${CC} mfscli.o -o mfscli -L. -lmfs

clean:
	rm -f ${PROGS} ${OBJS} libmfs.so udp.o libmfs
