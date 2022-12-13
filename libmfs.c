#include "mfs.h"
#include "udp.h"

__MFS_Message_t *send_message(int sd, struct sockaddr_in *addr, int type, char *buffer, int size)
{
    __MFS_Message_t *msg = (__MFS_Message_t *)malloc(sizeof(__MFS_Message_t));
    msg->type = type;
    msg->size = size;
    memcpy(msg->buffer, buffer, size);

    int rc = UDP_Write(sd, addr, (char *)msg, sizeof(__MFS_Message_t));
    if (rc < 0)
    {
        printf("client:: failed to send\n");
        exit(1);
    }

    return msg;
}

__MFS_Message_t *recv_message(int sd, struct sockaddr_in *addr)
{
    char buffer[MFS_BUFFER_SIZE];
    int rc = UDP_Read(sd, addr, buffer, MFS_BUFFER_SIZE);
    if (rc < 0)
    {
        printf("client:: failed to receive\n");
        exit(1);
    }

    __MFS_Message_t *msg = (__MFS_Message_t *)malloc(sizeof(__MFS_Message_t));
    memcpy(msg, buffer, sizeof(__MFS_Message_t));

    return msg;
}

int init_connection(char *hostname, int port)
{
    struct sockaddr_in addrSnd;

    int sd = UDP_Open(0);
    if (sd < 0)
    {
        printf("client:: failed to open socket\n");
        exit(1);
    }
    int rc = UDP_FillSockAddr(&addrSnd, hostname, port);
    if (rc < 0)
    {
        printf("client:: failed to fill address\n");
        exit(1);
    }

    return sd;
}

int send_api_message(int sd, struct sockaddr_in *addr, int type, char *buffer, int size)
{
    // __MFS_Message_t *msg = send_message(sd, addr, type, buffer, size);
    send_message(sd, addr, type, buffer, size);

    __MFS_Message_t *recvMsg = recv_message(sd, addr);

    if (recvMsg->type == type)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int MFS_Init(char *hostname, int port)
{
    // MFS_Init() takes a host name and port number and uses those to find the server exporting the file system.

    // store the connection in a global variable
    server_connection = init_connection(hostname, port);

    struct sockaddr_in addrSnd;
    int rc = send_api_message(server_connection, &addrSnd, MFS_INIT, NULL, 0);
    return rc;
}

int MFS_Lookup(int pinum, char *name)
{
    // MFS_Lookup() takes the parent inode number (which should be the inode number of a directory) and looks up the entry name in it. The inode number of name is returned. Success: return inode number of name; failure: return -1. Failure modes: invalid pinum, name does not exist in pinum.

    struct sockaddr_in addrSnd;
    int rc = send_api_message(server_connection, &addrSnd, MFS_LOOKUP, NULL, 0);
    return rc;
}

int MFS_Stat(int inum, MFS_Stat_t *m)
{
    // MFS_Stat() returns some information about the file specified by inum. Upon success, return 0, otherwise -1. The exact info returned is defined by MFS_Stat_t. Failure modes: inum does not exist. File and directory sizes are described below.

    struct sockaddr_in addrSnd;
    int rc = send_api_message(server_connection, &addrSnd, MFS_STAT, NULL, 0);
    return rc;
}

int MFS_Write(int inum, char *buffer, int offset, int nbytes)
{
    // MFS_Write() writes a buffer of size nbytes (max size: 4096 bytes) at the byte offset specified by offset. Returns 0 on success, -1 on failure. Failure modes: invalid inum, invalid nbytes, invalid offset, not a regular file (because you can't write to directories).

    struct sockaddr_in addrSnd;
    int rc = send_api_message(server_connection, &addrSnd, MFS_WRITE, NULL, 0);
    return rc;
}

int MFS_Read(int inum, char *buffer, int offset, int nbytes)
{
    // MFS_Read() reads nbytes of data (max size 4096 bytes) specified by the byte offset offset into the buffer from file specified by inum. The routine should work for either a file or directory; directories should return data in the format specified by MFS_DirEnt_t. Success: 0, failure: -1. Failure modes: invalid inum, invalid offset, invalid nbytes.

    struct sockaddr_in addrSnd;
    int rc = send_api_message(server_connection, &addrSnd, MFS_READ, NULL, 0);
    return rc;
}
int MFS_Creat(int pinum, int type, char *name)
{
    // MFS_Creat() makes a file (type == MFS_REGULAR_FILE) or directory (type == MFS_DIRECTORY) in the parent directory specified by pinum of name name. Returns 0 on success, -1 on failure. Failure modes: pinum does not exist, or name is too long. If name already exists, return success.

    struct sockaddr_in addrSnd;
    int rc = send_api_message(server_connection, &addrSnd, MFS_CREAT, NULL, 0);
    return rc;
}

int MFS_Unlink(int pinum, char *name)
{
    // MFS_Unlink() removes the file or directory name from the directory specified by pinum. 0 on success, -1 on failure. Failure modes: pinum does not exist, directory is NOT empty. Note that the name not existing is NOT a failure by our definition (think about why this might be).

    struct sockaddr_in addrSnd;
    int rc = send_api_message(server_connection, &addrSnd, MFS_UNLINK, NULL, 0);
    return rc;
}

int MFS_Shutdown()
{
    // MMFS_Shutdown() just tells the server to force all of its data structures to disk and shutdown by calling exit(0). This interface will mostly be used for testing purposes.

    struct sockaddr_in addrSnd;
    int rc = send_api_message(server_connection, &addrSnd, MFS_SHUTDOWN, NULL, 0);

    // close the connection
    close(server_connection);
    return rc;
}