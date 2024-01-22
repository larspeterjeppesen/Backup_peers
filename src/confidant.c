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
#include "job_queue.h"
#include "sha256.h"
#include "io_assist.h"
#include "confidant.h"
#include "input_validation.c"


//Set to 1 with command-line flag to enable printing workflow (not completed yet)
int verbose = 1;

void* worker_receive_files(void* arg) {
  char* listening_port = (char*)arg;
  listen_for_connection(listening_port); 
  return NULL;
}

typedef struct {
  char* port;
  job_queue_t* jq;
} ReceiverArgs_t;

typedef struct {
  int connfd;
  job_queue_t* jq;
} SenderArgs_t;

typedef struct {
  char* root_dir;
  job_queue_t* jq;
} PusherArgs_t;


void* worker_send_files(void* s_args) {
  SenderArgs_t* args = (SenderArgs_t*)s_args; 
  while (1) {
    char* path;
    int ret = job_queue_pop(args->jq, (void**)&path);
    if (ret == -1) {
      break; //Queue destroyed, no more work
    } else if (ret != 0) {
      fprintf(stderr, "Error trying to pop from job_queue in worker_send_files with error: %s\n", 
              strerror(errno));
      break;
    }
    transfer_file(args->connfd, path); 
    free(path);
  }
  signal_terminate_connection(args->connfd); 
  return NULL;
}

uint64_t get_last_time_modified(char* filename) {
  struct stat info = {0};
  stat(filename, &info); 
  return info.st_mtim.tv_sec;
}


//Depth-first traversal of a directory
void push_files_in_dir(job_queue_t* jq, char* dir) {
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
      push_files_in_dir(jq, element_path);
    } else {
      if (verbose) fprintf(stdout, "Pushing path to job queue: %s\n", element_path);
      job_queue_push(jq, (void*)strdup(element_path));
    } 
  }
  closedir(dir_p);
  return;
}

//Traverse phone directory for files to send
void* worker_push_files_for_transfer(void* arg) {
  PusherArgs_t* args = (PusherArgs_t*) arg;
  push_files_in_dir(args->jq, args->root_dir);
  job_queue_destroy(args->jq); //waits until empty
  return NULL; 
}

int main(int argc, char** argv) {
  char self_port[PORT_LEN];
  char target_ip[IP_LEN];
  char target_port[PORT_LEN];
  char root_dir[PATH_LEN];
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
  else if (3 <= argc && argc <= 8) {  
    for (int i = 1; i < argc;) {
      // Sender mode parsing
      if (argc - i > 0 && strcmp(argv[i], "-s") == 0) {
        assert(isValidIP(argv[i+1]));
        assert(isValidPort(argv[i+2]));
        assert(isValidFile(argv[i+3]));

        strcpy(target_ip,argv[i+1]);
        strcpy(target_port,argv[i+2]);
        strcpy(root_dir, argv[i+3]);
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

  job_queue_t jq;
  job_queue_init(&jq);

  pthread_t receiver_tid;
  pthread_t sender_tid;
  pthread_t pusher_tid;

  if (receiver) {
    pthread_create(&receiver_tid, NULL, worker_receive_files, (void*)&self_port);
  }

  PusherArgs_t p_args;
  SenderArgs_t s_args;

  if (sender) {
    int connfd = io_assist_open_clientfd(target_ip, target_port);
    p_args.root_dir = root_dir; p_args.jq = &jq;
    s_args.connfd = connfd; s_args.jq = &jq;
    pthread_create(&pusher_tid, NULL, worker_push_files_for_transfer, (void*)&p_args);
    pthread_create(&sender_tid, NULL, worker_send_files, (void*)&s_args);
  }

  //Now what?
  
  if (receiver) {
    pthread_join(receiver_tid, NULL);
  }
  if (sender) {
    pthread_join(pusher_tid, NULL);
    pthread_join(sender_tid, NULL);
    close(s_args.connfd);
  }

  
  return EXIT_SUCCESS;
}

