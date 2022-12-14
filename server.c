#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <sys/socket.h>

#include "mfs.h"
#include "udp.h"
#include "ufs.h"

#define min(X, Y) ((X) < (Y) ? (X) : (Y))
#define max(X, Y) ((X) > (Y) ? (X) : (Y))

unsigned int get_bit(unsigned int *bitmap, int position)
{
	int index = position / 32;
	int offset = 31 - (position % 32);
	return (bitmap[index] >> offset) & 0x1;
}

void set_bit(unsigned int *bitmap, int position)
{
	int index = position / 32;
	int offset = 31 - (position % 32);
	bitmap[index] |= (0x1 << offset);
}

int Server_Create(void *image, uint *inode_bitmapptr, int pinum, int type, char *name)
{
	int get_presence_of_inode = get_bit(inode_bitmapptr, pinum);
	if (get_presence_of_inode == 0)
		return -1;
	// name is too long
	if (strlen(name) > 28)
		return -1;
	super_t *s = (super_t *)image;
	inode_t *inode_table = image + (s->inode_region_addr * UFS_BLOCK_SIZE);
	inode_t *dir_inode = inode_table;
	dir_ent_t *dir = image + (dir_inode[pinum].direct[0] * UFS_BLOCK_SIZE);
	int file_absent = 1;
	int inum_of_already_present_file;
	for (int i = 0; i < (UFS_BLOCK_SIZE / sizeof(dir_ent_t)); i++)
	{
		if (dir[i].inum != -1)
		{
			if (strcmp(dir[i].name, name) == 0)
			{
				file_absent = 0;
				inum_of_already_present_file = dir[i].inum;
				// printf("File already present at inode number %d\n",inum_of_already_present_file);
				break;
			}
			// printf("File at inode number %d is %s\n",dir[i].inum,dir[i].name);
		}
	}
	if (file_absent == 1)
	{
		printf("File not present, need to add it to the directory\n");
		// find empty directory entry for file.
		int empty_dir_entry;

		int free_inode_number = 0;
		for (int i = 0; i < 1000; i++)
		{
			if (get_bit(inode_bitmapptr, i) == 0)
			{
				free_inode_number = i;
				break;
			}
		}
		printf("Free inode number is %d\n", free_inode_number);
		set_bit(inode_bitmapptr, free_inode_number);
		for (int i = 0; i < (UFS_BLOCK_SIZE / sizeof(dir_ent_t)); i++)
		{
			if (dir[i].inum == -1)
			{
				strcpy(dir[i].name, name);
				dir[i].inum = free_inode_number;
				dir_inode[pinum].size = max(dir_inode[pinum].size, ((i + 1) * sizeof(dir_ent_t)));
				break;
			}
			// printf("Value of inum is %d\n",dir[i].inum);
		}
		inode_t *inode_ptr_of_interest = inode_table + free_inode_number;
		inode_ptr_of_interest->type = type;
		inode_ptr_of_interest->size = 0;
		for (int i = 0; i < DIRECT_PTRS; i++)
		{
			inode_ptr_of_interest->direct[i] = -1;
		}
		if (type == UFS_DIRECTORY)
		{
			int free_data_block_number = 0;
			uint *data_bitmapptr = (uint *)(image + (s->data_bitmap_addr * UFS_BLOCK_SIZE));
			for (int i = 0; i < 32; i++)
			{
				if (get_bit(data_bitmapptr, i) == 0)
				{
					free_data_block_number = i;
					break;
				}
			}
			printf("Free data block number is %d\n", free_data_block_number);
			inode_ptr_of_interest->direct[0] = s->data_region_addr + free_data_block_number;
			set_bit(data_bitmapptr, free_data_block_number);
			// set all directories to -1.
			inode_t *dir_inode = inode_table;
			dir_ent_t *dir = image + (inode_ptr_of_interest->direct[0] * UFS_BLOCK_SIZE);
			printf("Setting all directory contents to -1\n");
			for (int i = 0; i < (UFS_BLOCK_SIZE / sizeof(dir_ent_t)); i++)
			{
				dir[i].inum = -1;
			}
		}
	}
	int rc;
	rc = msync(s, sizeof(super_t), MS_SYNC);
	assert(rc > -1);
	return 0;
}

int Server_Lookup(void *image, uint *inode_bitmapptr, int pinum, char *name)
{
	int get_presence_of_inode = get_bit(inode_bitmapptr, pinum);
	if (get_presence_of_inode == 0)
		return -1;
	super_t *s = (super_t *)image;
	inode_t *inode_table = image + (s->inode_region_addr * UFS_BLOCK_SIZE);
	inode_t *dir_inode = inode_table;
	dir_ent_t *dir = image + (dir_inode[pinum].direct[0] * UFS_BLOCK_SIZE);
	int file_absent = 1;
	int inum_of_already_present_file;
	for (int i = 0; i < (UFS_BLOCK_SIZE / sizeof(dir_ent_t)); i++)
	{
		if (dir[i].inum != -1)
		{
			if (strcmp(dir[i].name, name) == 0)
			{
				file_absent = 0;
				inum_of_already_present_file = dir[i].inum;
				// printf("File already present at inode number %d\n",inum_of_already_present_file);
				break;
			}
			// printf("File at inode number %d is %s\n",dir[i].inum,dir[i].name);
		}
	}
	if (file_absent == 0)
		return inum_of_already_present_file;
	else
		return -1;
}

int Server_Stat(void *image, uint *inode_bitmapptr, int pinum, MFS_Stat_t *m)
{
	int get_presence_of_inode = get_bit(inode_bitmapptr, pinum);
	if (get_presence_of_inode == 0)
		return -1;
	super_t *s = (super_t *)image;
	inode_t *inode_table = image + (s->inode_region_addr * UFS_BLOCK_SIZE);
	// inode_t* temp = s->data_region_addr+pinum;

	// stat command.
	printf("File size is %d\n", inode_table[pinum].size);
	printf("File type is %d\n", inode_table[pinum].type);
	m->type = inode_table[pinum].type;
	m->size = inode_table[pinum].size;
	return 0;
}

int Server_Unlink(void *image, uint *inode_bitmapptr, int pinum, char *name)
{
	int get_presence_of_inode = get_bit(inode_bitmapptr, pinum);
	if (get_presence_of_inode == 0)
		return -1;
	super_t *s = (super_t *)image;
	inode_t *inode_table = image + (s->inode_region_addr * UFS_BLOCK_SIZE);
	inode_t *dir_inode = inode_table;
	dir_ent_t *dir = image + (dir_inode[pinum].direct[0] * UFS_BLOCK_SIZE);
	for (int i = 0; i < (UFS_BLOCK_SIZE / sizeof(dir_ent_t)); i++)
	{
		if (dir[i].inum != -1)
		{
			if (strcmp(dir[i].name, name) == 0)
			{
				// printf("File already present at inode number %d\n",inum_of_already_present_file);
				// check if it is a file or directory.
				if (inode_table[dir[i].inum].type == 0)
				{
					dir_ent_t *dir1 = image + (inode_table[dir[i].inum].direct[0] * UFS_BLOCK_SIZE);
					int file_absent = 1;
					int inum_of_already_present_file;
					// cannot unlink a non-empty directory
					for (int i = 0; i < (UFS_BLOCK_SIZE / sizeof(dir_ent_t)); i++)
					{
						if (dir1[i].inum != -1)
						{
							return -1;
						}
					}
					// check that the directory is empty.
					// if it is not, return -1.
				}
				dir[i].inum = -1;
				// file_absent= 0;
				break;
			}
			printf("File at inode number %d is %s\n", dir[i].inum, dir[i].name);
		}
		// printf("Value of inum is %d\n",dir[i].inum);
	}

	int rc = msync(s, sizeof(super_t), MS_SYNC);
	assert(rc > -1);
	return 0;
}

int Server_Write(void *image, uint *inode_bitmapptr, int inum, char *buffer, int offset_to_write_to, int size_to_write)
{
	int blk_offset = (offset_to_write_to) % 4096;
	int excess_write = 0;
	int unaligned_write = 0;
	super_t *s = (super_t *)image;
	inode_t *inode_table = image + (s->inode_region_addr * UFS_BLOCK_SIZE);
	if ((blk_offset + size_to_write) > 4096)
	{
		excess_write = (blk_offset + size_to_write) - 4096;
		unaligned_write = 1;
	}
	printf("Amount of excess write is %d\n", excess_write);
	int get_presence_of_inode = get_bit(inode_bitmapptr, inum);
	if (get_presence_of_inode == 0)
		return -1;

	if (size_to_write > 4096)
	{
		printf("Max write amount is 4096B\n");
		return -1;
	}
	inode_t *inode_ptr_of_interest = inode_table + inum;
	if (inode_ptr_of_interest->type == 0)
	{
		printf("Can't write to directory\n");
		return -1;
	}
	else
	{
		// check if offset and upto offset+size have valid entries.
		printf("Inode number of file to write to is %d\n", inum);
		inode_t *inode_ptr_of_interest = inode_table + inum;
		int file_block_to_write_to = offset_to_write_to / 4096;
		// check if the file block is valid, if not we need to find a free block to write to.
		uint *data_bitmapptr = (uint *)(image + (s->data_bitmap_addr * UFS_BLOCK_SIZE));
		if (inode_ptr_of_interest->direct[file_block_to_write_to] == -1)
		{
			printf("File block to write to is invalid\n");
			// find a free data block to write to
			int free_data_block_number = 0;
			for (int i = 0; i < 32; i++)
			{
				if (get_bit(data_bitmapptr, i) == 0)
				{
					free_data_block_number = i;
					break;
				}
			}
			printf("Free data block number is %d\n", free_data_block_number);
			inode_ptr_of_interest->direct[file_block_to_write_to] = s->data_region_addr + free_data_block_number;
			set_bit(data_bitmapptr, free_data_block_number);
		}
		memcpy((char *)(image + (inode_ptr_of_interest->direct[file_block_to_write_to] * UFS_BLOCK_SIZE) + blk_offset), buffer, (size_to_write - excess_write));
		if (excess_write > 0)
		{
			if (inode_ptr_of_interest->direct[file_block_to_write_to + 1] == -1)
			{
				printf("File block to write to is invalid\n");
				// find a free data block to write to
				int free_data_block_number = 0;
				for (int i = 0; i < 32; i++)
				{
					if (get_bit(data_bitmapptr, i) == 0)
					{
						free_data_block_number = i;
						break;
					}
				}
				printf("Free data block number for excess is %d\n", free_data_block_number);
				inode_ptr_of_interest->direct[file_block_to_write_to + 1] = s->data_region_addr + free_data_block_number;
				set_bit(data_bitmapptr, free_data_block_number);
			}
			// write from the start of the next block
			memcpy((char *)(image + (inode_ptr_of_interest->direct[file_block_to_write_to + 1] * UFS_BLOCK_SIZE)), buffer + (size_to_write - excess_write), excess_write);
		}
		inode_ptr_of_interest->size = max(inode_ptr_of_interest->size, (offset_to_write_to + size_to_write));
		printf("Size of file is %d\n", inode_ptr_of_interest->size);
	}
	int rc = msync(s, sizeof(super_t), MS_SYNC);
	assert(rc > -1);
	return 0;
}

int Server_Read(void *image, uint *inode_bitmapptr, int inum, char *buffer, int offset_to_read_from, int size_to_read)
{
	int blk_offset = (offset_to_read_from) % 4096;
	int excess_read = 0;
	int unaligned_write = 0;
	if ((blk_offset + size_to_read) > 4096)
	{
		excess_read = (blk_offset + size_to_read) - 4096;
		unaligned_write = 1;
	}
	printf("Amount of excess read is %d\n", excess_read);

	super_t *s = (super_t *)image;
	inode_t *inode_table = image + (s->inode_region_addr * UFS_BLOCK_SIZE);
	printf("Amount of excess write is %d\n", excess_read);
	int get_presence_of_inode = get_bit(inode_bitmapptr, inum);
	if (get_presence_of_inode == 0)
		return -1;

	if (size_to_read > 4096)
	{
		printf("Max read amount is 4096B\n");
		return -1;
	}

	else
	{
		// check if offset and upto offset+size have valid entries.
		printf("Inode number of file to write to is %d\n", inum);
		inode_t *inode_ptr_of_interest = inode_table + inum;
		int file_block_to_read_from = offset_to_read_from / 4096;
		// check if the file block is valid, if not we need to find a free block to write to.
		// uint* data_bitmapptr = (uint*)(image +(s->data_bitmap_addr*UFS_BLOCK_SIZE));

		if (inode_ptr_of_interest->direct[file_block_to_read_from] == -1)
		{
			printf("File block to read is absent\n");
			return -1;
		}

		if ((offset_to_read_from + size_to_read) > inode_ptr_of_interest->size)
		{
			printf("Access out of bounds\n");
			return -1;
		}
		memcpy(buffer, (char *)(image + (inode_ptr_of_interest->direct[file_block_to_read_from] * UFS_BLOCK_SIZE) + blk_offset), (size_to_read - excess_read));

		if (excess_read > 0)
		{
			if (inode_ptr_of_interest->direct[file_block_to_read_from + 1] == -1)
			{
				printf("File block to read is not fully present\n");
			}
			// read from the start of the next block
			memcpy((buffer + (size_to_read - excess_read)), (char *)(image + (inode_ptr_of_interest->direct[file_block_to_read_from + 1] * UFS_BLOCK_SIZE)), excess_read);
		}
	}
}

int Server_Shutdown(super_t *s, int fd)
{
	int rc = msync(s, sizeof(super_t), MS_SYNC);
	assert(rc > -1);

	close(fd);
}

int Server_Init(char *filename)
{
	// open the file system image

	printf("server:: filename is %s\n", filename);
	int fd = open(filename, O_RDWR);
	assert(fd > -1);

	return fd;
}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		printf("Usage: ./server [portnum] [file-system-image]\n");
		exit(1);
	}

	printf("server:: start...\n");

	// convert argv[1] to an integer port number
	int port = atoi(argv[1]);
	assert(port > 0);

	int sd = UDP_Open(port);
	assert(sd > -1);
	int fd = open("test.img", O_RDWR);
	assert(fd > -1);
	
	struct stat sbuf;
	int rc = fstat(fd, &sbuf);
	assert(rc > -1);

    	int image_size = (int) sbuf.st_size;	
	void *image = mmap(NULL, image_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	assert(image != MAP_FAILED);
	super_t *s = (super_t *)image;
	uint *inode_bitmapptr = (uint *)(image + (s->inode_bitmap_addr * UFS_BLOCK_SIZE));

	struct sockaddr_in *addr;


	// loop over reading requests and processing them
	while (1)
	{
		printf("server:: waiting...\n");

		__MFS_Message_t *recvMsg = recv_message(sd, addr);
		// switch on the type of the message

		switch (recvMsg->type)
		{
			case MFS_INIT:
			{
				printf("server:: init request...\n");
				recvMsg->status = Server_Init(argv[2]);
				send_message(sd, addr, recvMsg);
				break;
			}
			case MFS_LOOKUP:
			{
				recvMsg->status = Server_Lookup(image, inode_bitmapptr, recvMsg->inum, recvMsg->name);
				//send back the message with the return status set with the inode number.
				send_message(sd, addr, recvMsg);
				break;
			}
			case MFS_STAT:
			{
				 recvMsg->status = Server_Stat(image, inode_bitmapptr, recvMsg->inum, &(recvMsg->m)); 
				 send_message(sd, addr, recvMsg);
				break;
			}
			case MFS_WRITE:
			{
				recvMsg->status = Server_Write(image, inode_bitmapptr, recvMsg->inum, recvMsg->buffer, recvMsg->offset, recvMsg->nbytes); 
				send_message(sd, addr, recvMsg);
				break;
			}
			case MFS_READ:
			{
				recvMsg->status = Server_Read(image, inode_bitmapptr, recvMsg->inum, recvMsg->buffer, recvMsg->offset, recvMsg->nbytes); 
				send_message(sd, addr, recvMsg);
				break;
			}
			case MFS_CREAT:
			{
				recvMsg->status = Server_Create(image, inode_bitmapptr, recvMsg->inum, recvMsg->file_type, recvMsg->name); 
				send_message(sd, addr, recvMsg);
				break;
			}
			case MFS_UNLINK:
			{
				recvMsg->status = Server_Unlink(image, inode_bitmapptr, recvMsg->inum, recvMsg->name); 
				send_message(sd, addr, recvMsg);
				break;
			}
			case MFS_SHUTDOWN:
			{
				recvMsg->status = Server_Shutdown(image, inode_bitmapptr); 
				send_message(sd, addr, recvMsg);
				break;
			}
		}

		// check if the request is valid

		// force change
		rc = msync(s, sizeof(super_t), MS_SYNC);
		assert(rc > -1);

		close(fd);
	}

	return 0;
}
