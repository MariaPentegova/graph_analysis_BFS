#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "utils.h"

static int compare_ints(const void* a, const void* b) {
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    return (ia > ib) - (ia < ib);
}

Graph* create_graph(int n) {
    Graph* graph = (Graph*)malloc(sizeof(Graph));
    if (graph == NULL) {
        printf("Error: failed to allocate memory for the graph\n");
        return NULL;
    }
    graph->num_of_vertices = n;
    graph->adjLists = (Node**)malloc(n * sizeof(Node*));
    if (graph->adjLists == NULL) {
        printf("Error: failed to allocate memory for adjacency lists\n");
        free(graph);
        return NULL;
    }
    for (int i = 0; i < n; i++) {
        graph->adjLists[i] = NULL;
    }
    return graph;
}

void add_edge(Graph* g, int src, int dest) {
    if (g == NULL){
        return;
    }
    if (src < 0 || src >= g->num_of_vertices || dest < 0 || dest >= g->num_of_vertices) {
        printf("Error: invalid vertex indices (%d, %d)\n", src, dest);
        return;
    }
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        printf("Error: failed to allocate memory for the edge (%d, %d)\n", src, dest);
        return;
    }
    newNode->vertex = dest;
    newNode->next = g->adjLists[src];
    g->adjLists[src] = newNode;

    newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        printf("Error: failed to allocate memory for the reverse edge (%d, %d)\n", dest, src);
        return;
    }
    newNode->vertex = src;
    newNode->next = g->adjLists[dest];
    g->adjLists[dest] = newNode;
}

Graph* load_matrix(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (f == NULL) {
        printf("Error: failed to open file %s\n", filename);
        return NULL;
    }
    char line[256];
    if (fgets(line, sizeof(line), f) == NULL) {
        printf("Error: empty file\n");
        fclose(f);
        return NULL;
    }
    if (strstr(line, "%%MatrixMarket") == NULL) {
        printf("Error: wrong format (не .mtx)\n");
        fclose(f);
        return NULL;
    }
    if (strstr(line, "coordinate") == NULL) {
        printf("Error: only coordinate is supported\n");
        fclose(f);
        return NULL;
    }
    int is_symmetric = 0;
    if (strstr(line, "symmetric") != NULL) {
        is_symmetric = 1;
    } else if (strstr(line, "general") != NULL) {
        is_symmetric = 0;
    } else {
        printf("Error: unsupported type (symmetric/general only)\n");
        fclose(f);
        return NULL;
    }
    if (strstr(line, "pattern") == NULL) {
        printf("Error: only pattern is supported (without weights)\n");
        fclose(f);
        return NULL;
    }
    while (fgets(line, sizeof(line), f) != NULL) {
        if (line[0] != '%'){
            break;
        }
    }
    int rows, cols, entries;
    if (sscanf(line, "%d %d %d", &rows, &cols, &entries) != 3) {
        printf("Error: failed to read dimensions\n");
        fclose(f);
        return NULL;
    }
    if (rows != cols) {
        printf("Error: matrix is not square (%d x %d)\n", rows, cols);
        fclose(f);
        return NULL;
    }
    Graph* graph = create_graph(rows);
    int src, dest;
    for (int i = 0; i < entries; i++) {
        if (fscanf(f, "%d %d", &src, &dest) != 2) {
            printf("Error: failed to read edge %d\n", i);
            delete_graph(graph);
            fclose(f);
            return NULL;
        }
        src--;
        dest--;
        if (is_symmetric) {
            add_edge(graph, src, dest);
        } else {
            Node* newNode = (Node*)malloc(sizeof(Node));
            if (newNode == NULL) {
                printf("Error: failed to allocate memory for the edge (%d, %d)\n", src, dest);
                delete_graph(graph);
                fclose(f);
                return NULL;
            }
            newNode->vertex = dest;
            newNode->next = graph->adjLists[src];
            graph->adjLists[src] = newNode;
        }
    }
    fclose(f);
    printf("Graph loaded: %d vertices, %d edges\n", rows, entries);
    return graph;
}

CSRMatrix* graph_to_csr(Graph* graph) {
    if (graph == NULL){
        return NULL;
    }
    CSRMatrix* csr = (CSRMatrix*)malloc(sizeof(CSRMatrix));
    csr->n = graph->num_of_vertices;
    csr->row_ptr = (int*)malloc((graph->num_of_vertices + 1) * sizeof(int));
    int total_edges = 0;
    csr->row_ptr[0] = 0;
    for (int i = 0; i < graph->num_of_vertices; i++) {
        int degree = 0;
        Node* current = graph->adjLists[i];
        while (current != NULL) {
            degree++;
            current = current->next;
        }
        total_edges += degree;
        csr->row_ptr[i + 1] = csr->row_ptr[i] + degree;
    }
    csr->col_idx = (int*)malloc(total_edges * sizeof(int));
    int* current_pos = (int*)calloc(graph->num_of_vertices, sizeof(int));
    for (int i = 0; i < graph->num_of_vertices; i++) {
        Node* current = graph->adjLists[i];
        while (current != NULL) {
            int pos = csr->row_ptr[i] + current_pos[i];
            csr->col_idx[pos] = current->vertex;
            current_pos[i]++;
            current = current->next;
        }
    }
    free(current_pos);

    for (int i = 0; i < graph->num_of_vertices; i++) {
        int start = csr->row_ptr[i];
        int len   = csr->row_ptr[i + 1] - start;
        if (len <= 1){
            continue;
        }

        int sorted = 1;
        for (int j = start + 1; j < start + len; j++) {
            if (csr->col_idx[j] < csr->col_idx[j - 1]) {
                sorted = 0;
                break;
            }
        }
        if (!sorted) {
            qsort(&csr->col_idx[start], (size_t)len, sizeof(int), compare_ints);
        }
    }

    return csr;
}

int find_max_degree_vertex(Graph* graph) {
    if (graph == NULL) return -1;
    int max_degree = -1;
    int max_vertex = 0;
    for (int i = 0; i < graph->num_of_vertices; i++) {
        int degree = 0;
        Node* current = graph->adjLists[i];
        while (current != NULL) {
            degree++;
            current = current->next;
        }
        if (degree > max_degree) {
            max_degree = degree;
            max_vertex = i;
        }
    }
    return max_vertex;
}

void find_top_degree_vertices(Graph* g, int k, int* out) {
    if (g == NULL || out == NULL || k <= 0){
        return;
    }
    int n = g->num_of_vertices;
    if (k > n){
        k = n;
    }
    int* degrees = (int*)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) {
        int degree = 0;
        for (Node* cur = g->adjLists[i]; cur != NULL; cur = cur->next) {
            degree++;
        }
        degrees[i] = degree;
    }
    for (int position = 0; position < k; position++) {
        int chosen_vertex = -1;
        int max_degree = -1;
        for (int i = 0; i < n; i++) {
            if (degrees[i] > max_degree) {
                max_degree = degrees[i];
                chosen_vertex = i;
            }
        }
        out[position] = chosen_vertex;
        degrees[chosen_vertex] = -1;
    }
    free(degrees);
}

void delete_graph(Graph* g) {
    if (g == NULL){
        return;
    }
    for (int i = 0; i < g->num_of_vertices; i++) {
        Node* current = g->adjLists[i];
        while (current != NULL) {
            Node* temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(g->adjLists);
    free(g);
}

void delete_csr(CSRMatrix* csr) {
    if (csr == NULL){
        return;
    }
    free(csr->row_ptr);
    free(csr->col_idx);
    free(csr);
}


void print_level_stats(const int* level, int n, const char* graph_name, int true_edges, int* out_max_width, int* out_max_level) {
    int max_level = 0;
    int reachable = 0;
    int unreachable = 0;

    for (int i = 0; i < n; i++) {
        if (level[i] == -1) {
            unreachable++;
        } else {
            reachable++;
            if (level[i] > max_level){
                max_level = level[i];
            }
        }
    }

    int* width = (int*)calloc(max_level + 1, sizeof(int));
    for (int i = 0; i < n; i++) {
        if (level[i] >= 0){
            width[level[i]]++;
        }
    }

    int max_width = 0;
    for (int l = 0; l <= max_level; l++) {
        if (width[l] > max_width) {
            max_width = width[l];
        }
    }

    if (out_max_width != NULL) {
        *out_max_width = max_width;
    }
    if (out_max_level != NULL) {
        *out_max_level = max_level;
    }

    double avg_degree = (double)true_edges / n;

    printf("\n Level statistics for %s (single-source from max-degree):\n", graph_name);
    printf("   reachable   = %d (%.1f%%)\n", reachable, 100.0 * reachable / n);
    printf("   unreachable = %d (%.1f%%)\n", unreachable, 100.0 * unreachable / n);
    printf("   max_level   = %d\n", max_level);
    printf("   max_width   = %d\n", max_width);
    printf("   avg_degree (|E|/|V|) = %.2f\n", avg_degree);

    printf(" RECOMMENDATION: ");

    int use_graphblas = 0;
    const char* zone_name = "";
    const char* reason = "";

    if (avg_degree < 1.4) {
        use_graphblas = 0;
        zone_name = "Classic BFS (Base Zone)";
        reason = "avg_degree < 1.4 (graph is too sparse)";
    }
    else if (avg_degree < 4.4 && max_width <= 600000) {
        use_graphblas = 0;
        zone_name = "Classic BFS (Base Zone)";
        reason = "avg_degree < 4.4 AND max_width <= 600k";
    }
    else if (avg_degree < 31.0 && max_width <= 290000) {
        use_graphblas = 0;
        zone_name = "Classic BFS (Base Zone)";
        reason = "avg_degree < 31.0 AND max_width <= 290k";
    }
    else if (avg_degree >= 31.0 && max_width <= 60000) {
        use_graphblas = 0;
        zone_name = "Classic BFS (Base Zone)";
        reason = "avg_degree >= 31.0 BUT max_width <= 60k";
    }
    else {
        use_graphblas = 1;

        if (avg_degree >= 31.0) {
            zone_name = "GraphBLAS (Zone 1: Dense)";
            reason = "avg_degree >= 31 AND max_width > 60k";
        } else if (avg_degree >= 4.4) {
            zone_name = "GraphBLAS (Zone 2: Wide)";
            reason = "avg_degree >= 4.4 AND max_width > 290k";
        } else {
            zone_name = "GraphBLAS (Zone 3: Sparse & Huge)";
            reason = "avg_degree >= 1.4 AND max_width > 600k";
        }
    }

    printf("%s\n", zone_name);
    printf(" Reason: %s\n", reason);

    free(width);
}
