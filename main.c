#include <stdlib.h>
#define TINY_SWARM_IMPLEMENTATION
#include "tiny_swarm.h"

#include <stdio.h>

typedef struct {
  int coef;
  int term;
} Config;

SWARM_KERNEL(void, int, set_id) {
  *item = index;
}

SWARM_KERNEL(Config, int, mul2) {
  *item *= ctx->coef;
  *item += ctx->term;
}

int main() {
  long N = 1000000000L;
  int *numbers = malloc(sizeof(int) * N);

  if (!numbers) {
    printf("malloc failed");
    return 1;
  }

  Swarm s = {
    .ctx = &(Config) { .coef = 2, .term = 1 },
    .thread_count = 8,
    .data = (void*) numbers,
    .stride = sizeof(int),
    .count = N,
  };

  swarm_spawn(s, set_id);
  swarm_spawn(s, mul2);

  printf("%zu Calculations done.\n", N);
}