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
    int status; // for return values
    //space for all possible arguments
    //read what is necessary at the server end depending
    //on request type
    int inum;
    char name[30];
    MFS_Stat_t m;
    int file_type;
    int offset;
    int nbytes;
    char buffer[MFS_BUFFER_SIZE];
} __MFS_Message_t;

// helper functions
typedef struct sockaddr_in sockaddr_in;

int send_message(int sd, struct sockaddr_in *addr, __MFS_Message_t *msg);
int recv_message(int sd, struct sockaddr_in *addr, __MFS_Message_t *msg);

// store the connection sd in a global variable
extern int client_connection;

// store sockaddr_in in a global variable
sockaddr_in* addrSnd;

int MFS_Init(char *hostname, int port);
int MFS_Lookup(int pinum, char *name);
int MFS_Stat(int inum, MFS_Stat_t *m);
int MFS_Write(int inum, char *buffer, int offset, int nbytes);
int MFS_Read(int inum, char *buffer, int offset, int nbytes);
int MFS_Creat(int pinum, int type, char *name);
int MFS_Unlink(int pinum, char *name);
int MFS_Shutdown();

#endif // __MFS_h__
