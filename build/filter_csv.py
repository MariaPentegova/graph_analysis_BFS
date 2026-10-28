import csv, os

GRAPHS_DIR = '../graphs'   # путь к папке с .mtx
IN_CSV  = 'benchmark_results.csv'
OUT_CSV = 'benchmark_results_clean.csv'

# Собираем имена всех .mtx, которые реально есть на диске (рекурсивно)
existing = set()
for root, _, files in os.walk(GRAPHS_DIR):
    for fn in files:
        if fn.endswith('.mtx'):
            existing.add(fn)

kept, dropped = [], []
with open(IN_CSV, 'r') as f:
    reader = csv.DictReader(f)
    fieldnames = reader.fieldnames
    for row in reader:
        if row['Graph'] in existing:
            kept.append(row)
        else:
            dropped.append(row['Graph'])

with open(OUT_CSV, 'w', newline='') as f:
    writer = csv.DictWriter(f, fieldnames=fieldnames)
    writer.writeheader()
    writer.writerows(kept)

print(f"Оставлено строк: {len(kept)}")
print(f"Выкинуто строк (файла нет на диске): {len(dropped)}")
for g in sorted(set(dropped)):
    print(f"  - {g}")
