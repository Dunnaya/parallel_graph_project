#pragma once

#include "graph.h"
#include <vector>
#include <utility>
#include <string>
#include <atomic>

std::pair<std::vector<std::vector<int>>, std::string> 
connected_components_algorithm_parallel(const GraphAdjList& graph, int num_threads = 0);

std::pair<Matrix, std::string> 
floyd_algorithm_parallel(const GraphMatrix& graph, int num_threads = 0);

std::pair<std::string, std::string> 
compare_algorithms(const GraphMatrix& matrix_graph, const GraphAdjList& list_graph, int num_threads = 4);