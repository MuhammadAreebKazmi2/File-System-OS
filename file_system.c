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
int current_block = 3;                // starts allocating from block 3

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
// create file function CR
int CR(char* filename, int size) {

  // Checking if file already exists
  for (int i = 3; i < NUM_BLOCKS; i++) {
    inode* fnode = (inode*)myfs[i];
    if (fnode->used && cmp(fnode->name, filename) == 0) {
      printf("Error: The file '%s' already exists.\n", filename );
      return -1;
    }
  }

  // Checking if directory exists already
  char path[strlen(filename)+1];
  strcpy(path, filename);
  char *dir = dirname(path);
  bool dir_exists = false;

  for (int i = 3; i < NUM_BLOCKS; i++) {
    inode* fnode = (inode*)myfs[i];
    if (fnode->used && strcmp(fnode->name, dir) == 0) {
      dir_exists = true;
      break;
    }
  }

  if (!dir_exists) {
    printf("Error: The directory '%s' in the given path does not exist.\n", dir);
    return -1;
  }

  // Calculating the number of blocks needed
  int num_blocks_needed = (size + BLOCK_SIZE -1) / BLOCK_SIZE;

  // Checking if it exceeds the limit
  if (current_block + num_blocks_needed > NUM_BLOCKS) {
    printf("Error: Not enough space available.\n");
    return -1;
  }

  // Since all the checks now are done
  // Creating the file
  inode new_inode;
  new_inode.dir = 0; // Not a directory
  strcpy(new_inode.name, filename);
  new_inode.size = size;
  new_inode.used = 1; // It is in use
  new_inode.rsvd = 0; // Not reserved for future use  


  // Allocate data blocks and fill them with small alphabets
  for (int i = 0; i < num_blocks_needed; i++) {
      new_inode.blockptrs[i] = current_block++;
      char data[BLOCK_SIZE];
      for (int j = 0; j < BLOCK_SIZE; j++) {
          data[j] = 'a' + (i % 26); // Fill with small alphabets [a-z]
      }
      memcpy(myfs[new_inode.blockptrs[i]], data, BLOCK_SIZE);
  }

  // Write the new inode to the file system
  memcpy(myfs[current_block++], &new_inode, sizeof(inode));
  return 0;
}


// copy file
// remove/delete file
// move a file
// list file info
// create directory
// remove a directory


// Function to write the hard disk state to the "myfs_f" file
void writeHardDiskState() {
  int myfs_f = open("myfs", O_CREAT | O_RDWR, 0666);
    if (myfs_f == -1) {
        printf("Error: Cannot create file system myfs");
        exit(1);
    }

    for (int i = 0; i < NUM_BLOCKS; i++) {
        lseek(myfs_f, i * BLOCK_SIZE, SEEK_SET);
        write(myfs_f, myfs[i], BLOCK_SIZE);
    }

    close(myfs_f);  
}


/*
 * main
 */
int main (int argc, char* argv[]) {

  myfs_f = open("./myfs", O_RDWR);
  if (myfs_f == -1) {
    myfs_f = mem_init();
  }

  FILE *input_file = fopen("sample.txt", "r");
  if (input_file == NULL) {
    printf("Error opening file.\n");
    exit(1);
  }

  char command[1000];

  // while not EOF
  while (fgets(command, sizeof(command), input_file) != NULL) {
    
    // removing the next line character
    size_t len = strlen(command);
    if (len > 0 && command[len-1] == '\n') {
      command[len-1] = '\0';    // replaced with NULL character
    }

    // read command by parsing it
    char *token = strtok(command, " ");

    if (token != NULL) {
      if (strcmp(token, "CR") == 0) {
        // Parse and execute the CR command
        char* filename = strtok(NULL, " ");
        int size = atoi(strtok(NULL, " "));
        CR(filename, size);                 // CR function callled
        writeHardDiskState();               // Written to hard disk
      }
    }
  }
    
    
    
    // call appropriate function
  close(myfs_f);
	return 0;
}
