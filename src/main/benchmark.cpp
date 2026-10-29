#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <string>
#include <vector>
#include <algorithm>
#include <utility>
#include <dirent.h>

#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/visitors.hpp>

extern "C" {
    #include "utils.h"
    #include "classic_bfs.h"
    #include "graphblas_bfs.h"
}

double get_time_in_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

double average(double* arr, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; i++){
        sum += arr[i];
    }
    return sum / n;
}

double min_value(double* arr, int n) {
    double min = arr[0];
    for (int i = 1; i < n; i++){
        if (arr[i] < min){
            min = arr[i];
        }
    }
    return min;
}

double max_value(double* arr, int n) {
    double max = arr[0];
    for (int i = 1; i < n; i++){
        if (arr[i] > max){
            max = arr[i];
        }
    }
    return max;
}

void run_benchmark(const char* graph_file) {
    printf("\nBenchmark: %s\n", graph_file);

    Graph* g = load_matrix(graph_file);
    if (g == NULL){
        return;
    }

    int is_symmetric = 0;
    FILE* f = fopen(graph_file, "r");
    if (f) {
        char line[256];
        if (fgets(line, sizeof(line), f)) {
            if (strstr(line, "symmetric")){
                is_symmetric = 1;
            }
        }
        fclose(f);
    }

    int start_vertex = find_max_degree_vertex(g);
    CSRMatrix* csr = graph_to_csr(g);
    if (csr == NULL) {
        delete_graph(g);
        return;
    }

    int total_edges_in_csr = csr->row_ptr[csr->n];
    int true_edges = is_symmetric ? (total_edges_in_csr / 2) : total_edges_in_csr;

    int num_sources = 4;
    int* sources = (int*)malloc(num_sources * sizeof(int));
    find_top_degree_vertices(g, num_sources, sources);

    int* parent = (int*)malloc(csr->n * sizeof(int));
    int* level = (int*)malloc(csr->n * sizeof(int));
    if (parent == NULL || level == NULL) {
        free(sources);
        free(parent);
        free(level);
        delete_graph(g);
        delete_csr(csr);
        return;
    }

    typedef boost::compressed_sparse_row_graph<
    boost::directedS, boost::no_property, boost::no_property,
    boost::no_property, int, int> BGLGraph;

    double t_bgl_build = get_time_in_seconds();
    std::vector<std::pair<int, int>> edge_list;
    edge_list.reserve(total_edges_in_csr);
    for (int u = 0; u < csr->n; ++u) {
        for (int i = csr->row_ptr[u]; i < csr->row_ptr[u + 1]; ++i) {
            edge_list.push_back(std::make_pair(u, csr->col_idx[i]));
        }
    }
    BGLGraph bgl_g(boost::edges_are_sorted, edge_list.begin(), edge_list.end(), csr->n);
    std::vector<std::pair<int, int>>().swap(edge_list);

    void* gb_matrix = NULL;
    if (graphblas_build_matrix(csr, &gb_matrix) != 0) {
        free(sources);
        free(parent);
        free(level);
        delete_graph(g);
        delete_csr(csr);
        return;
    }

    graphblas_level_bfs(csr, gb_matrix, start_vertex, level);

    int local_max_width = 0;
    int local_max_level = 0;
    print_level_stats(level, csr->n, graph_file, true_edges, &local_max_width, &local_max_level);

    int num_runs = 10;
    double t_classic_parent[10];
    double t_bgl_parent[10];
    double t_classic_multisource[10];
    double t_graphblas_level[10];
    double t_graphblas_multisource[10];
    double start, end;

    csr_parent_bfs(csr, start_vertex, parent);
    for (int run = 0; run < num_runs; run++) {
        start = get_time_in_seconds();
        csr_parent_bfs(csr, start_vertex, parent);
        end = get_time_in_seconds();
        t_classic_parent[run] = (end - start) * 1e6;
    }

    csr_multisource_bfs(csr, sources, num_sources, parent);
    for (int run = 0; run < num_runs; run++) {
        start = get_time_in_seconds();
        csr_multisource_bfs(csr, sources, num_sources, parent);
        end = get_time_in_seconds();
        t_classic_multisource[run] = (end - start) * 1e6;
    }

    for (int run = 0; run < num_runs; run++) {
        for (int i = 0; i < csr->n; i++) parent[i] = -1;
        parent[start_vertex] = start_vertex;
        start = get_time_in_seconds();
        boost::breadth_first_search(bgl_g, start_vertex,
                                    boost::visitor(boost::make_bfs_visitor(
                                        boost::record_predecessors(parent, boost::on_tree_edge()))));
        end = get_time_in_seconds();
        t_bgl_parent[run] = (end - start) * 1e6;
    }

    graphblas_parent_bfs(csr, gb_matrix, start_vertex, parent);
    for (int run = 0; run < num_runs; run++) {
        start = get_time_in_seconds();
        graphblas_parent_bfs(csr, gb_matrix, start_vertex, parent);
        end = get_time_in_seconds();
        t_graphblas_level[run] = (end - start) * 1e6;
    }

    graphblas_multisource_parent_bfs(csr, gb_matrix, sources, num_sources, parent);
    for (int run = 0; run < num_runs; run++) {
        start = get_time_in_seconds();
        graphblas_multisource_parent_bfs(csr, gb_matrix, sources, num_sources, parent);
        end = get_time_in_seconds();
        t_graphblas_multisource[run] = (end - start) * 1e6;
    }

    printf("\n Results (microseconds) \n");
    printf("Metric                    | Avg    | Min    | Max\n");
    printf("--------------------------|--------|--------|--------\n");
    printf("Classic Parent BFS        | %6.0f | %6.0f | %6.0f\n",
           average(t_classic_parent, num_runs), min_value(t_classic_parent, num_runs), max_value(t_classic_parent, num_runs));
    printf("Boost BGL Parent BFS      | %6.0f | %6.0f | %6.0f\n",
           average(t_bgl_parent, num_runs), min_value(t_bgl_parent, num_runs), max_value(t_bgl_parent, num_runs));
    printf("Classic Multisource BFS   | %6.0f | %6.0f | %6.0f\n",
           average(t_classic_multisource, num_runs), min_value(t_classic_multisource, num_runs), max_value(t_classic_multisource, num_runs));
    printf("GraphBLAS Parent BFS      | %6.0f | %6.0f | %6.0f\n",
           average(t_graphblas_level, num_runs), min_value(t_graphblas_level, num_runs), max_value(t_graphblas_level, num_runs));
    printf("GraphBLAS Multisource BFS | %6.0f | %6.0f | %6.0f\n",
           average(t_graphblas_multisource, num_runs), min_value(t_graphblas_multisource, num_runs), max_value(t_graphblas_multisource, num_runs));

    printf("\n Speedups \n");
    printf("GraphBLAS Parent / Classic Parent: %.2fx\n", average(t_classic_parent, num_runs) / average(t_graphblas_level, num_runs));
    printf("Boost BGL Parent / Classic Parent: %.2fx\n", average(t_classic_parent, num_runs) / average(t_bgl_parent, num_runs));

    FILE* csv = fopen("benchmark_results.csv", "a");
    if (csv != NULL) {
        fseek(csv, 0, SEEK_END);
        if (ftell(csv) == 0) {
            fprintf(csv, "Graph,V,E,avg_degree,max_width,max_level,CP_us,GB-P_us,CM_us,GB-M_us,BGL_us,Speedup_GB-P_CP,Speedup_GB-M_CM,Speedup_CP_BGL\n");
        }

        double avg_degree = (double)true_edges / csr->n;
        const char* g_name = strrchr(graph_file, '/');
        if (g_name) g_name++; else g_name = graph_file;

        double cp_avg = average(t_classic_parent, num_runs);
        double cm_avg = average(t_classic_multisource, num_runs);
        double gb_p_avg = average(t_graphblas_level, num_runs);
        double gb_m_avg = average(t_graphblas_multisource, num_runs);
        double bgl_avg = average(t_bgl_parent, num_runs);

        fprintf(csv, "%s,%d,%d,%.2f,%d,%d,%.0f,%.0f,%.0f,%.0f,%.0f,%.2f,%.2f,%.2f\n",
                g_name, csr->n, true_edges, avg_degree,
                local_max_width, local_max_level,
                cp_avg,
                gb_p_avg,
                cm_avg,
                gb_m_avg,
                bgl_avg,
                cp_avg / gb_p_avg,
                cm_avg / gb_m_avg,
                cp_avg / bgl_avg
                );
        fclose(csv);
    }
    graphblas_free_matrix(gb_matrix);
    free(sources); free(parent); free(level);
    delete_graph(g); delete_csr(csr);
}

int main(int argc, char* argv[]) {
    if (graphblas_init() != 0) return 1;

    if (argc == 2) {
        run_benchmark(argv[1]);
        graphblas_finalize();
        return 0;
    }

    std::vector<std::string> graphs;
    const char* dir_path = "../graphs";
    DIR* dir = opendir(dir_path);

    if (dir == nullptr) {
        fprintf(stderr, "Cannot open directory %s\n", dir_path);
        graphblas_finalize();
        return 1;
    }

    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL) {
        std::string name = ent->d_name;

        if (name.size() > 4 && name.substr(name.size() - 4) == ".mtx") {
            graphs.push_back(std::string(dir_path) + "/" + name);
        }
    }
    closedir(dir);

    std::sort(graphs.begin(), graphs.end());

    printf("Running benchmarks on %zu graphs from %s\n", graphs.size(), dir_path);
    for (const auto& g : graphs) {
        run_benchmark(g.c_str());
    }

    graphblas_finalize();
    return 0;
}
