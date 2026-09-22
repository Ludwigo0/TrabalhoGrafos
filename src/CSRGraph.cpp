// Trabalho P1 - Teoria dos Grafos (COS 242)
// ---------------------------------------------------------------
// Implementacao da lista CSR. A construcao acontece em duas fases:
//   1) conta o grau de cada vertice;
//   2) soma acumulada -> offsets; depois distribui os vizinhos.
// Isso evita realocacoes e deixa os vetores no tamanho exato.

#include "CSRGraph.hpp"
#include <cassert>
#include <stdexcept>

CSRGraph::CSRGraph(int n, std::vector<int> offsets, std::vector<int> edges, long long m)
    : n_(n), m_(m), offsets_(std::move(offsets)), edges_(std::move(edges)) {
    if ((int)offsets_.size() != n_ + 1)
        throw std::invalid_argument("CSR: offsets com tamanho errado");
}

CSRGraph CSRGraph::fromEdgeList(int n, const std::vector<std::pair<int, int>> &pairs) {
    // Fase 1: conta quantos vizinhos cada vertice tem. Como o grafo e
    // nao-direcionado, cada aresta (u,v) conta para os dois lados.
    // Self-loop ocupa 1 slot so (nao afeta a BFS de jeito nenhum).
    std::vector<int> deg(n, 0);
    for (const auto &e : pairs) {
        int u = e.first, v = e.second;
        if (u < 0 || u >= n || v < 0 || v >= n)
            throw std::out_of_range("CSR::fromEdgeList: vertice fora do intervalo");
        if (u == v) {
            deg[u] += 1; // self-loop ocupa 1 slot
        } else {
            deg[u] += 1;
            deg[v] += 1;
        }
    }
    // Fase 2a: soma acumulada dos graus -> cada offsets[v] diz onde
    // comeca a fatia do vertice v dentro de `edges`.
    std::vector<int> offsets(n + 1, 0);
    for (int i = 0; i < n; ++i)
        offsets[i + 1] = offsets[i] + deg[i];
    // Fase 2b: `cursor` e uma copia de offsets que avanca a cada vizinho
    // inserido, de modo que cada vertice preenche exatamente a sua fatia.
    std::vector<int> edges(offsets[n], 0);
    std::vector<int> cursor(offsets.begin(), offsets.begin() + n);
    for (const auto &e : pairs) {
        int u = e.first, v = e.second;
        if (u == v) {
            edges[cursor[u]++] = v;
        } else {
            edges[cursor[u]++] = v;
            edges[cursor[v]++] = u;
        }
    }
    return CSRGraph(n, std::move(offsets), std::move(edges), (long long)pairs.size());
}

int CSRGraph::numVertices() const { return n_; }
long long CSRGraph::numEdges() const { return m_; }

int CSRGraph::degree(int v) const { return offsets_[v + 1] - offsets_[v]; }

int CSRGraph::neighbor(int v, int k) const {
    // Em Release (NDEBUG) nao custa nada; em Debug pega na hora qualquer
    // chamada com vertice ou indice invalido, em vez de ler lixo do vetor.
    assert(v >= 0 && v < n_);
    assert(k >= 0 && k < degree(v));
    return edges_[offsets_[v] + k];
}
