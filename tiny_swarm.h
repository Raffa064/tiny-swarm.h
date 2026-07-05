#ifndef TINY_SWARM_H
#define TINY_SWARM_H

#include <stdint.h>
#include <sched.h>
#include <stddef.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdatomic.h>

#define SWARM_KERNEL(ctx_t, item_t, name) \
   void __sw_kernel_##name(ctx_t *ctx, int index, item_t *item); \
   void name(void *ctx, int index, void *item) {  __sw_kernel_##name(ctx, index, item);  } \
   void __sw_kernel_##name(ctx_t *ctx, int index, item_t *item)

typedef void (*kernel_fn)(void *ctx, int index, void *item);;

typedef struct {
  void *ctx;
  uint8_t *data;
  size_t stride, count;
  size_t workers_count;
  size_t chunk_size; // 0 means each worker process a single item at a time
} Swarm;

typedef struct {
  void *ctx;
  uint8_t *data;
  size_t stride, count, chunk_size;
  _Atomic size_t index;
} Workload;

typedef struct {
  pthread_t tid;
  Workload *load;
  kernel_fn kernel;
} SwarmWorker;

void swarm_spawn(Swarm s, kernel_fn kernel);

#ifdef TINY_SWARM_IMPLEMENTATION 

void *swarm_worker_routine(void *args) {
  SwarmWorker *worker = (SwarmWorker*) args;

  while (1) {
    size_t chunk_index = atomic_fetch_add(&worker->load->index, worker->load->chunk_size);
    if (chunk_index >= worker->load->count) break;

    size_t chunk_size = worker->load->chunk_size;
    size_t remaining = worker->load->count - chunk_index;
    if (chunk_size > remaining)
      chunk_size = remaining;
    
    for (size_t i = 0; i < chunk_size; i++) {
      size_t index = chunk_index + i;
      void *item = worker->load->data + index * worker->load->stride;
      worker->kernel(worker->load->ctx, index, item);
    }
  }

  return NULL;
}

void swarm_spawn(Swarm s, kernel_fn kernel) {
  SwarmWorker workers[s.workers_count];

  Workload load = {
    .ctx = s.ctx,
    .data = s.data,
    .stride = s.stride,
    .count = s.count,
    .chunk_size = s.chunk_size? s.chunk_size : 1,
    .index = 0,
  };

  for (size_t i = 0; i < s.workers_count; i++) {
    workers[i].load = &load;
    workers[i].kernel = kernel;
    pthread_create(&workers[i].tid, NULL, swarm_worker_routine, &workers[i]);
  }

  for (size_t i = 0; i < s.workers_count; i++)
    pthread_join(workers[i].tid, NULL);
}

#endif

#endif