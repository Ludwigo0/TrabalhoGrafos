// Trabalho P1 - Teoria dos Grafos (COS 242)
// ---------------------------------------------------------------
// Lista de adjacencia em formato CSR ("compressed sparse row").
//
// Em vez de um vector por vertice (que custa ~24 bytes de administracao
// por vertice + folgas de capacidade), guardamos tudo em dois vetores
// corridos:
//   offsets[v] .. offsets[v+1]-1  -> fatia de `edges` com os vizinhos de v.
//
// Exemplo: 0 ligado a {1,2}, 1 ligado a {0}, 2 ligado a {0,1}:
//   offsets = [0, 2, 3, 5]
//   edges   = [1, 2, 0, 0, 1]
// Os vizinhos de v vao de edges[offsets[v]] ate edges[offsets[v+1]].
//
// Memoria total: (n+1)*4 + 2m*4 bytes. No maior grafo (4,8M vertices e
// 46,5M arestas) isso da ~373 MB, enquanto vector<vector<int>> estouraria
// so com o custo dos 4,8 milhoes de vectors (~115 MB) mais fragmentacao.
// O preco e que o grafo fica estatico: depois de montado nao da para
// adicionar arestas. Para este trabalho isso e perfeito, porque o grafo
// e lido uma vez e so consultado depois.

#pragma once
#include "Graph.hpp"
#include <utility>
#include <vector>

// Lista de adjacencia em formato CSR (compressed sparse row):
//   offsets[v] .. offsets[v+1]-1  = fatia de edges com os vizinhos de v.
// Memoria: (n+1)*4 + 2m*4 bytes. Sem overhead por vertice, ao contrario
// de vector<vector<int>> (~24 B por vertice + folgas de capacidade).
class CSRGraph : public Graph {
public:
    CSRGraph(int n, std::vector<int> offsets, std::vector<int> edges, long long m);

    // Constroi a partir de lista de pares 0-indexados (usado nos testes).
    static CSRGraph fromEdgeList(int n, const std::vector<std::pair<int, int>> &pairs);

    int numVertices() const override;
    long long numEdges() const override;
    int degree(int v) const override;
    int neighbor(int v, int k) const override;

private:
    int n_;
    long long m_;
    std::vector<int> offsets_;
    std::vector<int> edges_;
};
