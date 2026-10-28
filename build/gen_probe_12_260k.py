import random
import os

n = 500000
density = 12.0
m = int(n * density)
out_dir = '../graphs/probe'
os.makedirs(out_dir, exist_ok=True)
path = os.path.join(out_dir, 'probe_n500k_d12.0_w260k.mtx')

random.seed(42)
edges = set()

# 3 слоя по ~166k вершин.
# Чтобы ширина была ~260k, мы позволим рёбрам гулять внутри соседних слоёв.
# При плотности 12 граф очень плотный, поэтому чтобы ограничить ширину, 
# будем часто кидать рёбра внутри одного слоя.
layer_size = 166666

while len(edges) < m:
    u = random.randrange(n)
    v = random.randrange(n)
    if u != v:
        layer_u = u // layer_size
        layer_v = v // layer_size
        
        # 70% рёбер — строго внутри слоя (чтобы не расширять фронт)
        # 30% — между соседними слоями (чтобы граф был связным)
        r = random.random()
        if r < 0.7:
            if layer_u != layer_v:
                continue
        elif r < 0.95:
            if abs(layer_u - layer_v) > 1:
                continue
        # 5% рёбер — случайные (для связности)
        
        edges.add((min(u,v), max(u,v)))

with open(path, 'w') as f:
    f.write('%%MatrixMarket matrix coordinate pattern symmetric\n')
    f.write(f'% Generated probe for width ~260k, density 12\n')
    f.write(f'{n} {n} {len(edges)}\n')
    for u, v in edges:
        f.write(f'{u+1} {v+1}\n')

print(f"Создан {path}: вершин={n}, плотность={density}, рёбер={len(edges)}")
