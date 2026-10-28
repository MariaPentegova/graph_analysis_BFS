#ifndef UTILS_H
#define UTILS_H

struct Node {
    int vertex;
    struct Node* next;
};

struct Graph {
    int num_of_vertices;
    struct Node** adjLists;
};

struct CSRMatrix {
    int n;
    int* row_ptr;
    int* col_idx;
};

typedef struct Node Node;
typedef struct Graph Graph;
typedef struct CSRMatrix CSRMatrix;

extern int g_last_max_width;
extern int g_last_max_level;

Graph* create_graph(int n);
void add_edge(struct Graph* g, int src, int dest);
Graph* load_matrix(const char* filename);
CSRMatrix* graph_to_csr(Graph* g);
int find_max_degree_vertex(Graph* g);
void find_top_degree_vertices(Graph* g, int k, int* out);
void delete_graph(Graph* g);
void delete_csr(CSRMatrix* csr);
void print_level_stats(const int* level, int n, const char* graph_name, int true_edges, int* out_max_width, int* out_max_level);

#endif
