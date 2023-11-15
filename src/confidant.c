#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "transfer.h"
#include "sha256.h"
#include "io_assist.h"
#include "confidant.h"
#include "input_val.c"


//Set to 1 with command-line flag to enable printing workflow (not completed yet)
int verbose = 1;

void* listener_thread(void* arg) {
  char* listening_port = (char*)arg;
  listen_for_requests(listening_port); 
  return NULL;
}

typedef struct {
  PeerAddress_t target_address;
  char file_path[PATH_LEN];
} SenderArgs_t;

void* sender_thread(void* arg) {
  SenderArgs_t* args = (SenderArgs_t*)arg; 
  printf("%s\n", args->file_path);
  transfer_file(args->target_address, args->file_path); 

  free(args);
  return NULL;
}

uint64_t get_last_time_modified(char* filename) {
  struct stat info = {0};
  stat(filename, &info); 
  return info.st_mtim.tv_sec;
}

// Push a file to a job queue for processing
void push(void* work) {
  char* path = (char*) work;

  fprintf(stdout, "Received path to push: %s\n", path);

  free(path);
  return;
}

//Depth-first traversal of a directory
void traverse_dir(char* dir) {
  DIR* dir_p = opendir(dir);
  if (!dir_p) {
    fprintf(stderr, "Could not open dir %s\n", dir);
    return;
  }
  if (verbose) fprintf(stdout, "Looking for files in %s\n", dir);

  struct dirent* element;
  while ((element = readdir(dir_p)) != NULL) {
    if (strcmp(element->d_name, "..") == 0 || strcmp(element->d_name, ".") == 0) {
      continue;
    }
    char element_path[PATH_LEN] = {0};
    strcpy(element_path, dir);
    strcat(element_path, "/");
    strcat(element_path, element->d_name);
    if (element->d_type == IS_DIR) {
      traverse_dir(element_path);
    } else {
      push((void*)strdup(element_path));
    } 
  }
  closedir(dir_p);
  return;
}

void push_files_for_transfer(void) {
  //Traverse all folders in ../../storage/, 
  //looking up each file in transfer_record in the process
  //Push file for sending if file doesnt exist or is modified from last transfer

  char path[PATH_LEN] = {0};
  char* base = "../../storage";
  strcpy(path, base);
  traverse_dir(path);
  return; 
}


int main(int argc, char** argv) {
  push_files_for_transfer();
  return 0;
  PeerAddress_t target_address;
  char file_path[PATH_LEN];
  char self_port[PORT_LEN];
  bool sender = false;
  bool receiver = false;

  if (argc == 1) {
    print_help();
    return EXIT_FAILURE;
  } 
  if (argc == 2) {
    if (strcmp(argv[1], "--phone") == 0) {
      //Phone stuff
      fprintf(stdout, "Mode not implemented\n");
      return EXIT_FAILURE;
    }
    else if (strcmp(argv[1], "--pi") == 0) {
      //Pi stuff
      fprintf(stdout, "Mode not implemented\n");
      return EXIT_FAILURE;
    }
    else if (strcmp(argv[1], "--desktop") == 0) {
      //Desktop stuff
      fprintf(stdout, "Mode not implemented\n");
      return EXIT_FAILURE;
    }
    else {
      print_help();
      return EXIT_FAILURE;
    }
  } 
  else if (3 <= argc && argc <= 7) {  
    for (int i = 1; i < argc;) {
      // Sender mode parsing
      if (argc - i > 0 && strcmp(argv[i], "-s") == 0) {
        assert(isValidIP(argv[i+1]));
        assert(isValidPort(argv[i+2]));
        assert(isValidFile(argv[i+3]));

        strcpy(target_address.ip,argv[i+1]);
        strcpy(target_address.port,argv[i+2]);
        strcpy(file_path, argv[i+3]);
        sender = true;
        i += 4;
      }
      // Receiver mode parsing
      else if (strcmp(argv[i], "-r") == 0) {
        assert(isValidPort(argv[i+1]));
        strcpy(self_port,argv[i+1]);
        receiver = true;
        i += 2;
      }
      else {
        break;
      }
    }
  }
  else {
    print_help();
    return EXIT_FAILURE;
  }
  
  pthread_t receiver_tid;
  pthread_t sender_tid;

  if (receiver) {
    pthread_create(&receiver_tid, NULL, listener_thread, (void*)&self_port);
  }

  if (sender) {
    SenderArgs_t* s_args = malloc(sizeof(SenderArgs_t));
    memcpy(&s_args->target_address, &target_address, sizeof(target_address));
    strcpy(s_args->file_path, file_path);    
    pthread_create(&sender_tid, NULL, sender_thread, (void*)s_args);
  }

  //Now what?
  
  if (receiver) {
    pthread_join(receiver_tid, NULL);
  }
  if (sender) {
    pthread_join(sender_tid, NULL);
  }

  
  return EXIT_SUCCESS;
}

