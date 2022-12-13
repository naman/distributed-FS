CC     := gcc
CFLAGS := -Wall 

SRCS   := mfscli.c \
	server-fs.c 

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

server-fs: server-fs.o
	${CC} server-fs.o -o server-fs 

mkfs: mkfs.o
	${CC} mkfs.o -o mkfs

clean:
	rm -f ${PROGS} ${OBJS} libmfs.so udp.o libmfs
