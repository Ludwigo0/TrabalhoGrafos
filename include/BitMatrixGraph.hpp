// Trabalho P1 - Teoria dos Grafos (COS 242)
// ---------------------------------------------------------------
// Matriz de adjacencia densa guardada em bits: 1 bit por par (i,j).
// Como o grafo e nao-direcionado, a matriz e simetrica: ao inserir a
// aresta (u,v) marcamos os bits (u,v) e (v,u).
//
// Por que bits e nao bytes? Cada casa em 1 byte custa n^2 bytes; em
// bits custa n^2/8 (8x menos). No grafo_2 (n ~ 50 mil) isso e a diferenca
// entre 2,3 GB e 297 MB - ou seja, entre caber ou nao na memoria. O preco
// e que cada acesso precisa de deslocamento e mascara, e a BFS precisa
// varrer a linha palavra por palavra (popcount/ctz), ficando mais lenta.
//
// Limite de seguranca (kMaxN = 60000, ou ~450 MB): a partir do grafo_3
// (n = 375 mil) nem o bitset cabe (16 GB > RAM da maquina). Em vez de tentar alocar
// dezenas de GB e travar o computador, o construtor recusa com uma
// mensagem dizendo quanta memoria seria necessaria - essa mensagem vira
// a linha "OOM teorico" na tabela do relatorio.
//
// Detalhe importante: a posicao do bit e calculada em size_t (64 bits).
// Com int de 32 bits, n*n estoura em 2,1 bilhoes e o calculo "da a volta",
// alocando o tamanho errado. Esse bug so apareceria nos grafos grandes.

#pragma once
#include "Graph.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

// Matriz de adjacencia densa em bits: 1 bit por par (i,j), simetrica.
// Blocos de 64 bits para alinhamento. Acesso a aresta (i,j):
//   pos = (size_t)i * n + j;  palavra = dados[pos/64], bit = pos%64.
// Troca em relacao a matriz de bytes: 8x menos memoria (grafo_2 cabe),
// ao custo de shift+mask por acesso e varredura por palavras na BFS.
// Limite de seguranca: n > kMaxN (20000) e recusado antes de alocar,
// pois n^2 bits explode (grafo_3 precisaria de 16 GB).
class BitMatrixGraph : public Graph {
public:
    // Teto de seguranca: n = 60000 -> 450 MB. Cabe folgado ate em maquina
    // de 8 GB (como a do colega/professor). O grafo_2 (n ~ 50 mil = 312 MB)
    // passa; o grafo_3 (n = 375 mil = 16 GB) e recusado antes de alocar.
    static constexpr int kMaxN = 60000;

    explicit BitMatrixGraph(int n);
    // u, v 0-indexados. Conta 1 aresta por chamada (self-loop = 1).
    void addEdge(int u, int v);

    int numVertices() const override;
    long long numEdges() const override;
    int degree(int v) const override;      // O(n/64) via popcount da linha
    int neighbor(int v, int k) const override; // k-esimo bit da linha, O(n/64)

    // Itera os vizinhos de v varrendo a linha UMA unica vez, palavra por
    // palavra, chamando f(to) para cada bit ligado. f deve retornar true
    // para continuar ou false para parar (a distancia usa isso para sair
    // mais cedo). E template de proposito: o laco e expandido no ponto de
    // chamada, sem std::function no caminho quente. E essa primitiva que
    // permite a BFS/DFS da matriz em O(n^2/64) por busca em vez de
    // O(m*n/64) da versao ingenua via neighbor() repetido.
    template <typename F>
    void forEachNeighbor(int v, F &&f) const {
        size_t base = (size_t)v * (size_t)n_;
        size_t end = base + (size_t)n_;
        size_t pos = base;
        while (pos < end) {
            size_t w = pos / 64;
            int b = (int)(pos % 64);
            int take = 64 - b;
            if ((size_t)take > end - pos)
                take = (int)(end - pos);
            uint64_t word = data_[w] >> b;
            if (take < 64 - b)
                word &= ((1ULL << take) - 1ULL);
            while (word != 0) {
                int i = __builtin_ctzll(word); // indice do menor bit ligado
                if (!f((int)(pos - base) + i))
                    return;
                word &= word - 1; // apaga o bit ja visitado
            }
            pos += (size_t)take;
        }
    }

private:
    bool get(int i, int j) const;

    int n_;
    long long m_;
    std::vector<uint64_t> data_;
};
