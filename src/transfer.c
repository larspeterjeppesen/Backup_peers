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


void listen_for_requests(char* port) {
  int listenfd;
  int connfd;
  struct sockaddr_storage clientaddr;
  socklen_t clientlen;
  io_assist_state_t state;
  char msg_buf[MAX_MSG_LEN];

  listenfd = io_assist_open_listenfd(port);
  clientlen = sizeof(struct sockaddr_storage);
  connfd = accept(listenfd, (struct sockaddr*) &clientaddr, &clientlen);
 
  io_assist_readinitb(&state, connfd);
  io_assist_readnb(&state, msg_buf, REQUEST_HEADER_LEN);

  uint32_t length = ntohl(*(uint32_t*)msg_buf);
  uint32_t command = ntohl(*(uint32_t*)(msg_buf+4));
  hashdata_t request_hash;
  memcpy(request_hash, msg_buf+8, SHA256_HASH_SIZE);

  if (command == CMD_INITIALIZE_TRANSFER) {
    char payload_buf[length+1];
    io_assist_readnb(&state, msg_buf, length); 

    memcpy(payload_buf, msg_buf, length);
    hashdata_t payload_hash;
    get_data_sha(payload_buf, payload_hash, length);

    if (cmp_hash(payload_hash, request_hash) == 0) {
      fprintf(stderr, "Corrupted request received\n");
      close(connfd);
      return;   
    }

    Transfer_Metadata_t metadata;
    metadata.path_len = ntohl(*(uint32_t*)payload_buf);
    metadata.block_count = ntohl(*(uint32_t*)(payload_buf+4));
    metadata.file_size = be64toh(*(uint64_t*)(payload_buf+8));
    memcpy(metadata.total_hash, payload_buf+16, SHA256_HASH_SIZE);
    strcpy(metadata.file_path, payload_buf+16+SHA256_HASH_SIZE);
    fprintf(stdout, "Request to transfer file: %s\nTotal size: %ld\nBlock count: %d\n", metadata.file_path, metadata.file_size,  metadata.block_count);
    //Add check to see if file exists already
    receive_file(connfd, command, &metadata);
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


//Called by the host that wants to receive files
void receive_file(int connfd, uint32_t command, void* received_metadata) {
  if (command == CMD_INITIALIZE_TRANSFER) {
    char msg_buf[MAX_MSG_LEN];
    char payload_buf[TRANSFER_PAYLOAD_LEN+1];
    io_assist_state_t state;
    io_assist_readinitb(&state, connfd); 

    //Accept incoming transfer
    Response_t response;
    response.status = htonl(OK);
    memcpy(msg_buf, &response, RESPONSE_LEN);
    io_assist_writen(connfd, msg_buf, RESPONSE_LEN);

    //Receive file
    Transfer_Metadata_t metadata = *(Transfer_Metadata_t*)received_metadata;
    char file_path[PATH_LEN];
    char* file_path_prefix = "downloads/";
    strcpy(file_path, file_path_prefix);
    strcpy(file_path+strlen(file_path_prefix), metadata.file_path);
    
    //Check all folders exist and create them if not
    init_dir_structure(file_path);

    printf("Storing file at: %s\n", file_path);
    int dest_fd = open(file_path, O_WRONLY | O_CREAT, S_IRUSR|S_IWUSR); 
    for (uint32_t i = 0; i < metadata.block_count; i++) {
      io_assist_readnb(&state, msg_buf, MAX_MSG_LEN);
      
      uint32_t length = ntohl(*(uint32_t*)msg_buf);
      uint32_t status = ntohl(*(uint32_t*)(msg_buf+4));
      uint32_t block_number = ntohl(*(uint32_t*)(msg_buf+8));
      hashdata_t block_hash;
      memcpy(block_hash, msg_buf+12, SHA256_HASH_SIZE);
      memcpy(payload_buf, msg_buf+TRANSFER_HEADER_LEN, length);
      payload_buf[length] = 0;

      hashdata_t payload_hash;
      get_data_sha(payload_buf, payload_hash, length);
      
      if (cmp_hash(payload_hash, block_hash) == 0) {
        fprintf(stderr, "Corrupted payload received at block %d\n", i);
        break;
      }
     
      write(dest_fd, payload_buf, length);
    }
    close(connfd);
    close(dest_fd);
    
    hashdata_t file_hash;
    get_file_sha(file_path, file_hash, metadata.file_size);
    if (cmp_hash(file_hash, metadata.total_hash) == 0) {
      fprintf(stderr, "file %s corrupted after transfer\n", metadata.file_path);
      
    }

  printf("File received\n");
  }
  return;
}

//Called by the host that wants to transfer files
void transfer_file(PeerAddress_t peer_address, char* file_path) {
  Request_t request;
  Transfer_Metadata_t metadata;
  memset(metadata.file_path, 0, PATH_LEN);
  printf("%s\n", file_path);
  int fd = open(file_path, O_RDONLY);
  assert(fd != -1);
  struct stat statbuf = {0};
  fstat(fd, &statbuf);
  uint64_t file_size = statbuf.st_size;
  close(fd);
 
  //Build transfer_metadata
  uint32_t block_count = ceil((double)file_size/(double)(TRANSFER_PAYLOAD_LEN));
  metadata.path_len = htonl((uint32_t)strlen(file_path));
  metadata.block_count = htonl(block_count);
  metadata.file_size = htobe64(file_size); 
  get_file_sha(file_path, metadata.total_hash, file_size); 
  strcpy(metadata.file_path, file_path);
 
  //Build transfer_request header
  uint32_t length = TRANSFER_METADATA_LEN+strlen(file_path)+1; 
  request.header.length = htonl(length);
  request.header.command = htonl((uint32_t)CMD_INITIALIZE_TRANSFER);
  get_data_sha((void*)&metadata, request.header.payload_hash, length);

  //Assemble request
  memcpy(request.payload, &metadata, length);
  io_assist_state_t state;
  char read_buf[TRANSFER_PAYLOAD_LEN+1];
  char msg_buf[MAX_MSG_LEN];  
  memcpy(msg_buf, &request, REQUEST_HEADER_LEN + length);
  
  //Connect to peer and send request
  int connfd = io_assist_open_clientfd(peer_address.ip, peer_address.port);
  io_assist_readinitb(&state, connfd); 
  io_assist_writen(connfd, msg_buf, REQUEST_HEADER_LEN + length);
  
  //Receive response
  ssize_t n = io_assist_readnb(&state, read_buf, RESPONSE_LEN);
  if (n < 0) {
    fprintf(stderr, "Error when reading from socket: %s\n", strerror(errno));
    close(connfd);
    return;
  }
  if (n != RESPONSE_LEN) {
    fprintf(stderr, 
            "Incorrect number of bytes read from socket. Expected %d, read %ld.\n", RESPONSE_LEN, n);
  }

  uint32_t status = ntohl(*(uint32_t*)read_buf);
  if (status != OK) {
    fprintf(stderr, "Transfer initialization failed with status %d\n", status);
    close(connfd);
    return;
  }

  //Transfer actual file
  FILE* fp = fopen(file_path, "r");
  assert(fp);
  for (uint32_t i = 0; i < block_count; i++) {
    printf("Block %d\n", i);
    //Read data to send
    size_t n = fread(read_buf, sizeof(char), TRANSFER_PAYLOAD_LEN, fp); 
    read_buf[n] = 0;
   
    //Build message
    TransferHeader_t header;
    header.length = htonl(n);
    header.status = htonl(OK);
    header.block_number = htonl(i);
    get_data_sha(read_buf, header.block_hash, n);

    //Send message
    memcpy(msg_buf, &header, TRANSFER_HEADER_LEN);
    memcpy(msg_buf+TRANSFER_HEADER_LEN, read_buf, n);
    io_assist_writen(connfd, msg_buf, TRANSFER_HEADER_LEN+n);
  }
  
  fclose(fp);
  close(connfd);
  return;
}
