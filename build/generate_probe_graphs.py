import random
import os

# Стратегия:
# 1. Берём базовую топологию "звезда + случайные рёбра" (даёт controllable width)
# 2. Или используем Barabasi-Albert / Erdős–Rényi с параметрами, которые дают разную ширину
#
# Для нащупывания границы нужны графы с:
#   - n ~ 400k-900k (как в твоих wiki_*)
#   - density от 4 до 20
#   - max_width от ~150k до ~700k
#
# Проще всего: генерировать графы с заданным n и density, но разной "структурой":
#   - "wide": много вершин достижимо за мало шагов (широкий фронт)
#   - "deep": цепочки/кластеры (узкий фронт)
#
# Ты потом прогонишь бенчмарк и увидишь реальные max_width.

def generate_graph(n, density, structure, out_dir, name):
    """
    structure: 'random' (ER), 'preferential' (BA-like), 'layered'
    """
    m = int(n * density)
    edges = set()

    if structure == 'random':
        # Erdős–Rényi: почти все вершины в одной компоненте, ширина зависит от density
        while len(edges) < m:
            u = random.randrange(n)
            v = random.randrange(n)
            if u != v:
                edges.add((min(u,v), max(u,v)))

    elif structure == 'layered':
        # Слоистая структура: контролируем ширину через число слоёв и связность между ними
        # Это даёт более предсказуемый max_width
        num_layers = max(3, int(n / 50000))  # ~8 слоёв для 400k
        layer_size = n // num_layers
        # рёбра внутри слоёв + между соседними
        while len(edges) < m:
            layer = random.randrange(num_layers)
            base = layer * layer_size
            size = min(layer_size, n - base)
            u = base + random.randrange(size)
            if random.random() < 0.6:
                v = base + random.randrange(size)
            else:
                next_layer = min(layer + 1, num_layers - 1)
                v_base = next_layer * layer_size
                v_size = min(layer_size, n - v_base)
                v = v_base + random.randrange(v_size)
            if u != v:
                edges.add((min(u,v), max(u,v)))

    elif structure == 'hub':
        # Граф с несколькими хабами: старт с max-degree хаба даст широкий фронт
        num_hubs = max(10, n // 10000)
        hubs = random.sample(range(n), num_hubs)
        # Каждый хаб соединяем с случайными вершинами
        for h in hubs:
            for _ in range(m // num_hubs):
                v = random.randrange(n)
                if v != h:
                    edges.add((min(h,v), max(h,v)))
        # добиваем случайными
        while len(edges) < m:
            u = random.randrange(n)
            v = random.randrange(n)
            if u != v:
                edges.add((min(u,v), max(u,v)))

    # Записываем .mtx
    path = os.path.join(out_dir, name + '.mtx')
    with open(path, 'w') as f:
        f.write('%%MatrixMarket matrix coordinate pattern symmetric\n')
        f.write(f'% generated probe: n={n}, density={density}, structure={structure}\n')
        f.write(f'{n} {n} {len(edges)}\n')
        for u, v in edges:
            f.write(f'{u+1} {v+1}\n')
    print(f'  {name}: n={n}, density={density:.1f}, edges={len(edges)}, structure={structure}')

def main():
    out_dir = '../graphs/probe'
    os.makedirs(out_dir, exist_ok=True)

    random.seed(42)

    print("Generating probe graphs...")

    # Серия A: фиксируем n и density, варьируем structure (чтобы получить разный max_width)
    # Это даст вертикальный срез: density фиксирована, ширина разная
    print("\n--- Series A: fixed density, varying structure (width) ---")
    n_base = 400000
    for density in [5.0, 6.5, 8.0, 10.0, 15.0]:
        for struct in ['random', 'layered', 'hub']:
            name = f'probe_n{n_base//1000}k_d{density}_s{struct}'
            generate_graph(n_base, density, struct, out_dir, name)

    # Серия B: фиксируем structure (random), варьируем n и density
    # Это даст горизонтальный срез: ширина примерно ~ n * const, density разная
    print("\n--- Series B: varying n, fixed structure, density sweep ---")
    for n in [300000, 500000, 700000, 900000]:
        for density in [4.0, 5.0, 6.5, 8.0, 10.0, 15.0]:
            name = f'probe_n{n//1000}k_d{density}_srandom'
            generate_graph(n, density, 'random', out_dir, name)

    # Серия C: целевые точки около предполагаемых порогов
    # Если из твоих данных видно, что граница где-то density ~6.5-8, width ~300k-600k
    print("\n--- Series C: targeted near thresholds ---")
    targets = [
        (500000, 5.5, 'random'),
        (500000, 7.0, 'random'),
        (600000, 6.0, 'random'),
        (600000, 7.5, 'random'),
        (700000, 6.0, 'layered'),
        (700000, 7.0, 'layered'),
        (400000, 12.0, 'hub'),
        (400000, 18.0, 'hub'),
    ]
    for n, d, s in targets:
        name = f'probe_target_n{n//1000}k_d{d}_s{s}'
        generate_graph(n, d, s, out_dir, name)

    print(f"\nDone. Graphs written to {out_dir}/")

if __name__ == '__main__':
    main()
