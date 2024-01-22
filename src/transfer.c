#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "transfer.h"
#include "sha256.h"
#include "io_assist.h"

int t_verbose = 1;

/*
 * Gets a sha256 hash of specified data, sourcedata. The hash itself is
 * placed into the given variable 'hash'. Any size can be created, but a
 * a normal size for the hash would be given by the global variable
 * 'SHA256_HASH_SIZE', that has been defined in sha256.h
 */
void get_data_sha(const char* sourcedata, hashdata_t hash, uint32_t data_size)
{
    int hash_size = SHA256_HASH_SIZE;
    SHA256_CTX shactx;
    unsigned char shabuffer[hash_size];
    sha256_init(&shactx);
    sha256_update(&shactx, sourcedata, data_size);
    sha256_final(&shactx, shabuffer);

    for (int i=0; i<hash_size; i++)
    {
        hash[i] = shabuffer[i];
    }
}

/*
 * Gets a sha256 hash of specified data file, sourcefile. The hash itself is
 * placed into the given variable 'hash'. Any size can be created, but a
 * a normal size for the hash would be given by the global variable
 * 'SHA256_HASH_SIZE', that has been defined in sha256.h
 */


void get_file_sha(const char* sourcefile, hashdata_t hash, long file_size)
{
    FILE* fp = fopen(sourcefile, "rb");
    if (fp == 0)
    {
        printf("Failed to open source: %s\n", sourcefile);
        return;
    }

    //char buffer[file_size];
    char* buffer = malloc(sizeof(char) * file_size);
    fread(buffer, file_size, 1, fp);
    fclose(fp);
    
    get_data_sha(buffer, hash, file_size);
}

int cmp_hash(hashdata_t a, hashdata_t b) {
  int hash_size = SHA256_HASH_SIZE;
  for (int i = 0; i < hash_size; i++) {
    if (a[i] != b[i]) {
      return 0;
    }
  }
  return 1;
}

void listen_for_connection(char* port) {
  int listenfd;
  int connfd;
  struct sockaddr_storage clientaddr;
  socklen_t clientlen;
  listenfd = io_assist_open_listenfd(port);
  clientlen = sizeof(struct sockaddr_storage);
  connfd = accept(listenfd, (struct sockaddr*) &clientaddr, &clientlen);
  receive_files(connfd);
  close(connfd);
  return;
}

void receive_files(int connfd) {
  char payload_buf[MAX_MSG_LEN];
  io_assist_state_t state;
  io_assist_readinitb(&state, connfd);
  ssize_t bytes_read;
 
  //printf("receiving files on connfd %d\n", connfd);


  char* file_path_prefix = "downloads/";
  //Receive files
  while(1) {  
    memset(payload_buf, 0, MAX_MSG_LEN);
    //Read and parse file data
    bytes_read = io_assist_readnb(&state, payload_buf, FILEDATA_HEADER_LEN);
    //printf("Read %ld bytes from expected FILEDATA_HEADER, expected %d bytes\n", bytes_read, FILEDATA_HEADER_LEN);
  
    int sum = 0;
    for (int i = 0; i < FILEDATA_HEADER_LEN; i++) {
      sum |= payload_buf[i]; 
    }     
    if (sum == 0) {
      printf("Received termination signal from sender\n"); 
      break;
    }

    //Parse received FileData header
    uint32_t path_len = ntohl(*(uint32_t*)payload_buf);
    uint32_t block_count = ntohl(*(uint32_t*)(payload_buf+4));
    uint64_t file_size = be64toh(*(uint64_t*)(payload_buf+8));
    hashdata_t file_hash;
    memcpy(file_hash, payload_buf+16, SHA256_HASH_SIZE);

    //Read and init destination path
    bytes_read = io_assist_readnb(&state, payload_buf, path_len);
    char file_path[PATH_LEN] = {0};
    strncpy(file_path, file_path_prefix, strlen(file_path_prefix));
    strncpy(file_path+strlen(file_path_prefix), payload_buf, path_len);
    init_dir_structure(file_path);

    if (t_verbose) fprintf(stdout, "Request to transfer file: %s\nTotal size: %ld\nBlock count: %d\n", file_path, file_size, block_count);

    //Check if file has been received already


    //File is new, accept it
    uint32_t response = htonl(ACCEPT_FILE);
    //printf("Sending response: %d\n", ACCEPT_FILE);
    char buffer[4];
    memcpy(buffer, &response, 4);
    io_assist_writen(connfd, buffer, RESPONSE_LEN);

    //Receive file
    if (t_verbose) printf("File transfer request accepted, storing at: %s\n", file_path);
    int dest_fd = open(file_path, O_WRONLY | O_CREAT, S_IRUSR|S_IWUSR);
    assert(dest_fd != -1);
    for (uint32_t i = 0; i < block_count; i++) {
      //Read and parse PackageData header
      io_assist_readnb(&state, payload_buf, PACKAGE_HEADER_LEN); 
      uint32_t payload_length = ntohl(*(uint32_t*)payload_buf);
      uint32_t block_number = ntohl(*(uint32_t*)(payload_buf+4));
      hashdata_t block_hash;
      memcpy(block_hash, payload_buf+8, SHA256_HASH_SIZE);

      //Read payload
      io_assist_readnb(&state, payload_buf, payload_length);

      //Verify payload
      hashdata_t payload_hash;
      get_data_sha(payload_buf, payload_hash, payload_length); 
      if (cmp_hash(payload_hash, block_hash) == 0) {
        fprintf(stderr, "Corrupted payload received at block, exiting %d\n", i);
        exit(1);
      }
      //Write package payload to disk
      write(dest_fd, payload_buf, payload_length);
    }
    
    //Verify file integrity
    hashdata_t actual_file_hash;
    get_file_sha(file_path, actual_file_hash, file_size);
    if (cmp_hash(actual_file_hash, file_hash) == 0) {
      //Report failed transfer
      fprintf(stderr, "file %s corrupted after transfer\n", file_path); 
      response = htonl(FILE_CORRUPTED);
      io_assist_writen(connfd, &response, 4);
      exit(0);
    }
    
    //Report succesfull transfer
    if (t_verbose) fprintf(stdout, "File correctly received\n");
    response = htonl(FILE_RECEIVED);
    io_assist_writen(connfd, &response, 4);
  }
  return;
}


void init_dir_structure(char* file_path) {
  char* fp = file_path;
  char folder_name[PATH_LEN];
  char c[2];
  char partial_path[PATH_LEN];
  memset(partial_path, 0, PATH_LEN);
  int val;
  struct stat statbuf = {0};
  //Extract each folder in file path and mk them if needed
  while((val = sscanf(fp, "%255[^/]%1[/]", folder_name, c)) == 2) { 
    if (val == -1) {
      fprintf(stderr, "Error while scanning file path: %s\n", strerror(errno));
      break;
    }
    //Assemble partial path
    strcpy(partial_path+strlen(partial_path), folder_name);
    partial_path[strlen(partial_path)] = '/';
    //Check if folder exists
    if (stat(partial_path, &statbuf) == -1) {
      val = mkdir(partial_path, 0777); // S_IRWXU | S_IRWXG | S_IRWXO);
    }
    if (val == -1) {
      fprintf(stderr, "Error when creating dir: %s\n", strerror(errno));
      break;
    } 
    fp += strlen(folder_name)+1; 
  }
  return;
}

void signal_terminate_connection(int connfd) {
  FileData_t file_data;
  memset(&file_data, 0, FILEDATA_HEADER_LEN);
  io_assist_writen(connfd, &file_data, FILEDATA_HEADER_LEN);
  close(connfd);
  return;
}

//Called by the host that wants to transfer files
uint32_t transfer_file(int connfd, char* file_path) {
  FileData_t file_data; 
  memset(file_data.file_path, 0, PATH_LEN);
  if (t_verbose) printf("Initializing transfer for file: %s\n", file_path);
  int fd = open(file_path, O_RDONLY);
  assert(fd != -1);
  struct stat statbuf = {0};
  fstat(fd, &statbuf);
  uint64_t file_size = statbuf.st_size;
  close(fd);
  //FileData header data
  file_data.path_len = htonl((uint32_t)strlen(file_path));
  uint32_t block_count = ceil((double)file_size/(double)(PACKAGE_PAYLOAD_LEN));
  file_data.block_count = htonl(block_count);
  file_data.file_size = htobe64(file_size); 
  get_file_sha(file_path, file_data.file_hash, file_size); 
  strcpy(file_data.file_path, file_path);
 
  char send_buf[MAX_MSG_LEN];
  memcpy(send_buf, &file_data.path_len, 4);
  memcpy(send_buf+4, &file_data.block_count, 4);
  memcpy(send_buf+8, &file_data.file_size, 8);
  memcpy(send_buf+16, file_data.file_hash, SHA256_HASH_SIZE);
  memcpy(send_buf+16+SHA256_HASH_SIZE, file_path, strlen(file_path));


  //Send FileData to receiver
  io_assist_writen(connfd, send_buf, FILEDATA_HEADER_LEN+strlen(file_path));
 
  //Receive response
  char response_buf[MAX_MSG_LEN];
  io_assist_state_t state;
  io_assist_readinitb(&state, connfd);  
  
  ssize_t n = io_assist_readnb(&state, response_buf, RESPONSE_LEN);

  uint32_t response = ntohl(*(uint32_t*)response_buf);
  if (response == REJECT_FILE) {
    fprintf(stdout, "File rejected, continuing...\n");
    return -1;
  } else if (response != ACCEPT_FILE) {
    fprintf(stdout, "Malformed response, exiting\n");
    exit(1);
  } else {
    if (t_verbose) fprintf(stdout, "Transfer request accepted, starting transfer\n");
  }

  //Transfer actual file
  FILE* fp = fopen(file_path, "r");
  char read_buf[PACKAGE_PAYLOAD_LEN];
  char msg_buf[MAX_MSG_LEN]; 
  for (uint32_t i = 0; i < block_count; i++) {
    //Flush buffer
    memset(read_buf, 0, PACKAGE_PAYLOAD_LEN);
    memset(msg_buf, 0, MAX_MSG_LEN);

    //Read data from file to send
    size_t n = fread(read_buf, sizeof(char), PACKAGE_PAYLOAD_LEN, fp); 
    //Build message
    PackageData_t package;
    package.length = htonl(n);
    package.block_number = htonl(i);
    get_data_sha(read_buf, package.block_hash, n);

    //Send message
    memcpy(msg_buf, &package, PACKAGE_HEADER_LEN);
    memcpy(msg_buf+PACKAGE_HEADER_LEN, read_buf, n);
    io_assist_writen(connfd, msg_buf, PACKAGE_HEADER_LEN+n);
  }
  fclose(fp);

  //Read response from receiver
  io_assist_readnb(&state, response_buf, RESPONSE_LEN);
  response = ntohl(*(uint32_t*)response_buf);
  if (t_verbose) fprintf(stdout, "File transfer succesful\n");
  return response;
}
