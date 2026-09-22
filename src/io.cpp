// Trabalho P1 - Teoria dos Grafos (COS 242)
// ---------------------------------------------------------------
// Leitura do arquivo e medicao de memoria. Os dois pontos delicados:
// 1) VELOCIDADE: o grafo_6 tem ~93 milhoes de inteiros em 722 MB de texto.
//    Com cin >> isso levaria dezenas de minutos; o FastScanner abaixo le
//    por blocos de 1 MB via fread e converte na mao (segundos).
// 2) MEMORIA NA CARGA: para montar o CSR sem guardar a lista temporaria
//    de arestas (~370 MB no grafo_6), lemos o arquivo DUAS vezes: na
//    primeira so contamos os graus, montamos os offsets e alocamos o
//    tamanho exato; na segunda preenchemos os vizinhos. O custo e reler
//    o arquivo, mas a economia de RAM compensa nos grafos grandes.

#include "io.hpp"
#include "BitMatrixGraph.hpp"
#include "CSRGraph.hpp"
#include "Graph.hpp"

#include <cstdio>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// Leitor de inteiros por blocos: em vez de pedir numero por numero ao
// sistema, lemos 1 MB de cada vez com fread e extraimos os inteiros do
// buffer. E o que torna a leitura do grafo_6 viavel (ver cabecalho acima).
class FastScanner {
public:
    explicit FastScanner(FILE *f) : f_(f), pos_(0), len_(0), eof_(false) {
        buf_.resize(BUFSIZE);
        refill();
    }

    // Retorna false no EOF (sem inteiro parcial pendente).
    bool nextInt(int &out) {
        int c;
        do {
            c = peek();
            if (c < 0)
                return false;
            advance();
        } while (c != '-' && (c < '0' || c > '9'));
        bool neg = false;
        if (c == '-') {
            neg = true;
            c = peek();
            if (c < 0)
                throw std::runtime_error("arquivo malformado: '-' solitario no fim");
            advance();
        }
        long long val = 0;
        bool any = false;
        while (c >= '0' && c <= '9') {
            any = true;
            val = val * 10 + (c - '0');
            c = peek();
            if (c < 0)
                break;
            advance();
        }
        if (!any)
            throw std::runtime_error("arquivo malformado: esperado digito");
        out = neg ? -(int)val : (int)val;
        return true;
    }

private:
    static const size_t BUFSIZE = 1 << 20;
    FILE *f_;
    std::vector<char> buf_;
    size_t pos_, len_;
    bool eof_;

    void refill() {
        if (eof_) {
            len_ = 0;
            pos_ = 0;
            return;
        }
        len_ = fread(buf_.data(), 1, BUFSIZE, f_);
        pos_ = 0;
        if (len_ < BUFSIZE) {
            if (ferror(f_))
                throw std::runtime_error("erro de leitura do arquivo");
            eof_ = true;
        }
    }

    int peek() {
        if (pos_ >= len_) {
            if (eof_)
                return -1;
            refill();
            if (len_ == 0)
                return -1;
        }
        return (unsigned char)buf_[pos_];
    }

    void advance() { ++pos_; }
};

// Passada 1 (CSR): le n e conta os graus sem guardar aresta nenhuma.
// Duas passadas evitam a lista temporaria de 46M de pares (~370 MB).
// Os vertices vem de 1 a n no arquivo e viram 0 a n-1 aqui dentro.
static void countDegrees(const std::string &path, int &n, long long &m,
                         std::vector<int> &deg) {
    FILE *f = fopen(path.c_str(), "rb");
    if (!f)
        throw std::runtime_error("nao foi possivel abrir: " + path);
    FastScanner sc(f);
    if (!sc.nextInt(n)) {
        fclose(f);
        throw std::runtime_error("arquivo vazio: " + path);
    }
    if (n <= 0) {
        fclose(f);
        throw std::runtime_error("n invalido no arquivo");
    }
    deg.assign(n, 0);
    m = 0;
    int u, v;
    while (true) {
        if (!sc.nextInt(u))
            break; // fim normal (numero par de inteiros apos n)
        if (!sc.nextInt(v)) {
            fclose(f);
            throw std::runtime_error("arquivo malformado: aresta incompleta");
        }
        if (u < 1 || u > n || v < 1 || v > n) {
            fclose(f);
            throw std::runtime_error("vertice fora do intervalo 1..n no arquivo");
        }
        int a = u - 1, b = v - 1;
        if (a == b) {
            deg[a] += 1;
        } else {
            deg[a] += 1;
            deg[b] += 1;
        }
        ++m;
    }
    fclose(f);
}

static std::unique_ptr<Graph> loadCSR(const std::string &path) {
    int n;
    long long m;
    std::vector<int> deg;
    countDegrees(path, n, m, deg);

    std::vector<int> offsets(n + 1, 0);
    for (int i = 0; i < n; ++i)
        offsets[i + 1] = offsets[i] + deg[i];
    std::vector<int> edges(offsets[n], 0);
    std::vector<int> cursor(offsets.begin(), offsets.begin() + n);

    // Passada 2: com os offsets prontos e o vetor no tamanho exato,
    // relemos o arquivo preenchendo os vizinhos. O `cursor` (copia dos
    // offsets) avanca a cada insercao, entao cada vertice escreve so
    // dentro da sua propria fatia.
    FILE *f = fopen(path.c_str(), "rb");
    if (!f)
        throw std::runtime_error("nao foi possivel reabrir: " + path);
    FastScanner sc(f);
    int first;
    sc.nextInt(first); // n, ja conhecido
    int u, v;
    while (sc.nextInt(u)) {
        if (!sc.nextInt(v)) {
            fclose(f);
            throw std::runtime_error("arquivo mudou entre passadas?");
        }
        int a = u - 1, b = v - 1;
        if (a == b) {
            edges[cursor[a]++] = b;
        } else {
            edges[cursor[a]++] = b;
            edges[cursor[b]++] = a;
        }
    }
    fclose(f);
    return std::unique_ptr<Graph>(new CSRGraph(n, std::move(offsets), std::move(edges), m));
}

static std::unique_ptr<Graph> loadBitMatrix(const std::string &path) {
    // A matriz so precisa de n para ser alocada, entao UMA passada basta:
    // lemos o n, alocamos e saimos marcando os bits. Se o n passar do
    // limite, o construtor lanca a excecao com a memoria necessaria
    // (o main transforma isso em LOAD_FAIL para o relatorio).
    FILE *f = fopen(path.c_str(), "rb");
    if (!f)
        throw std::runtime_error("nao foi possivel abrir: " + path);
    FastScanner sc(f);
    int n;
    if (!sc.nextInt(n)) {
        fclose(f);
        throw std::runtime_error("arquivo vazio: " + path);
    }
    // O construtor lanca runtime_error com a memoria necessaria se n > limite.
    BitMatrixGraph *g = nullptr;
    try {
        g = new BitMatrixGraph(n);
    } catch (...) {
        fclose(f);
        throw;
    }
    std::unique_ptr<BitMatrixGraph> mat(g);
    int u, v;
    while (sc.nextInt(u)) {
        if (!sc.nextInt(v)) {
            fclose(f);
            throw std::runtime_error("arquivo malformado: aresta incompleta");
        }
        if (u < 1 || u > n || v < 1 || v > n) {
            fclose(f);
            throw std::runtime_error("vertice fora do intervalo 1..n no arquivo");
        }
        mat->addEdge(u - 1, v - 1);
    }
    fclose(f);
    return std::unique_ptr<Graph>(mat.release());
}

std::unique_ptr<Graph> loadGraph(const std::string &path, const std::string &rep) {
    if (rep == "lista")
        return loadCSR(path);
    if (rep == "matriz")
        return loadBitMatrix(path);
    throw std::invalid_argument("representacao desconhecida: use lista|matriz");
}

double peakMemoryMB() {
    // Pico (VmHWM) em /proc/self/status; cai para getrusage se indisponivel.
    FILE *f = fopen("/proc/self/status", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            long kb = 0;
            if (sscanf(line, "VmHWM: %ld kB", &kb) == 1) {
                fclose(f);
                return (double)kb / 1024.0;
            }
        }
        fclose(f);
    }
    return -1.0;
}
