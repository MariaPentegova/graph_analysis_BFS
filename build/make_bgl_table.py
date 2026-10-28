import csv
import matplotlib.pyplot as plt
from matplotlib.backends.backend_pdf import PdfPages

IN_CSV = 'benchmark_results.csv'
OUT_PDF = 'bfs_bgl_comparison_table.pdf'

rows = []
with open(IN_CSV) as f:
    for r in csv.DictReader(f):
        try:
            cp = float(r['CP_us'])
            bgl = float(r['BGL_us'])
            if cp > 0 and bgl > 0:
                speedup = bgl / cp 
                rows.append([r['Graph'], f"{cp:.0f}", f"{bgl:.0f}", f"{speedup:.2f}x"])
        except ValueError:
            pass

rows.sort(key=lambda x: x[0])

chunk_size = 35 
chunks = [rows[i:i + chunk_size] for i in range(0, len(rows), chunk_size)]

with PdfPages(OUT_PDF) as pdf:
    for page_num, chunk in enumerate(chunks):
        fig, ax = plt.subplots(figsize=(8.27, 11.69))
        ax.axis('off')
        
        col_labels = ['Граф', 'Classic C (us)', 'BGL (us)', 'Собственный код\nбыстрее в:']
        col_widths = [0.35, 0.18, 0.15, 0.32] 
        
        # Настраиваем заголовок и отступ сверху для разных страниц
        if page_num == 0:
            ax.set_title('Сравнение Classic C vs BGL', fontsize=14, pad=20)
            fig.subplots_adjust(top=0.90) # Оставляем 10% места сверху под заголовок
        else:
            ax.set_title(f'Продолжение таблицы (Стр. {page_num+1})', fontsize=10, pad=10)
            fig.subplots_adjust(top=0.95) # Оставляем 5% места сверху

        # loc='upper center' прижимает таблицу к верхнему краю (сразу под заголовком)
        table = ax.table(cellText=chunk, colLabels=col_labels, 
                         colWidths=col_widths,
                         loc='upper center', cellLoc='center')
        
        table.auto_set_font_size(False)
        table.set_fontsize(9)
        table.scale(1, 1.5)
        
        for (row, col), cell in table.get_celld().items():
            if row == 0:
                cell.set_text_props(weight='bold')
                cell.set_facecolor('#e0e0e0')
            if col == 0 and row > 0:
                cell.set_text_props(ha='left')
                cell.set_x(cell.get_x() - 0.02) 

        pdf.savefig(fig, bbox_inches='tight')
        plt.close(fig)

print(f"Таблица сохранена в {OUT_PDF}")
