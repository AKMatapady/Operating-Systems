// ***********************************************************************
// File Name: fsaccess.c
//
// Program written by : 
// Amith Kumar Matapady (axm180029)
// Surajit Baitalik		(sxb180026)
// Swapnil Bansal		(sxb180020)
//
// Objective: To design a modified version of Unix V6 file-system with  
// 1024 bytes of block size and 4 GB max file size.
//
// To execute program, the following commands can be used on Linux server:
// cc fsaccess.c
// ./a.out
//
// Supported commands: initfs and q.
// ***********************************************************************

//header files
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>
#include<string.h>

//inode flags		- extra 0 has been added in the beginning to indicate that these are positive numbers
const unsigned short directory = 040000;
const unsigned short plainfile = 000000;
const unsigned short largefile = 010000;
const unsigned short allocatedinode = 0100000;

//Structure of superblock (total size - 1022 Bytes)
typedef struct
{
	unsigned int isize;
	unsigned int fsize;
	unsigned int nfree;
	unsigned int free[148];
	unsigned int ninode;
	unsigned int inode[100];
	unsigned short flock;
	unsigned short ilock;
	unsigned short fmod;
	unsigned int time[2];
}super_Block;

//Structure of inode (total size - 128 Bytes)	//8 inodes per block
typedef struct
{
	unsigned short flags;
	unsigned int nlinks;
	unsigned short uid;
	unsigned short gid;
	unsigned int size1;			// restricts max size to 2^32 = 4 GB
	unsigned int addr[27];
	unsigned short actime[1];
	unsigned short modtime[2];
}inode_Struct;

//Structure of directory (total size - 18 Bytes)
typedef struct
{
	unsigned int inode;
	char filename[14];
}dirStruct;

//Constants
#define BLOCK_SIZE 1024
#define INODES_PER_BLOCK 8
const unsigned short INODE_SIZE = 128;				//size of each INODE (not the value of isize on superblock)

//global instances
super_Block super_Block_inst;
inode_Struct inoderef;
dirStruct root_dir;

//Global Variables
int fd;		//file descriptor 
unsigned int last_free_block[256];		//array used to fill free[0] with the link to next set of free array

//function to write integer array to the required block
void writeinttoblock(unsigned int *source, unsigned int blocknum)
{
	int bytes_written;
	if (blocknum >= super_Block_inst.fsize )
		printf(" Block number is greater than max file system block size for writing\n");
	else
	{
		lseek(fd, blocknum*BLOCK_SIZE, 0);
		if ((bytes_written = write(fd, source, BLOCK_SIZE)) < BLOCK_SIZE)
			printf("\n Error in writing block number : %d", blocknum);
	}
}

// Data blocks are linked in sets of 148 so as to access all free blocks
void set_up_freeblock_links(unsigned int total_no_blocks)
{
	int i = 0;																						//index to be used
	for (i = 0; i < 256; i++)
		last_free_block[i] = 0;																		//clearing last_free_block array (to 0)

	unsigned int emptybuffer[256];																	// buffer to fill an entire block with zeros. Since integer size is 4 bytes, 256 * 4 = 1024 Bytes.
	
	unsigned int no_of_sets = (total_no_blocks - (2 + super_Block_inst.isize)) / 148;				//splitting into blocks of 148 as free array can only store 148 logical addresses
	unsigned int remainingblocks = (total_no_blocks - (2 + super_Block_inst.isize)) % 148;			//remaining blocks
								

	//fill the emptybuffer[256] with zeroes
	for (i = 0; i < 256; i++)
		emptybuffer[i] = 0;																			//initializing emptybuffer array to 0 - may be redundant	

	unsigned int set_no = 0;

	//setting up linked sets of 148 blocks at a time
	for (set_no = 0; set_no < no_of_sets; set_no++)
	{
		if (set_no != (no_of_sets - 1))
		{
			last_free_block[0] = 148;
		}
		else
		{			
			last_free_block[0] = remainingblocks;													//last set only has 'remainingblocks' no of elements
		}

		for (i = 0; i < 148; i++)
		{
			if (set_no == (no_of_sets - 1) && remainingblocks == 0 && i == 0)
			{
				last_free_block[i + 1] = 0;
				break;
			}
			else
			{
				int temp_Block_no = 2 + super_Block_inst.isize + (148 * (set_no + 1)) + i;
				if (set_no == (no_of_sets - 1) && remainingblocks > 0 && temp_Block_no >= total_no_blocks)
				{
					last_free_block[i + 1] = 0;
				}
				else
				{
					last_free_block[i + 1] = temp_Block_no;
				}
			}			
		}
		writeinttoblock(last_free_block, 2 + super_Block_inst.isize + (148 * set_no));

		for (i = 1; i < 148; i++)
			writeinttoblock(emptybuffer, 2 + super_Block_inst.isize + i + 148 * set_no);
	}

	for (i = 0; i < 256; i++)
		last_free_block[i] = 0;
	
	writeinttoblock(last_free_block, 2 + super_Block_inst.isize + (148 * set_no));					//write into the '0th' block of last set

	//clear all the memory blocks indicated in the last set
	for (i = 1; i < remainingblocks; i++)
		writeinttoblock(emptybuffer, 2 + super_Block_inst.isize + i + (148 * set_no));
}

//free data blocks and initialize free array
int add_freeblock(unsigned int blockNum)
{
	if (super_Block_inst.nfree < 148)
	{
		super_Block_inst.free[super_Block_inst.nfree] = blockNum;
		++super_Block_inst.nfree;
	}
	else
	{
		int i = 0;
		for (i = 0; i < 256; i++)
			last_free_block[i] = 0;

		last_free_block[0] = super_Block_inst.nfree;

		for (i = 0; i < 148; i++)
			last_free_block[i+1] = super_Block_inst.free[i];

		for (i = 0; i < 148; i++)
			super_Block_inst.free[i] = 0;

		super_Block_inst.nfree = 0;		

		writeinttoblock(last_free_block, blockNum);

		super_Block_inst.free[super_Block_inst.nfree] = blockNum;
		++super_Block_inst.nfree;

		for (i = 0; i < 256; i++)
			last_free_block[i] = 0;

		lseek(fd, BLOCK_SIZE, 0);
		int bytes_written;
		if ((bytes_written = write(fd, &super_Block_inst, BLOCK_SIZE)) < BLOCK_SIZE)
		{
			printf("\nERROR : error in writing the super block after re-writing free block");
			return 1;
		}		
	}
	return 0;
}

//function to read integer array from the required block
void read_int_from_block(int *dest, unsigned int blocknum)
{
	if (blocknum > super_Block_inst.isize + super_Block_inst.fsize + 2)
		printf(" Block number is greater than max file system block size for reading\n");
	else
	{
		lseek(fd, blocknum*BLOCK_SIZE, 0);
		read(fd, dest, BLOCK_SIZE);
	}
}

//Get a free data block.
unsigned int allocatedatablock()
{
	unsigned int free_block_no;

	super_Block_inst.nfree--;

	free_block_no = super_Block_inst.free[super_Block_inst.nfree];
	super_Block_inst.free[super_Block_inst.nfree] = 0;

	if (super_Block_inst.nfree == 0)
	{
		int n = 0;
		read_int_from_block(last_free_block, free_block_no);
		super_Block_inst.nfree = last_free_block[0];
		for (n = 0; n < 148; n++)
			super_Block_inst.free[n] = last_free_block[n + 1];
	}
	return free_block_no;
}


//function to write to an inode given the inode number	
void write_to_inode(inode_Struct inodeinstance, unsigned int inodenumber)
{
	int bytes_written;
	lseek(fd, 2 * BLOCK_SIZE + inodenumber * INODE_SIZE, 0);
	if ((bytes_written = write(fd, &inodeinstance, INODE_SIZE)) < INODE_SIZE)
		printf("\n Error in writing inode number : %d", inodenumber);

}

//function to create root directory and its corresponding inode.
void create_root()
{
	unsigned int i = 0;
	unsigned short bytes_written;
	unsigned int datablock = allocatedatablock();

	root_dir.filename[0] = '.';			//root directory's file name is .
	root_dir.filename[1] = '\0';
	root_dir.inode = 1;					// root directory's inode number is 1.

	inoderef.flags = allocatedinode | directory | 000077;   		// flag for root directory 
	inoderef.nlinks = 2;
	inoderef.uid = 0;
	inoderef.gid = 0;
	inoderef.size1 = 0;
	inoderef.addr[0] = datablock;

	for (i = 1; i < 28; i++)
		inoderef.addr[i] = 0;

	inoderef.actime[0] = 0;
	inoderef.modtime[0] = 0;
	inoderef.modtime[1] = 0;

	write_to_inode(inoderef, 0);

	lseek(fd, datablock*BLOCK_SIZE, 0);

	//fill first 16 Bytes with .
	if ((bytes_written = write(fd, &root_dir, 16)) < 16)
		printf("\n Error in writing root directory '.' \n ");

	root_dir.filename[1] = '.';
	root_dir.filename[2] = '\0';
	
	// fill .. in next 16 Bytes of data block.
	if ((bytes_written = write(fd, &root_dir, 16)) < 16)
		printf("\n Error in writing root directory '..' \n");

}

//function initializes the file system: 
//parameters - path, total number of blocks and total number of inodes as input.
//Command : initfs <file_name> <total_number_of_blocks_in_disk> <total_number_of_inodes>
//The given path directory is where the file system begins.
int initfs(char* path, unsigned int total_no_blocks, unsigned int total_inodes)
{

	char buffer[BLOCK_SIZE];
	int bytes_written;

	if ((total_inodes % INODES_PER_BLOCK) == 0)
		super_Block_inst.isize = total_inodes / INODES_PER_BLOCK;
	else
		super_Block_inst.isize = (total_inodes / INODES_PER_BLOCK) + 1;

	super_Block_inst.fsize = total_no_blocks;

	unsigned int i = 0;

	if ((fd = open(path, O_RDWR | O_CREAT, 0600)) == -1)			//0600 -> owner can read and write
	{
		printf("\n open() call for the path [%s] failed with error [%s]\n", path, strerror(errno));
		return 1;
	}

	for (i = 0; i < 148; i++)
		super_Block_inst.free[i] = 0;			//initializing free array to 0 to remove junk data. free array will be stored with data block numbers shortly.

	super_Block_inst.nfree = 0;


	super_Block_inst.ninode = 100;

	if (total_inodes < 100)
		super_Block_inst.ninode = total_inodes;

	for (i = 0; i < super_Block_inst.ninode; i++)
		super_Block_inst.inode[i] = i;		//initializing inode array to store inumbers.

	super_Block_inst.flock = '1'; 					//flock,ilock and fmode are not used.
	super_Block_inst.ilock = '1';					//initializing to fill up block
	super_Block_inst.fmod = '1';
	super_Block_inst.time[0] = 0;
	super_Block_inst.time[1] = 0;
	lseek(fd, BLOCK_SIZE, 0);

	// Writing super block 
	if ((bytes_written = write(fd, &super_Block_inst, sizeof(super_Block)) < sizeof(super_Block)))
	{
		printf("\nERROR : error in writing the super block");
		return 0;
	}

	lseek(fd, 2 * BLOCK_SIZE, 0);

	// writing zeroes to all inodes in ilist
	for (i = 0; i < BLOCK_SIZE; i++)
		buffer[i] = 0;

	for (i = 0; i < super_Block_inst.isize; i++)
		write(fd, buffer, BLOCK_SIZE);

	// calling chaining data blocks procedure
	set_up_freeblock_links(total_no_blocks);

	//filling free array to first 148 data blocks
	for (i = 0; i < 148; i++)
		add_freeblock(i + 2 + super_Block_inst.isize);

	// Make root directory
	create_root();

	return 1;
}

int main()
{
	unsigned int number_of_blocks = 0, number_of_inodes = 0;
	char *partialString;
	int fsinit = 0/*, i = 0*/;
	char input[1024];

	unsigned short bytes_written;

	printf("\nThis program accepts the following commands with specified parameters\n");
	printf("1. initfs <file_name_to_represent_disk> <total_no_of_blocks> <total_no_of_inodes>\n");
	printf("2. q\n");

	printf("Enter a command from the above list with required (indicated) parameters:\n");

	while (1)
	{
		//gets("%s", &input);
		scanf(" %[^\n]s", input);
		partialString = strtok(input, " ");

		if (strcmp(partialString, "initfs") == 0)
		{
			char *filepath;
			char *n1, *n2;

			//populate the expected fields from the command entered
			filepath = strtok(NULL, " ");
			n1 = strtok(NULL, " ");
			n2 = strtok(NULL, " ");

			if (access(filepath, F_OK) != -1)
			{
				if ((fd = open(filepath, O_RDWR, 0600)) == -1)
				{
					printf("\n filesystem already exists but open() failed with error [%s]\n", strerror(errno));
					return 1;
				}
				printf("The filesystem already exists. The same will be used.\n");
				fsinit = 1;
			}
			else
			{
				if (!n1 || !n2)
				{
					printf(" All arguments (path, number of inodes and total number of blocks) have not been entered\n");
				}
				else
				{
					number_of_blocks = atoi(n1);
					number_of_inodes = atoi(n2);

					if (initfs(filepath, number_of_blocks, number_of_inodes))
					{
						printf("The file system has been initialized\n");
						fsinit = 1;
					}
					else
					{
						printf("Error initializing file system. Exiting... \n");
						return 1;
					}
				}
			}
			partialString = NULL;
		}
		else if (strcmp(partialString, "q") == 0)
		{
			lseek(fd, BLOCK_SIZE, 0);

			if ((bytes_written = write(fd, &super_Block_inst, BLOCK_SIZE)) < BLOCK_SIZE)
			{
				printf("\nERROR : error in writing the super block");
				return 1;
			}
			break;
		}
		else
		{
			printf("\nInvalid command\n ");
		}
	}
	return 0;
}