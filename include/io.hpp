// Trabalho P1 - Teoria dos Grafos (COS 242)
// ---------------------------------------------------------------
// Entrada e saida: leitura do arquivo texto do grafo e medicao de memoria.
//
// Formato do arquivo (ver Figura 1 do enunciado): a primeira linha tem o
// numero de vertices n; cada linha seguinte tem uma aresta "u v" com
// vertices de 1 a n. O grafo e nao-direcionado, entao cada aresta vale
// nos dois sentidos.

#pragma once
#include <memory>
#include <string>

class Graph;

// Le o grafo do disco ja na representacao pedida ("lista" = CSR,
// "matriz" = bits). Se a matriz nao couber na memoria, lanca
// runtime_error com a quantidade de memoria que seria necessaria - o
// programa principal transforma isso numa linha "LOAD_FAIL", que o script
// de experimentos registra como OOM teorico (nao como bug).
std::unique_ptr<Graph> loadGraph(const std::string &path, const std::string &rep);

// Pico de memoria do processo em MB (campo VmHWM de /proc/self/status).
// E a medicao "honesta" que o estudo de caso pede: em vez de pausar o
// programa e olhar o monitor, o proprio programa informa quanto usou.
// Retorna -1 se nao for possivel medir (ex.: outro sistema operacional).
double peakMemoryMB();
