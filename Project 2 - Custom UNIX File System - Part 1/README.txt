////////////////////////////////////////////////////////////////////////////////////////////////

Project 2 - part 1

Modified V6 file system
----------------------------------

This project is a part of the course CS5348.001 Operating System Concepts offered during Fall 2018 at UTD.

The problem statement is to re-design the V6 file system to support files upto 4 GB in size with a block size of 1024 Bytes. All the fields of the Super Block are doubled and the free[] array is expanded so that the information stored in the super block is closer to 1024 Bytes.


Files
----------------------------------

fsaccess.c
contains the program to create a file system similar to the unix v6 file system. The program exposes two functions to the user - 'initfs' and 'q'.

Participants
--------------------------------------
axm180029 - Amith Kumar Matapady
sxb180020 - Swapnil Bansal
sxb180026 - Surajit Baitalik


Comments
--------------------------------------
The details of inode number 1 (the root directory) is stored in the data block number given by (2 + total no of inodes + 147).
initfs - accepts "file system name", "total no of blocks" and "total no of inodes" as input and creates a file system, initailizing root node and free blocks.
q - saves the changes and quits the program.


/////////////////////////////////////////////////////////////////////////////////////////////////