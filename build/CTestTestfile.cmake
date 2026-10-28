# CMake generated Testfile for 
# Source directory: /home/masha/graph_blas_bfs-2
# Build directory: /home/masha/graph_blas_bfs-2/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(ClassicBFS "/home/masha/graph_blas_bfs-2/build/test_classic_bfs")
set_tests_properties(ClassicBFS PROPERTIES  _BACKTRACE_TRIPLES "/home/masha/graph_blas_bfs-2/CMakeLists.txt;172;add_test;/home/masha/graph_blas_bfs-2/CMakeLists.txt;0;")
add_test(GraphBLASBFS "/home/masha/graph_blas_bfs-2/build/test_graphblas_bfs")
set_tests_properties(GraphBLASBFS PROPERTIES  _BACKTRACE_TRIPLES "/home/masha/graph_blas_bfs-2/CMakeLists.txt;173;add_test;/home/masha/graph_blas_bfs-2/CMakeLists.txt;0;")
add_test(Comparison "/home/masha/graph_blas_bfs-2/build/test_comparison")
set_tests_properties(Comparison PROPERTIES  _BACKTRACE_TRIPLES "/home/masha/graph_blas_bfs-2/CMakeLists.txt;174;add_test;/home/masha/graph_blas_bfs-2/CMakeLists.txt;0;")
