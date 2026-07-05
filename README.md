# Tiny Swarm

🇺🇸 English | [🇧🇷 Português](README.pt-BR.md)

Conceptually similar to the `map()` function, Tiny Swarm is a small paralelism library designed for cases where you have an vector of idependent elements
and want to apply the same function (kernel) for each of them. It's API is intentionally simple: a single function, `swarm_spawn()`, creates the worker threads, distribute the workload among them, and blocks until every element in the input array has been processed.

```c
Swarm cfg = {
  .workers_count = 4,
  .data = (void*) input,
  .stride = sizeof(input[0]),
  .count = INPUT_COUNT,
  .chunk_size = 32
};

swarm_spawn(cfg, my_kernel_func);
swarm_spawn(cfg, another_kernel_func);
```