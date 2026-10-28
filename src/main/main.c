#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"
#include "graphblas_bfs.h"

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        printf("Error: incorrect number of arguments\n");
        return 1;
    }
    const char* filename = argv[1];

    Graph* g = load_matrix(filename);
    if (g == NULL) {
        printf("Error: failed to load the graph\n");
        return 1;
    }

    CSRMatrix* csr = graph_to_csr(g);
    if (csr == NULL) {
        printf("Error: failed to convert to CSR\n");
        delete_graph(g);
        return 1;
    }

    int start_vertex;
    if (argc == 3) {
        start_vertex = atoi(argv[2]);
        if (start_vertex < 0 || start_vertex >= csr->n) {
            printf("Error: start vertex %d is out of range [0, %d)\n", start_vertex, csr->n);
            delete_graph(g);
            delete_csr(csr);
            return 1;
        }
    } else {
        start_vertex = find_max_degree_vertex(g);
    }

    printf("Start vertex: %d\n", start_vertex);

    int is_symmetric = 0;
    FILE* f = fopen(filename, "r");
    if (f) {
        char line[256];
        if (fgets(line, sizeof(line), f)) {
            if (strstr(line, "symmetric")) is_symmetric = 1;
        }
        fclose(f);
    }
    int total_edges_in_csr = csr->row_ptr[csr->n];
    int true_edges = is_symmetric ? (total_edges_in_csr / 2) : total_edges_in_csr;

    int* level = (int*)malloc(csr->n * sizeof(int));
    if (level == NULL) {
        printf("Error: failed to allocate memory for level\n");
        delete_graph(g);
        delete_csr(csr);
        return 1;
    }

    if (graphblas_init() != 0) {
        printf("Error: failed to initialize GraphBLAS\n");
        free(level);
        delete_graph(g);
        delete_csr(csr);
        return 1;
    }

    void* gb_matrix = NULL;
    if (graphblas_build_matrix(csr, &gb_matrix) != 0) {
        printf("Error: failed to construct GraphBLAS matrix\n");
        graphblas_finalize();
        free(level);
        delete_graph(g);
        delete_csr(csr);
        return 1;
    }

    graphblas_level_bfs(csr, gb_matrix, start_vertex, level);

    print_level_stats(level, csr->n, filename, true_edges, NULL, NULL);

    graphblas_free_matrix(gb_matrix);
    graphblas_finalize();

    free(level);
    delete_graph(g);
    delete_csr(csr);

    return 0;
}
