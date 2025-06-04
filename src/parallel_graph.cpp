#include "httplib.h"
#include "json.hpp"
#include "graph.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <iomanip>

using json = nlohmann::json;
using namespace std;
using namespace std::chrono;

void add_edge_matrix(GraphMatrix& graph, size_t from, size_t to, int weight, int offset = 0, bool isDirected = true)
{
    from = from - offset;
    to = to - offset;
    graph.weight_matrix[from][to] = weight;
    if (!isDirected)
        graph.weight_matrix[to][from] = weight;
}

void add_edge_adjList(GraphAdjList& graph, size_t from, size_t to, int weight, int offset = 0, bool isDirected = true)
{
    from = from - offset;
    to = to - offset;
    assert(from < graph.num_vert);
    assert(to < graph.num_vert);
    graph.adjList[from].push_back(make_pair(to, weight));
    if (!isDirected && from != to)
        graph.adjList[to].push_back(make_pair(from, weight));
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
        for(const pair<int, int>& edge : list.adjList[i])
        {
            size_t j = edge.first;
            int weight = edge.second;
            add_edge_matrix(matrix, i, j, weight);
        }
    }
    return matrix;
}

string format_time(double time_ms) 
{
    stringstream ss;
    ss << fixed << setprecision(3);
    
    if (time_ms < 1.0) {
        ss << (time_ms * 1000.0) << " μs";
    } else if (time_ms < 1000.0) {
        ss << time_ms << " ms";
    } else {
        ss << (time_ms / 1000.0) << " s";
    }
    
    return ss.str();
}

// Floyd-Warshall
pair<Matrix, string> floyd_algorithm(const GraphMatrix& graph)
{
    auto start_time = high_resolution_clock::now();
    
    Matrix dist = graph.weight_matrix;
    stringstream result;
    
    result << "Floyd-Warshall Algorithm (All Pairs Shortest Paths)\n";
    result << "Graph size: " << graph.num_vert << " vertices\n";
    result << "Time complexity: O(V³) = O(" << graph.num_vert << "³) = O(" << 
              (graph.num_vert * graph.num_vert * graph.num_vert) << ")\n\n";
    
    result << "Initial distance matrix:\n";
    
    for (const auto& row : dist) {
        for (int val : row) {
            if (val == INF)
                result << "∞ ";
            else
                result << val << " ";
        }
        result << "\n";
    }
    
    auto algorithm_start = high_resolution_clock::now();
    
    for(size_t k = 0; k < graph.num_vert; k++)
    {
        result << "\nStep " << (k+1) << " (via vertex " << k << "):\n";
        
        for(size_t i = 0; i < graph.num_vert; i++)
        {
            for(size_t j = 0; j < graph.num_vert; j++)
            {
                if(dist[i][k] != INF && dist[k][j] != INF && dist[i][k] + dist[k][j] < dist[i][j])
                {
                    result << "Update dist[" << i << "][" << j << "] from " << 
                        (dist[i][j] == INF ? "∞" : to_string(dist[i][j])) << 
                        " to " << (dist[i][k] + dist[k][j]) << "\n";
                    dist[i][j] = dist[i][k] + dist[k][j];
                }
            }
        }
        
        for (const auto& row : dist) {
            for (int val : row) {
                if (val == INF)
                    result << "∞ ";
                else
                    result << val << " ";
            }
            result << "\n";
        }
    }
    
    auto algorithm_end = high_resolution_clock::now();
    auto end_time = high_resolution_clock::now();
    
    auto total_duration = duration_cast<microseconds>(end_time - start_time);
    auto algorithm_duration = duration_cast<microseconds>(algorithm_end - algorithm_start);
    
    double total_time_ms = total_duration.count() / 1000.0;
    double algorithm_time_ms = algorithm_duration.count() / 1000.0;
    
    result << "\nFinal shortest paths matrix:\n";
    for (const auto& row : dist) {
        for (int val : row) {
            if (val == INF)
                result << "∞ ";
            else
                result << val << " ";
        }
        result << "\n";
    }
    
    result << "\n" << string(50, '=') << "\n";
    result << "PERFORMANCE BENCHMARK:\n";
    result << string(50, '=') << "\n";
    result << "Algorithm execution time: " << format_time(algorithm_time_ms) << "\n";
    result << "Total time (including I/O): " << format_time(total_time_ms) << "\n";
    result << "Operations performed: " << (graph.num_vert * graph.num_vert * graph.num_vert) << "\n";
    result << "Operations per second: " << fixed << setprecision(0) << 
              (graph.num_vert * graph.num_vert * graph.num_vert * 1000.0 / algorithm_time_ms) << "\n";

    return make_pair(dist, result.str());
}

// structures for Kruskal's algorithm
struct Edge 
{
    int from, to, weight;
    Edge(int from, int to, int weight) : from(from), to(to), weight(weight) {}
};

bool edge_compare(const Edge& a, const Edge& b) 
{
    return a.weight < b.weight;
}

int find_parent(int vert, vector<int>& parent) 
{
    if (parent[vert] == -1)
        return vert;
    return find_parent(parent[vert], parent);
}

void union_sets(int x, int y, vector<int>& parent) 
{
    int parentX = find_parent(x, parent);
    int parentY = find_parent(y, parent);
    parent[parentY] = parentX;
}

// Kruskal
pair<GraphAdjList, string> kruskal_algorithm(const GraphAdjList& graph)
{
    auto start_time = high_resolution_clock::now();
    
    stringstream result;
    result << "Kruskal's Algorithm (Minimum Spanning Tree)\n";
    
    vector<Edge> edges;
    for (size_t i = 0; i < graph.num_vert; ++i) 
    {
        for (const auto& neighbor : graph.adjList[i]) 
        {
            // Добавляем ребро только один раз для неориентированного графа
            if (i < neighbor.first) {
                edges.push_back(Edge(i, neighbor.first, neighbor.second));
            }
        }
    }
    
    result << "Graph size: " << graph.num_vert << " vertices, " << edges.size() << " edges\n";
    result << "Time complexity: O(E log E) = O(" << edges.size() << " log " << edges.size() << ")\n\n";

    result << "All edges sorted by weight:\n";
    
    auto sort_start = high_resolution_clock::now();
    sort(edges.begin(), edges.end(), edge_compare);
    auto sort_end = high_resolution_clock::now();
    
    for (const auto& edge : edges) {
        result << "(" << edge.from << "-" << edge.to << ", weight: " << edge.weight << ")\n";
    }
    result << "\n";

    auto algorithm_start = high_resolution_clock::now();
    
    vector<int> parent(graph.num_vert, -1);
    GraphAdjList mst(graph.num_vert);

    int totalWeight = 0;
    int edgesAdded = 0;
    result << "Building MST:\n";
    
    for (const auto& edge : edges) 
    {
        int from = edge.from;
        int to = edge.to;
        int weight = edge.weight;

        int parentFrom = find_parent(from, parent);
        int parentTo = find_parent(to, parent);

        if (parentFrom != parentTo) 
        {
            result << "Adding edge (" << from << "-" << to << ", weight: " << weight << 
                      ") - No cycle formed\n";
            totalWeight += weight;
            edgesAdded++;
            union_sets(parentFrom, parentTo, parent);
            add_edge_adjList(mst, from, to, weight, 0, false);
        }
        else 
        {
            result << "Skipping edge (" << from << "-" << to << ", weight: " << weight << 
                      ") - Would create cycle\n";
        }
    }
    
    auto algorithm_end = high_resolution_clock::now();
    auto end_time = high_resolution_clock::now();
    
    auto total_duration = duration_cast<microseconds>(end_time - start_time);
    auto algorithm_duration = duration_cast<microseconds>(algorithm_end - algorithm_start);
    auto sort_duration = duration_cast<microseconds>(sort_end - sort_start);
    
    double total_time_ms = total_duration.count() / 1000.0;
    double algorithm_time_ms = algorithm_duration.count() / 1000.0;
    double sort_time_ms = sort_duration.count() / 1000.0;
    
    result << "\nMinimal Spanning Tree:\n";
    for (size_t i = 0; i < mst.num_vert; ++i) 
    {
        result << "Vertex " << i << ":";
        for (const auto& neighbor : mst.adjList[i]) 
        {
            result << " -> " << neighbor.first << "(" << neighbor.second << ")";
        }
        result << "\n";
    }
    
    result << "\nMST Statistics:\n";
    result << "Total weight of MST: " << totalWeight << "\n";
    result << "Edges in MST: " << edgesAdded << " (expected: " << (graph.num_vert - 1) << ")\n";
    
    result << "\n" << string(50, '=') << "\n";
    result << "PERFORMANCE BENCHMARK:\n";
    result << string(50, '=') << "\n";
    result << "Sorting time: " << format_time(sort_time_ms) << "\n";
    result << "Algorithm execution time: " << format_time(algorithm_time_ms) << "\n";
    result << "Total time (including I/O): " << format_time(total_time_ms) << "\n";
    result << "Edges processed: " << edges.size() << "\n";
    result << "Edges per second: " << fixed << setprecision(0) << 
              (edges.size() * 1000.0 / algorithm_time_ms) << "\n";
    
    if (edgesAdded == graph.num_vert - 1) {
        result << "✓ MST is complete (connected graph)\n";
    } else {
        result << "⚠ MST is incomplete (disconnected graph)\n";
    }

    return make_pair(mst, result.str());
}

int main() {
    httplib::Server svr;

    svr.set_mount_point("/", "./web");

    svr.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        return httplib::Server::HandlerResponse::Unhandled;
    });

    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        return;
    });

    // API
    svr.Post("/generate", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);

            size_t num_vert = j.at("num_vert").get<size_t>();
            size_t num_edges = j.at("num_edges").get<size_t>();
            int max_weight = j.at("max_weight").get<int>();
            bool is_directed = j.at("is_directed").get<bool>();
            int graph_type = j.at("graph_type").get<int>();

            json response_json;

            if (graph_type == 0) {
                GraphMatrix g = generate_random_graph_matrix(num_vert, max_weight, num_edges, is_directed);
                
                if (!g.valid) {
                    res.status = 400;
                    res.set_content("Error: Invalid graph parameters", "text/plain");
                    return;
                }
                
                json matrix_json = json::array();
                for (const auto& row : g.weight_matrix) {
                    json row_json = json::array();
                    for (int val : row) {
                        if (val == INF) {
                            row_json.push_back(json(nullptr));
                        } else {
                            row_json.push_back(val);
                        }
                    }
                    matrix_json.push_back(row_json);
                }
                response_json["matrix"] = matrix_json;
            } else {
                GraphAdjList g = generate_random_graph_list(num_vert, max_weight, num_edges, is_directed);
                
                if (!g.valid) {
                    res.status = 400;
                    res.set_content("Error: Invalid graph parameters", "text/plain");
                    return;
                }
                
                json list_json = json::array();
                for (const auto& adj : g.adjList) {
                    json adj_json = json::array();
                    for (const auto& neighbor : adj) {
                        adj_json.push_back({{"to", neighbor.first}, {"weight", neighbor.second}});
                    }
                    list_json.push_back(adj_json);
                }
                response_json["adjList"] = list_json;
            }

            res.set_content(response_json.dump(4), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(std::string("Error: ") + e.what(), "text/plain");
        }
    });

    svr.Post("/create_custom", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            
            size_t num_vert = j.at("num_vert").get<size_t>();
            bool is_directed = j.at("is_directed").get<bool>();
            int graph_type = j.at("graph_type").get<int>();
            auto edges = j.at("edges");

            json response_json;

            if (graph_type == 0) {
                GraphMatrix g(num_vert);
                
                for (const auto& edge : edges) {
                    size_t from = edge.at("from").get<size_t>();
                    size_t to = edge.at("to").get<size_t>();
                    int weight = edge.at("weight").get<int>();
                    
                    if (from >= num_vert || to >= num_vert) {
                        res.status = 400;
                        res.set_content("Error: Vertex index out of range", "text/plain");
                        return;
                    }
                    
                    add_edge_matrix(g, from, to, weight, 0, is_directed);
                }
                
                json matrix_json = json::array();
                for (const auto& row : g.weight_matrix) {
                    json row_json = json::array();
                    for (int val : row) {
                        if (val == INF) {
                            row_json.push_back(json(nullptr));
                        } else {
                            row_json.push_back(val);
                        }
                    }
                    matrix_json.push_back(row_json);
                }
                response_json["matrix"] = matrix_json;
            } else {
                GraphAdjList g(num_vert);
                
                for (const auto& edge : edges) {
                    size_t from = edge.at("from").get<size_t>();
                    size_t to = edge.at("to").get<size_t>();
                    int weight = edge.at("weight").get<int>();
                    
                    if (from >= num_vert || to >= num_vert) {
                        res.status = 400;
                        res.set_content("Error: Vertex index out of range", "text/plain");
                        return;
                    }
                    
                    add_edge_adjList(g, from, to, weight, 0, is_directed);
                }
                
                json list_json = json::array();
                for (const auto& adj : g.adjList) {
                    json adj_json = json::array();
                    for (const auto& neighbor : adj) {
                        adj_json.push_back({{"to", neighbor.first}, {"weight", neighbor.second}});
                    }
                    list_json.push_back(adj_json);
                }
                response_json["adjList"] = list_json;
            }

            res.set_content(response_json.dump(4), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(std::string("Error: ") + e.what(), "text/plain");
        }
    });

    // API for Floyd-Warshall
    svr.Post("/floyd", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            
            if (j.find("matrix") == j.end()) {
                res.status = 400;
                res.set_content("Error: Matrix data required for Floyd algorithm", "text/plain");
                return;
            }

            auto matrix_data = j.at("matrix");
            size_t num_vert = matrix_data.size();
            GraphMatrix graph(num_vert);

            for (size_t i = 0; i < num_vert; i++) {
                for (size_t j = 0; j < num_vert; j++) {
                    if (matrix_data[i][j].is_null()) {
                        graph.weight_matrix[i][j] = INF;
                    } else {
                        graph.weight_matrix[i][j] = matrix_data[i][j].get<int>();
                    }
                }
            }

            auto result = floyd_algorithm(graph);
            json response_json;
            response_json["result"] = result.second;

            res.set_content(response_json.dump(4), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(std::string("Error: ") + e.what(), "text/plain");
        }
    });

    // API for Kruskal
    svr.Post("/kruskal", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            
            GraphAdjList graph;
            
            if (j.find("adjList") != j.end()) {
                auto list_data = j.at("adjList");
                graph = GraphAdjList(list_data.size());
                
                for (size_t i = 0; i < list_data.size(); i++) {
                    for (const auto& neighbor : list_data[i]) {
                        size_t to = neighbor.at("to").get<size_t>();
                        int weight = neighbor.at("weight").get<int>();
                        graph.adjList[i].push_back(make_pair(to, weight));
                    }
                }
            } else if (j.find("matrix") != j.end()) {
                auto matrix_data = j.at("matrix");
                size_t num_vert = matrix_data.size();
                GraphMatrix temp_graph(num_vert);
                
                for (size_t i = 0; i < num_vert; i++) {
                    for (size_t j = 0; j < num_vert; j++) {
                        if (!matrix_data[i][j].is_null()) {
                            temp_graph.weight_matrix[i][j] = matrix_data[i][j].get<int>();
                        }
                    }
                }
                
                graph = matrix_to_list(temp_graph);
            } else {
                res.status = 400;
                res.set_content("Error: Graph data required for Kruskal algorithm", "text/plain");
                return;
            }

            auto result = kruskal_algorithm(graph);
            json response_json;
            response_json["result"] = result.second;

            res.set_content(response_json.dump(4), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(std::string("Error: ") + e.what(), "text/plain");
        }
    });

    std::cout << "Server started at http://localhost:8080\n";
    std::cout << "Open http://localhost:8080 in your browser\n";
    svr.listen("0.0.0.0", 8080);

    return 0;
}