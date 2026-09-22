// Trabalho P1 - Teoria dos Grafos (COS 242)
// ---------------------------------------------------------------
// Programa-cliente da biblioteca: e o "painel de botoes" que o corretor
// e o script de experimentos usam. Toda a logica de grafos mora na
// biblioteca (include/ + algoritmos); aqui so ha leitura de argumentos,
// chamada das funcoes, cronometragem e escrita dos arquivos de saida.
//
// Convenções importantes:
// - Vertices na linha de comando usam a numeracao DO ARQUIVO (1..n);
//   a conversao para 0..n-1 e feita aqui (funcao checkV).
// - O tempo medido EXCLUI leitura do disco e escrita da saida, como pede
//   o enunciado: o cronometro envolve so a chamada do algoritmo.
// - A saida padrao usa linhas "chave=valor" para o script
//   (scripts/run_estudos.sh) coletar com grep e montar o CSV.

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "Graph.hpp"
#include "algorithms.hpp"
#include "io.hpp"

namespace fs = std::filesystem;

static void usage(const char *prog) {
    std::cerr << "Uso: " << prog << " <arquivo> --rep lista|matriz [--out DIR] [opcoes]\n"
              << "Opcoes:\n"
              << "  --bfs S            BFS a partir de S (1-indexado), grava bfs_S.txt\n"
              << "  --dfs S            DFS a partir de S (1-indexado), grava dfs_S.txt\n"
              << "  --dist U,V         distancia entre U e V via BFS\n"
              << "  --diameter-exact   diametro exato (n BFSs; so grafos pequenos)\n"
              << "  --diameter-approx [S]  double-sweep a partir de S (padrao 1)\n"
              << "  --stats            estatisticas de grau, grava stats.txt\n"
              << "  --components       componentes conexas, grava components.txt\n"
              << "  --benchmark-bfs K  K BFSs de origens distribuidas, tempo medio\n"
              << "  --benchmark-dfs K  idem para DFS\n";
}

static double nowSeconds() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

static void writeTree(const std::string &path, const std::vector<int> &parent,
                      const std::vector<int> &level) {
    // Formato pedido no enunciado: uma linha por vertice com "vertice pai
    // nivel", tudo 1-indexado. Pai -1 = raiz ou inalcançavel; nivel -1 =
    // inalcançavel (a raiz tem nivel 0).
    std::ofstream out(path);
    if (!out)
        throw std::runtime_error("nao foi possivel escrever: " + path);
    int n = (int)parent.size();
    for (int v = 0; v < n; ++v) {
        int p = parent[v] < 0 ? -1 : parent[v] + 1; // 1-indexado; -1 = sem pai
        out << (v + 1) << ' ' << p << ' ' << level[v] << '\n';
    }
}

int main(int argc, char **argv) {
    if (argc < 4) {
        usage(argv[0]);
        return 1;
    }
    std::string file = argv[1];
    std::string rep;
    std::string outDir = "./out";
    std::vector<int> bfsSrc, dfsSrc;
    std::vector<std::pair<int, int>> dists;
    bool diamExact = false;
    bool diamApprox = false;
    int diamStart = 1;
    bool doStats = false, doComp = false;
    int benchBfs = 0, benchDfs = 0;

    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];
        auto need = [&](int j) {
            if (j >= argc) {
                std::cerr << "Falta valor para " << a << "\n";
                std::exit(1);
            }
        };
        if (a == "--rep") {
            need(i + 1);
            rep = argv[++i];
        } else if (a == "--out") {
            need(i + 1);
            outDir = argv[++i];
        } else if (a == "--bfs") {
            need(i + 1);
            bfsSrc.push_back(std::atoi(argv[++i]));
        } else if (a == "--dfs") {
            need(i + 1);
            dfsSrc.push_back(std::atoi(argv[++i]));
        } else if (a == "--dist") {
            need(i + 1);
            std::string s = argv[++i];
            auto comma = s.find(',');
            if (comma == std::string::npos) {
                std::cerr << "--dist espera U,V\n";
                return 1;
            }
            dists.emplace_back(std::atoi(s.substr(0, comma).c_str()),
                               std::atoi(s.substr(comma + 1).c_str()));
        } else if (a == "--diameter-exact") {
            diamExact = true;
        } else if (a == "--diameter-approx") {
            diamApprox = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                diamStart = std::atoi(argv[++i]);
            }
        } else if (a == "--stats") {
            doStats = true;
        } else if (a == "--components") {
            doComp = true;
        } else if (a == "--benchmark-bfs") {
            need(i + 1);
            benchBfs = std::atoi(argv[++i]);
        } else if (a == "--benchmark-dfs") {
            need(i + 1);
            benchDfs = std::atoi(argv[++i]);
        } else {
            std::cerr << "Opcao desconhecida: " << a << "\n";
            usage(argv[0]);
            return 1;
        }
    }
    if (rep != "lista" && rep != "matriz") {
        std::cerr << "--rep deve ser lista|matriz\n";
        return 1;
    }

    std::unique_ptr<Graph> g;
    try {
        double t0 = nowSeconds();
        g = loadGraph(file, rep);
        double t1 = nowSeconds();
        double peak = peakMemoryMB();
        printf("graph file=%s n=%d m=%lld rep=%s load_s=%.3f peak_mb=%.1f\n", file.c_str(),
               g->numVertices(), g->numEdges(), rep.c_str(), t1 - t0, peak);
    } catch (const std::exception &e) {
        // Falha de carga (ex.: matriz grande demais) NAO e bug: imprimimos
        // LOAD_FAIL com o motivo e saimos com codigo 2, para o script de
        // experimentos registrar "OOM teorico" na tabela do relatorio.
        printf("graph file=%s rep=%s LOAD_FAIL: %s\n", file.c_str(), rep.c_str(), e.what());
        return 2;
    }

    int n = g->numVertices();
    auto checkV = [&](int s) {
        if (s < 1 || s > n) {
            std::cerr << "Vertice " << s << " fora de 1.." << n << "\n";
            std::exit(1);
        }
        return s - 1;
    };

    std::error_code ec;
    fs::create_directories(outDir, ec);
    if (ec) {
        std::cerr << "Nao foi possivel criar dir de saida: " << outDir << "\n";
        return 1;
    }

    if (doStats) {
        double t0 = nowSeconds();
        Stats s = computeStats(*g);
        double t1 = nowSeconds();
        printf("stats gmin=%d gmax=%d gavg=%.4f gmed=%.2f time_ms=%.1f\n", s.gmin, s.gmax,
               s.gavg, s.gmed, (t1 - t0) * 1000.0);
        std::ofstream out(outDir + "/stats.txt");
        out << "n " << s.n << "\nm " << s.m << "\ngmin " << s.gmin << "\ngmax " << s.gmax
            << "\ngavg " << s.gavg << "\ngmed " << s.gmed << "\n";
    }

    for (int s : bfsSrc) {
        int src = checkV(s);
        double t0 = nowSeconds();
        BFSResult r = bfsAuto(*g, src);
        double t1 = nowSeconds();
        printf("bfs src=%d time_ms=%.1f\n", s, (t1 - t0) * 1000.0);
        writeTree(outDir + "/bfs_" + std::to_string(s) + ".txt", r.parent, r.level);
    }
    for (int s : dfsSrc) {
        int src = checkV(s);
        double t0 = nowSeconds();
        DFSResult r = dfsAuto(*g, src);
        double t1 = nowSeconds();
        printf("dfs src=%d time_ms=%.1f\n", s, (t1 - t0) * 1000.0);
        writeTree(outDir + "/dfs_" + std::to_string(s) + ".txt", r.parent, r.level);
    }
    for (auto &p : dists) {
        int u = checkV(p.first), v = checkV(p.second);
        double t0 = nowSeconds();
        int d = distanceAuto(*g, u, v);
        double t1 = nowSeconds();
        printf("dist u=%d v=%d d=%d time_ms=%.1f\n", p.first, p.second, d,
               (t1 - t0) * 1000.0);
    }
    if (diamExact) {
        if (n > 50000)
            fprintf(stderr, "AVISO: diametro exato com n=%d pode levar muito tempo\n", n);
        double t0 = nowSeconds();
        DiameterResult d = diameterExact(*g);
        double t1 = nowSeconds();
        printf("diameter mode=exact value=%d a=%d b=%d time_ms=%.1f\n", d.value, d.a + 1,
               d.b + 1, (t1 - t0) * 1000.0);
    }
    if (diamApprox) {
        int s0 = checkV(diamStart);
        double t0 = nowSeconds();
        DiameterResult d = diameterApproxDoubleSweep(*g, s0);
        double t1 = nowSeconds();
        printf("diameter mode=approx value=%d a=%d b=%d time_ms=%.1f\n", d.value, d.a + 1,
               d.b + 1, (t1 - t0) * 1000.0);
    }
    if (doComp) {
        double t0 = nowSeconds();
        Components c = componentsAuto(*g);
        double t1 = nowSeconds();
        size_t largest = c.members.empty() ? 0 : c.members.front().size();
        size_t smallest = c.members.empty() ? 0 : c.members.back().size();
        printf("components count=%d largest=%d smallest=%d time_ms=%.1f\n",
               (int)c.members.size(), (int)largest, (int)smallest, (t1 - t0) * 1000.0);
        std::ofstream out(outDir + "/components.txt");
        out << c.members.size() << "\n";
        for (auto &mem : c.members) {
            out << mem.size();
            for (int v : mem)
                out << ' ' << (v + 1);
            out << '\n';
        }
    }
    if (benchBfs > 0) {
        // Benchmark do enunciado: K buscas de origens DIFERENTES e
        // distribuidas pelo grafo (i*n/K), para a media nao depender de
        // uma regiao so. O tempo de cada busca exclui I/O (so o algoritmo).
        int k = benchBfs > n ? n : benchBfs;
        double total = 0;
        for (int i = 0; i < k; ++i) {
            int src = (int)((long long)i * n / k); // origens distribuidas
            double t0 = nowSeconds();
            BFSResult r = bfsAuto(*g, src);
            (void)r;
            double t1 = nowSeconds();
            total += (t1 - t0) * 1000.0;
        }
        printf("bench_bfs k=%d avg_ms=%.2f\n", k, total / k);
    }
    if (benchDfs > 0) {
        int k = benchDfs > n ? n : benchDfs;
        double total = 0;
        for (int i = 0; i < k; ++i) {
            int src = (int)((long long)i * n / k);
            double t0 = nowSeconds();
            DFSResult r = dfsAuto(*g, src);
            (void)r;
            double t1 = nowSeconds();
            total += (t1 - t0) * 1000.0;
        }
        printf("bench_dfs k=%d avg_ms=%.2f\n", k, total / k);
    }
    return 0;
}
