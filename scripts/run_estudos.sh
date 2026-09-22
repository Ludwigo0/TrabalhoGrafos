#!/bin/bash
# Roteiro dos estudos de caso do PDF. Gera resultados.csv com uma linha por
# (grafo, representacao) e colunas para cada pergunta do estudo.
#
# Uso: ./scripts/run_estudos.sh [DIR_DADOS] [DIR_SAIDA]
#   DIR_DADOS: onde estao grafo_1.txt.gz .. grafo_6.txt.gz (padrao: .)
#   DIR_SAIDA: onde gravar resultados (padrao: ./out_estudos)
#
# Binario esperado em ./build/grafo (compile antes com cmake).
# Atencao: os .gz sao descompactados em $TMPDIR (grafo_6 precisa de ~1 GB
# livre). Matriz e tentada nos grafos 1 e 2; do 3 em diante registra-se OOM
# teorico sem tentar alocar (ver memoria_necessaria()).

set -u
BIN="${BIN:-./build/grafo}"
DATA="${1:-.}"
OUT="${2:-./out_estudos}"
BENCH_K="${BENCH_K:-100}"
mkdir -p "$OUT"

if [ ! -x "$BIN" ]; then
  echo "Binario nao encontrado: $BIN (compile com cmake primeiro)" >&2
  exit 1
fi

# Memoria teorica da matriz de bits em GB: n^2/8 bytes.
mem_matriz_gb() {
  python3 -c "n=$1; print(f'{(n*n/8/1e9):.2f}')"
}

header="grafo,rep,n,m,mem_carga_mb,bench_bfs_ms,bench_dfs_ms,pai10_bfs1,pai20_bfs1,pai30_bfs1,pai10_dfs1,pai20_dfs1,pai30_dfs1,dist_10_20,dist_10_30,dist_20_30,ncomp,maior,menor,diametro_aprox,obs"
echo "$header" > "$OUT/resultados.csv"

# Extrai "chave=valor" da saida do programa.
get() { echo "$2" | grep -o "$1=[^ ]*" | head -1 | cut -d= -f2; }

for i in 1 2 3 4 5 6; do
  gz="$DATA/grafo_$i.txt.gz"
  [ -f "$gz" ] || { echo "pulando grafo_$i (sem $gz)"; continue; }
  n=$(zcat "$gz" | head -1 | tr -d ' \r\n')
  echo "== grafo_$i (n=$n) =="
  txt="$OUT/grafo_$i.txt"
  echo "  descompactando..."
  zcat "$gz" > "$txt"

  for rep in lista matriz; do
    if [ "$rep" = "matriz" ] && [ "$i" -ge 3 ]; then
      mem=$(mem_matriz_gb "$n")
      echo "  [$rep] OOM teorico: precisaria de ~${mem} GB so para a matriz"
      echo "grafo_$i,$rep,$n,,OOM-teorico-${mem}GB,,,,,,,,,,,,,,," >> "$OUT/resultados.csv"
      continue
    fi
    wdir="$OUT/grafo_${i}_${rep}"
    mkdir -p "$wdir"
    echo "  [$rep] rodada completa..."
    full=$("$BIN" "$txt" --rep "$rep" --out "$wdir" --stats --components \
      --bfs 1 --dfs 1 \
      --bfs 2 --bfs 3 --dfs 2 --dfs 3 \
      --dist 10,20 --dist 10,30 --dist 20,30 \
      --diameter-approx 1 \
      --benchmark-bfs "$BENCH_K" --benchmark-dfs "$BENCH_K" 2>"$wdir/stderr.log") || {
      echo "  [$rep] FALHA (ver $wdir/stderr.log)"
      echo "grafo_$i,$rep,$n,,,,,,,,,,,,,,,,,LOAD_FAIL" >> "$OUT/resultados.csv"
      continue
    }
    echo "$full" > "$wdir/saida.log"
    m=$(get m "$full"); mem=$(get peak_mb "$full")
    bb=$(echo "$full" | grep bench_bfs | tail -1 | sed 's/.*avg_ms=//')
    bd=$(echo "$full" | grep bench_dfs | tail -1 | sed 's/.*avg_ms=//')
    # Pais dos vertices 10/20/30 nas arvores de 1/2/3: coluna 2 do arquivo.
    pai() { awk -v v="$2" '$1==v{print $2}' "$wdir/$1_$3.txt"; }
    p10b=$(pai bfs 10 1); p20b=$(pai bfs 20 1); p30b=$(pai bfs 30 1)
    p10d=$(pai dfs 10 1); p20d=$(pai dfs 20 1); p30d=$(pai dfs 30 1)
    d12=$(echo "$full" | grep "dist u=10 v=20" | sed 's/.*d=//;s/ .*//')
    d13=$(echo "$full" | grep "dist u=10 v=30" | sed 's/.*d=//;s/ .*//')
    d23=$(echo "$full" | grep "dist u=20 v=30" | sed 's/.*d=//;s/ .*//')
    nc=$(get count "$full"); maior=$(get largest "$full"); menor=$(get smallest "$full")
    diam=$(echo "$full" | grep "diameter mode=approx" | sed 's/.*value=//;s/ .*//')
    echo "grafo_$i,$rep,$n,$m,$mem,$bb,$bd,$p10b,$p20b,$p30b,$p10d,$p20d,$p30d,$d12,$d13,$d23,$nc,$maior,$menor,$diam," \
      >> "$OUT/resultados.csv"
  done
  rm -f "$txt" # libera ~1 GB no caso do grafo_6
done
echo "Pronto: $OUT/resultados.csv"
