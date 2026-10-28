import random
import os

n = 400000
density = 12.0
m = int(n * density)
path = '../graphs/probe_target_n400k_d12.0_shub.mtx'

print(f"Генерация {path}...")
random.seed(42)
edges = set()
while len(edges) < m:
    u = random.randrange(n)
    v = random.randrange(n)
    if u != v:
        edges.add((min(u,v), max(u,v)))

with open(path, 'w') as f:
    f.write('%%MatrixMarket matrix coordinate pattern symmetric\n')
    f.write(f'{n} {n} {len(edges)}\n')
    for u, v in edges:
        f.write(f'{u+1} {v+1}\n')
print("Готово!")
