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

// Function to update the parent directory's block with the new directory entry
void updateParentDirectory(const char* parent_dirname, int new_directory_inode) {
  // Find the parent directory's inode
  inode* parent_inode = NULL;
  for (int i = 3; i < NUM_BLOCKS; i++) {
    inode* fnode = (inode*)myfs[i];
    if (fnode->used && strcmp(fnode->name, parent_dirname) == 0) {
        parent_inode = fnode;
          break;
      }
  }

// Check if the parent directory's fnode was found
  if (parent_inode == NULL) {
      printf("Error: Parent directory '%s' not found.\n", parent_dirname);
      return;
  }

  // Find the parent directory's data block
  int parent_block_index = parent_inode->blockptrs[0];
  char* parent_block = myfs[parent_block_index];

  // Create a new directory entry for the new directory
  dirent new_entry;
  strcpy(new_entry.name, "newdir"); // Replace with the actual directory name
  new_entry.namelen = strlen(new_entry.name);
  new_entry.inode = new_directory_inode;

  // Append the new entry to the parent directory's block
  int num_entries = parent_inode->size / sizeof(dirent);
  memcpy(&parent_block[num_entries * sizeof(dirent)], &new_entry, sizeof(struct Dirent));

  // Update the parent directory's size to reflect the new entry
  parent_inode->size += sizeof(dirent);

  // Save the changes to the parent directory's block
  memcpy(myfs[parent_block_index], parent_block, BLOCK_SIZE);
}



// create directory
int CD(const char* dir_name) {
  
  // Check if the directory already exists
  for (int i = 3; i < NUM_BLOCKS; i++) {
      inode* fnode = (inode*)myfs[i];
      if (fnode->used && strcmp(fnode->name, dir_name) == 0) {
          printf("Error: The directory '%s' already exists.\n", dir_name);
          return -1;
      }
  }

  // Extract the parent directory path
  char path[strlen(dir_name) + 1];
  strcpy(path, dir_name);
  char* parent_dirname = dirname(path);

  // Check if the parent directory exists
  bool parent_dir_exists = false;
  int parent_inode_index = -1;
  for (int i = 3; i < NUM_BLOCKS; i++) {
      inode* fnode = (inode*)myfs[i];
      if (fnode->used && strcmp(fnode->name, parent_dirname) == 0) {
          parent_dir_exists = true;
          break;
      }
  }

  if (!parent_dir_exists) {
      printf("Error: The directory '%s' in the given path does not exist.\n", parent_dirname);
      return -1;
  }

  // Create the new directory
  inode new_inode;
  new_inode.dir = 1; // It is a directory
  strcpy(new_inode.name, dir_name);
  new_inode.size = 0; // Directory size is initially 0
  new_inode.used = 1; // It is in use
  new_inode.rsvd = 0; // Not reserved for future use

  // Allocate a data block for the directory
  new_inode.blockptrs[0] = current_block++;

  // Initialize the directory block 
  dirent dir_entries[2];
  strcpy(dir_entries[0].name, ".");  // Entry for the directory itself
  dir_entries[0].namelen = 1;
  dir_entries[0].inode = current_block;
  
  strcpy(dir_entries[1].name, ".."); // Entry for the parent directory
  dir_entries[1].namelen = strlen(parent_dirname);
  dir_entries[1].inode = parent_inode_index;

  // Write the new inode to the file system
  memcpy(myfs[current_block], &new_inode, sizeof(inode));
  current_block++;

  // Write the directory block to the file system
  memcpy(myfs[current_block], dir_entries, sizeof(dir_entries));
  current_block++;

  // Write the new inode to the file system
  memcpy(myfs[current_block++], &new_inode, sizeof(inode));

  // Update the parent directory's block with the new directory entry
  updateParentDirectory(parent_dirname, new_inode.blockptrs );
  return 0;
}

// copy file
int CP(char* src_name, char* dest_name) {

  return 0;
}

// remove/delete file
int DL(char* filename) {

  return 0;
}

// move a file
int MV(char* src_name, char* dest_name) {

  return 0;
}

// list file info
void LL () {

}

// remove a directory
int DD(char* dirname) {

  return 0;
}


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
      else if (strcmp(token, "CD") == 0) {
        char* dirname = strtok(NULL, " ");
        CD(dirname);

        // updating the disk state
        writeHardDiskState();
      }
      else if (strcmp(token, "DL") == 0) {
        // parse and execute the DL command
        char* filename = strtok(NULL, " ");
        DL(filename);
        writeHardDiskState();
      } 
      else if (strcmp(token, "CP") == 0) {
        // parse and execute the CP command
        char* src_name = strtok(NULL, " ");
        char* dest_name = strtok(NULL, " ");
        CP(src_name, dest_name);
        writeHardDiskState();        
      }
      else if (strcmp(token, "MV") == 0) {
        // parse and execute the MV command
        char* src_name = strtok(NULL, " ");
        char* dest_name = strtok(NULL, " ");
        MV(src_name, dest_name);
        writeHardDiskState();          
      } 
      else if (strcmp(token, "DD") == 0) {
        // parse and execute the DD command
        char* dir_name = strtok(NULL, " ");
        DD(dir_name);
        writeHardDiskState();
      }
      else if (strcmp(token, "LL") == 0) {
        LL();
      }
    }
  }
    
    
    
  close(myfs_f);
  fclose(input_file);
	return 0;
}
