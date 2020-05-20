// ***********************************************************************
// File Name: fsaccess.c
//
// Program written by :
// Amith Kumar Matapady 	(axm180029)
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
// Supported commands: initfs, cpin, cout, mkdir, rm and q.
// format:
//		initfs <file_name_to_represent_disk> <total_no_of_blocks> <total_no_of_inodes>
//		cpin <external_file_name> <file_system_file_name>
//		cout <file_system_file_name> <external_file_name>
//		mkdir <file_system_file_name>
//		rm <file_system_file_name>
//		q
// ***********************************************************************

//#include "stdafx.h"

//#include "pch.h"

//header files
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>
#include<string.h>
#include <stdbool.h>

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

//Structure of directory (total size - 16 Bytes)
typedef struct
{
	unsigned int inode;
	char filename[12];
}dirStruct;

//Constants
#define BLOCK_SIZE 1024
#define INODES_PER_BLOCK 8
const unsigned short INODE_SIZE = 128;				//size of each INODE (not the value of isize on superblock)

													//global instances
super_Block super_Block_inst;
inode_Struct inoderef;
dirStruct root_dir;
dirStruct new_dir;
//Global Variables
int fd;		//file descriptor
unsigned int last_free_block[256];		//array used to fill free[0] with the link to next set of free array

										//function to read character array from the required block
void blockreaderchar(char *chararray, unsigned int blocknum)
{

	if (blocknum >= super_Block_inst.fsize)
	{
		printf(" Block number is greater than max file system block size for reading\n");
	}
	else
	{
		lseek(fd, blocknum*BLOCK_SIZE, 0);
		read(fd, chararray, BLOCK_SIZE);
	}
}

//function to write character array to the required block
void blockwriterchar(char *chararray, unsigned int blocknum)
{
	int bytes_written;
	if (blocknum >= super_Block_inst.fsize)
		printf(" Block number is greater than max file system block size for writing\n");
	else {
		lseek(fd, blocknum*BLOCK_SIZE, 0);
		if ((bytes_written = write(fd, chararray, BLOCK_SIZE)) < BLOCK_SIZE)
			printf("\n Error in writing block number : %d\n", blocknum);
	}
}

//need to add inodes for > 100
unsigned int GetFreeInode()
{
	super_Block_inst.ninode--;
	unsigned int ReturnInodeNumber = super_Block_inst.inode[super_Block_inst.ninode];
	return ReturnInodeNumber;
}

//function to write integer array to the required block
void writeinttoblock(unsigned int *source, unsigned int blocknum)
{
	int bytes_written;
	if (blocknum >= super_Block_inst.fsize)
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
			last_free_block[i + 1] = super_Block_inst.free[i];

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
void read_int_from_block(unsigned int *dest, unsigned int blocknum)
{
	if (blocknum >= super_Block_inst.fsize)
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
//here inodenumber = (actual_inode - 1)
void write_to_inode(inode_Struct inodeinstance, unsigned int inodenumberMinusOne)
{
	int bytes_written;
	lseek(fd, 2 * BLOCK_SIZE + inodenumberMinusOne * INODE_SIZE, 0);
	if ((bytes_written = write(fd, &inodeinstance, INODE_SIZE)) < INODE_SIZE)
		printf("\n Error in writing inode number : %d", (inodenumberMinusOne + 1));
}

inode_Struct read_from_inode(unsigned int inodeNumber)
{
	inode_Struct returnStruct;
	lseek(fd, 2 * BLOCK_SIZE + (inodeNumber - 1) * INODE_SIZE, 0);
	read(fd, &returnStruct, sizeof(inode_Struct));
	return returnStruct;
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

//function to write to directory's data block
//gets inode(always root directory's inode from mkdir) and directory (struct's) reference as inputs.
int write_to_directory(inode_Struct rootinode, dirStruct dir)
{

	int duplicate = 0;  		//to find duplicate named directories.
	unsigned short addrcount = 0;
	char dirbuf[BLOCK_SIZE];		//array to
	int i = 0;
	for (addrcount = 0; addrcount <= 27; addrcount++)
	{
		lseek(fd, rootinode.addr[addrcount] * BLOCK_SIZE, 0);
		for (i = 0; i < 128; i++)
		{
			/*char *FN1;
			strcpy(FN1, new_dir.filename);

			char *FN2;
			strcpy(FN2, dir.filename);*/

			read(fd, &new_dir, 16);
			if (strcmp(new_dir.filename, dir.filename) == 0)			//check for duplicate named directories
			{
				printf("Cannot create directory.The directory name already exists.\n");
				duplicate = 1;
				break;
			}
		}
	}
	if (duplicate != 1)
	{
		for (addrcount = 0; addrcount <= 27; addrcount++)			//for each of the address elements ( addr[0],addr[1] till addr[27]), check which inode is not allocated
		{
			blockreaderchar(dirbuf, rootinode.addr[addrcount]);
			for (i = 0; i < 128; i++)										//Looping for each directory entry (2048/16 = 128 entries in total, where 2048 is block size and 16 bytes is directory entry size)
			{

				if (dirbuf[16 * i] == 0) // if inode is not allocated
				{
					memcpy(dirbuf + 16 * i, &dir.inode, sizeof(dir.inode));
					memcpy(dirbuf + 16 * i + sizeof(dir.inode), &dir.filename, sizeof(dir.filename));		//using memcpy function to copy contents of filename and inode number, to store it in directory entry.
					blockwriterchar(dirbuf, rootinode.addr[addrcount]);
					return duplicate;
				}
			}
		}
	}
	return duplicate;
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

//remove a directory of file
void RemoveDirectory(char* DirectoryName)
{
	char *newPartialPath = (char*)malloc(strlen(DirectoryName));
	strcpy(newPartialPath, DirectoryName);
	char* partialPath = strtok(newPartialPath, "/");

	char* FinalFileName = (char *)malloc(sizeof(char) * 12);

	while (partialPath)
	{
		if (strlen(partialPath) < 11)
			strcpy(FinalFileName, partialPath);
		else
			strncpy(FinalFileName, partialPath, 11);

		partialPath = strtok(NULL, "/");
	}

	partialPath = strtok(DirectoryName, "/");

	int ParentInodeNumber = 1;
	bool skippedRoot = true;
	inode_Struct TempStruct;

	while (partialPath)
	{
		bool foundParent = false;
		if (skippedRoot)
		{
			if (strcmp(FinalFileName, partialPath) != 0)
			{
				TempStruct = read_from_inode(ParentInodeNumber);

				int AddressArrayindex = 0;
				for (; AddressArrayindex < 27; AddressArrayindex++)
				{
					unsigned int dataBlock = TempStruct.addr[AddressArrayindex];
					lseek(fd, dataBlock * BLOCK_SIZE, 0);

					int index = 0;
					for (; index < 16; index++)
					{
						dirStruct TempReadDir;

						read(fd, &TempReadDir, sizeof(dirStruct));
						if (strcmp(TempReadDir.filename, partialPath) == 0)
						{
							ParentInodeNumber = TempReadDir.inode;
							foundParent = true;

							strcpy(TempReadDir.filename, "");
							TempReadDir.inode = 0;

							lseek(fd, (dataBlock * BLOCK_SIZE) + (index * (sizeof(TempReadDir))), 0);
							write(fd, &TempReadDir, sizeof(TempReadDir));

							//AddressArrayindex = 27;					//done to break from all the outer loop
							//break;
						}
						else if (strcmp(TempReadDir.filename, "") == 0)
						{
							//AddressArrayindex = 27;					//done to break from all the outer loop
							//break;
						}
						else if (foundParent)
						{
							lseek(fd, (dataBlock * BLOCK_SIZE) + ((index-1) * (sizeof(TempReadDir))), 0);
							write(fd, &TempReadDir, sizeof(TempReadDir));
						}
					}
					if (foundParent)
						break;
				}

				if (!foundParent)
				{
					printf("\n The given Hierarchy of directory doesn't exist. Create all the parent directories using \"mkdir\" function before trying to create sub directories \n");
					return;
				}
			}
			else
			{
				//this is the actual filename at the destination which conatins the contents of the external file
				TempStruct = read_from_inode(ParentInodeNumber);

				//unsigned short LFile = largefile | allocatedinode | plainfile | 000777;
				//unsigned short SFile = allocatedinode | plainfile | 000777;
				//unsigned short Directory = directory | 000777;

				unsigned short check = TempStruct.flags | directory;

				if (TempStruct.flags == check)	//directory
				{
					int AddressArrayindex = 0;
					for (; AddressArrayindex < 27; AddressArrayindex++)
					{
						unsigned int dataBlock = TempStruct.addr[AddressArrayindex];
						add_freeblock(dataBlock);
					}
				}
				else		//file
				{
					int index = 0;
					for (; index < 27; index++)
					{
						add_freeblock(TempStruct.addr[index]);
					}

					int finish = 0;

					unsigned short filecheck = TempStruct.flags | largefile;
					if (TempStruct.flags == filecheck)	//large file
					{
						int x, y, z = 0;
						unsigned int array1[256];
						unsigned int array2[256];
						unsigned int array3[256];

						for (x = 0; x < 256; x++)
						{
							array1[x] = 0;
							array2[x] = 0;
							array3[x] = 0;
						}

						lseek(fd, TempStruct.addr[26] * BLOCK_SIZE, 0);
						read(fd, array1, BLOCK_SIZE);

						for (x = 0; x < 256; x++)
						{
							if (array1[x] < 2)
							{
								finish = 1;
								break;
							}

							lseek(fd, array1[x] * BLOCK_SIZE, 0);
							read(fd, array2, BLOCK_SIZE);

							for (y = 0; y < 256; y++)
							{
								if (array2[y] < 2)
								{
									finish = 1;
									break;
								}
								lseek(fd, array2[y] * BLOCK_SIZE, 0);
								read(fd, array3, BLOCK_SIZE);

								for (z = 0; z < 256; z++)
								{
									if (array3[z] < 2)
									{
										finish = 1;
										break;
									}
									add_freeblock(array3[z]);
								}
								add_freeblock(array2[y]);
								if (finish)
									break;
							}
							add_freeblock(array1[x]);
							if (finish)
								break;
						}
						
					}
					add_freeblock(TempStruct.addr[26]);
				}

				TempStruct.flags = 0;

				int i = 0;
				for (; i < 27; i++)
					TempStruct.addr[i] = 0;
				write_to_inode(TempStruct, (ParentInodeNumber - 1));

				if (super_Block_inst.ninode < 100)
				{
					super_Block_inst.inode[super_Block_inst.ninode] = ParentInodeNumber;
					super_Block_inst.ninode++;
				}

				break;
			}

		}

		skippedRoot = true;
		partialPath = strtok(NULL, "/");
	}
	printf("Remove Success");
}

void CreateDirectory(char* DirectoryName, unsigned int NewInodeNumber)
{
	//char* partialPath = strtok(DirectoryName, "/");
	//char *FinalFileName = GetFinalFolderOrPath(DirectoryName);

	char *newPartialPath = (char*)malloc(strlen(DirectoryName));
	strcpy(newPartialPath, DirectoryName);

	char* partialPath = strtok(newPartialPath, "/");

	char* FinalFileName = (char *)malloc(sizeof(char) * 12);

	while (partialPath)
	{
		if (strlen(partialPath) < 11)
			strcpy(FinalFileName, partialPath);
		else
			strncpy(FinalFileName, partialPath, 11);

		partialPath = strtok(NULL, "/");
	}

	//printf("file name is %s\n", FinalFileName);

	partialPath = strtok(DirectoryName, "/");

	int ParentInodeNumber = 1;
	bool skippedRoot = true;
	inode_Struct TempStruct;

	while (partialPath)
	{
		bool foundParent = false;
		if (skippedRoot)
		{
			if (strcmp(FinalFileName, partialPath) != 0)
			{
				TempStruct = read_from_inode(ParentInodeNumber);

				int AddressArrayindex = 0;
				for (; AddressArrayindex < 27; AddressArrayindex++)
				{
					unsigned int dataBlock = TempStruct.addr[AddressArrayindex];
					lseek(fd, dataBlock * BLOCK_SIZE, 0);

					int index = 0;
					for (; index < 16; index++)
					{
						dirStruct TempReadDir;

						read(fd, &TempReadDir, sizeof(dirStruct));
						if (strcmp(TempReadDir.filename, partialPath) == 0)
						{
							ParentInodeNumber = TempReadDir.inode;
							foundParent = true;
							//AddressArrayindex = 27;					//done to break from all the outer loop
							break;
						}
						else if (strcmp(TempReadDir.filename, "") == 0)
						{
							//AddressArrayindex = 27;					//done to break from all the outer loop
							break;
						}
					}
				}

				if (!foundParent)
				{
					printf("\n The given Hierarchy of directory doesn't exist. Create all the parent directories using \"mkdir\" function before trying to create sub directories \n");
					return;
				}
			}
			else
			{
				//this is the actual filename at the destination which conatins the contents of the external file
				TempStruct = read_from_inode(ParentInodeNumber);

				int AddressArrayindex = 0;
				for (; AddressArrayindex < 27; AddressArrayindex++)
				{
					unsigned int dataBlock = TempStruct.addr[AddressArrayindex];
					lseek(fd, dataBlock * BLOCK_SIZE, 0);

					int index = 0;
					for (; index < 16; index++)
					{
						dirStruct TempReadDir;

						read(fd, &TempReadDir, sizeof(dirStruct));
						if (strcmp(TempReadDir.filename, FinalFileName) == 0)
						{
							printf("Cannot create the given directory as a directory with the same name already exists.\n");
							AddressArrayindex = 27;					//done to break from all the outer loop
							break;
						}
						else if (strcmp(TempReadDir.filename, "") == 0)
						{
							unsigned int block_num = allocatedatablock();
							strcpy(TempReadDir.filename, FinalFileName);
							TempReadDir.inode = NewInodeNumber;
							lseek(fd, (dataBlock * BLOCK_SIZE) + (index * (sizeof(TempReadDir))), 0);
							write(fd, &TempReadDir, sizeof(TempReadDir));

							// set up this directory's inode and store at inode number - NewInodeNumber
							inoderef.flags = allocatedinode | directory | 000777;
							inoderef.nlinks = 2;
							inoderef.uid = '0';
							inoderef.gid = '0';
							inoderef.size1 = 128;
							int i = 0;
							for (i = 1; i < 28; i++)
								inoderef.addr[i] = 0;
							inoderef.addr[0] = block_num;

							inoderef.actime[0] = 0;
							inoderef.modtime[0] = 0;
							inoderef.modtime[1] = 0;
							write_to_inode(inoderef, (NewInodeNumber - 1));

							strcpy(TempReadDir.filename, ".");
							TempReadDir.inode = NewInodeNumber;
							lseek(fd, (block_num * BLOCK_SIZE) + (0 * (sizeof(TempReadDir))), 0);
							write(fd, &TempReadDir, sizeof(TempReadDir));

							strcpy(TempReadDir.filename, "..");
							TempReadDir.inode = ParentInodeNumber;
							lseek(fd, (block_num * BLOCK_SIZE) + (1 * (sizeof(TempReadDir))), 0);
							write(fd, &TempReadDir, sizeof(TempReadDir));

							printf("\n Directory created \n");

							AddressArrayindex = 27;					//done to break from all the outer loop
							break;
						}
					}
				}
				break;
			}

		}

		skippedRoot = true;
		partialPath = strtok(NULL, "/");
	}
}


int CheckParentExists(char* path, int iNumber)
{
	//char* partialPath = strtok(path, "/");
	//char *FinalFileName = GetFinalFolderOrPath(path);

	char* partialPath = strtok(path, "/");

	char* FinalFileName = (char *)malloc(sizeof(char) * 12);

	while (partialPath)
	{
		if (strlen(partialPath) < 11)
			strcpy(FinalFileName, partialPath);
		else
			strncpy(FinalFileName, partialPath, 11);

		partialPath = strtok(NULL, "/");
	}

	//printf("file name is %s\n", FinalFileName);

	partialPath = strtok(path, "/");

	int ParentInodeNumber = 1;
	bool skippedRoot = true;
	inode_Struct TempStruct;

	while (partialPath)
	{
		bool foundParent = false;
		if (skippedRoot)
		{
			if (strcmp(FinalFileName, partialPath) != 0)
			{
				TempStruct = read_from_inode(ParentInodeNumber);

				int AddressArrayindex = 0;
				for (; AddressArrayindex < 27; AddressArrayindex++)
				{
					unsigned int dataBlock = TempStruct.addr[AddressArrayindex];
					lseek(fd, dataBlock * BLOCK_SIZE, 0);

					int index = 0;
					for (; index < 16; index++)
					{
						dirStruct TempReadDir;

						read(fd, &TempReadDir, sizeof(dirStruct));
						if (strcmp(TempReadDir.filename, partialPath) == 0)
						{
							ParentInodeNumber = TempReadDir.inode;
							foundParent = true;
							break;
						}
						else if (strcmp(TempReadDir.filename, "") == 0)
						{
							break;
						}
					}
				}

				if (!foundParent)
				{
					printf("\n The given Hierarchy of directory doesn't exist. Create all the parent directories using \"mkdir\" function before trying to copy a file to that location \n");
					return;
				}
			}
			else
			{
				//this is the actual filename at the destination which conatins the contents of the external file
				TempStruct = read_from_inode(ParentInodeNumber);

				int AddressArrayindex = 0;
				for (; AddressArrayindex < 27; AddressArrayindex++)
				{
					unsigned int dataBlock = TempStruct.addr[AddressArrayindex];
					lseek(fd, dataBlock * BLOCK_SIZE, 0);

					int index = 0;
					for (; index < 16; index++)
					{
						dirStruct TempReadDir;

						read(fd, &TempReadDir, sizeof(dirStruct));
						if (strcmp(TempReadDir.filename, "") == 0)
						{
							strcpy(TempReadDir.filename, FinalFileName);
							TempReadDir.inode = iNumber;
							lseek(fd, (dataBlock * BLOCK_SIZE) + (index * (sizeof(TempReadDir))), 0);
							write(fd, &TempReadDir, sizeof(TempReadDir));
							break;
						}
					}
				}
				break;
			}

		}

		skippedRoot = true;
		partialPath = strtok(NULL, "/");
	}

}


int cpin(char* source, char* dest)
{
	//printf("cpin start\n");
	char ReadBuffer[BLOCK_SIZE];
	int sfd;
	unsigned long sfileSize = 0;
	unsigned long TotalBytesRead = 0;
	int CurrentBytesRead = 0;

	FILE *fp = fopen(source, "r");
	fseek(fp, 0, SEEK_END);
	sfileSize = ftell(fp);
	fclose(fp);

	//printf("\nfile size is %ld\n", sfileSize);

	//open external file
	if ((sfd = open(source, O_RDONLY)) == -1)
	{
		printf("\nCan't open Source file %s. Permission Issue \n", source);
		return -1;
	}

	//printf("\nSource file open success %s\n", source);

	unsigned int inumber = GetFreeInode();
	//printf("inode no %d\n", inumber);
	if (inumber < 2) // 1 for root
	{
		printf("Error : ran out of inodes \n");
		return;
	}

	/*if (!CheckParentExists(dest, inumber))
	{
	printf("\n Invalid file-System File Name \n");
	return -1;
	}*/

	char* partialPath = strtok(dest, "/");

	char* FinalFileName = (char *)malloc(sizeof(char) * 12);

	while (partialPath)
	{
		if (strlen(partialPath) < 11)
			strcpy(FinalFileName, partialPath);
		else
			strncpy(FinalFileName, partialPath, 11);

		partialPath = strtok(NULL, "/");
	}

	//printf("file name is %s\n", FinalFileName);

	//new file - modified V6 file system
	new_dir.inode = inumber;
	if (strlen(FinalFileName) < 12)
	{
		memcpy(new_dir.filename, FinalFileName, strlen(FinalFileName));
	}
	else
	{
		memcpy(new_dir.filename, dest, 11);
	}

	//New file - inode struct
	inoderef.flags = plainfile | allocatedinode | 000777;
	inoderef.nlinks = 1;
	inoderef.uid = '0';
	inoderef.gid = '0';
	inoderef.size1 = 0;


	if (sfileSize <= 27648)
	{
		//small file
		int index = 0;
		while (1)
		{
			memset(ReadBuffer, 0x00, BLOCK_SIZE);
			if ((CurrentBytesRead = read(sfd, ReadBuffer, BLOCK_SIZE)) != 0 && index < 27)
			{
				int NewBlockNum = allocatedatablock();
				//printf("Blk_no is %d\n", NewBlockNum);
				blockwriterchar(ReadBuffer, NewBlockNum);
				inoderef.addr[index] = NewBlockNum;
				TotalBytesRead += CurrentBytesRead;
				index++;
				printf("Small file - current read %d, totalFileSize now %ld\n", CurrentBytesRead, TotalBytesRead);
			}
			else
			{
				inoderef.size1 = TotalBytesRead;
				printf("Small file copied\n");
				printf("External file Size %lu, Copies file size %lu\n", sfileSize, TotalBytesRead);
				break;
			}
		}
	}
	else
	{
		//large file
		inoderef.flags = largefile | plainfile | allocatedinode | 000777;
		int index = 0;
		for (; index < 26; index++)
		{
			memset(ReadBuffer, 0x00, BLOCK_SIZE);
			CurrentBytesRead = read(sfd, ReadBuffer, BLOCK_SIZE);
			int NewBlockNum = allocatedatablock();
			blockwriterchar(ReadBuffer, NewBlockNum);
			inoderef.addr[index] = NewBlockNum;
			TotalBytesRead += CurrentBytesRead;
			printf("large file - current read %d, totalFileSize now %ld\n", CurrentBytesRead, TotalBytesRead);
		}


		//triple indirect block allocation
		inoderef.addr[index] = allocatedatablock();
		printf("allocate data block for 26th addr %d\n", inoderef.addr[index]);

		int x, y, z = 0;
		unsigned int array1[256];
		unsigned int array2[256];
		unsigned int array3[256];

		for (x = 0; x < 256; x++)
		{
			array1[x] = 0;
			array2[x] = 0;
			array3[x] = 0;
		}

		int finish = 0;

		for (x = 0; x < 256; x++)
		{
			if ((CurrentBytesRead = read(sfd, ReadBuffer, BLOCK_SIZE)) != 0)
			{
				TotalBytesRead += CurrentBytesRead;
				printf("large file - just read - current read %d, totalFileSize now %ld\n", CurrentBytesRead, TotalBytesRead);
				array1[x] = allocatedatablock();
				printf("allocate data block for array1 %d - %d\n", x, array1[x]);
				for (y = 0; y < 256; y++)
				{
					array2[y] = allocatedatablock();
					printf("allocate data block for array2 %d - %d\n", y, array2[y]);
					for (z = 0; z < 256; z++)
					{
						array3[z] = allocatedatablock();
						printf("allocate data block for array3 %d - %d\n", z, array3[z]);
						blockwriterchar(ReadBuffer, array3[z]);
						memset(ReadBuffer, 0x00, BLOCK_SIZE);
						if ((CurrentBytesRead = read(sfd, ReadBuffer, BLOCK_SIZE)) != 0)
						{
							TotalBytesRead += CurrentBytesRead;
							//Read success
							printf("large file - just read - current read %d, totalFileSize now %ld\n", CurrentBytesRead, TotalBytesRead);
						}
						else
						{
							inoderef.size1 = TotalBytesRead;
							finish = 1;
							break;
						}
					}
					writeinttoblock(array3, array2[y]);
					if (finish)
						break;
				}
				writeinttoblock(array2, array1[x]);
				if (finish)
					break;
			}
			else
			{
				inoderef.size1 = TotalBytesRead;
				finish = 1;
				printf("Large file copied\n");
				printf("External file Size %lu, Copies file size %lu\n", sfileSize, TotalBytesRead);
				break;
			}

		}
		writeinttoblock(array1, inoderef.addr[26]);

	}

	inoderef.actime[0] = 0;
	inoderef.modtime[0] = 0;
	inoderef.modtime[1] = 0;

	write_to_inode(inoderef, (inumber - 1));
	lseek(fd, 2 * BLOCK_SIZE, 0);
	read(fd, &inoderef, INODE_SIZE);
	inoderef.nlinks++;
	write_to_directory(inoderef, new_dir);
	printf("cpin END\n");
	return 0;
}

void cpout(char* source, char* dest)
{
	char *newPartialPath = (char*)malloc(strlen(source));
	strcpy(newPartialPath, source);
	char* sourcePartialPath = strtok(newPartialPath, "/");
	char* FileName = (char *)malloc(sizeof(char) * 256);

	while (sourcePartialPath)
	{
		if (strlen(sourcePartialPath) < 255)
			strcpy(FileName , sourcePartialPath);
		else
			strncpy(FileName , sourcePartialPath, 255);

		sourcePartialPath = strtok(NULL, "/");
	}


	
	int dfd;
	//open or create external file(target file) for read and write
	if ((dfd = open(dest, O_RDWR | O_CREAT, 0600)) == -1)
	{
		printf("\nerror opening file: %s\n", dest);
		return;
	}

	//CheckParentExists(source)

	inode_Struct TempStruct;
	TempStruct = read_from_inode(1);

	int InodeToCheck = 1;
	int found = 0;
	int end = 0;
	char ReadBuffer[BLOCK_SIZE];

	int AddressArrayindex = 0;
	for (; AddressArrayindex < 27; AddressArrayindex++)
	{
		unsigned int dataBlock = TempStruct.addr[AddressArrayindex];
		printf("dataBlock  %d\n", dataBlock );
		lseek(fd, dataBlock * BLOCK_SIZE, 0);

		int index = 0;
		for (; index < 16; index++)
		{
			dirStruct TempReadDir;

			read(fd, &TempReadDir, sizeof(dirStruct));
			printf("filename check %s, %s\n", TempReadDir.filename, source);
			if (strcmp(TempReadDir.filename, FileName) == 0)
			{
				InodeToCheck = TempReadDir.inode;
				found = 1;
				break;
			}
			else if (strcmp(TempReadDir.filename, "") == 0)
			{
				end = 0;
				break;
			}
		}
		if (found || end)
			break;
	}

	printf("inode to check %d\n", InodeToCheck);

	TempStruct = read_from_inode(InodeToCheck);

	printf("Read from Inode %d", TempStruct.addr[0]);

	unsigned int SSize = TempStruct.size1;
	unsigned int TotalReadBytes = 0;
	unsigned int ReadBytes = 0;
	int LargeFile = 0;
	int finish = 0;

	unsigned short compare = largefile | plainfile | allocatedinode | 000777;

	if (TempStruct.flags == compare)
	{
		LargeFile = 1;
	}

	int sfd = 0;
	if ((sfd = open(dest, O_RDWR | O_CREAT, 0600)) == -1)
	{
		printf("\n open() call for the path [%s] failed with error [%s]\n", dest, strerror(errno));
		return;
	}

	//while (TotalReadBytes < SSize)
	{
		int i = 0;
		for (; i < 26; i++)
		{
			int written = 0;

			if (TempStruct.addr[i] < 2)
			{
				finish = 1;
				break;
			}

			lseek(fd, TempStruct.addr[i] * BLOCK_SIZE, 0);
			ReadBytes = read(fd, ReadBuffer, BLOCK_SIZE);


			if ((written = write(sfd, ReadBuffer, BLOCK_SIZE)) < 1)
			{
				printf("written %d", written);
			}
			TotalReadBytes += ReadBytes;
			if (ReadBytes < 1 /*|| TotalReadBytes >= SSize*/)
			{
				finish = 1;
				break;
			}
		}

		if (LargeFile)
		{
			int x, y, z = 0;
			unsigned int array1[256];
			unsigned int array2[256];
			unsigned int array3[256];

			for (x = 0; x < 256; x++)
			{
				array1[x] = 0;
				array2[x] = 0;
				array3[x] = 0;
			}

			lseek(fd, TempStruct.addr[26] * BLOCK_SIZE, 0);
			read(fd, array1, BLOCK_SIZE);

			for (x = 0; x < 256; x++)
			{
				if (array1[x] < 2)
				{
					finish = 1;
					break;
				}

				lseek(fd, array1[x] * BLOCK_SIZE, 0);
				read(fd, array2, BLOCK_SIZE);

				for (y = 0; y < 256; y++)
				{
					if (array2[y] < 2)
					{
						finish = 1;
						break;
					}
					lseek(fd, array2[y] * BLOCK_SIZE, 0);
					read(fd, array3, BLOCK_SIZE);

					for (z = 0; z < 256; z++)
					{
						if (array3[z] < 2)
						{
							finish = 1;
							break;
						}
						lseek(fd, array3[z] * BLOCK_SIZE, 0);
						ReadBytes = read(fd, ReadBuffer, BLOCK_SIZE);

						int written = 0;
						if ((written = write(sfd, ReadBuffer, BLOCK_SIZE)) < 1)
						{
							printf("written %d", written);
						}

						if (ReadBytes < 1)
						{
							finish = 1;
							break;
						}
					}
					if (finish)
						break;
				}
				if (finish)
					break;
			}
		}

		close(sfd);
	}

}

int main()
{
	unsigned int number_of_blocks = 0, number_of_inodes = 0;
	char *partialString;
	int fsinit = 0/*, i = 0*/;
	char input[1024];

	//unsigned short n = 0;
	//char dirbuf[BLOCK_SIZE];
	unsigned short bytes_written;


	printf("\nThis program accepts the following commands with specified parameters\n");
	printf("1. initfs <file_name_to_represent_disk> <total_no_of_blocks> <total_no_of_inodes>\n");
	printf("2. q\n");
	printf("3. cpin <external_file_name> <file_system_file_name>\n");
	printf("4. cout <file_system_file_name> <external_file_name>\n");
	printf("5. mkdir <file_system_file_name>\n");
	printf("6. rm <file_system_file_name>\n");

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

			if (access(filepath, 0) != -1)
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
		else if (strcmp(partialString, "cpin") == 0)
		{
			//check if file system is initialised
			if (fsinit == 0)
			{
				printf("\n The file system has not been initialised yet. Please Initialise before trying out the other operations \n");
				return 0;
			}

			char *source;
			char *dest;

			//populate the expected fields from the command entered
			source = strtok(NULL, " ");
			dest = strtok(NULL, " ");

			if (!source || !dest)
			{
				printf("\n Source path and destination path are not entered \n");
			}
			else
			{
				cpin(source, dest);
			}
			partialString = NULL;
		}
		else if (strcmp(partialString, "cpout") == 0)
		{
			//check if file system is initialised
			if (fsinit == 0)
			{
				printf("\n The file system has not been initialised yet. Please Initialise before trying out the other operations \n");
				return 0;
			}
			char *source;
			char *dest;

			//populate the expected fields from the command entered
			source = strtok(NULL, " ");
			dest = strtok(NULL, " ");

			if (!source || !dest)
			{
				printf("\n Source path and destination path are not entered \n");
			}
			else
			{
				//CheckFileExists(source);
				cpout(source, dest);
			}

			partialString = NULL;
		}
		else if (strcmp(partialString, "mkdir") == 0)
		{
			//check if file system is initialised
			if (fsinit == 0)
			{
				printf("\n The file system has not been initialised yet. Please Initialise before trying out the other operations \n");
				return 0;
			}
			char *DirectoryName = strtok(NULL, " "); ;
			if (!DirectoryName)
			{
				printf("Enter a directory name for mkdir\n");
				return 0;
			}
			//get a free inode
			unsigned int NewInodeNumber = GetFreeInode();
			if (NewInodeNumber < 2)
			{
				printf("No free inodes remain\n");
				return 0;
			}

			CreateDirectory(DirectoryName, NewInodeNumber);

			partialString = NULL;
		}
		else if (strcmp(partialString, "rm") == 0)
		{
			//check if file system is initialised
			if (fsinit == 0)
			{
				printf("\n The file system has not been initialised yet. Please Initialise before trying out the other operations \n");
				return 0;
			}

			char *DirectoryName = strtok(NULL, " "); ;
			if (!DirectoryName)
			{
				printf("Enter a directory name to remove\n");
				return 0;
			}

			RemoveDirectory(DirectoryName);
			partialString = NULL;
		}
		else
		{
			printf("\nInvalid command\n ");
		}
	}
	return 0;
}
