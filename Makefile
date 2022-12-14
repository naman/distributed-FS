CC     := gcc
CFLAGS := -Wall
LDFLAGS:= -L. -lmfs

SRCS   := mfscli.c \
	server.c 

OBJS   := ${SRCS:c=o}
PROGS  := ${SRCS:.c=}

.PHONY: all
all: ${PROGS}

mfs: libmfs.c mfs.h
	${CC} -fPIC -c libmfs.c

libmfs.so: mfs
	${CC} -shared -Wl,-soname,libmfs.so -o libmfs.so libmfs.o udp.o -lc

libmfs: libmfs.so
	ln -sf libmfs.so libmfs.o

mfscli: mfscli.o libmfs
	${CC} mfscli.o -o mfscli -L. -lmfs

server: server.o udp.o
	${CC} server.o udp.o -o server  -L. -lmfs

# %.o: %.c Makefile
# 	${CC} ${CFLAGS} -c $<

mkfs: mkfs.o
	${CC} mkfs.o -o mkfs

clean:
	rm -f ${PROGS} ${OBJS} libmfs mkfs *.o *.so *.log mfscli
