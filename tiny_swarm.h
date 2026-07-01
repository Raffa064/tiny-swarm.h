#ifndef TINY_SWARM_H
#define TINY_SWARM_H

#include <sched.h>
#include <stddef.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#define SWARM_KERNEL(ctx_t, item_t, name) \
   void __sw_kernel_##name(ctx_t *ctx, int index, item_t *item); \
   void name(void *ctx, int index, void *item) {  __sw_kernel_##name(ctx, index, item);  } \
   void __sw_kernel_##name(ctx_t *ctx, int index, item_t *item)

typedef void (*kernel_fn)(void *ctx, int index, void *item);;

typedef struct {
  void *ctx; // optional
  char *data;
  size_t stride, count;
  int thread_count;
} Swarm;


void swarm_spawn(Swarm s, kernel_fn kernel);

#ifdef TINY_SWARM_IMPLEMENTATION

typedef struct {
  void *ctx;
  pthread_t thread;
  int status;
  size_t index, count, stride;
  char* item;
  kernel_fn kernel;
} SwarmWorker;

void *swarm_spawn_worker(void *_args) {
  SwarmWorker *args = (SwarmWorker*)_args;

  for (int i = 0; i < args->count; i++) {
    args->kernel(args->ctx, args->index + i, args->item + args->stride * i);
  }

  args->status = 0;

  return NULL;
}

void swarm_spawn(Swarm s, kernel_fn kernel) {  
  SwarmWorker workers[s.thread_count];
  memset(workers, 0, sizeof(workers));
  
  size_t worker_ptr = 0;
  size_t item_ptr = 0;

  size_t ratio = s.count / (s.thread_count * 2);
  if (ratio == 0) ratio = 1;

  while (item_ptr < s.count) {
    while (workers[worker_ptr].status != 0) {
      worker_ptr = (worker_ptr + 1) % s.thread_count; // go to next worker
      sched_yield();
    }

    size_t count = item_ptr + ratio > s.count? (s.count - item_ptr) : ratio;
    workers[worker_ptr] = (SwarmWorker) {
      .ctx = s.ctx,
      .status = 1,
      .index = item_ptr,
      .count = count,
      .stride = s.stride,
      .item = s.data + s.stride * item_ptr,
      .kernel = kernel,
    };
    
    pthread_create(&workers[worker_ptr].thread, NULL, swarm_spawn_worker, &workers[worker_ptr]);
    
    worker_ptr++;
    item_ptr += count;
  }

  // Wait for all thread to finish.
  for (int i = 0; i < s.thread_count; i++) {
    if (workers[i].status) {
      i--;
      sched_yield();
    }
  }
}

#endif

#endif