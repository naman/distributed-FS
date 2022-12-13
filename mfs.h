#ifndef __MFS_h__
#define __MFS_h__

#define MFS_DIRECTORY    (0)
#define MFS_REGULAR_FILE (1)

#define MFS_BLOCK_SIZE   (4096)

typedef struct __MFS_Stat_t {
    int type;   // MFS_DIRECTORY or MFS_REGULAR
    int size;   // bytes
    // note: no permissions, access times, etc.
} MFS_Stat_t;

typedef struct __MFS_DirEnt_t {
    char name[28];  // up to 28 bytes of name in directory (including \0)
    int  inum;      // inode number of entry (-1 means entry not used)
} MFS_DirEnt_t;

#define MFS_BUFFER_SIZE 4096

#define MFS_INIT 0
#define MFS_LOOKUP 1
#define MFS_STAT 2
#define MFS_WRITE 3
#define MFS_READ 4
#define MFS_CREAT 5
#define MFS_UNLINK 6
#define MFS_SHUTDOWN 7

typedef struct __MFS_Message_t
{
    int type;
    int size;
    char buffer[MFS_BUFFER_SIZE];
} __MFS_Message_t;

// helper functions
typedef struct sockaddr_in sockaddr_in;

int init_connection(char *hostname, int port);
__MFS_Message_t *send_message(int sd, sockaddr_in *addrSnd, int type, char *buffer, int size);
__MFS_Message_t *recv_message(int sd, sockaddr_in *addrSnd);


// store the connection sd in a global variable
int server_connection = -1;

int MFS_Init(char *hostname, int port);
int MFS_Lookup(int pinum, char *name);
int MFS_Stat(int inum, MFS_Stat_t *m);
int MFS_Write(int inum, char *buffer, int offset, int nbytes);
int MFS_Read(int inum, char *buffer, int offset, int nbytes);
int MFS_Creat(int pinum, int type, char *name);
int MFS_Unlink(int pinum, char *name);
int MFS_Shutdown();

#endif // __MFS_h__
