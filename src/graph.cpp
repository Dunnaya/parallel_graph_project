#include "graph.h"
#include <iostream>

GraphMatrix generate_random_graph_matrix(size_t num_vert, int max_weight, int num_edges, bool isDirected) 
{
    static bool initialized = false;
    if (!initialized) {
        srand(static_cast<unsigned int>(time(nullptr)));
        initialized = true;
    }

    size_t max_possible_edges = isDirected ? num_vert * (num_vert - 1) : num_vert * (num_vert - 1) / 2;
    
    if(num_edges > max_possible_edges)
    {
        std::cout << "Invalid input: too many edges requested.\n";
        return GraphMatrix(); // invalid graph
    }

    GraphMatrix graph(num_vert);

    int edges_added = 0;
    while (edges_added < static_cast<int>(num_edges)) 
    {
        size_t u = rand() % num_vert;
        size_t v = rand() % num_vert;
        int weight = 1 + rand() % max_weight;

        if (u != v && graph.weight_matrix[u][v] == INF) 
        {
            graph.weight_matrix[u][v] = weight;
            if(!isDirected) graph.weight_matrix[v][u] = weight;
            edges_added++;
        }
    }

    return graph;
}

GraphAdjList generate_random_graph_list(size_t num_vert, int max_weight, int num_edges, bool isDirected)
{
    static bool initialized = false;
    if (!initialized) {
        srand(static_cast<unsigned int>(time(nullptr)));
        initialized = true;
    }

    size_t max_possible_edges = isDirected ? num_vert * (num_vert - 1) : num_vert * (num_vert - 1) / 2;
    
    if(num_edges > max_possible_edges)
    {
        std::cout << "Invalid input: too many edges requested.\n";
        return GraphAdjList(); // invalid graph
    }

    GraphAdjList graph(num_vert);

    int edges_added = 0;
    while (edges_added < static_cast<int>(num_edges))
    {
        size_t from = rand() % num_vert;
        size_t to = rand() % num_vert;

        if (from != to)
        {
            int weight = 1 + rand() % max_weight;

            bool edge_exists = false;
            for (const auto& neighbor : graph.adjList[from])
            {
                if (neighbor.first == to)
                {
                    edge_exists = true;
                    break;
                }
            }

            if (!edge_exists)
            {
                graph.adjList[from].push_back(std::make_pair(to, weight));
                if (!isDirected)
                    graph.adjList[to].push_back(std::make_pair(from, weight));
                edges_added++;
            }
        }
    }

    return graph;
}

void print_matrix(const Matrix& matrix, bool benchmark) 
{
    if (benchmark) return;

    for (const auto& row : matrix) 
    {
        for (int val : row) 
        {
            if (val == INF)
                std::cout << "- ";
            else
                std::cout << val << " ";
        }
        std::cout << "\n";
    }
}

void print_adjList(const GraphAdjList& graph, bool benchmark) 
{
    if (benchmark) return;

    for (size_t i = 0; i < graph.num_vert; ++i) 
    {
        std::cout << "Vertex " << i << ":";
        for (const auto& [to, weight] : graph.adjList[i]) 
        {
            std::cout << " -> " << to << "(" << weight << ")";
        }
        std::cout << "\n";
    }
}