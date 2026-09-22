// Trabalho P1 - Teoria dos Grafos (COS 242)
// ---------------------------------------------------------------
// Implementacao dos algoritmos. Observacao geral: BFS, DFS, componentes e
// estatisticas usam so degree()/neighbor(), por isso o mesmo codigo serve
// para a lista CSR e para a matriz de bits.

#include "algorithms.hpp"
#include "Graph.hpp"
#include <algorithm>
#include <numeric>
#include <utility>
#include <vector>

BFSResult bfs(const Graph &g, int src) {
    int n = g.numVertices();
    BFSResult r;
    r.parent.assign(n, -1);
    r.level.assign(n, -1);
    // `seen` e char (nao vector<bool>, que e lento por ser compactado).
    // A fila e um vector com indice de cabeca: evita o overhead de std::queue
    // nos grafos de milhoes de vertices.
    std::vector<char> seen(n, 0);
    // Marcamos como visitado NA HORA DE ENFILEIRAR (nao ao desenfileirar):
    // assim cada vertice entra na fila uma unica vez, mesmo com varias
    // arestas chegando nele. O pai e o nivel sao fixados nesse momento,
    // o que garante que o nivel e a distancia minima (propriedade da BFS).
    std::vector<int> q;
    q.reserve(n);
    q.push_back(src);
    seen[src] = 1;
    r.level[src] = 0;
    size_t head = 0;
    while (head < q.size()) {
        int v = q[head++];
        int deg = g.degree(v);
        for (int k = 0; k < deg; ++k) {
            int to = g.neighbor(v, k);
            if (!seen[to]) {
                seen[to] = 1;
                r.parent[to] = v;
                r.level[to] = r.level[v] + 1;
                q.push_back(to);
            }
        }
    }
    return r;
}

BFSResult bfs(const BitMatrixGraph &g, int src) {
    // Identica a generica em contrato e resultado; a diferenca e que cada
    // linha e varrida uma unica vez via forEachNeighbor, em vez de uma
    // varredura por vizinho via neighbor().
    int n = g.numVertices();
    BFSResult r;
    r.parent.assign(n, -1);
    r.level.assign(n, -1);
    std::vector<char> seen(n, 0);
    std::vector<int> q;
    q.reserve(n);
    q.push_back(src);
    seen[src] = 1;
    r.level[src] = 0;
    size_t head = 0;
    while (head < q.size()) {
        int v = q[head++];
        g.forEachNeighbor(v, [&](int to) {
            if (!seen[to]) {
                seen[to] = 1;
                r.parent[to] = v;
                r.level[to] = r.level[v] + 1;
                q.push_back(to);
            }
            return true;
        });
    }
    return r;
}

BFSResult bfsAuto(const Graph &g, int src) {
    // dynamic_cast e barato (uma vez por busca) e escolhe a via rapida
    // sem expor a representacao para o chamador.
    if (const auto *m = dynamic_cast<const BitMatrixGraph *>(&g))
        return bfs(*m, src);
    return bfs(g, src);
}

DFSResult dfs(const Graph &g, int src) {
    int n = g.numVertices();
    DFSResult r;
    r.parent.assign(n, -1);
    r.level.assign(n, -1);
    std::vector<char> seen(n, 0);
    // A pilha guarda (vertice, proximo vizinho a tentar). Quando voltamos
    // ao topo depois de explorar um ramo, continuamos de onde paramos
    // (indice k) - e exatamente o que a recursao faria, mas sem o risco
    // de estourar a pilha de chamadas nos grafos grandes.
    std::vector<std::pair<int, int>> st;
    st.reserve(n);
    st.emplace_back(src, 0);
    seen[src] = 1;
    r.level[src] = 0;
    while (!st.empty()) {
        int v = st.back().first;
        int &k = st.back().second;
        int deg = g.degree(v);
        if (k < deg) {
            int to = g.neighbor(v, k);
            ++k;
            if (!seen[to]) {
                seen[to] = 1;
                r.parent[to] = v;
                r.level[to] = r.level[v] + 1;
                st.emplace_back(to, 0);
            }
        } else {
            st.pop_back();
        }
    }
    return r;
}

DFSResult dfs(const BitMatrixGraph &g, int src) {
    // Mesma DFS em contrato; a diferenca e que os vizinhos de cada vertice
    // sao coletados UMA vez na descoberta (forEachNeighbor) e depois
    // percorridos por indice. Sem isso, cada aresta custaria uma varredura
    // da linha desde o inicio.
    int n = g.numVertices();
    DFSResult r;
    r.parent.assign(n, -1);
    r.level.assign(n, -1);
    std::vector<char> seen(n, 0);
    struct Frame {
        int v;
        std::vector<int> nbrs;
        size_t idx = 0;
    };
    std::vector<Frame> st;
    st.reserve(n);
    Frame root;
    root.v = src;
    g.forEachNeighbor(src, [&](int to) {
        root.nbrs.push_back(to);
        return true;
    });
    st.push_back(std::move(root));
    seen[src] = 1;
    r.level[src] = 0;
    while (!st.empty()) {
        // ATENCAO: nao guardar referencia para o topo entre push_backs, pois
        // o vector pode realocar e invalidar a referencia. Reobtemos o topo
        // por indice a cada passo.
        size_t top = st.size() - 1;
        if (st[top].idx < st[top].nbrs.size()) {
            int to = st[top].nbrs[st[top].idx++];
            if (!seen[to]) {
                seen[to] = 1;
                r.parent[to] = st[top].v;
                r.level[to] = r.level[st[top].v] + 1;
                Frame nf;
                nf.v = to;
                g.forEachNeighbor(to, [&](int w) {
                    nf.nbrs.push_back(w);
                    return true;
                });
                st.push_back(std::move(nf));
            }
        } else {
            st.pop_back();
        }
    }
    return r;
}

DFSResult dfsAuto(const Graph &g, int src) {
    if (const auto *m = dynamic_cast<const BitMatrixGraph *>(&g))
        return dfs(*m, src);
    return dfs(g, src);
}

int distanceBFS(const Graph &g, int u, int v) {
    if (u == v)
        return 0;
    // BFS normal, mas retorna assim que `v` e alcancado: nao ha motivo para
    // explorar o resto do grafo se so queremos a distancia desse par.
    int n = g.numVertices();
    std::vector<int> level(n, -1);
    std::vector<char> seen(n, 0);
    std::vector<int> q;
    q.reserve(n);
    q.push_back(u);
    seen[u] = 1;
    level[u] = 0;
    size_t head = 0;
    while (head < q.size()) {
        int cur = q[head++];
        int deg = g.degree(cur);
        for (int k = 0; k < deg; ++k) {
            int to = g.neighbor(cur, k);
            if (!seen[to]) {
                seen[to] = 1;
                level[to] = level[cur] + 1;
                if (to == v)
                    return level[to];
                q.push_back(to);
            }
        }
    }
    return -1;
}

int distanceBFS(const BitMatrixGraph &g, int u, int v) {
    // Igual a generica, mas a varredura por linha permite parar no meio da
    // linha (basta o callback retornar false) assim que `v` aparece.
    if (u == v)
        return 0;
    int n = g.numVertices();
    std::vector<int> level(n, -1);
    std::vector<char> seen(n, 0);
    std::vector<int> q;
    q.reserve(n);
    q.push_back(u);
    seen[u] = 1;
    level[u] = 0;
    size_t head = 0;
    int found = -1;
    while (head < q.size() && found < 0) {
        int cur = q[head++];
        g.forEachNeighbor(cur, [&](int to) {
            if (!seen[to]) {
                seen[to] = 1;
                level[to] = level[cur] + 1;
                if (to == v) {
                    found = level[to];
                    return false;
                }
                q.push_back(to);
            }
            return true;
        });
    }
    return found;
}

int distanceAuto(const Graph &g, int u, int v) {
    if (const auto *m = dynamic_cast<const BitMatrixGraph *>(&g))
        return distanceBFS(*m, u, v);
    return distanceBFS(g, u, v);
}

DiameterResult diameterExact(const Graph &g) {
    // Forca bruta por definicao: o diametro e o maior caminho minimo,
    // entao rode a BFS de CADA vertice e fique com o maior nivel visto.
    // Funciona, mas custa O(n*(n+m)) - por isso so usamos nos pequenos.
    int n = g.numVertices();
    DiameterResult best{0, 0, 0};
    for (int s = 0; s < n; ++s) {
        BFSResult r = bfsAuto(g, s);
        for (int v = 0; v < n; ++v) {
            if (r.level[v] > best.value) {
                best.value = r.level[v];
                best.a = s;
                best.b = v;
            }
        }
    }
    return best;
}

static int farthest(const std::vector<int> &level) {
    int arg = 0;
    for (int i = 1; i < (int)level.size(); ++i)
        if (level[i] > level[arg])
            arg = i;
    return arg;
}

DiameterResult diameterApproxDoubleSweep(const Graph &g, int start) {
    // Ideia: as pontas do diametro ficam "longe de todo mundo", entao uma
    // BFS de qualquer ponto tende a cair perto de uma ponta. Rodando a
    // segunda BFS dessa ponta, chegamos na outra - e a distancia entre elas
    // e uma boa estimativa (sempre <= o valor real).
    BFSResult r1 = bfsAuto(g, start);
    int a = farthest(r1.level);
    BFSResult r2 = bfsAuto(g, a);
    int b = farthest(r2.level);
    int value = r2.level[b] < 0 ? 0 : r2.level[b];
    return DiameterResult{value, a, b};
}

// Segunda metade do algoritmo de componentes, comum as duas versoes: monta
// os membros, ordena por tamanho decrescente e renumera os ids. Ficou
// separada para nao duplicar essa logica na versao da matriz.
static void finishComponents(Components &c, int cid);

Components connectedComponents(const Graph &g) {
    // Rotulagem classica: para cada vertice ainda sem rotulo, uma BFS pinta
    // todos os alcancaveis com um id novo. Cada BFS encontra exatamente uma
    // componente. No final, ordenamos por tamanho decrescente (o enunciado
    // pede a maior primeiro) e refazemos os ids para acompanhar a ordem.
    int n = g.numVertices();
    Components c;
    c.compOf.assign(n, -1);
    std::vector<int> q;
    q.reserve(n);
    int cid = 0;
    for (int s = 0; s < n; ++s) {
        if (c.compOf[s] != -1)
            continue;
        q.clear();
        q.push_back(s);
        c.compOf[s] = cid;
        size_t head = 0;
        while (head < q.size()) {
            int v = q[head++];
            int deg = g.degree(v);
            for (int k = 0; k < deg; ++k) {
                int to = g.neighbor(v, k);
                if (c.compOf[to] == -1) {
                    c.compOf[to] = cid;
                    q.push_back(to);
                }
            }
        }
        ++cid;
    }
    finishComponents(c, cid);
    return c;
}

static void finishComponents(Components &c, int cid) {
    int n = (int)c.compOf.size();
    c.members.assign(cid, {});
    for (int v = 0; v < n; ++v)
        c.members[c.compOf[v]].push_back(v);
    // Ordena as componentes por tamanho decrescente. O stable_sort com
    // desempate pelo menor vertice deixa a saida deterministica: rodando
    // duas vezes, o arquivo sai identico.
    std::vector<int> order(cid);
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](int x, int y) {
        return c.members[x].size() > c.members[y].size();
    });
    std::vector<std::vector<int>> sorted;
    sorted.reserve(cid);
    std::vector<int> remap(cid, -1);
    for (int i = 0; i < cid; ++i) {
        sorted.push_back(std::move(c.members[order[i]]));
        remap[order[i]] = i;
    }
    c.members = std::move(sorted);
    for (int v = 0; v < n; ++v)
        c.compOf[v] = remap[c.compOf[v]];
}

Components connectedComponents(const BitMatrixGraph &g) {
    // Mesma rotulagem da versao generica, mas percorrendo vizinhos com
    // forEachNeighbor (uma varredura por linha).
    int n = g.numVertices();
    Components c;
    c.compOf.assign(n, -1);
    std::vector<int> q;
    q.reserve(n);
    int cid = 0;
    for (int s = 0; s < n; ++s) {
        if (c.compOf[s] != -1)
            continue;
        q.clear();
        q.push_back(s);
        c.compOf[s] = cid;
        size_t head = 0;
        while (head < q.size()) {
            int v = q[head++];
            g.forEachNeighbor(v, [&](int to) {
                if (c.compOf[to] == -1) {
                    c.compOf[to] = cid;
                    q.push_back(to);
                }
                return true;
            });
        }
        ++cid;
    }
    finishComponents(c, cid);
    return c;
}

Components componentsAuto(const Graph &g) {
    if (const auto *m = dynamic_cast<const BitMatrixGraph *>(&g))
        return connectedComponents(*m);
    return connectedComponents(g);
}

Stats computeStats(const Graph &g) {
    int n = g.numVertices();
    Stats s{};
    s.n = n;
    s.m = g.numEdges();
    if (n == 0) {
        s.gmin = 0;
        s.gmax = 0;
        s.gavg = 0;
        s.gmed = 0;
        return s;
    }
    std::vector<int> degs(n);
    long long sum = 0;
    s.gmin = g.degree(0);
    s.gmax = g.degree(0);
    for (int v = 0; v < n; ++v) {
        int d = g.degree(v);
        degs[v] = d;
        sum += d;
        if (d < s.gmin)
            s.gmin = d;
        if (d > s.gmax)
            s.gmax = d;
    }
    s.gavg = (double)sum / (double)n;
    // Mediana sem ordenar tudo: nth_element coloca o elemento da posicao
    // pedida no lugar certo em tempo linear medio. Com n par, tiramos a
    // media dos dois do meio (definicao classica de mediana).
    if (n % 2 == 1) {
        std::nth_element(degs.begin(), degs.begin() + n / 2, degs.end());
        s.gmed = (double)degs[n / 2];
    } else {
        std::nth_element(degs.begin(), degs.begin() + n / 2, degs.end());
        int hi = degs[n / 2];
        std::nth_element(degs.begin(), degs.begin() + n / 2 - 1, degs.end());
        int lo = degs[n / 2 - 1];
        s.gmed = ((double)lo + (double)hi) / 2.0;
    }
    return s;
}
