# Tiny Swarm

[🇺🇸 English](README.md) | 🇧🇷 Português

Conceitualmente semelhante à função `map()`, **Tiny Swarm** é uma pequena biblioteca de paralelismo perfeita para casos onde você tem um vetor de elementos independentes e deseja aplicar uma determinada função (kernel) sobre cada item. A API dela é bem simples: uma única função, `swarm_spawn()`, inicia as threads (workers), distribui a carga entre elas e aguarda até que todos os elementos do vetor de entrada sejam processados.

```c
Swarm cfg = {
  .workers_count = 4,
  .data = (void*) entrada,
  .stride = sizeof(entrada[0]),
  .count = QUANTIDADE_DE_ELEMENTOS_DE_ENTRADA,
  .chunk_size = 32
};

swarm_spawn(cfg, minha_funcao_kernel);
swarm_spawn(cfg, outro_funcao__kernel);
```