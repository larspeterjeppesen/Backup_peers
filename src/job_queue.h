#include <stdlib.h>
#include <pthread.h>

#define JOB_QUEUE_SIZE (u_int8_t)256

typedef struct {
  void** jobs; 
  u_int8_t size;
  u_int8_t head;
  u_int8_t tail;
  pthread_mutex_t mutex;
  pthread_cond_t full;
  pthread_cond_t empty;
  int is_destroyed;
} job_queue_t;

int job_queue_init(job_queue_t*);
int job_queue_push(job_queue_t*, void*);
int job_queue_pop(job_queue_t*, void**);
int job_queue_destroy(job_queue_t*);
