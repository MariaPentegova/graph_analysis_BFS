#include <stdio.h>
#include <stdlib.h>
#undef NDEBUG
#include <assert.h>
#include <vector>
#include <utility>

extern "C" {
    #include "test_utils.h"
    #include "../main/classic_bfs.h"
    #include "../main/graphblas_bfs.h"
}

#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/visitors.hpp>

typedef boost::compressed_sparse_row_graph<
boost::directedS, boost::no_property, boost::no_property,
boost::no_property, int, int> BGLGraph;

void compare_with_bgl(CSRMatrix* csr, int start_vertex, const char* graph_name) {
    int n = csr->n;
    int* parent_classic = (int*)malloc(n * sizeof(int));
    int* parent_graphblas = (int*)malloc(n * sizeof(int));

    for (int i = 0; i < n; i++) {
        parent_classic[i] = -1;
        parent_graphblas[i] = -1;
    }

    std::vector<int> parent_bgl(n, -1);

    csr_parent_bfs(csr, start_vertex, parent_classic);

    void* A = NULL;
    int rc = graphblas_build_matrix(csr, &A);
    if (rc != 0) {
        printf("  [FAIL] %s: graphblas_build_matrix failed (rc=%d)\n", graph_name, rc);
        exit(1);
    }

    rc = graphblas_parent_bfs(csr, A, start_vertex, parent_graphblas);
    graphblas_free_matrix(A);
    if (rc != 0) {
        printf("  [FAIL] %s: graphblas_parent_bfs failed (rc=%d)\n", graph_name, rc);
        exit(1);
    }

    std::vector<std::pair<int, int>> edge_list;
    for (int u = 0; u < n; ++u) {
        for (int i = csr->row_ptr[u]; i < csr->row_ptr[u + 1]; ++i) {
            edge_list.push_back(std::make_pair(u, csr->col_idx[i]));
        }
    }
    BGLGraph bgl_g(boost::edges_are_sorted, edge_list.begin(), edge_list.end(), n);

    parent_bgl[start_vertex] = start_vertex;

    boost::breadth_first_search(bgl_g, start_vertex,
                                boost::visitor(boost::make_bfs_visitor(
                                    boost::record_predecessors(parent_bgl.data(), boost::on_tree_edge()))));

    int errors = 0;
    for (int i = 0; i < n; i++) {
        bool reach_c = (parent_classic[i] != -1);
        bool reach_gb = (parent_graphblas[i] != -1);
        bool reach_bgl = (parent_bgl[i] != -1);

        if (reach_c != reach_bgl || reach_gb != reach_bgl) {
            printf("  [FAIL] %s: mismatch at vertex %d (C: %d, GB: %d, BGL: %d)\n",
                   graph_name, i, reach_c, reach_gb, reach_bgl);
            errors++;
        }
    }

    if (errors == 0) {
        printf("  [OK] %s: Classic, GraphBLAS and BGL visited the same vertices.\n", graph_name);
    } else {
        printf("  [FAIL] %s: %d vertices mismatch!\n", graph_name, errors);
        exit(1);
    }

    free(parent_classic);
    free(parent_graphblas);
}

static void check_parent_tree(CSRMatrix* csr, int* parent, int start_vertex) {
    int n = csr->n;
    assert(parent[start_vertex] == start_vertex);
    for (int v = 0; v < n; v++) {
        if (v == start_vertex) continue;
        if (parent[v] == -1) continue;
        assert(parent[v] != v);
        int found = 0;
        int begin = csr->row_ptr[v];
        int end = csr->row_ptr[v + 1];
        for (int j = begin; j < end; j++) {
            if (csr->col_idx[j] == parent[v]) { found = 1; break; }
        }
        assert(found == 1);
    }
}

void compare_single_source_classic_vs_graphblas() {
    printf("Test Single-Source BFS (Classic vs GraphBLAS)\n");
    CSRMatrix* csr = create_test_csr();
    int n = csr->n;
    int* parent_classic = (int*)malloc(n * sizeof(int));
    int* parent_graphblas = (int*)malloc(n * sizeof(int));

    Graph* temp_g = create_test_graph();
    int start_vertex = find_max_degree_vertex(temp_g);
    delete_graph(temp_g);

    csr_parent_bfs(csr, start_vertex, parent_classic);
    void* A = NULL;
    assert(graphblas_build_matrix(csr, &A) == 0);
    assert(graphblas_parent_bfs(csr, A, start_vertex, parent_graphblas) == 0);
    graphblas_free_matrix(A);

    check_parent_tree(csr, parent_classic, start_vertex);
    check_parent_tree(csr, parent_graphblas, start_vertex);

    for (int i = 0; i < n; i++) {
        assert((parent_classic[i] == -1) == (parent_graphblas[i] == -1));
    }
    free(parent_classic); free(parent_graphblas); delete_csr(csr);
    printf("Test Single-Source compare passed!\n\n");
}

void compare_multisource_classic_vs_graphblas() {
    printf("Test Multisource BFS (Classic vs GraphBLAS)\n");
    CSRMatrix* csr = create_test_csr();
    int n = csr->n;
    int* parent_classic = (int*)malloc(n * sizeof(int));
    int* parent_graphblas = (int*)malloc(n * sizeof(int));

    int num_sources = 4;
    int* sources = (int*)malloc(num_sources * sizeof(int));
    int step = n / num_sources;
    for (int i = 0; i < num_sources; i++) {
        sources[i] = i * step;
    }
    csr_multisource_bfs(csr, sources, num_sources, parent_classic);
    void* A = NULL;
    assert(graphblas_build_matrix(csr, &A) == 0);
    assert(graphblas_multisource_parent_bfs(csr, A, sources, num_sources, parent_graphblas) == 0);
    graphblas_free_matrix(A);

    for (int i = 0; i < n; i++) {
        assert((parent_classic[i] == -1) == (parent_graphblas[i] == -1));
    }
    for (int i = 0; i < num_sources; i++) {
        assert(parent_classic[sources[i]] == sources[i]);
        assert(parent_graphblas[sources[i]] == sources[i]);
    }

    free(parent_classic); free(parent_graphblas);
    free(sources);
    delete_csr(csr);
    printf("Test Multisource compare passed!\n\n");
}

void compare_disconnected_single_source() {
    printf("\nTest Disconnected Single-Source (Classic vs GraphBLAS)\n");
    CSRMatrix* csr = create_disconnected_test_csr();
    int n = csr->n;
    int* parent_classic = (int*)malloc(n * sizeof(int));
    int* parent_gb = (int*)malloc(n * sizeof(int));

    csr_parent_bfs(csr, 0, parent_classic);
    void* A = NULL;
    assert(graphblas_build_matrix(csr, &A) == 0);
    assert(graphblas_parent_bfs(csr, A, 0, parent_gb) == 0);
    graphblas_free_matrix(A);

    for (int i = 0; i < n; i++) {
        assert((parent_classic[i] == -1) == (parent_gb[i] == -1));
    }
    assert(parent_classic[4] == -1 && parent_gb[4] == -1);

    free(parent_classic);
    free(parent_gb);
    delete_csr(csr);
    printf("Disconnected Single-Source compare passed!\n");
}

void compare_disconnected_multisource() {
    printf("\nTest Disconnected Multisource (Classic vs GraphBLAS)\n");
    CSRMatrix* csr = create_disconnected_test_csr();
    int n = csr->n;
    int* parent_classic = (int*)malloc(n * sizeof(int));
    int* parent_gb = (int*)malloc(n * sizeof(int));

    int sources[2] = {0, 2};
    csr_multisource_bfs(csr, sources, 2, parent_classic);
    void* A = NULL;
    assert(graphblas_build_matrix(csr, &A) == 0);
    assert(graphblas_multisource_parent_bfs(csr, A, sources, 2, parent_gb) == 0);
    graphblas_free_matrix(A);

    for (int i = 0; i < n; i++) {
        assert((parent_classic[i] == -1) == (parent_gb[i] == -1));
    }
    assert(parent_classic[4] == -1 && parent_gb[4] == -1);

    free(parent_classic);
    free(parent_gb);
    delete_csr(csr);
    printf("Disconnected Multisource compare passed!\n");
}


int main() {
    if (graphblas_init() != 0) {
        return 1;
    }

    compare_disconnected_single_source();
    compare_disconnected_multisource();
    compare_single_source_classic_vs_graphblas();
    compare_multisource_classic_vs_graphblas();

    printf("\nRunning BGL (Boost) Comparison Tests...\n");

    CSRMatrix* csr_connected = create_test_csr();
    compare_with_bgl(csr_connected, 0, "Connected Graph (start=0)");
    delete_csr(csr_connected);

    CSRMatrix* csr_disconnected = create_disconnected_test_csr();
    compare_with_bgl(csr_disconnected, 0, "Disconnected Graph (start=0)");
    delete_csr(csr_disconnected);

    graphblas_finalize();
    printf("\nAll Tests (including BGL comparison) passed!\n");
    return 0;
}
