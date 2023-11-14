#include "sha256.h"


#if defined(__clang__)
#define IS_DIR 10
#else
#define IS_DIR DT_DIR
#endif


#define MAX_MSG_LEN 1024

#define RESPONSE_LEN 4

#define REQUEST_HEADER_LEN    40 
#define REQUEST_PAYLOAD_LEN   MAX_MSG_LEN - REQUEST_HEADER_LEN

#define PATH_LEN 256
#define TRANSFER_METADATA_LEN 48

#define TRANSFER_HEADER_LEN 44
#define TRANSFER_PAYLOAD_LEN MAX_MSG_LEN - TRANSFER_HEADER_LEN

#define IP_LEN 16
#define PORT_LEN 8




void receive_file(int connfd, uint32_t command, void* metadata);

typedef uint8_t hashdata_t[SHA256_HASH_SIZE];

typedef struct {
  uint32_t status;
} Response_t;

typedef struct {
  uint32_t path_len;
  uint32_t block_count;
  uint64_t file_size;
  hashdata_t total_hash;
  char file_path[PATH_LEN];
} Transfer_Metadata_t;

typedef struct {
  uint32_t length;
  uint32_t command;
  hashdata_t payload_hash;
} RequestHeader_t;

typedef struct {
  RequestHeader_t header;
  char payload[REQUEST_PAYLOAD_LEN]; //eg Transfer_Metadata_t
} Request_t;

typedef struct {
  uint32_t length;
  uint32_t status;
  uint32_t block_number;
  hashdata_t block_hash;
} TransferHeader_t;

typedef struct {
  TransferHeader_t transfer_header;
  char payload[TRANSFER_PAYLOAD_LEN];
} Transfer_t;

typedef enum {
  OK
} Enum_Status;

typedef enum {
  CMD_AUTHENTICATION,
  CMD_INITIALIZE_TRANSFER,
  CMD_FILE_TRANSFER,
  CMD_CLOSE_CONN
} Enum_Commands ;

//const char enum_strings[] = {
//  "AUTHENTICATION",
//  "INITIALIZE_TRANSFER",
//  "FILE_TRANSFER",
//  "CLOSE_CONN"
//};


typedef struct {
  char ip[IP_LEN];
  char port[PORT_LEN];
} PeerAddress_t;




