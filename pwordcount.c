// @author: Michael Blakley
// @email: <mzb0155@auburn.edu>
// @course: CS7500 - Adv Op Systems
// Project 2: A Pipe-based WordCount Tool

// compile with gcc -o pwordcount pwordcount.c
// run with ./pwordcount <file_name.txt>

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#define BUFFER_SIZE 1000
#define FILE_SIZE_LIMIT 50000
#define READ_END	0
#define WRITE_END	1

char write_msg[FILE_SIZE_LIMIT];
char read_msg[FILE_SIZE_LIMIT];
int file_size;
int totalWordCount;

int loadFile(int argc, const char *argv[]) {;
  
  // check for filename
  if (argc != 2) {
    printf("Please enter a file name.\n");
    printf("Usage: %s, <file_name>\n", argv[0]);
    return 1;
  }

  // check for proper file extension
  const char * namestr = argv[1];
  const char *ext = strchr(namestr, '.');
 
  if (strcmp(ext, ".txt") != 0) {
    printf("File format is incorrect, please use .txt file.\n");
    return 1;
  }

  // open file
  FILE *file = fopen(argv[1], "r");

  // check if file can open
  if (file == 0) {
    printf("Could not open file: %s.\n", argv[1]);
    return 1;
  }

  fseek(file, 0, SEEK_END);
  file_size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (file_size > FILE_SIZE_LIMIT) {
    printf("File size is too large.\n");
    return 1;
  }

  if (file_size == 0) {
    printf("File is empty, no words to count.\n");
    return 1;
  }

  printf("Process 1 is reading file %s now...", argv[1]);
  char temp[BUFFER_SIZE];

  while(fgets(temp, BUFFER_SIZE, file)) {
    printf(".");
    strcat(write_msg, temp);
  }
  printf("\n\n");

  fclose(file);

  return 0;
}

int wordCounter(char *text_string){
  int i, count;

  for(i = 0; i < strlen(text_string); i++) {
    if (text_string[i] == ' ' || text_string[i] == '\n') {
      count++;
    }
  }

  return count;
} 

void write_to_p2(int *fd_pipe1) {
  // send text file contents through pipe1 to p2 for counting

  if (file_size >= BUFFER_SIZE) {
    int xfers = file_size / BUFFER_SIZE;
    char msg_chunk[BUFFER_SIZE-1];
    int i;
    
    for (i = 0; i <= xfers; i ++) {
      strncpy(msg_chunk, write_msg+(BUFFER_SIZE*i), BUFFER_SIZE);
      write(fd_pipe1[WRITE_END], msg_chunk, BUFFER_SIZE);
    }
  } else {
    write(fd_pipe1[WRITE_END], write_msg, strlen(write_msg)+1);
  }
}

void read_from_p2(int *fd_pipe2) {
  read(fd_pipe2[READ_END], &totalWordCount, INT_MAX);
}

void read_from_p1(int *fd_pipe1) {
  // read message from pipe1
  char temp[BUFFER_SIZE];
  while (read(fd_pipe1[READ_END], temp, BUFFER_SIZE) > 0) {
    strcat(read_msg, temp);
  }
}

void write_to_p1(int *fd_pipe2, int *count) {
    write(fd_pipe2[WRITE_END], count, INT_MAX);
}

int main(int argc, char const *argv[]) {
  
  pid_t pid;
	int fd_pipe1[2];
  int fd_pipe2[2];

  // error check and load file
  if (loadFile(argc, argv) != 0) {
    return 1;
  }

  //create pipes
  if (pipe(fd_pipe1) == -1 || pipe(fd_pipe2) == -1) {
    fprintf(stderr, "A Pipe Creation Failed");
    return -1;
  }

  // create p1 and p2 
  pid = fork();

  if (pid < 0) {
    fprintf(stderr, "Fork failed!\n");
    return 1;
  }

  if (pid > 0) { // parent process
    close(fd_pipe1[READ_END]);
    close(fd_pipe2[WRITE_END]);

    printf("Process 1 sending data to Process 2...\n\n");

    write_to_p2(fd_pipe1);
    
    close(fd_pipe1[WRITE_END]);

    read_from_p2(fd_pipe2);

    printf("Process 1:  The total number of words is %d.\n\n", totalWordCount);
  }

  else {  //child process
    int count;

    close(fd_pipe1[WRITE_END]);
    close(fd_pipe2[READ_END]);

    read_from_p1(fd_pipe1);
    
    printf("Process 2 finished receiving data from Process 1...\n\n");
    
    printf("Process 2 counting words...\n\n");
    count = wordCounter(read_msg);

    printf("Process 2 sending back results to Process 1....\n\n");
    write_to_p1(fd_pipe2, &count);

    close(fd_pipe1[READ_END]);
  }

  return 0;
}