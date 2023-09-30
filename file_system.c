// including C libraries 
#include <stdio.h>
#include <stdlib.h>
// additional libraries added go below:
#include <string.h> 	// for string handling
#include <stdbool.h> 	// for implementing boolean values in true sense of modern programming. Had issues in lab4 previously working with the C language boolean variables
#include <fcntl.h> 	// for file open and close control
#include <unistd.h>	// for various system calls

/*
 *   ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ ___ 
 *  |   |   |   |   |                       |   |
 *  | 0 | 1 | 2 | 3 |     .....             |127|
 *  |___|___|___|___|_______________________|___|
 *  |   \    <-----  data blocks ------>
 *  |     \
 *  |       \
 *  |         \
 *  |           \
 *  |             \
 *  |               \
 *  |                 \
 *  |                   \
 *  |                     \
 *  |                       \
 *  |                         \
 *  |                           \
 *  |                             \
 *  |                               \
 *  |                                 \
 *  |                                   \
 *  |                                     \
 *  |                                       \
 *  |                                         \
 *  |                                           \
 *  |     <--- super block --->                   \
 *  |______________________________________________|
 *  |               |      |      |        |       |
 *  |        free   |      |      |        |       |
 *  |       block   |inode0|inode1|   .... |inode15|
 *  |        list   |      |      |        |       |
 *  |_______________|______|______|________|_______|
 *
 *
 */

#define BLOCK_SIZE 1024 	// each block will be 1KB
#define NUM_BLOCKS 128		// 128 blocks of memory
#define NUM_INODES 16		// maximum number of files

#define FILENAME_MAXLEN 8  // including the NULL char

/* 
 * inode 
 */

typedef struct inode {
  int  dir;  // boolean value. 1 if it's a directory.
  char name[FILENAME_MAXLEN];
  int  size;  // actual file/directory size in bytes.
  int  blockptrs [8];  // direct pointers to blocks containing file's content.
  int  used;  // boolean value. 1 if the entry is in use.
  int  rsvd;  // reserved for future use
} inode;


/* 
 * directory entry
 */

typedef struct dirent {
  char name[FILENAME_MAXLEN];
  int  namelen;  // length of entry name
  int  inode;  // this entry inode index
} dirent;

char myfs[NUM_BLOCKS][BLOCK_SIZE];		// my file system root
int myfs_f;

// Creating a function that initializes the memory

int mem_init() {

  // Initializing the Super Block
  char fs_dbm_data[2] = {'A', 1};

  // writes 'A' and 1 to the first 2 bytes of myfs
  memcpy(myfs[0], fs_dbm_data, 2);  // from fs_dbm_data to myfs[0]

  // Initializing the Root Inode
  inode root_inode;
  root_inode.dir = 1;            // Root inode is a directory true
  strcpy(root_inode.name, "/");  // Root directory names as "/"
  root_inode.size = sizeof(struct dirent);
  root_inode.blockptrs[0] = 1;   
  root_inode.used = 1;          // It is in use
  root_inode.rsvd = 0;          // not reserved for future use

  // storing the root inode in block 1
  memcpy(myfs[1], &root_inode, sizeof(struct inode));

  // Initializing the root directory entry
  struct dirent root_dirent;
  strcpy(root_dirent.name, ".");
  root_dirent.namelen = 1;
  root_dirent.inode = 1;        // will point to the root inode in block 1

  // storing the root dirent in block 2
  memcpy(myfs[2], &root_dirent, sizeof(struct dirent));

  char Data[BLOCK_SIZE];
  memset(Data, 0, BLOCK_SIZE);  // filling the single data block with NULL characters
  
  // writing the zeroes to the data blocks from 3 to 127.
  for (int i = 3; i < NUM_BLOCKS; i++) {
    memcpy(myfs[i], Data, BLOCK_SIZE);
  }

  // Simulating writing the myfs data to a file 
  int myfs_fd = open("myfs", O_CREAT | O_RDWR, 0666);

  // error checking
  if (myfs_fd == -1) {
    fprintf(stderr, "Error: Cannot create file system myfs\n");
    return -1;        // returned with error code -1
  }

  // writing data of the blocks to the file
  for (int i = 0; i < NUM_BLOCKS; i++) {
    write(myfs_fd, myfs[i], BLOCK_SIZE);
  }

  return myfs_fd;
}
/*
 * functions
 */
// create file
// copy file
// remove/delete file
// move a file
// list file info
// create directory
// remove a directory




/*
 * main
 */
int main (int argc, char* argv[]) {

  myfs_f = open("./myfs", O_RDWR);
  if (myfs_f == -1) {
    myfs_f = mem_init();
  }



  // while not EOF
    // read command
    
    // parse command
    
    // call appropriate function
  close(myfs_f);
	return 0;
}
