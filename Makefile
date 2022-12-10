CC     := gcc
CFLAGS := -Wall -Werror 

SRCS   := mfscli.c \
	mfsserver.c 

OBJS   := ${SRCS:c=o}
PROGS  := ${SRCS:.c=}

.PHONY: all
all: ${PROGS}

${PROGS} : % : %.o Makefile
	${CC} $< -o $@ mkfsc.c

libmfs: mkfsc.o
	${CC} mkfsc.o -o libmfs

clean:
	rm -f ${PROGS} ${OBJS}

%.o: %.c Makefile
	${CC} ${CFLAGS} -c $<
