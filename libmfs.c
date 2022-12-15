#include "mfs.h"
#include "udp.h"

#include <stdlib.h>
#include <time.h>

int client_connection = -1;

int send_message(int sd, struct sockaddr_in *addr, __MFS_Message_t *msg)
{
    // printf("sending type: %d, size: %d", type, size);
    int rc = UDP_Write(sd, addr, (char *)msg, sizeof(__MFS_Message_t));
    if (rc < 0)
    {
        // printf("failed to send\n");
        exit(1);
    }

    return rc;
}

int recv_message(int sd, struct sockaddr_in *addr, __MFS_Message_t *msg)
{
    int rc = UDP_Read(sd, addr, (char *)msg, sizeof(__MFS_Message_t));
    if (rc < 0)
    {
        // printf("failed to receive\n");
        return -1;
    }

    return rc;
}

int send_api_message(int sd, struct sockaddr_in *addr, __MFS_Message_t *msg)
{
    // __MFS_Message_t *msg = send_message(sd, addr, type, buffer, size);
    int rc = send_message(sd, addr, msg);
    // assert(rc == 0);

    rc = recv_message(sd, addr, msg);
    // assert(rc == 0);

    return msg->status;
}

int MFS_Init(char *hostname, int port)
{
    // MFS_Init() takes a host name and port number and uses those to find the server exporting the file system.

    // store the connection in a global variable

    int MIN_PORT = 20000;
    int MAX_PORT = 40000;

    srand(time(0));
    int port_num = (rand() % (MAX_PORT - MIN_PORT) + MIN_PORT);

    client_connection = UDP_Open(port_num);

    if (client_connection < 0)
    {
        // printf("client:: failed to open socket\n");
        exit(1);
    }

    addrSnd = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));

    int rc = UDP_FillSockAddr(addrSnd, hostname, port);
    if (rc < 0)
    {
        // printf("client:: failed to fill address\n");
        exit(1);
    }
    // printf("client:: initializing connection\n");

    __MFS_Message_t *msg = (__MFS_Message_t *)malloc(sizeof(__MFS_Message_t));
    msg->type = MFS_INIT;
    rc = send_api_message(client_connection, addrSnd, msg);
    assert(rc == 0);
    free(msg);
    return rc;
}

int MFS_Lookup(int pinum, char *name)
{
    // MFS_Lookup() takes the parent inode number (which should be the inode number of a directory) and looks up the entry name in it. The inode number of name is returned. Success: return inode number of name; failure: return -1. Failure modes: invalid pinum, name does not exist in pinum.
    __MFS_Message_t *msg = (__MFS_Message_t *)malloc(sizeof(__MFS_Message_t));
    msg->type = MFS_LOOKUP;
    msg->inum = pinum;
    strcpy(msg->name, name);
    int rc = send_api_message(client_connection, addrSnd, msg);
    free(msg);
    return rc;
}

int MFS_Stat(int inum, MFS_Stat_t *m)
{
    // MFS_Stat() returns some information about the file specified by inum. Upon success, return 0, otherwise -1. The exact info returned is defined by MFS_Stat_t. Failure modes: inum does not exist. File and directory sizes are described below.
    __MFS_Message_t *msg = (__MFS_Message_t *)malloc(sizeof(__MFS_Message_t));
    msg->type = MFS_STAT;
    msg->inum = inum;
    int rc = send_api_message(client_connection, addrSnd, msg);
    if (rc == -1)
    {
        return -1;
    }
    m->type = msg->m.type;
    m->size = msg->m.size;
    free(msg);
    return rc;
}

int MFS_Write(int inum, char *buffer, int offset, int nbytes)
{
    // MFS_Write() writes a buffer of size nbytes (max size: 4096 bytes) at the byte offset specified by offset. Returns 0 on success, -1 on failure. Failure modes: invalid inum, invalid nbytes, invalid offset, not a regular file (because you can't write to directories).
    __MFS_Message_t *msg = (__MFS_Message_t *)malloc(sizeof(__MFS_Message_t));
    msg->type = MFS_WRITE;
    msg->inum = inum;
    memcpy(msg->buffer, buffer, nbytes);
    msg->offset = offset;
    int rc = send_api_message(client_connection, addrSnd, msg);
    free(msg);
    return rc;
}

int MFS_Read(int inum, char *buffer, int offset, int nbytes)
{
    // MFS_Read() reads nbytes of data (max size 4096 bytes) specified by the byte offset offset into the buffer from file specified by inum. The routine should work for either a file or directory; directories should return data in the format specified by MFS_DirEnt_t. Success: 0, failure: -1. Failure modes: invalid inum, invalid offset, invalid nbytes.
    __MFS_Message_t *msg = (__MFS_Message_t *)malloc(sizeof(__MFS_Message_t));
    msg->type = MFS_READ;
    msg->inum = inum;
    msg->offset = offset;
    int rc = send_api_message(client_connection, addrSnd, msg);
    return rc;
}

int MFS_Creat(int pinum, int type, char *name)
{
    // MFS_Creat() makes a file (type == MFS_REGULAR_FILE) or directory (type == MFS_DIRECTORY) in the parent directory specified by pinum of name name. Returns 0 on success, -1 on failure. Failure modes: pinum does not exist, or name is too long. If name already exists, return success.
    __MFS_Message_t *msg = (__MFS_Message_t *)malloc(sizeof(__MFS_Message_t));
    msg->type = MFS_CREAT;
    msg->inum = pinum;
    msg->file_type = type;
    strcpy(msg->name, name);
    int rc = send_api_message(client_connection, addrSnd, msg);
    return rc;
}

int MFS_Unlink(int pinum, char *name)
{
    // MFS_Unlink() removes the file or directory name from the directory specified by pinum. 0 on success, -1 on failure. Failure modes: pinum does not exist, directory is NOT empty. Note that the name not existing is NOT a failure by our definition (think about why this might be).
    __MFS_Message_t *msg = (__MFS_Message_t *)malloc(sizeof(__MFS_Message_t));
    msg->type = MFS_UNLINK;
    msg->inum = pinum;
    strcpy(msg->name, name);
    int rc = send_api_message(client_connection, addrSnd, msg);
    return rc;
}

int MFS_Shutdown()
{
    // MMFS_Shutdown() just tells the server to force all of its data structures to disk and shutdown by calling exit(0). This interface will mostly be used for testing purposes.
    __MFS_Message_t *msg = (__MFS_Message_t *)malloc(sizeof(__MFS_Message_t));
    msg->type = MFS_SHUTDOWN;
    int rc = send_api_message(client_connection, addrSnd, msg);
    assert(rc == 0);

    // free the address
    free(addrSnd);
    // close the connection
    close(client_connection);
    return rc;
}
