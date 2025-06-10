#include "graph.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <iomanip>
#include <stack>
#include <vector>
#include <atomic>
#include <queue>
#include <string>

using namespace std;
using namespace std::chrono;

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
        cout << "Invalid input: too many edges requested.\n";
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
        cout << "Invalid input: too many edges requested.\n";
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
                cout << "- ";
            else
                cout << val << " ";
        }
        cout << "\n";
    }
}

void print_adjList(const GraphAdjList& graph, bool benchmark) 
{
    if (benchmark) return;

    for (size_t i = 0; i < graph.num_vert; ++i) 
    {
        cout << "Vertex " << i << ":";
        for (const auto& [to, weight] : graph.adjList[i]) 
        {
            cout << " -> " << to << "(" << weight << ")";
        }
        cout << "\n";
    }
}

void add_edge_matrix(GraphMatrix& graph, size_t from, size_t to, int weight, int offset, bool isDirected)
{
    from = from - offset;
    to = to - offset;
    graph.weight_matrix[from][to] = weight;
    if (!isDirected)
        graph.weight_matrix[to][from] = weight;
}

void add_edge_adjList(GraphAdjList& graph, size_t from, size_t to, int weight, int offset, bool isDirected)
{
    from = from - offset;
    to = to - offset;
    assert(from < graph.num_vert);
    assert(to < graph.num_vert);
    graph.adjList[from].push_back(std::make_pair(to, weight));
    if (!isDirected && from != to)
        graph.adjList[to].push_back(std::make_pair(from, weight));
}

GraphAdjList matrix_to_list(const GraphMatrix& matrix)
{
    GraphAdjList list(matrix.num_vert);

    for(size_t i = 0; i < matrix.num_vert; i++)
    {
        for(size_t j = 0; j < matrix.num_vert; j++)
        {
            if(matrix.weight_matrix[i][j] != INF)
                add_edge_adjList(list, i, j, matrix.weight_matrix[i][j]);
        }
    }
    return list;
}

GraphMatrix list_to_matrix(const GraphAdjList& list) 
{
    GraphMatrix matrix(list.num_vert);

    for(size_t i = 0; i < list.num_vert; i++)
    {
        for(const auto& edge : list.adjList[i]) 
        {
            size_t j = edge.first;
            int weight = edge.second;
            add_edge_matrix(matrix, i, j, weight);
        }
    }
    return matrix;
}

// floyd-warshall sequential
pair<Matrix, string> floyd_algorithm(const GraphMatrix& graph)
{
    auto start_time = high_resolution_clock::now();
    
    Matrix dist = graph.weight_matrix;
    stringstream result;
    
    result << "Floyd-Warshall Algorithm (All Pairs Shortest Paths)\n";
    result << "Graph size: " << graph.num_vert << " vertices\n";
    result << "Time complexity: O(V³) = O(" << graph.num_vert << "³) = O(" << 
              (graph.num_vert * graph.num_vert * graph.num_vert) << ")\n\n";

    // show initial matrix only for small graphs
    if (graph.num_vert <= 20) 
    {
        result << "Initial distance matrix:\n";
        for (const auto& row : dist) 
        {
            for (int val : row) 
            {
                if (val == INF)
                    result << "∞ ";
                else
                    result << val << " ";
            }
            result << "\n";
        }
        result << "\n";
    }
    
    // show only execution time
    auto algorithm_start = high_resolution_clock::now();
    
    for(size_t k = 0; k < graph.num_vert; k++)
    {
        for(size_t i = 0; i < graph.num_vert; i++)
        {
            for(size_t j = 0; j < graph.num_vert; j++)
            {
                if(dist[i][k] != INF && dist[k][j] != INF && dist[i][k] + dist[k][j] < dist[i][j])
                {
                    dist[i][j] = dist[i][k] + dist[k][j];
                }
            }
        }
    }
    
    auto algorithm_end = high_resolution_clock::now();
    auto end_time = high_resolution_clock::now();
    
    auto total_duration = duration_cast<microseconds>(end_time - start_time);
    auto algorithm_duration = duration_cast<microseconds>(algorithm_end - algorithm_start);
    
    double total_time_ms = total_duration.count() / 1000.0;
    double algorithm_time_ms = algorithm_duration.count() / 1000.0;

    // show final matrix only for small graphs
    if (graph.num_vert <= 20) 
    {
        result << "Final shortest paths matrix:\n";
        for (const auto& row : dist) 
        {
            for (int val : row) 
            {
                if (val == INF)
                    result << "∞ ";
                else
                    result << val << " ";
            }
            result << "\n";
        }
        result << "\n";
    }
    
    result << string(50, '=') << "\n";
    result << "PERFORMANCE BENCHMARK:\n";
    result << string(50, '=') << "\n";
    result << "Algorithm execution time: " << format_time(algorithm_time_ms) << "\n";
    result << "Total time (including I/O): " << format_time(total_time_ms) << "\n";
    result << "Operations performed: " << (graph.num_vert * graph.num_vert * graph.num_vert) << "\n";
    if (algorithm_time_ms > 0) 
    {
        result << "Operations per second: " << fixed << setprecision(0) << 
                  (graph.num_vert * graph.num_vert * graph.num_vert * 1000.0 / algorithm_time_ms) << "\n";
    }

    return make_pair(dist, result.str());
}

// connected components DFS helper func
void connected_comp_DFS(const GraphAdjList& graph, size_t vert, std::vector<bool>& isVisited, std::vector<int>& comp)
{
    isVisited[vert] = true;
    comp.push_back(vert);
    for(const auto& neighbor : graph.adjList[vert])
    {
        size_t neighbor_vert = neighbor.first;
        if(!isVisited[neighbor_vert])
            connected_comp_DFS(graph, neighbor_vert, isVisited, comp);
    }
}

// sequential connected components
pair<vector<vector<int>>, string> connected_components_algorithm(const GraphAdjList& graph)
{
    auto start_time = high_resolution_clock::now();
    
    stringstream result;
    result << "Connected Components Algorithm (DFS-based)\n";
    result << "Graph size: " << graph.num_vert << " vertices\n";
    
    // count edges
    size_t edge_count = 0;
    for (size_t i = 0; i < graph.num_vert; ++i) 
    {
        edge_count += graph.adjList[i].size();
    }
    edge_count /= 2; // for undirected graphs
    
    result << "Number of edges: " << edge_count << "\n";
    result << "Time complexity: O(V + E) = O(" << graph.num_vert << " + " << edge_count << ") = O(" << 
              (graph.num_vert + edge_count) << ")\n\n";
    
    // execution time only
    auto algorithm_start = high_resolution_clock::now();
    
    vector<vector<int>> connected_components;
    vector<bool> isVisited(graph.num_vert, false);
    
    for (size_t i = 0; i < graph.num_vert; ++i)
    {
        if (!isVisited[i])
        {
            vector<int> component;
            connected_comp_DFS(graph, i, isVisited, component);
            connected_components.push_back(component);
        }
    }
    
    auto algorithm_end = high_resolution_clock::now();
    auto end_time = high_resolution_clock::now();
    
    auto total_duration = duration_cast<microseconds>(end_time - start_time);
    auto algorithm_duration = duration_cast<microseconds>(algorithm_end - algorithm_start);
    
    double total_time_ms = total_duration.count() / 1000.0;
    double algorithm_time_ms = algorithm_duration.count() / 1000.0;
    
    // show connected components only for small graphs
    if (graph.num_vert <= 20) 
    {
        result << "Connected Components found:\n";
        for (size_t i = 0; i < connected_components.size(); ++i) 
        {
            result << "Component " << (i + 1) << ": ";
            for (size_t j = 0; j < connected_components[i].size(); ++j) 
            {
                result << connected_components[i][j];
                if (j < connected_components[i].size() - 1) result << " ";
            }
            result << " (size: " << connected_components[i].size() << ")\n";
        }
        result << "\n";
    }
    
    result << "Statistics:\n";
    result << "Number of connected components: " << connected_components.size() << "\n";
    result << "Largest component size: ";
    if (!connected_components.empty()) 
    {
        size_t max_size = 0;
        for (const auto& comp : connected_components) 
        {
            max_size = max(max_size, comp.size());
        }
        result << max_size << "\n";
    } else {
        result << "0\n";
    }
    
    result << string(50, '=') << "\n";
    result << "PERFORMANCE BENCHMARK:\n";
    result << string(50, '=') << "\n";
    result << "Algorithm execution time: " << format_time(algorithm_time_ms) << "\n";
    result << "Total time (including I/O): " << format_time(total_time_ms) << "\n";
    result << "Vertices processed: " << graph.num_vert << "\n";
    result << "Edges processed: " << edge_count << "\n";
    if (algorithm_time_ms > 0) 
    {
        result << "Vertices per second: " << fixed << setprecision(0) << 
                  (graph.num_vert * 1000.0 / algorithm_time_ms) << "\n";
    }
    
    if (connected_components.size() == 1) {
        result << "✓ Graph is connected\n";
    } else {
        result << "⚠ Graph has " << connected_components.size() << " disconnected components\n";
    }

    return make_pair(connected_components, result.str());
}