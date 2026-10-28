import csv, os
import numpy as np
import matplotlib.pyplot as plt
from collections import defaultdict

IN_CSV  = 'benchmark_results.csv' 
OUT_PDF = 'bfs_applicability_boundary.pdf'
GRAPHS_DIR = '../graphs'

existing = set(fn for fn in os.listdir(GRAPHS_DIR) if fn.endswith('.mtx'))

raw = defaultdict(list)
with open(IN_CSV) as f:
    for row in csv.DictReader(f):
        g = row['Graph']
        if g in existing:
            raw[g].append(row)

points = []
for gname, rows in raw.items():
    points.append({
        'name':       gname,
        'avg_degree': np.mean([float(r['avg_degree']) for r in rows]),
        'width':      np.mean([int(r['max_width']) for r in rows]) / 1_000_000,
        'speedup':    np.mean([float(r['Speedup_GB-P_CP']) for r in rows]),
    })

dens = np.array([p['avg_degree'] for p in points])
wid  = np.array([p['width'] for p in points])
spd  = np.array([p['speedup'] for p in points])
is_gb = spd >= 1.0

print(f"Точек на графике: {len(points)}")

# КОРРЕКТИРОВКА: Опустили W0 до 0.60
D0, W0 = 1.4, 0.60    # Верхняя ступенька
D1, W1 = 4.4, 0.290   # Основная ступенька
D2, W2 = 31.0, 0.06   # Правая ступенька

pred = np.zeros_like(is_gb)
for i in range(len(dens)):
    d, w = dens[i], wid[i]
    if d >= D2:
        pred[i] = w > W2
    elif d >= D1:
        pred[i] = w > W1
    elif d >= D0:
        pred[i] = w > W0
    else:
        pred[i] = False

wrong = pred != is_gb
acc = (pred == is_gb).mean()

print(f"Accuracy: {acc:.3f}, Ошибок: {wrong.sum()} из {len(points)}")
for i in np.where(wrong)[0]:
    print(f"  Ошибка: {points[i]['name']} (d={dens[i]:.2f}, w={wid[i]:.3f}, s={spd[i]:.2f})")

# Рисунок
fig, ax = plt.subplots(figsize=(12, 8))
colors = np.where(is_gb, '#2ca02c', '#d62728')
ax.scatter(dens, wid, c=colors, s=110, alpha=0.9, edgecolors='black', linewidths=0.8, zorder=5)

y_top = wid.max() * 1.1

ax.plot([D0, D0], [y_top, W0], color='black', linestyle='--', linewidth=2.5)
ax.plot([D0, D1], [W0, W0], color='black', linestyle='--', linewidth=2.5)
ax.plot([D1, D1], [W0, W1], color='black', linestyle='--', linewidth=2.5)
ax.plot([D1, D2], [W1, W1], color='black', linestyle='--', linewidth=2.5)
ax.plot([D2, D2], [W1, W2], color='black', linestyle='--', linewidth=2.5)
ax.plot([D2, 50], [W2, W2], color='black', linestyle='--', linewidth=2.5)

if wrong.sum() > 0:
    ax.scatter(dens[wrong], wid[wrong], facecolors='none', edgecolors='blue',
               s=240, linewidths=2.2, zorder=6, label=f'Ошибки ({wrong.sum()})')

ax.set_xlabel('Средняя степень вершины (|E| / |V|)', fontsize=13)
ax.set_ylabel('Максимальная ширина фронта (max_width, млн вершин)', fontsize=13)
ax.set_title(f'Ступенчатая граница применимости (Accuracy = {acc:.2f})', fontsize=13)
ax.grid(True, alpha=0.3)
ax.set_xlim(0, 50)
ax.set_ylim(-0.05, wid.max() * 1.1)

# ШАГ СЕТКИ 5
ax.set_xticks(np.arange(0, 51, 5))

ax.text(0.5, 2.0, 'Классика\nбыстрее', fontsize=14, color='darkred', weight='bold')
ax.text(20, 2.0, 'GraphBLAS\nбыстрее', fontsize=14, color='darkgreen', weight='bold')

ax.text(0.98, 0.03, f'Точек: {len(points)}\nAccuracy: {acc:.2f}\nОшибок: {int(wrong.sum())}',
        transform=ax.transAxes, fontsize=11, ha='right', va='bottom',
        bbox=dict(boxstyle='round', facecolor='white', alpha=0.85, edgecolor='gray'))

plt.savefig(OUT_PDF, bbox_inches='tight')
print(f"График сохранён в {OUT_PDF}")
