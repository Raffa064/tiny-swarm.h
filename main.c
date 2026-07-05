#include <stdlib.h>
#include <stdio.h>

#define TINY_SWARM_IMPLEMENTATION
#include "tiny_swarm.h"

#define N 100000000L

SWARM_KERNEL(void, int, kernel_1) {
  *item = index * 2 + 1;
}

SWARM_KERNEL(void, int, kernel_2) {
  if (*item != index * 2 + 1) {
    fprintf(stderr, "Failed at %d\n", index);
    exit(1);
  }
}

int main() {
  int *numbers = malloc(sizeof(int) * N);

  if (!numbers) {
    printf("malloc failed");
    return 1;
  }

  Swarm s = {
    .ctx = numbers,
    .workers_count = 4,
    .data = (void*) numbers,
    .stride = sizeof(int),
    .count = N,
    .chunk_size = 32
  };

  swarm_spawn(s, kernel_1);
  swarm_spawn(s, kernel_2);

  printf("%zu Calculations done.\n", N);
}