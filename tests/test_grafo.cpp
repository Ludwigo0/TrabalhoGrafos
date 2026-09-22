// Trabalho P1 - Teoria dos Grafos (COS 242)
// ---------------------------------------------------------------
// Teste de fumaca (sem framework), em tres partes:
// 1) grafo-exemplo da Figura 1 do enunciado nas DUAS representacoes, com
//    valores calculados a mao (arestas 1-indexadas: 1-2, 2-5, 5-3, 4-5, 1-5);
// 2) vias rapidas da matriz (bfsAuto/dfsAuto/...): tem que dar o MESMO
//    resultado das genericas, vértice por vértice;
// 3) carga de arquivo de verdade (via loadGraph) num grafo DESCONEXO de
//    6 vertices, conferindo componentes, distancia -1 e a trava da matriz.
//
// Valores esperados da parte 1 (conferir no desenho do enunciado):
// - graus: 1:2, 2:2, 3:1, 4:1, 5:4 (media 2, mediana 2);
// - BFS da origem 1: nivel 0 = {1}, nivel 1 = {2,5}, nivel 2 = {3,4};
// - distancias: d(3,4) = 2 (3-5-4), d(1,3) = 2 (1-5-3);
// - 1 componente conexa com os 5 vertices; diametro exato = 2.
// A DFS nao tem gabarito fixo de pais (depende da ordem dos vizinhos),
// entao nela so checamos alcance total e coerencia pai/nivel.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "BitMatrixGraph.hpp"
#include "CSRGraph.hpp"
#include "Graph.hpp"
#include "algorithms.hpp"
#include "io.hpp"

static int failures = 0;
#define CHECK(cond, msg)                                                  \
    do {                                                                  \
        if (!(cond)) {                                                    \
            printf("FALHOU: %s (linha %d)\n", msg, __LINE__);             \
            ++failures;                                                   \
        }                                                                 \
    } while (0)

int main() {
    // Pares 0-indexados equivalentes ao exemplo do enunciado.
    std::vector<std::pair<int, int>> pairs = {{0, 1}, {1, 4}, {4, 2}, {3, 4}, {0, 4}};
    CSRGraph csr = CSRGraph::fromEdgeList(5, pairs);
    BitMatrixGraph mat(5);
    for (auto &e : pairs)
        mat.addEdge(e.first, e.second);

    const Graph *reps[2] = {&csr, &mat};
    const char *names[2] = {"CSR", "BitMatrix"};
    // Graus esperados: 1:2, 2:2, 3:1, 4:1, 5:4 (0-index: 0:2,1:2,2:1,3:1,4:4).
    int expDeg[5] = {2, 2, 1, 1, 4};
    for (int r = 0; r < 2; ++r) {
        const Graph &g = *reps[r];
        CHECK(g.numVertices() == 5, "n==5");
        CHECK(g.numEdges() == 5, "m==5");
        for (int v = 0; v < 5; ++v) {
            char msg[64];
            snprintf(msg, sizeof(msg), "%s grau(%d)", names[r], v + 1);
            CHECK(g.degree(v) == expDeg[v], msg);
        }
        // BFS a partir do vertice 1 (indice 0): niveis {0,1,1,2,2}.
        BFSResult b = bfs(g, 0);
        int expLvl[5] = {0, 1, 2, 2, 1};
        for (int v = 0; v < 5; ++v) {
            char msg[64];
            snprintf(msg, sizeof(msg), "%s bfs nivel(%d)", names[r], v + 1);
            CHECK(b.level[v] == expLvl[v], msg);
        }
        CHECK(b.parent[0] == -1, "bfs raiz sem pai");
        CHECK(b.parent[1] == 0 && b.parent[4] == 0, "bfs pais do nivel 1");
        // DFS: todos alcancados, raiz sem pai, niveis consistentes.
        DFSResult d = dfs(g, 0);
        for (int v = 0; v < 5; ++v) {
            char msg[64];
            snprintf(msg, sizeof(msg), "%s dfs alcanca %d", names[r], v + 1);
            CHECK(d.level[v] >= 0, msg);
        }
        CHECK(d.parent[0] == -1, "dfs raiz sem pai");
        for (int v = 1; v < 5; ++v) {
            char msg[64];
            snprintf(msg, sizeof(msg), "%s dfs nivel coerente %d", names[r], v + 1);
            CHECK(d.level[v] == d.level[d.parent[v]] + 1, msg);
        }
        // Distancias conhecidas: d(3,4)=2 (3-5-4), d(1,3)=2 (1-5-3).
        CHECK(distanceBFS(g, 2, 3) == 2, "dist 3-4");
        CHECK(distanceBFS(g, 0, 2) == 2, "dist 1-3");
        CHECK(distanceBFS(g, 0, 0) == 0, "dist igual");
        // Componentes: 1 so, tamanho 5.
        Components c = connectedComponents(g);
        CHECK(c.members.size() == 1, "1 componente");
        CHECK(c.members[0].size() == 5, "componente com 5");
        // Diametro exato = 2; aproximado >= 1 e <= exato.
        DiameterResult ex = diameterExact(g);
        CHECK(ex.value == 2, "diametro exato 2");
        DiameterResult ap = diameterApproxDoubleSweep(g, 0);
        CHECK(ap.value >= 1 && ap.value <= ex.value, "aprox entre 1 e exato");
        // Estatisticas.
        Stats s = computeStats(g);
        CHECK(s.gmin == 1 && s.gmax == 4, "gmin/gmax");
        CHECK(s.gavg == 2.0, "gavg");
        CHECK(s.gmed == 2.0, "gmed");
        // Vias rapidas tem que coincidir com as genericas em tudo.
        BFSResult bf = bfsAuto(g, 0);
        CHECK(bf.parent == b.parent && bf.level == b.level, "bfsAuto == bfs");
        DFSResult df = dfsAuto(g, 0);
        CHECK(df.parent == d.parent && df.level == d.level, "dfsAuto == dfs");
        CHECK(distanceAuto(g, 2, 3) == 2, "distanceAuto");
        Components ca = componentsAuto(g);
        CHECK(ca.members == c.members, "componentsAuto == components");
    }

    // Parte 3: carga de arquivo + grafo desconexo. Fixture (1-indexado):
    //   6 / 1-2 / 2-3 / 4-5   -> comps {1,2,3}, {4,5}, {6}; m = 3.
    const std::string tmp = "/tmp/opencode/teste_p1.txt";
    {
        std::ofstream out(tmp);
        out << "6\n1 2\n2 3\n4 5\n";
    }
    for (const char *rep : {"lista", "matriz"}) {
        std::unique_ptr<Graph> g(loadGraph(tmp, rep));
        CHECK(g->numVertices() == 6, "load n==6");
        CHECK(g->numEdges() == 3, "load m==3");
        CHECK(g->degree(0) == 1 && g->degree(1) == 2 && g->degree(5) == 0,
              "load graus");
        Components c = componentsAuto(*g);
        CHECK(c.members.size() == 3, "3 componentes");
        CHECK(c.members[0].size() == 3 && c.members[1].size() == 2 &&
                  c.members[2].size() == 1,
              "tamanhos 3,2,1 decrescentes");
        CHECK(distanceAuto(*g, 0, 2) == 2, "dist dentro da componente");
        CHECK(distanceAuto(*g, 0, 3) == -1, "dist entre componentes = -1");
        CHECK(distanceAuto(*g, 5, 5) == 0, "dist vertice isolado p/ si");
        BFSResult b = bfsAuto(*g, 0);
        CHECK(b.level[3] == -1 && b.parent[3] == -1, "bfs nao alcanca");
        DiameterResult ex = diameterExact(*g);
        CHECK(ex.value == 2, "diametro exato do desconexo = 2");
        DiameterResult ap = diameterApproxDoubleSweep(*g, 0);
        CHECK(ap.value >= 1 && ap.value <= ex.value, "aprox contido");
    }
    // Trava da matriz: n gigante sem nenhuma aresta deve falhar na carga
    // com a mensagem de memoria, antes de alocar qualquer coisa.
    {
        std::ofstream out(tmp);
        out << "375000\n";
    }
    bool falhou = false;
    try {
        std::unique_ptr<Graph> g(loadGraph(tmp, "matriz"));
    } catch (const std::runtime_error &) {
        falhou = true;
    }
    CHECK(falhou, "trava da matriz recusa n=375000");
    // Representacao invalida tambem deve falhar com mensagem clara.
    falhou = false;
    try {
        std::unique_ptr<Graph> g(loadGraph(tmp, "adjacencia"));
    } catch (const std::invalid_argument &) {
        falhou = true;
    }
    CHECK(falhou, "rep invalida rejeitada");

    if (failures == 0)
        printf("OK: todos os testes passaram (CSR + BitMatrix)\n");
    else
        printf("%d falha(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
