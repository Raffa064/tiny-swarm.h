#ifndef TINYSWARM_H
#define TINYSWARM_H

#include <stdlib.h>
#include <stdint.h>
#include <sched.h>
#include <stddef.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdatomic.h>

#ifndef TINYSWARM_DEFAULT_WORKERS_COUNT
#define TINYSWARM_DEFAULT_WORKERS_COUNT 2
#endif

#define TINYSWARM_OK   0
#define TINYSWARM_FAIL 1

// This macro is just a convenience for defining swarm kernels.
// All kernels exposes 3 parameters:
// Context, index and the current item pointer.
#define TINYSWARM_KERNEL(ctx_t, item_t, name) \
   void __sw_kernel_##name(ctx_t *ctx, size_t index, item_t *item); \
   void name(void *ctx, size_t index, void *item) {  __sw_kernel_##name(ctx, index, item);  } \
   void __sw_kernel_##name(ctx_t *ctx, size_t index, item_t *item)

typedef void (*SwarmKernel)(void *ctx, size_t index, void *item);;

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
} SwarmWorkload;

typedef struct {
  pthread_t tid;
  SwarmWorkload *load;
  SwarmKernel kernel;
} SwarmWorker;

// Initializes a swarm of works in parallel, process data array and return when finished.
// If it returns 1, somthing went wrong with given configuration, or it couldn't spawn 
// any threads for some reason
int swarm_spawn(Swarm s, SwarmKernel kernel);

#ifdef TINYSWARM_IMPLEMENTATION 

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

int swarm_spawn(Swarm swarm, SwarmKernel kernel) {
  if (swarm.stride == 0)
    return TINYSWARM_FAIL;

  if (swarm.count == 0) 
    return TINYSWARM_OK; // Nothing to do...


  size_t worker_count = swarm.workers_count;
  if (worker_count == 0)
    worker_count = TINYSWARM_DEFAULT_WORKERS_COUNT;

  SwarmWorker workers[worker_count];
  SwarmWorkload load = {
    .ctx = swarm.ctx,
    .data = swarm.data,
    .stride = swarm.stride,
    .count = swarm.count,
    .chunk_size = swarm.chunk_size? swarm.chunk_size : 1,
    .index = 0,
  };

  size_t spawned_threads = 0;
  for (size_t i = 0; i < worker_count; i++) {
    workers[i].load = &load;
    workers[i].kernel = kernel;
    if (pthread_create(&workers[i].tid, NULL, swarm_worker_routine, &workers[i]) != 0)
      break;

    spawned_threads++;
  }

  if (spawned_threads == 0)
    return TINYSWARM_FAIL;

  for (size_t i = 0; i < spawned_threads; i++)
    pthread_join(workers[i].tid, NULL);

  return TINYSWARM_OK;
}

#endif

#endif