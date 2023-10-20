#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "sha256.h"
#include "compsys_helpers.h"
#include "client.h"


#if defined(__clang__)
  u_int32_t htonl(u_int32_t x) {
  #if BYTE_ORDER == LITTLE_ENDIAN
    unsigned char *s = (unsigned char *)&x;
    return (u_int32_t)(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
  }
  #else
    return x;
  #endif

u_int32_t ntohl(u_int32_t x) {
  #if BYTE_ORDER == LITTLE_ENDIAN
    unsigned char *s = (unsigned char *)&x;
    return (u_int32_t)(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
  }
  #else
    return x;
  #endif
#endif



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

//man 7 ip

//Initialization:
//  Pi sends request to start transfer
//  Desktop sends response to begin transfer
//  Pi sends response(s) with file parts as payload

//Transfer
//  Pi begins sending the file in partitioned messages
//  Desktop receives the partitions and writes them to disk
//  If a block is corrupted, desktop asks for it to be sent again

//Finalization
//  Pi says that the file transfer is done
//  Desktop informs that the transfer is successful
//  Connection is closed
//  Pi registers that the file has been transferred


//Step 1: Establish connection between pi and desktop, and transfer a small file.

void listen_for_conn(char* port) {
  int listenfd;
  int connfd;
  struct sockaddr_storage clientaddr;
  socklen_t clientlen;
  compsys_helper_state_t state;
  char msg_buf[MAX_MSG_LEN];

  listenfd = compsys_helper_open_listenfd(port);
  clientlen = sizeof(struct sockaddr_storage);
  connfd = accept(listenfd, (struct sockaddr*) &clientaddr, &clientlen);
 
  compsys_helper_readinitb(&state, connfd);
  compsys_helper_readnb(&state, msg_buf, REQUEST_HEADER_LEN);

  uint32_t length = ntohl(*(uint32_t*)msg_buf);
  uint32_t command = ntohl(*(uint32_t*)(msg_buf+4));
  hashdata_t request_hash;
  memcpy(request_hash, msg_buf+8, SHA256_HASH_SIZE);

  if (command == CMD_INITIALIZE_TRANSFER) {
    char payload_buf[length+1];
    compsys_helper_readnb(&state, msg_buf, length); 

    memcpy(payload_buf, msg_buf, length);
    //payload_buf[length] = 0;
    printf("length %d\n", length);
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
    metadata.file_size = ntohl(*(uint32_t*)(payload_buf+8));
    memcpy(metadata.total_hash, payload_buf+12, SHA256_HASH_SIZE);
    strcpy(metadata.file_path, payload_buf+12+SHA256_HASH_SIZE);
    printf("%d %d %d %s\n", metadata.path_len, metadata.block_count, metadata.file_size, metadata.file_path);
    //Add check to see if file exists already
    receive_file(connfd, command, &metadata);
  }

  return;
}



//Called by the host that wants to receive files
void receive_file(int connfd, uint32_t command, void* received_metadata) {
  printf("receiving file\n");
  if (command == CMD_INITIALIZE_TRANSFER) {
    char msg_buf[MAX_MSG_LEN];
    char payload_buf[TRANSFER_PAYLOAD_LEN+1];
    compsys_helper_state_t state;
    compsys_helper_readinitb(&state, connfd); 

    //Accept incoming transfer
    Response_t response;
    response.status = htonl(OK);
    memcpy(msg_buf, &response, RESPONSE_LEN);
    compsys_helper_writen(connfd, msg_buf, RESPONSE_LEN);

    //Receive file
    Transfer_Metadata_t metadata = *(Transfer_Metadata_t*)received_metadata;
    int dest_fd = open(metadata.file_path, O_WRONLY | O_CREAT, S_IRUSR|S_IWUSR); 
    for (uint32_t i = 0; i < metadata.block_count; i++) {
      compsys_helper_readnb(&state, msg_buf, MAX_MSG_LEN);
      
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
    get_file_sha(metadata.file_path, file_hash, metadata.file_size);
    if (cmp_hash(file_hash, metadata.total_hash) == 0) {
      fprintf(stderr, "file %s corrupted after transfer\n", metadata.file_path);
      
    }

  printf("File received\n");
  }
  return;
}

//Called by the host that wants to transfer files
void transfer_file(PeerAddress_t peer_address, char* file_path) {
  //Build request
  Request_t request;
  Transfer_Metadata_t metadata;
  memset(metadata.file_path, 0, PATH_LEN);
  FILE* fp = fopen(file_path, "r");
  fseek(fp, 0, SEEK_END);
  uint32_t file_size = ftell(fp);
  fclose(fp);

  uint32_t block_count = ceil((double)file_size/(double)(TRANSFER_PAYLOAD_LEN));
  metadata.path_len = htonl((uint32_t)strlen(file_path));
  metadata.block_count = htonl(block_count);
  metadata.file_size = htonl(file_size); 
  get_file_sha(file_path, metadata.total_hash, file_size); 
  strcpy(metadata.file_path, file_path);

  uint32_t length = TRANSFER_HEADER_LEN+strlen(file_path)+1;
  printf("length %d\n", length);
  request.header.length = htonl(length);
  request.header.command = htonl((uint32_t)CMD_INITIALIZE_TRANSFER);
  get_data_sha((void*)&metadata, request.header.payload_hash, length);

  memcpy(request.payload, &metadata, length);
  compsys_helper_state_t state;
  char read_buf[TRANSFER_PAYLOAD_LEN+1];
  char msg_buf[MAX_MSG_LEN];  
  memcpy(msg_buf, &request, REQUEST_HEADER_LEN + length);
  //Connect to peer
  int connfd = compsys_helper_open_clientfd(peer_address.ip, peer_address.port);
  compsys_helper_readinitb(&state, connfd);
  compsys_helper_writen(connfd, msg_buf, REQUEST_HEADER_LEN + length);
 
  //Receive cornfirmation
  compsys_helper_readnb(&state, read_buf, RESPONSE_LEN);
  uint32_t status = ntohl(*(uint32_t*)read_buf);
  if (status != OK) {
    fprintf(stderr, "Transfer initialization failed with status %d\n", status);
    close(connfd);
    return;
  }

  //Transfer actual file
  fp = fopen(file_path, "r");
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
    compsys_helper_writen(connfd, msg_buf, TRANSFER_HEADER_LEN+n);
  }
  
  fclose(fp);
  close(connfd);
  printf("File sent\n"); 
  return;
}


int main() { 
  
  //Load configuration 
  char input[256];
  char input2[256];
  PeerAddress_t peer_address = {0};
  
  FILE* f = fopen("config", "r");
  while (peer_address.ip[0] == 0 || peer_address.port[0] == 0) {
    fscanf(f, "%s %s", input, input2);
    if (strcmp(input, "desktop_ip:") == 0) {
      strcpy(peer_address.ip, input2); 
    }
    if (strcmp(input, "desktop_port:") == 0) {
      strcpy(peer_address.port, input2); 
    }
  }

  printf("Enter mode:\n");  
  scanf("%s", input);

  if (strcmp(input, "sender")==0) {
    printf("Sending file to %s:%s\n", peer_address.ip, peer_address.port);
    transfer_file(peer_address, "files/file.txt");
  } 
  if (strcmp(input, "receiver")==0) {
    printf("Running as receiver\n");
    listen_for_conn(peer_address.port); 
  }


  return 0;

}


