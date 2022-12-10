#include "mfs.h"

int MFS_Init(char *hostname, int port)
{
    // connect to server
    // send init message
    // receive init response
    // return 0 on success, -1 on failure
}

int MFS_Lookup(int pinum, char *name)
{
    // send lookup message
    // receive lookup response
    // return inum on success, -1 on failure
}

int MFS_Stat(int inum, MFS_Stat_t *m)
{

    // send stat message
    // receive stat response
    // return 0 on success, -1 on failure
}

int MFS_Write(int inum, char *buffer, int offset, int nbytes)
{

    // send write message
    // receive write response
    // return 0 on success, -1 on failure
}

int MFS_Read(int inum, char *buffer, int offset, int nbytes)
{

    // send read message
    // receive read response
    // return 0 on success, -1 on failure
}
int MFS_Creat(int pinum, int type, char *name)
{

    // send creat message
    // receive creat response
    // return 0 on success, -1 on failure
}

int MFS_Unlink(int pinum, char *name)
{

    // send unlink message
    // receive unlink response
    // return 0 on success, -1 on failure
}