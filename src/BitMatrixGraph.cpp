// Trabalho P1 - Teoria dos Grafos (COS 242)
// ---------------------------------------------------------------
// Implementacao da matriz de bits. Os pontos delicados:
// - todo calculo de posicao usa size_t (ver comentario no .hpp);
// - grau e vizinhos sao obtidos varrendo a linha por palavras de 64 bits
//   com popcount, em vez de testar bit a bit (bem mais rapido).

#include "BitMatrixGraph.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

// Conta bits de uma palavra (popcount). __builtin_popcountll existe no GCC/Clang.
static int popcnt(uint64_t w) { return __builtin_popcountll(w); }

BitMatrixGraph::BitMatrixGraph(int n) : n_(n), m_(0) {
    if (n_ <= 0)
        throw std::invalid_argument("BitMatrix: n deve ser positivo");
    if (n_ > kMaxN) {
        double needGB = (double)(size_t)n_ * (size_t)n_ / 8.0 / 1e9;
        throw std::runtime_error("BitMatrix inviavel: n=" + std::to_string(n_) +
                                 " precisaria de ~" + std::to_string(needGB) +
                                 " GB so para a matriz");
    }
    size_t bits = (size_t)n_ * (size_t)n_;
    size_t words = (bits + 63) / 64;
    data_.assign(words, 0);
}

void BitMatrixGraph::addEdge(int u, int v) {
    // Grafo nao-direcionado: marca (u,v) e (v,u). Self-loop marca 1 bit so
    // e conta como 1 aresta (igual ao criterio da lista CSR).
    size_t p1 = (size_t)u * (size_t)n_ + (size_t)v;
    data_[p1 / 64] |= (1ULL << (p1 % 64));
    if (u != v) {
        size_t p2 = (size_t)v * (size_t)n_ + (size_t)u;
        data_[p2 / 64] |= (1ULL << (p2 % 64));
    }
    m_ += 1;
}

int BitMatrixGraph::numVertices() const { return n_; }
long long BitMatrixGraph::numEdges() const { return m_; }

bool BitMatrixGraph::get(int i, int j) const {
    size_t p = (size_t)i * (size_t)n_ + (size_t)j;
    return (data_[p / 64] >> (p % 64)) & 1ULL;
}

int BitMatrixGraph::degree(int v) const {
    // Grau = numero de bits ligados na linha v. Como a linha quase nunca
    // comeca/termina alinhada em multiplo de 64, o primeiro e o ultimo
    // pedaco usam mascara para nao contar bits da linha vizinha.
    int deg = 0;
    size_t base = (size_t)v * (size_t)n_;
    int remain = n_;
    size_t word = base / 64;
    int off = (int)(base % 64);
    if (off != 0) {
        int take = 64 - off < remain ? 64 - off : remain;
        uint64_t mask = take == 64 ? ~0ULL : (((1ULL << take) - 1ULL) << off);
        deg += popcnt(data_[word] & mask);
        remain -= take;
        word += 1;
    }
    while (remain >= 64) {
        deg += popcnt(data_[word]);
        remain -= 64;
        word += 1;
    }
    if (remain > 0) {
        uint64_t mask = (1ULL << remain) - 1ULL;
        deg += popcnt(data_[word] & mask);
    }
    return deg;
}

int BitMatrixGraph::neighbor(int v, int k) const {
    // k-esimo vizinho = k-esimo bit ligado da linha. Em vez de testar os
    // n bits um a um, pulamos de palavra em palavra: se a palavra tem `c`
    // bits e ainda faltam `skip > c`, pulamos ela inteira; senao, o vizinho
    // procurado esta aqui dentro e achamos com um laco nos 64 bits.
    // O assert chama degree() (caro na matriz), mas so existe em Debug.
    assert(v >= 0 && v < n_);
    assert(k >= 0 && k < degree(v));
    size_t base = (size_t)v * (size_t)n_;
    size_t end = base + (size_t)n_;
    int skip = k;
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
        int c = popcnt(word);
        if (skip < c) {
            for (int i = 0; i < take; ++i) {
                if (word & (1ULL << i)) {
                    if (skip == 0)
                        return (int)(pos - base) + i;
                    --skip;
                }
            }
        } else {
            skip -= c;
        }
        pos += (size_t)take;
    }
    return -1; // k fora do intervalo (nao deve acontecer se 0 <= k < degree)
}
