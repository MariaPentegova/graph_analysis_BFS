import random
import os

n = 500000
density = 14.0
m = int(n * density)
out_dir = '../graphs/probe'
os.makedirs(out_dir, exist_ok=True)
path = os.path.join(out_dir, 'probe_n500k_d14.0_wide.mtx')

random.seed(42)
edges = set()

# Генерируем случайный граф (Erdos-Renyi).
# Это даст нам достаточно широкий фронт, так как большинство вершин достижимы.
while len(edges) < m:
    u = random.randrange(n)
    v = random.randrange(n)
    if u != v:
        edges.add((min(u,v), max(u,v)))

with open(path, 'w') as f:
    f.write('%%MatrixMarket matrix coordinate pattern symmetric\n')
    f.write(f'% Generated probe for width ~260k, density 14\n')
    f.write(f'{n} {n} {len(edges)}\n')
    for u, v in edges:
        f.write(f'{u+1} {v+1}\n')

print(f"Создан {path}: вершин={n}, плотность={density}, рёбер={len(edges)}")
