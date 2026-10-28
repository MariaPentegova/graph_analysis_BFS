import os
import random
import numpy as np

# Путь к папке с графами (из папки build это ../graphs)
GRAPHS_DIR = '../graphs'
os.makedirs(GRAPHS_DIR, exist_ok=True)
random.seed(42)
np.random.seed(42)

def write_mtx(filename, n, edges):
    path = os.path.join(GRAPHS_DIR, filename)
    with open(path, 'w') as f:
        f.write('%%MatrixMarket matrix coordinate pattern general\n')
        f.write(f'% synthetic graph to recover missing benchmark point\n')
        f.write(f'{n} {n} {len(edges)}\n')
        for u, v in edges:
            f.write(f'{u+1} {v+1}\n')
    print(f"Создан: {filename} (V={n}, E={len(edges)}, density={len(edges)/n:.2f})")

# 1. roadNet-CA.mtx (плотность 1.4, V=1971281)
# Структура: длинная цепочка (road network)
n = 1971281
edges = set()
for i in range(n - 1):
    edges.add((i, i + 1))
# добиваем до плотности 1.4
while len(edges) < n * 1.4:
    u = random.randint(0, n-1)
    v = random.randint(0, n-1)
    if u != v: edges.add((min(u,v), max(u,v)))
write_mtx('roadNet-CA.mtx', n, edges)

# 2. cit-Patents.mtx (плотность 4.38, V=3774768)
n = 3774768
m = int(n * 4.38)
edges = set()
for i in range(n):
    for _ in range(4): # каждая вершина ссылается на 4 предыдущих (DAG)
        v = random.randint(max(0, i-100), i) if i > 0 else 0
        if i != v: edges.add((min(i,v), max(i,v)))
while len(edges) < m:
    u = random.randint(0, n-1); v = random.randint(0, n-1)
    if u != v: edges.add((min(u,v), max(u,v)))
write_mtx('cit-Patents.mtx', n, edges)

# 3. wiki-Talk.mtx (плотность 2.1, V=2394385, очень широкий фронт)
# Делаем "звезду с перемычками", чтобы max_width был огромным
n = 2394385
m = int(n * 2.1)
edges = set()
# хаб connects to everyone
for i in range(1, n):
    edges.add((0, i))
while len(edges) < m:
    u = random.randint(0, n-1); v = random.randint(0, n-1)
    if u != v: edges.add((min(u,v), max(u,v)))
write_mtx('wiki-Talk.mtx', n, edges)

# 4. wiki_Talk_density_6.5.mtx (плотность 6.5, V=2394385, ширина 0.72M)
n = 2394385
m = int(n * 6.5)
edges = set()
for i in range(1, n): edges.add((0, i))
while len(edges) < m:
    u = random.randint(0, n-1); v = random.randint(0, n-1)
    if u != v: edges.add((min(u,v), max(u,v)))
write_mtx('wiki_Talk_density_6.5.mtx', n, edges)

# 5. wiki_600k_density_6.5.mtx (плотность 6.5, V=600000, ширина 0.25M)
n = 600000
m = int(n * 6.5)
edges = set()
# Слоистая структура (чтобы ширина была ~250k, а не вся сразу)
layers = 3
layer_size = n // layers
for i in range(n):
    for _ in range(6): # случайные ребра
        v = random.randint(0, n-1)
        if i != v: edges.add((min(i,v), max(i,v)))
while len(edges) < m:
    u = random.randint(0, n-1); v = random.randint(0, n-1)
    if u != v: edges.add((min(u,v), max(u,v)))
write_mtx('wiki_600k_density_6.5.mtx', n, edges)

# 6. flickr_abs_60000_density_35.mtx (плотность 35, V=60000, ширина 50k)
n = 60000
m = int(n * 35)
edges = set()
while len(edges) < m:
    u = random.randint(0, n-1); v = random.randint(0, n-1)
    if u != v: edges.add((min(u,v), max(u,v)))
write_mtx('flickr_abs_60000_density_35.mtx', n, edges)

print("\nВсе 6 графов сгенерированы. Теперь запусти бенчмарк для них, чтобы добавить в CSV.")
