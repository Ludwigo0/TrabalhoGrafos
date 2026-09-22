# Trabalho P1 — Teoria dos Grafos (COS 242): biblioteca de grafos em C++

Biblioteca para grafos **não-direcionados** com duas representações
(**lista CSR** e **matriz de bits**), BFS/DFS iterativas, distância via BFS,
diâmetro exato e aproximado (double-sweep), componentes conexas e
estatísticas de grau.

## Estrutura

```
CMakeLists.txt  receita de compilacao (CMake, C++17)
include/        Graph.hpp, CSRGraph.hpp, BitMatrixGraph.hpp, algorithms.hpp, io.hpp
src/            implementacao + main.cpp (programa-cliente)
tests/          test_grafo.cpp (exemplo Fig.1 + vias rapidas + carga/desconexo + trava)
scripts/        run_estudos.sh (roteiro dos estudos de caso -> CSV)
docs/relatorio/ main.tex + main.pdf (relatorio, max. 5 paginas)
docs/apresentacao/ slides.tex + roteiro.md (apresentacao de 8 min)
data/           exemplo_fig1.txt (os grafos grandes NAO vao para o git)
```

## Compilar

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/test_grafo        # exemplo Fig.1 + vias rapidas + carga/desconexo + trava
ctest --test-dir build    # via CTest
# Build Debug (asserts de neighbor() ativos):
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug && cmake --build build-debug -j
```

## Usar

```bash
./build/grafo data/exemplo_fig1.txt --rep lista --out out/ \
  --stats --components --bfs 1 --dfs 1 --dist 1,3 --diameter-approx 1
```

Vertices na CLI usam a numeracao do arquivo (1..n). Opcoes completas em
`./build/grafo` sem argumentos. Benchmark: `--benchmark-bfs 100`.

## Formatos de saida (`--out DIR`)

- `stats.txt`: `n m gmin gmax gavg gmed` (um por linha, `chave valor`).
- `bfs_S.txt` / `dfs_S.txt`: uma linha por vertice, `vertice pai nivel`
  (1-indexados; pai `-1` = raiz ou inalancavel; nivel `-1` = inalancavel).
- `components.txt`: 1a linha = no. de componentes; demais = `tamanho m1 m2 ...`
  em ordem decrescente de tamanho.
- stdout: linhas `chave=valor` (carga, tempos em ms, memoria de pico) para o script coletar.

## Estudos de caso

```bash
./scripts/run_estudos.sh . ./out_estudos
# usa ./build/grafo; descompacta cada .gz em temp (~1 GB livre p/ o grafo_6)
# resultado em ./out_estudos/resultados.csv
```

Matriz e tentada nos grafos 1 e 2; do 3 em diante registra-se OOM teorico
(a matriz completa em bits precisaria de ~17,6 GB / ~2,93 TB). Diametro exato so nos pequenos
(`--diameter-exact`); nos grandes, `--diameter-approx`.

## Notas de projeto

- I/O com scanner proprio (`fread` + parse manual): o grafo_6 tem ~93M de
  inteiros; iostream ingenuo e 10-50x mais lento.
- CSR em duas passadas sobre o arquivo (conta graus -> prefix-sum -> preenche):
  evita lista temporaria de arestas (~370 MB no grafo_6).
- `n*n` sempre em `size_t` (64 bits): em `int` de 32 bits estoura já no grafo_2.
- BFS/DFS iterativas: DFS recursiva estoura a pilha nos grafos grandes.
- Matriz com **vias rapidas**: `forEachNeighbor` varre a linha uma vez por
  vertice (O(n^2) por busca); os despachantes `bfsAuto/dfsAuto/distanceAuto/
  componentsAuto` escolhem a via rapida via `dynamic_cast`, sem expor a
  representacao ao chamador. Sem isso, a BFS-matriz revarreria a linha a cada
  aresta (penalidade de 400x+ em vez de ~11x na BFS e ~7x na DFS, no grafo 2).
- Trava da matriz em `kMaxN = 60000` (~450 MB): grafo_2 (~297 MB) passa,
  grafo_3+ (~17,6 GB / ~2,93 TB) e recusado com a memoria necessaria.
- `neighbor()` tem `assert` de limites (custo zero em Release, ativo em Debug).
