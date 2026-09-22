// Trabalho P1 - Teoria dos Grafos (COS 242)
// ---------------------------------------------------------------
// Este arquivo define o "contrato" que as duas representacoes de
// grafo precisam cumprir (lista CSR e matriz de bits). A ideia e
// que os algoritmos (BFS, DFS, diametro...) recebam um `const Graph&`
// sem precisar saber qual representacao esta por tras: para trocar
// de uma para outra basta mudar a flag --rep na linha de comando.
// Com isso a comparacao de tempo/memoria do estudo de caso fica justa.
//
// Convenção: vertices numerados de 0 a n-1. O arquivo de entrada usa
// 1..n, entao a conversao (subtrair 1) acontece na camada de I/O.

#pragma once

// Interface abstrata de grafo nao-direcionado.
// Vertices sao numerados de 0 a n-1 (o arquivo usa 1..n; a conversao
// acontece na camada de I/O). Grafo e estatico: construido uma vez,
// so consultado depois.

class Graph {
public:
    virtual ~Graph() = default;

    virtual int numVertices() const = 0;
    // Numero de arestas nao-direcionadas como lidas (cada linha = 1).
    virtual long long numEdges() const = 0;
    virtual int degree(int v) const = 0;
    // k-esimo vizinho de v, com 0 <= k < degree(v). A ordem e arbitraria
    // mas deterministica para a mesma construcao.
    virtual int neighbor(int v, int k) const = 0;
};
