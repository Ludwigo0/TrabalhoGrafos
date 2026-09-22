# Roteiro de fala — Apresentação P1 (~7–8 minutos)

Divisão recomendada: **Carlos Bruno** (slides 1–8, arquitetura e representações)
→ **Arthur Cury** (slides 9–13, escala e resultados). A troca acontece depois
da explicação da matriz em bits. Os três slides com código são trechos reduzidos
dos arquivos reais; a função deles é permitir que o professor veja a decisão
estrutural, não acompanhar cada linha durante a apresentação.

## Slide 1 — Título (Carlos, 20 s)

> "Boa tarde. Somos Carlos Bruno e Arthur Cury. No Trabalho 1 construímos uma
> biblioteca de grafos em C++: um conjunto de componentes reutilizáveis para
> representar grafos e executar os algoritmos pedidos. O código, o relatório e
> os resultados estão neste repositório. Vamos focar nas decisões de
> implementação e nos resultados que elas produziram."

## Slide 2 — Biblioteca, não programa (Carlos, 35 s)

> "O enunciado pedia uma biblioteca reutilizável, então não concentramos tudo
> no `main`. A `libgrafo` contém as representações e os algoritmos; o programa
> `grafo` apenas interpreta os argumentos, chama a biblioteca, cronometra e
> escreve os resultados. Assim, outro programa poderia incluir nossos cabeçalhos
> e linkar a biblioteca sem copiar a implementação."

## Slide 3 — Organização das classes (Carlos, 45 s)

> "O núcleo é a classe abstrata `Graph`, que funciona como um contrato: qualquer
> representação precisa informar o número de vértices, o grau e o k-ésimo
> vizinho. `CSRGraph` e `BitMatrixGraph` implementam esse contrato. Na chamada,
> o programa sempre enxerga um `Graph`; as funções `*Auto` identificam se o
> objeto é CSR ou matriz de bits e selecionam o tratamento adequado. A matriz
> precisa de uma otimização especial porque a interface genérica acessa vizinhos
> individualmente. A interface e o resultado continuam iguais; esse detalhe
> fica encapsulado na biblioteca."

## Slide 4 — Código da interface `Graph` (Carlos, 25 s)

> "Este é o contrato no código. Os métodos são virtuais puros, indicados por
> `= 0`: a classe base não decide como encontrar um vizinho. Ela só define a
> operação que os algoritmos precisam. Uma busca pode pedir grau e vizinhos
> através de `Graph&`, sem conhecer os vetores ou bits que estão por trás.
> As funções `*Auto` fazem a seleção interna entre CSR e matriz; a matriz recebe
> um tratamento especial de otimização, mas isso não muda a interface usada pelo
> programa. O destrutor virtual completa o uso seguro da biblioteca."

## Slide 5 — Por que CSR (Carlos, 40 s)

> "Na lista de adjacência, cada vértice guarda apenas os seus vizinhos. O
> `vector<vector<int>>` seria simples, mas cada vetor custa cerca de 24 bytes
> de administração; nos 4,8 milhões de vértices isso representa aproximadamente
> 115 MB antes das arestas. O CSR usa dois vetores corridos, tem tamanho exato,
> melhora a localidade de memória e chega a cerca de 373 MB no maior grafo.
> Como o trabalho lê o grafo uma vez e depois só consulta, a restrição de ser
> uma estrutura estática não é um problema."

## Slide 6 — Código da CSR (Carlos, 35 s)

> "Aqui aparece a ideia operacional. O vetor `edges` é único e contínuo: ele
> guarda as listas de vizinhos uma depois da outra. O vetor `offsets` funciona
> como um mapa dessas listas. No exemplo, o vértice 0 usa `edges[0..2)`, que
> contém `{1,2}`; o vértice 1 usa `edges[2..3)`, que contém `{0}`; e o vértice
> 2 usa `edges[3..5)`, que contém `{0,1}`. Então `offsets[v+1]-offsets[v]`
> dá o grau, e `edges[offsets[v]+k]` dá o k-ésimo vizinho. A leitura monta isso
> em duas passadas, sem guardar uma lista temporária de arestas."

## Slide 7 — Matriz em bits (Carlos, 35 s)

> "A matriz reserva uma posição para cada par de vértices, mesmo quando não há
> aresta. A escolha foi guardar um bit por posição: zero significa ausência e
> um significa presença. No grafo 2, isso ocupa cerca de 297 MB, contra 2,3 GB
> em bytes. O acesso é um pouco mais elaborado, mas esse detalhe fica dentro
> da classe; para a apresentação, o ponto principal é a economia de memória e
> o fato de a matriz deixar de caber a partir do grafo 3."

## Slide 8 — Código da matriz em bits (Carlos, 35 s)

> "Aqui comparamos os dois acessos. Na matriz clássica, cada posição ocupa um
> byte e podemos ler diretamente `matriz[u*n+v]`. Na nossa matriz, a mesma
> posição é um bit: `p/64` encontra o bloco de 64 bits e `p%64` encontra a
> posição dentro dele. Isso reduz a memória em oito vezes, mas exige algumas
> operações extras. Por esse motivo a classe `BitMatrixGraph` trata o acesso
> internamente, sem mudar a interface `Graph` usada pelo restante do programa."

## Slide 9 — Detalhes de escala (Arthur, 35 s)

> "Os arquivos grandes exigiram decisões adicionais. O grafo 6 tem cerca de
> 93 milhões de inteiros, então usamos um leitor por blocos com `fread`. BFS e
> DFS são iterativas, porque a DFS recursiva poderia estourar a pilha. O tempo
> medido exclui leitura e escrita, como pede o enunciado, e a memória de pico é
> coletada pelo próprio processo. Para o diâmetro, o exato exigiria 4,8 milhões
> de BFSs no grafo 6, mais de 30 dias na taxa medida; por isso também implementamos
> o double-sweep, que usa apenas duas BFSs. A ideia é curta: começamos em um
> vértice qualquer, escolhemos o mais distante `a`, fazemos outra BFS a partir
> de `a` e usamos a distância até o novo mais distante `b` como estimativa."

## Slide 10 — Tabela de memória (Arthur, 35 s)

> "A lista usa de 6 MB no grafo 1 a 415 MB no grafo 6, e todos os grafos cabem.
> A matriz de bits usa 16,8 MB no grafo 1 e 302 MB no grafo 2. A partir do grafo
> 3 ela exigiria cerca de 17,6 GB; nos grafos 5 e 6, aproximadamente 2,93 TB.
> Portanto, OOM nos grafos maiores não é uma funcionalidade omitida: é o resultado
> teórico e experimental de uma representação densa incompatível com o hardware."

## Slide 11 — Tabela de tempos em milissegundos (Arthur, 35 s)

> "Esta é a média de 100 buscas; todos os valores da tabela são milissegundos
> por busca, separando o tempo do algoritmo do tempo de I/O.
> Na lista, os tempos crescem de menos de 1 milissegundo até algumas centenas
> de milissegundos. Na matriz, medimos 4,9 ms no grafo 1 e 28,5 ms no grafo 2
> para BFS, contra 0,8 e 2,6 ms na lista. A diferença é aproximadamente 6 vezes
> e 11 vezes. A causa é estrutural: lista percorre `O(n+m)`; matriz precisa
> examinar `O(n^2)` por busca."

## Slide 12 — Achados nos dados (Arthur, 35 s)

> "Os grafos também revelaram algumas características importantes. Eles são
> desconexos: o grafo 2 tem dez componentes, e os grafos 5 e 6 têm cinco; por
> isso algumas distâncias retornam menos um. Encontramos ainda arestas repetidas
> em ordem invertida, o que explica pequenas diferenças no grau médio: 0,047%
> no grafo 1 e 0,078% no grafo 2. Isso não altera as conclusões. Finalmente, os níveis das BFS coincidem nas
> duas representações, `CSRGraph` e `BitMatrixGraph`; os pais podem mudar quando
> há mais de um predecessor possível, porque a ordem de visita define qual é
> escolhido."

## Slide 13 — Conclusão (Arthur, 25 s)

> "A principal conclusão é que a matriz é útil para a comparação nos dois
> primeiros grafos, mas deixa de ser viável a partir do terceiro; a CSR é a
> representação que permite tratar todos os dados. A separação entre biblioteca
> e cliente, junto com as vias rápidas da matriz, torna o resultado reproduzível.
> Aqui, vias rápidas são rotinas internas específicas da matriz: elas evitam
> repetir trabalho causado pelo acesso genérico, sem mudar a interface nem o
> resultado dos algoritmos. O código, o relatório e os resultados estão no
> repositório."

## Slide 14 — Obrigado! (Arthur, 5 s)

> "Obrigado pela atenção."

## Dicas gerais

- Ensaiem para terminar entre 7 e 7 minutos e 30 segundos, deixando margem até
  os 8 minutos. Os slides 4, 6 e 8 devem ser explicados, não lidos linha por linha.
- Carlos deve usar o slide 4 para conectar a arquitetura ao código e os slides 6
  e 8 para explicar os índices; isso demonstra implementação, não apenas teoria.
- Arthur deve explicar “vias rápidas” apenas no slide 13: são rotinas internas
  específicas da matriz, que evitam trabalho repetido sem mudar a interface.
- Levem `docs/apresentacao/slides.pdf` no pendrive e mantenham o link do GitHub
  disponível como alternativa.
