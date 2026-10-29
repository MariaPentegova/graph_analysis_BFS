#include <stdio.h>
#include <stdlib.h>
#undef NDEBUG
#include <assert.h>
#include "test_utils.h"
#include "../main/classic_bfs.h"

void test_classic_bfs_from(CSRMatrix* csr, int start_vertex) {
    int n = csr->n;
    int* parent = (int*)malloc(n * sizeof(int));
    assert(parent != NULL);

    printf(" Testing start vertex: %d\n", start_vertex);
    csr_parent_bfs(csr, start_vertex, parent);

    int all_visited = 1;
    for (int i = 0; i < n; i++) {
        if (parent[i] == -1){
            all_visited = 0;
        }
    }
    assert(all_visited == 1);
    assert(parent[start_vertex] == start_vertex);

    if (start_vertex == 0) {
        assert(parent[0] == 0);
        assert(parent[1] == 0);
        assert(parent[2] == 1);
        assert(parent[3] == 0);
        assert(parent[4] == 1 || parent[4] == 3);
    } else if (start_vertex == 1) {
        assert(parent[1] == 1);
        assert(parent[0] == 1);
        assert(parent[2] == 1);
        assert(parent[4] == 1);
        assert(parent[3] == 0 || parent[3] == 4);
    }

    free(parent);
}

void test_classic_parent_bfs_correctness() {
    printf("\nTest Classic Parent BFS Correctness\n");

    CSRMatrix* csr = create_test_csr();
    assert(csr != NULL);

    test_classic_bfs_from(csr, 0);
    test_classic_bfs_from(csr, 1);

    delete_csr(csr);
    printf("Test Classic Parent BFS passed!\n");
}

void test_classic_multisource_bfs_correctness() {
    printf("\nTest Classic Multisource BFS Correctness\n");

    CSRMatrix* csr = create_test_csr();
    assert(csr != NULL);

    int n = csr->n;
    int* parent = (int*)malloc(n * sizeof(int));
    assert(parent != NULL);

    int num_sources = 4;
    int* sources = (int*)malloc(num_sources * sizeof(int));
    assert(sources != NULL);

    int step = n / num_sources;
    for (int i = 0; i < num_sources; i++) {
        sources[i] = i * step;
    }

    printf("Источники: ");
    for (int i = 0; i < num_sources; i++) {
        printf("%d ", sources[i]);
    }
    printf("\n");

    csr_multisource_bfs(csr, sources, num_sources, parent);

    int all_visited = 1;
    for (int i = 0; i < n; i++) {
        if (parent[i] == -1) {
            all_visited = 0;
            break;
        }
    }
    assert(all_visited == 1);
    printf("All vertices visited\n");

    int all_sources_root = 1;
    for (int i = 0; i < num_sources; i++) {
        if (parent[sources[i]] != sources[i]) {
            all_sources_root = 0;
            break;
        }
    }
    assert(all_sources_root == 1);
    printf("All sources point to themselves.\n");

    int no_self_loops = 1;
    for (int i = 0; i < n; i++) {
        int is_source = 0;
        for (int j = 0; j < num_sources; j++) {
            if (i == sources[j]) {
                is_source = 1;
                break;
            }
        }
        if (!is_source && parent[i] == i) {
            no_self_loops = 0;
            break;
        }
    }
    assert(no_self_loops == 1);
    printf("Non-root vertices have no loops.\n");

    for (int i = 0; i < n; i++) {
        if (i == 0 || i == 1 || i == 2 || i == 3) {
            assert(parent[i] == i);
        } else if (i == 4) {
            assert(parent[4] == 1 || parent[4] == 3);
        }
    }
    printf("The parents are valid for this graph.\n");

    for (int i = 0; i < n; i++) {
        if (parent[i] != -1) {
            if (parent[i] == i) {
                int is_source = 0;
                for (int j = 0; j < num_sources; j++) {
                    if (i == sources[j]) {
                        is_source = 1;
                        break;
                    }
                }
                assert(is_source == 1);
            }
        }
    }
    printf("No cycles in parent\n");

    free(parent);
    free(sources);
    delete_csr(csr);
    printf("Test Classic Multisource BFS passed!\n");
}

void test_disconnected_classic() {
    printf("\nTest Classic BFS on Disconnected Graph\n");
    CSRMatrix* csr = create_disconnected_test_csr();
    int* parent = malloc(csr->n * sizeof(int));

    csr_parent_bfs(csr, 0, parent);

    assert(parent[0] == 0);
    assert(parent[1] == 0);
    assert(parent[2] == -1);
    assert(parent[3] == -1);
    assert(parent[4] == -1);

    free(parent);
    delete_csr(csr);
    printf("Test Disconnected Classic BFS passed!\n");
}

void test_disconnected_multisource_classic() {
    printf("\nTest Classic Multisource BFS on Disconnected Graph\n");

    CSRMatrix* csr = create_disconnected_test_csr();
    int* parent = (int*)malloc(csr->n * sizeof(int));
    assert(parent != NULL);

    int sources[2] = {0, 2};
    csr_multisource_bfs(csr, sources, 2, parent);

    assert(parent[0] == 0);
    assert(parent[2] == 2);
    assert(parent[1] == 0);
    assert(parent[3] == 2);
    assert(parent[4] == -1);

    free(parent);
    delete_csr(csr);
    printf("Test Disconnected Classic Multisource BFS passed!\n");
}

int main() {
    test_classic_parent_bfs_correctness();
    test_classic_multisource_bfs_correctness();
    test_disconnected_classic();
    test_disconnected_multisource_classic();

    printf("All Tests Classic BFS passed!\n");
    return 0;
}
