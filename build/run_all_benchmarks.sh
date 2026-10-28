#!/bin/bash
echo "Graph,V,E,max_width,max_level,density,CP,GB-P,Speedup" > results.csv

for file in ../graphs/*.mtx; do
    graph=$(basename "$file" .mtx)
    echo "Running $graph..."
    
    # Запускаем бенчмарк и сохраняем вывод
    output=$(./bfs_benchmark "$file")
    
    # Парсим вывод с помощью grep и awk
    v=$(echo "$output" | grep "Graph loaded:" | awk '{print $3}' | tr -d ',')
    e=$(echo "$output" | grep "Graph loaded:" | awk '{print $5}' | tr -d ',')
    mw=$(echo "$output" | grep "max_width   =" | awk '{print $3}')
    ml=$(echo "$output" | grep "max_level   =" | awk '{print $3}')
    dens=$(echo "$output" | grep "density (|E|/|V|) =" | awk '{print $4}')
    cp=$(echo "$output" | grep "Classic Parent BFS        |" | awk '{print $4}')
    gb=$(echo "$output" | grep "GraphBLAS Parent BFS       |" | awk '{print $4}')
    speedup=$(echo "$output" | grep "GraphBLAS Parent / Classic Parent:" | awk '{print $6}' | tr -d 'x')
    
    # Записываем в CSV
    echo "$graph,$v,$e,$mw,$ml,$dens,$cp,$gb,$speedup" >> results.csv
done
