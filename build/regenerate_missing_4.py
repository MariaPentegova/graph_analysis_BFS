import os
import random

GRAPHS_DIR = '../graphs'
os.makedirs(GRAPHS_DIR, exist_ok=True)
random.seed(42)

def write_mtx(filename, n, target_density, structure='random'):
    edges = set()
    
    if structure == 'hub':
        # Звезда от вершины 0 (даёт огромный фронт на первом шаге)
        for i in range(1, n):
            edges.add((0, i))
    elif structure == 'layered':
        # Слоистая структура (даёт контролируемую ширину)
        layers = int(n / 100000) # примерно по 100k на слой
        if layers < 2: layers = 2
        layer_size = n // layers
        while len(edges) < n * target_density:
            u = random.randint(0, n-1)
            # 80% рёбер внутри слоя, 20% между соседними
            if random.random() < 0.8:
                layer = u // layer_size
                start = layer * layer_size
                end = min(start + layer_size, n) - 1
                if end > start:
                    v = random.randint(start, end)
                else:
                    v = random.randint(0, n-1)
            else:
                v = random.randint(0, n-1)
            if u != v:
                edges.add((min(u,v), max(u,v)))
        # Добиваем случайными, если не хватило
        while len(edges) < n * target_density:
            u = random.randint(0, n-1); v = random.randint(0, n-1)
            if u != v: edges.add((min(u,v), max(u,v)))
    
    # Записываем
    path = os.path.join(GRAPHS_DIR, filename)
    with open(path, 'w') as f:
        f.write('%%MatrixMarket matrix coordinate pattern symmetric\n')
        f.write(f'% synthesized missing graph\n')
        f.write(f'{n} {n} {len(edges)}\n')
        for u, v in edges:
            f.write(f'{u+1} {v+1}\n')
    print(f"Создан: {filename} (V={n}, E={len(edges)}, density={len(edges)/n:.2f})")

# 1. wiki_Talk_density_6.5.mtx (density 6.5, огромная ширина ~0.72M)
write_mtx('wiki_Talk_density_6.5.mtx', 2394385, 6.5, 'hub')

# 2. wiki_600k_density_6.5.mtx (density 6.5, ширина ~0.25M)
write_mtx('wiki_600k_density_6.5.mtx', 600000, 6.5, 'layered')

# 3. wiki_550k_density_6.5.mtx (density 6.5, ширина ~0.23M)
write_mtx('wiki_550k_density_6.5.mtx', 550000, 6.5, 'layered')

# 4. probe_target_n700k_d6.0_slayered.mtx (density 6.0, ширина ~0.10M)
write_mtx('probe_target_n700k_d6.0_slayered.mtx', 700000, 6.0, 'layered')

print("\nГотово. Эти 4 файла снова лежат в ../graphs/")
