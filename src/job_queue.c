#include <errno.h>
#include <string.h>
#include <stdio.h>
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

//Cyclic queue with intentional overflow
int job_queue_init(job_queue_t* jq) {
  jq->jobs = malloc(sizeof(void*)*JOB_QUEUE_SIZE);
  jq->size = JOB_QUEUE_SIZE;
  jq->head = 0;
  jq->tail = 0;
  jq->is_destroyed = 0;
  if (pthread_mutex_init(&jq->mutex, NULL) != 0) {
    fprintf(stderr, "Failed to initialize mutex in job_queue_init with error %s\n", strerror(errno));
    return 1;
  }
  if (pthread_cond_init(&jq->full, NULL) != 0) {
    fprintf(stderr, "Failed to initialize cond in job_queue_init with error %s\n", strerror(errno));
    return 1;
  } 
  if (pthread_cond_init(&jq->empty, NULL) != 0) {
    fprintf(stderr, "Failed to initialize cond in job_queue_init with error %s\n", strerror(errno));
    return 1;
  } 
  return 0;
}

int job_queue_push(job_queue_t* jq, void* data) {
  if (jq == NULL) {
    fprintf(stderr, "Tried to push job to invalid job_queue\n");
    return 1;
  }

  if (pthread_mutex_lock(&jq->mutex) != 0) {
    fprintf(stderr, "Failed to lock mutex in job_queue_push with error: %s\n", strerror(errno));
    return 1;
  }

  while (jq->tail + 1 == jq->head) { 
    pthread_cond_wait(&jq->full, &jq->mutex);
  }

  if (jq->is_destroyed) {
    fprintf(stderr, "Tried to push job to job_queue that is being destroyed\n");
    if (pthread_mutex_unlock(&jq->mutex) != 0) {
      fprintf(stderr, "Failed to unlock mutex in job_queue_push with error: %s\n", strerror(errno));
    }
    return 1;
  }

  jq->jobs[jq->tail] = data;
  jq->tail++;
  
  if (pthread_cond_signal(&jq->empty) != 0) { 
    fprintf(stderr, "Failed to cond signal in job_queue_push with error: %s\n", strerror(errno));
  }
   
  if (pthread_mutex_unlock(&jq->mutex) != 0) {
    fprintf(stderr, "Failed to unlock mutex in job_queue_push with error: %s\n", strerror(errno));
    return 1;
  } 
  
  return 0;
}


int job_queue_pop(job_queue_t* jq, void** dest) {
  if (jq == NULL) {
    fprintf(stderr, "Tried to pop from invalid job_queue\n");
    return 1;
  }

  if (pthread_mutex_lock(&jq->mutex) != 0) {
    fprintf(stderr, "Failed to lock mutex in job_queue_pop with error: %s\n", strerror(errno));
    return 1;
  }

  while (jq->head == jq->tail && !jq->is_destroyed) {
    pthread_cond_wait(&jq->empty, &jq->mutex);
  }

  if (jq->is_destroyed) {
    if (pthread_mutex_unlock(&jq->mutex) != 0) {
      fprintf(stderr, "Failed to unlock mutex in job_queue_pop with error: %s\n", strerror(errno));
      return 1;
    }
    return -1;
  }

  *dest = jq->jobs[jq->head];
  jq->head++;

  if (pthread_cond_signal(&jq->full) != 0) {
    fprintf(stderr, "Failed to cond signal in job_queue_pop with error: %s\n", strerror(errno));
  }
  
  if (pthread_mutex_unlock(&jq->mutex) != 0) {
    fprintf(stderr, "Failed to unlock mutex in job_queue_pop with error: %s\n", strerror(errno));
    return 1;
  }

  return 0;
}

int job_queue_destroy(job_queue_t* jq) {
  if (jq == NULL) {
    fprintf(stderr, "Tried to destroy invalid job queue\n");
    return 1;
  }

  if (pthread_mutex_lock(&jq->mutex) != 0) {
    fprintf(stderr, "Failed to lock mutex in job_queue_destroy with error: %s\n", strerror(errno));
    return 1;
  }

  jq->is_destroyed = 1;

  //Wait for all threads sitting in pop to finish
  while (jq->tail != jq->head) {
    pthread_cond_wait(&jq->empty, &jq->mutex);
  }

  free(jq->jobs);
  pthread_mutex_destroy(&jq->mutex);
  pthread_cond_destroy(&jq->full);
  pthread_cond_destroy(&jq->empty);

  return 0;
}








