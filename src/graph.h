#pragma once

#include <vector>
#include <list>
#include <utility>
#include <iostream>
#include <climits>
#include <cstdlib>
#include <ctime>

typedef std::vector<std::vector<int> > Matrix;
const int INF = INT_MAX / 2;

struct GraphMatrix 
{
    size_t num_vert;
    Matrix weight_matrix;
    bool valid;

    GraphMatrix(size_t num_vert) : num_vert(num_vert), weight_matrix(num_vert, std::vector<int>(num_vert, INF)), valid(true) 
    {
        for (size_t i = 0; i < num_vert; ++i)
            weight_matrix[i][i] = 0;
    }
    
    GraphMatrix() : num_vert(0), valid(false) {}
};

struct GraphAdjList 
{
    size_t num_vert;
    std::vector<std::list<std::pair<size_t, int> > > adjList;
    bool valid;

    GraphAdjList(size_t num_vert) : num_vert(num_vert), adjList(num_vert), valid(true) {}
    GraphAdjList() : num_vert(0), valid(false) {}
};

GraphMatrix generate_random_graph_matrix(size_t num_vert, int max_weight, int num_edges, bool isDirected = false);
GraphAdjList generate_random_graph_list(size_t num_vert, int max_weight, int num_edges, bool isDirected = false);
void print_matrix(const Matrix& matrix, bool benchmark = false);
void print_adjList(const GraphAdjList& graph, bool benchmark = false);