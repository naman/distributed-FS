#include <string.h>
#include <math.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ufs.h"


#define min(X,Y) ((X) < (Y) ? (X) : (Y))
#define max(X,Y) ((X) > (Y) ? (X) : (Y))

unsigned int get_bit(unsigned int *bitmap, int position) {
   int index = position / 32;
   int offset = 31 - (position % 32);
   return (bitmap[index] >> offset) & 0x1;
}

void set_bit(unsigned int *bitmap, int position) {
   int index = position / 32;
   int offset = 31 - (position % 32);
   bitmap[index] |= (0x1 << offset);
}

int main(int argc, char*argv){
	int fd = open("test.img",O_RDWR);
	assert(fd > -1);

	struct stat sbuf;
	int rc = fstat(fd, &sbuf);
	assert(rc > -1);

	int image_size = (int) sbuf.st_size;
	
	void *image = mmap(NULL,image_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	assert(image != MAP_FAILED);
	super_t* s = (super_t*)image;
	uint* inode_bitmapptr = (uint*)(image +(s->inode_bitmap_addr*UFS_BLOCK_SIZE));
	
	//search for inode of interest.
	
	int inode_of_interest = 0;
	
	int get_zero = get_bit(inode_bitmapptr, inode_of_interest);
	int get_one = get_bit(inode_bitmapptr, 1);
	int create_type = UFS_REGULAR_FILE; 
	printf("Inode zero is %d\n",get_zero);
	printf("Inode one is %d\n",get_one);
	printf("inode bitmap address %d [len %d]\n", s->inode_region_addr, s->inode_region_len);
	printf("data bitmap address %d [len %d]\n", s->data_region_addr, s->data_region_len);
        inode_t *inode_table = image + (s->inode_region_addr * UFS_BLOCK_SIZE);
        inode_t *root_inode = inode_table;
        printf("\nroot type:%d root size:%d\n", root_inode->type, root_inode->size);
        printf("direct pointers[0]:%d [1]:%d\n", root_inode->direct[0], root_inode->direct[1]);
	
	dir_ent_t *root_dir = image + (root_inode->direct[0] * UFS_BLOCK_SIZE);
    	printf("\nroot dir entries\n%d %s\n", root_dir[0].inum, root_dir[0].name);
    	printf("%d %s\n", root_dir[1].inum, root_dir[1].name);

    	printf("%d %s\n", root_dir[2].inum, root_dir[2].name);
	//try to create /home/f1.txt
	int inum_of_already_present_file = 0;
	int file_absent = 1;
	if (get_zero == 0){
		printf("Inode not present. Exiting\n");
		exit(0);
	}
	if (get_zero == 1){
		//check if name is not too long. 
		inode_t *inode_table = image + (s->inode_region_addr * UFS_BLOCK_SIZE);
		inode_t *dir_inode = inode_table;
		dir_ent_t *dir = image + (dir_inode->direct[0] * UFS_BLOCK_SIZE);

		for (int i = 0;i<(UFS_BLOCK_SIZE/sizeof(dir_ent_t));i++){
			if (dir[i].inum!=-1){
				if (strcmp(dir[i].name,"f1.txt") == 0){
					printf("File already present at inode number %d\n",i);
					file_absent = 0;
					inum_of_already_present_file = dir[i].inum;
					break;
				}
				printf("File at inode number %d is %s\n",dir[i].inum,dir[i].name);
			}
			//printf("Value of inum is %d\n",dir[i].inum);		
		}
		if (file_absent == 1){
			printf("File not present, need to add it to the directory\n");
			//find empty directory entry for file. 
			int empty_dir_entry;

			int free_inode_number = 0;
			for (int i = 0;i<1000;i++){
				if (get_bit(inode_bitmapptr,i) == 0){
					free_inode_number = i;
					break;
				}
			}
			printf("Free inode number is %d\n",free_inode_number);
			set_bit(inode_bitmapptr, free_inode_number);
			for (int i = 0;i<(UFS_BLOCK_SIZE/sizeof(dir_ent_t));i++){
				if (dir[i].inum==-1){
					strcpy(dir[i].name,"f1.txt");
					dir[i].inum = free_inode_number;
					printf("Copying f1.txt to dirent %d\n",i);
					break;
				}
				//printf("Value of inum is %d\n",dir[i].inum);		
			}
			inode_t *inode_ptr_of_interest = inode_table+(free_inode_number*sizeof(inode_t));
			inode_ptr_of_interest->type = create_type;
			inode_ptr_of_interest->size = 0;
			for (int i = 0;i<DIRECT_PTRS;i++){
				inode_ptr_of_interest->direct[i] = -1;
			}
		}
		//check that file does not already exist in the directory, if 
		//it does, nothing to do. 
		//find a free inode number.
	}	
        //lookup is similar to create, except that it returns the inode number if name is present in the directory.
	//if not, it returns -1.
	
	//mfs_stat is easy
	//check if inode is present.
	//if present, return size of file and type. 	
	
	//write functionality
//	int offset_to_write_to = 100;
//	int blk_offset = (offset_to_write_to)%4096;
//	int size_to_write = 10;
//	int excess_write = 0;
//	int unaligned_write = 0; 
//	if ((blk_offset+size_to_write)>4096){
//		excess_write = (blk_offset+size_to_write)-4096;
//		unaligned_write = 1;
//	}
//	printf("Amount of excess write is %d\n",excess_write);
//	if (file_absent == 1){
//		printf("Cannot write to an absent file \n");
//	}
//	else if (size_to_write > 4096){
//		printf("Max write amount is 4096B\n");
//	}
//	else if (file_type == DIRECTORY){
//	
//	}
//	else{
//		//check if offset and upto offset+size have valid entries.
//		printf("Inode number of file to write to is %d\n",inum_of_already_present_file);
//		inode_t *inode_ptr_of_interest = inode_table+(inum_of_already_present_file*sizeof(inode_t));
//                int file_block_to_write_to = offset_to_write_to/4096;
//		//check if the file block is valid, if not we need to find a free block to write to. 
//		uint* data_bitmapptr = (uint*)(image +(s->data_bitmap_addr*UFS_BLOCK_SIZE));
//		if (inode_ptr_of_interest->direct[file_block_to_write_to] == -1){
//			printf("File block to write to is invalid\n");
//			//find a free data block to write to 
//			int free_data_block_number = 0;
//			for (int i = 0;i<32;i++){
//				if (get_bit(data_bitmapptr,i) == 0){
//					free_data_block_number = i;
//					break;
//				}
//			}
//			printf("Free data block number is %d\n",free_data_block_number);
//			inode_ptr_of_interest->direct[file_block_to_write_to] = s->data_region_addr+free_data_block_number;
//			set_bit(data_bitmapptr, free_data_block_number);
//		}
//		char *str1 = "Nahi Nahi";
//		printf("Length of the string is %d\n",strlen(str1));
//		memcpy((char*)(image + (inode_ptr_of_interest->direct[file_block_to_write_to]*UFS_BLOCK_SIZE)+blk_offset),str1,(strlen(str1)+1-excess_write));
//		if (excess_write>0){
//			if (inode_ptr_of_interest->direct[file_block_to_write_to+1] == -1){
//				printf("File block to write to is invalid\n");
//				//find a free data block to write to 
//				int free_data_block_number = 0;
//				for (int i = 0;i<32;i++){
//					if (get_bit(data_bitmapptr,i) == 0){
//						free_data_block_number = i;
//						break;
//					}
//				}
//				printf("Free data block number for excess is %d\n",free_data_block_number);
//				inode_ptr_of_interest->direct[file_block_to_write_to+1] = s->data_region_addr+free_data_block_number;
//				set_bit(data_bitmapptr, free_data_block_number);
//			}
//			//write from the start of the next block
//			memcpy((char*)(image + (inode_ptr_of_interest->direct[file_block_to_write_to+1]*UFS_BLOCK_SIZE)),(str1+(strlen(str1)+1-excess_write)),excess_write);
//		}
//		inode_ptr_of_interest->size = max(inode_ptr_of_interest->size,(offset_to_write_to+size_to_write));
//		printf("Size of file is %d\n",inode_ptr_of_interest->size);
//	}	

//	//read functionality
//		
	int offset_to_read_from = 100;
	int blk_offset = (offset_to_read_from)%4096;
	int size_to_read = 10;
	int excess_read = 0;
	int unaligned_write = 0; 
	if ((blk_offset+size_to_read)>4096){
		excess_read = (blk_offset+size_to_read)-4096;
		unaligned_write = 1;
	}
	printf("Amount of excess read is %d\n",excess_read);
	if (file_absent == 1){
		printf("Cannot write to an absent file \n");
	}
	else if (size_to_read > 4096){
		printf("Max read amount is 4096B\n");
	}
	
	else{
		//check if offset and upto offset+size have valid entries.
		printf("Inode number of file to write to is %d\n",inum_of_already_present_file);
		inode_t *inode_ptr_of_interest = inode_table+(inum_of_already_present_file*sizeof(inode_t));
                int file_block_to_read_from = offset_to_read_from/4096;
		//check if the file block is valid, if not we need to find a free block to write to. 
		//uint* data_bitmapptr = (uint*)(image +(s->data_bitmap_addr*UFS_BLOCK_SIZE));
		
		if (inode_ptr_of_interest->direct[file_block_to_read_from] == -1){
			printf("File block to read is absent\n");
			exit(0);
		}
		
		if ((offset_to_read_from+size_to_read)>inode_ptr_of_interest->size){
			printf("Access out of bounds\n");
			exit(0);
		}
		char *str2 = malloc(sizeof(char)*size_to_read); 
		memcpy(str2,(char*)(image + (inode_ptr_of_interest->direct[file_block_to_read_from]*UFS_BLOCK_SIZE)+blk_offset),(size_to_read-excess_read));
		
		if (excess_read>0){
			if (inode_ptr_of_interest->direct[file_block_to_read_from+1] == -1){
				printf("File block to read is not fully present\n");
			}
			//read from the start of the next block
			memcpy((str2+(size_to_read-excess_read)),(char*)(image + (inode_ptr_of_interest->direct[file_block_to_read_from+1]*UFS_BLOCK_SIZE)),excess_read);
		}
		printf("The string read is %s\n",str2);
	}	

	
	//don't do this.
	//s->inode_bitmap_len = 1;

	//force change
	rc = msync(s, sizeof(super_t), MS_SYNC);
	assert(rc>-1);

	close(fd);
	return 0;




}
