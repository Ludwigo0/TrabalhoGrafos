// Trabalho P1 - Teoria dos Grafos (COS 242)
// ---------------------------------------------------------------
// Algoritmos da Parte 1 do trabalho. Todos recebem `const Graph&`, ou
// seja, funcionam igual na lista CSR e na matriz de bits - e disso que
// precisamos para comparar as duas representacoes de forma justa.
//
// Convenções de saida (iguais para BFS e DFS):
//   pai[]   -> pai de cada vertice na arvore de busca (-1 = raiz ou
//              vertice inalcançavel, que nao tem pai);
//   nivel[] -> distancia da origem em arestas (raiz = 0, inalcançavel = -1).
// Esses dois vetores sao exatamente o que o enunciado pede para imprimir
// no arquivo de saida ("informar o pai de cada vertice e seu nivel").

#pragma once
#include "BitMatrixGraph.hpp"
#include "Graph.hpp"
#include <vector>

// BFS (busca em largura): explora o grafo em "ondas" a partir da origem,
// usando uma fila. O nivel de cada vertice e a distancia minima ate a
// origem, por isso a BFS serve de primitiva para distancia e diametro.
// Custo: O(n+m) na lista, O(n^2) na matriz (precisa varrer a linha
// inteira de cada vertice). Iterativa para nao estourar a pilha.
struct BFSResult {
    std::vector<int> parent;
    std::vector<int> level;
};

BFSResult bfs(const Graph &g, int src);
// Via rapida para matriz de bits: mesma BFS e mesmo resultado, mas vizinhos
// obtidos com forEachNeighbor (uma varredura por linha) em vez de neighbor()
// repetido. Contrato e saida identicos aos da versao generica.
BFSResult bfs(const BitMatrixGraph &g, int src);
// Despachante: usa a via rapida quando o grafo e matriz de bits, sem que o
// chamador precise saber a representacao. E o que o programa principal usa.
BFSResult bfsAuto(const Graph &g, int src);

// DFS (busca em profundidade): mergulha num caminho ate o fim antes de
// voltar, usando uma pilha explicita. Propositalmente NAO e recursiva:
// com milhoes de vertices a recursao estouraria a pilha do programa.
// Aqui nivel[] significa profundidade de descoberta (nao distancia minima).
struct DFSResult {
    std::vector<int> parent;
    std::vector<int> level;
};

DFSResult dfs(const Graph &g, int src);
// Via rapida para matriz: coleta os vizinhos uma vez por vertice
// (forEachNeighbor) em vez de revarrer a linha a cada aresta.
DFSResult dfs(const BitMatrixGraph &g, int src);
DFSResult dfsAuto(const Graph &g, int src);

// Distancia entre u e v usando a BFS como primitiva (como pede o
// enunciado). E uma BFS com parada antecipada: ao encontrar v, retorna
// na hora em vez de explorar o resto. Retorna -1 se forem desconexos.
int distanceBFS(const Graph &g, int u, int v);
int distanceBFS(const BitMatrixGraph &g, int u, int v);
int distanceAuto(const Graph &g, int u, int v);

struct DiameterResult {
    int value; // maior distancia encontrada (diametro da maior componente
               // se o grafo for desconexo; documentado no relatorio)
    int a;     // extremidade 0-indexada
    int b;     // extremidade 0-indexada
};

// Diametro exato: a maior distancia entre qualquer par de vertices.
// Por definicao, calculamos uma BFS por vertice e ficamos com o maximo:
// O(n*(n+m)). No grafo_6 seriam 4,8 milhoes de BFSs (seculos), por isso
// esse metodo so e usado nos grafos pequenos, sob a flag --diameter-exact.
// Se o grafo for desconexo, retornamos o diametro da maior componente
// (o diametro "puro" seria infinito) - isso vai explicado no relatorio.
DiameterResult diameterExact(const Graph &g);
// Diametro aproximado (double-sweep), a dica do enunciado para grafos
// muito grandes: 1) BFS de um vertice qualquer -> pega o mais longe (a);
// 2) BFS de `a` -> pega o mais longe (b); 3) responde dist(a,b).
// Custa so 2 BFSs em vez de n. E sempre um limite inferior do diametro
// real (nunca "chuta para cima") e continua usando a BFS como primitiva,
// entao respeita o que o enunciado pediu.
DiameterResult diameterApproxDoubleSweep(const Graph &g, int start = 0);

struct Components {
    std::vector<int> compOf;                 // compOf[v] = indice da componente
    std::vector<std::vector<int>> members;   // ordenadas por tamanho decrescente
};

Components connectedComponents(const Graph &g);
Components connectedComponents(const BitMatrixGraph &g);
Components componentsAuto(const Graph &g);

struct Stats {
    int n;
    long long m;
    int gmin;
    int gmax;
    double gavg;
    double gmed;
};

Stats computeStats(const Graph &g);
