#include <unistd.h>

#include "sha256.h"

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

typedef uint8_t hashdata_t[SHA256_HASH_SIZE];
typedef struct {
  char ip[IP_LEN];
  char port[PORT_LEN];
} PeerAddress_t;

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


#if defined(__clang__)
u_int32_t htonl(u_int32_t x) {
  #if BYTE_ORDER == LITTLE_ENDIAN
    unsigned char *s = (unsigned char*)&x;
    return (u_int32_t)(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
  #else
    return x;
  #endif
}

u_int32_t ntohl(u_int32_t x) {
  #if BYTE_ORDER == LITTLE_ENDIAN
    unsigned char *s = (unsigned char *)&x;
    return (u_int32_t)(s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]);
  #else
    return x;
  #endif
}

u_int64_t be64toh(u_int64_t x) {
  #if BYTE_ORDER == LITTLE_ENDIAN
  unsigned char *s = (unsigned char*)&x;
  u_int64_t l[8];
  for (int i = 0; i < 8; i++) l[i] = (u_int64_t)s[i];
  return (u_int64_t)(l[0] << 56 | l[1] << 48 | l[2] << 40 | l[3] << 32 | l[4] << 24 | l[5] <<  16 | l[6] << 8 | l[7]);
  #else
    return x;
  #endif
}

u_int64_t htobe64(u_int64_t x) {
  #if BYTE_ORDER == LITTLE_ENDIAN
  unsigned char *s = (unsigned char*)&x;
   u_int64_t l[8];
  for (int i = 0; i < 8; i++) l[i] = (u_int64_t)s[i];
  return (u_int64_t)(l[0] << 56 | l[1] << 48 | l[2] << 40 | l[3] << 32 | l[4] << 24 | l[5] <<  16 | l[6] << 8 | l[7]); 
  #else
    return x;
  #endif
}
#endif



void get_data_sha(const char*, hashdata_t, uint32_t);
void get_file_sha(const char*, hashdata_t, long);
int cmp_hash(hashdata_t, hashdata_t);
void listen_for_requests(char*);
void init_dir_structure(char*);
void receive_file(int, uint32_t, void*);
void transfer_file(PeerAddress_t, char*);

