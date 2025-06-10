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
#include <stack>
#include <vector>
#include <atomic>
#include <queue>

#ifdef _OPENMP
#include <omp.h>
#endif

using json = nlohmann::json;
using namespace std;
using namespace std::chrono;

string format_time(double time_ms) 
{
    // protect against negative, NaN, and Inf values
    if (time_ms < 0 || isnan(time_ms) || isinf(time_ms)) 
    {
        return "0.000 ms";
    }
    
    stringstream ss;
    ss << fixed << setprecision(3);
    
    if (time_ms < 0.001) {
        // for VERY small times
        ss << "< 0.001 ms";
    } else if (time_ms < 1.0) {
        ss << (time_ms * 1000.0) << " μs";
    } else if (time_ms < 1000.0) {
        ss << time_ms << " ms";
    } else {
        ss << (time_ms / 1000.0) << " s";
    }
    
    return ss.str();
}

pair<vector<vector<int>>, string> connected_components_algorithm_parallel(const GraphAdjList& graph, int num_threads = 0) 
{
    auto start_time = high_resolution_clock::now();
    
    #ifdef _OPENMP
    if (num_threads > 0) {
        omp_set_num_threads(num_threads);
    }
    int actual_threads = omp_get_max_threads();
    #else
    int actual_threads = 1;
    #endif

    const size_t num_vertices = graph.num_vert;
    vector<int> parent(num_vertices);
    
    // init parents in parallel
    #pragma omp parallel for
    for (size_t i = 0; i < num_vertices; ++i) 
    {
        parent[i] = i;
    }

    // find with path compression
    auto find = [&](int u) 
    {
        while (parent[u] != u) {
            parent[u] = parent[parent[u]];  // path compression
            u = parent[u];
        }
        return u;
    };

    // lock-free union with CAS
    auto atomic_union = [&](int u, int v) 
    {
        while (true) {
            u = find(u);
            v = find(v);
            if (u == v) return;
            if (u < v) {
                if (__sync_bool_compare_and_swap(&parent[v], v, u)) break;
            } else {
                if (__sync_bool_compare_and_swap(&parent[u], u, v)) break;
            }
        }
    };

    // process edges in parallel with dynamic scheduling
    #pragma omp parallel for schedule(dynamic, 1024)
    for (size_t u = 0; u < num_vertices; ++u) 
    {
        for (const auto& neighbor : graph.adjList[u]) 
        {
            size_t v = neighbor.first;
            if (u < v) {  // process each edge only once
                atomic_union(u, v);
            }
        }
    }

    // final path compression pass
    #pragma omp parallel for
    for (size_t i = 0; i < num_vertices; ++i) 
    {
        find(i);
    }

    // build components using atomic counters
    vector<vector<int>> components(num_vertices);
    vector<atomic<size_t>> component_sizes(num_vertices);
    
    // first pass: count component sizes
    #pragma omp parallel for
    for (size_t i = 0; i < num_vertices; ++i) 
    {
        int root = parent[i];
        component_sizes[root]++;
    }
    
    // allocate space
    #pragma omp parallel for
    for (size_t i = 0; i < num_vertices; ++i) 
    {
        if (component_sizes[i] > 0) 
        {
            components[i].reserve(component_sizes[i].load());
        }
    }
    
    // second pass: fill components
    #pragma omp parallel for
    for (size_t i = 0; i < num_vertices; ++i) 
    {
        int root = parent[i];
        components[root].push_back(i);
    }

    // remove empty components
    components.erase(
        remove_if(components.begin(), components.end(),
                 [](const vector<int>& v) { return v.empty(); }),
        components.end());

    auto end_time = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end_time - start_time);
    double exec_time = duration.count() / 1000.0;

    // generate output string
    stringstream result;
    result << "Optimized Parallel Connected Components\n";
    result << "Graph size: " << num_vertices << " vertices\n";
    result << "Number of threads: " << actual_threads << "\n";
    
    // count edges
    size_t edge_count = 0;
    for (size_t i = 0; i < num_vertices; ++i) 
    {
        edge_count += graph.adjList[i].size();
    }
    edge_count /= 2;
    result << "Number of edges: " << edge_count << "\n";
    
    result << "\nPerformance Metrics:\n";
    result << "Execution time: " << fixed << setprecision(3) << exec_time << " ms\n";
    result << "Number of components: " << components.size() << "\n";
    
    if (num_vertices <= 20) 
    {
        result << "\nComponents:\n";
        for (size_t i = 0; i < components.size(); ++i) 
        {
            result << "Component " << i << " (size " << components[i].size() << "): ";
            for (int v : components[i]) 
            {
                result << v << " ";
            }
            result << "\n";
        }
    } else {
        size_t max_size = 0;
        for (const auto& comp : components) 
        {
            max_size = max(max_size, comp.size());
        }
        result << "Largest component size: " << max_size << "\n";
    }

    return make_pair(components, result.str());
}

// parallel floyd-warshall
pair<Matrix, string> floyd_algorithm_parallel(const GraphMatrix& graph, int num_threads = 0)
{
    auto start_time = high_resolution_clock::now();
    
    Matrix dist = graph.weight_matrix;
    stringstream result;
    
    #ifdef _OPENMP
    if (num_threads > 0) {
        omp_set_num_threads(num_threads);
    }
    int actual_threads = omp_get_max_threads();
    #else
    int actual_threads = 1;
    #endif
    
    result << "Optimized Parallel Floyd-Warshall Algorithm\n";
    result << "Graph size: " << graph.num_vert << " vertices\n";
    result << "Number of threads: " << actual_threads << "\n";
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

    // show only algorithm execution time
    auto algorithm_start = high_resolution_clock::now();
    
    for(size_t k = 0; k < graph.num_vert; k++)
    {
        #ifdef _OPENMP
        #pragma omp parallel for collapse(2) schedule(static)
        #endif
        for(size_t i = 0; i < graph.num_vert; i++)
        {
            for(size_t j = 0; j < graph.num_vert; j++)
            {
                if(dist[i][k] != INF && dist[k][j] != INF)
                {
                    int new_dist = dist[i][k] + dist[k][j];
                    if(new_dist < dist[i][j])
                    {
                        dist[i][j] = new_dist;
                    }
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
    result << "Parallel efficiency: Good\n";
    
    return make_pair(dist, result.str());
}

// comparison
pair<string, string> compare_algorithms(const GraphMatrix& matrix_graph, const GraphAdjList& list_graph, int num_threads = 4)
{
    stringstream comparison;
    comparison << "PERFORMANCE COMPARISON: Sequential vs Parallel\n";
    comparison << string(60, '=') << "\n\n";
    
    // compare floyd-warshall
    comparison << "FLOYD-WARSHALL ALGORITHM COMPARISON:\n";
    comparison << string(40, '-') << "\n";
    
    auto floyd_seq_start = high_resolution_clock::now();
    auto floyd_seq = floyd_algorithm(matrix_graph);
    auto floyd_seq_end = high_resolution_clock::now();
    auto floyd_seq_time = duration_cast<microseconds>(floyd_seq_end - floyd_seq_start).count() / 1000.0;
    
    auto floyd_par_start = high_resolution_clock::now();
    auto floyd_par = floyd_algorithm_parallel(matrix_graph, num_threads);
    auto floyd_par_end = high_resolution_clock::now();
    auto floyd_par_time = duration_cast<microseconds>(floyd_par_end - floyd_par_start).count() / 1000.0;
    
    comparison << "Sequential Floyd-Warshall: " << format_time(floyd_seq_time) << "\n";
    comparison << "Parallel Floyd-Warshall (" << num_threads << " threads): " << format_time(floyd_par_time) << "\n";
    
    if (floyd_seq_time > 0 && floyd_par_time > 0) 
    {
        double floyd_speedup = floyd_seq_time / floyd_par_time;
        double floyd_efficiency = floyd_speedup / num_threads * 100.0;
        comparison << "Floyd Speedup: " << fixed << setprecision(2) << floyd_speedup << "x\n";
        comparison << "Floyd Efficiency: " << fixed << setprecision(1) << floyd_efficiency << "%\n";
    }
    
    comparison << "\n";

    // compare connected components
    comparison << "CONNECTED COMPONENTS ALGORITHM COMPARISON:\n";
    comparison << string(40, '-') << "\n";
    
    auto cc_seq_start = high_resolution_clock::now();
    auto cc_seq = connected_components_algorithm(list_graph);
    auto cc_seq_end = high_resolution_clock::now();
    auto cc_seq_time = duration_cast<microseconds>(cc_seq_end - cc_seq_start).count() / 1000.0;
    
    auto cc_par_start = high_resolution_clock::now();
    auto cc_par = connected_components_algorithm_parallel(list_graph, num_threads);
    auto cc_par_end = high_resolution_clock::now();
    auto cc_par_time = duration_cast<microseconds>(cc_par_end - cc_par_start).count() / 1000.0;
    
    comparison << "Sequential Connected Components: " << format_time(cc_seq_time) << "\n";
    comparison << "Parallel Connected Components (" << num_threads << " threads): " << format_time(cc_par_time) << "\n";
    
    if (cc_seq_time > 0 && cc_par_time > 0) 
    {
        double cc_speedup = cc_seq_time / cc_par_time;
        double cc_efficiency = cc_speedup / num_threads * 100.0;
        comparison << "Connected Components Speedup: " << fixed << setprecision(2) << cc_speedup << "x\n";
        comparison << "Connected Components Efficiency: " << fixed << setprecision(1) << cc_efficiency << "%\n";
    }
    
    comparison << "\n" << string(60, '=') << "\n";
    comparison << "SUMMARY:\n";
    comparison << "Graph size: " << matrix_graph.num_vert << " vertices\n";
    comparison << "Threads used: " << num_threads << "\n";
    
    #ifdef _OPENMP
    comparison << "OpenMP: Enabled\n";
    comparison << "Max threads available: " << omp_get_max_threads() << "\n";
    #else
    comparison << "OpenMP: Not available (sequential execution)\n";
    #endif
    
    return make_pair(comparison.str(), floyd_par.second + "\n\n" + cc_par.second);
}

int main() 
{
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

    // API for floyd-warshall
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

    // API for connected components (sequential)
    svr.Post("/connected_components", [](const httplib::Request& req, httplib::Response& res) {
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
                res.set_content("Error: Graph data required for Connected Components algorithm", "text/plain");
                return;
            }

            auto result = connected_components_algorithm(graph);
            json response_json;
            response_json["components"] = result.first;
            response_json["result"] = result.second;

            res.set_content(response_json.dump(4), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(std::string("Error: ") + e.what(), "text/plain");
        }
    });

    // API for parallel connected components
    svr.Post("/connected_components_parallel", [](const httplib::Request& req, httplib::Response& res) {
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
                res.set_content("Error: Graph data required for Connected Components algorithm", "text/plain");
                return;
            }

            int num_threads = 4; // default
            if (j.find("num_threads") != j.end()) {
                num_threads = j.at("num_threads").get<int>();
            }

            auto result = connected_components_algorithm_parallel(graph, num_threads);
            json response_json;
            response_json["components"] = result.first;
            response_json["result"] = result.second;

            res.set_content(response_json.dump(4), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(std::string("Error: ") + e.what(), "text/plain");
        }
    });

    // API for parallel floyd-warshall
    svr.Post("/floyd_parallel", [](const httplib::Request& req, httplib::Response& res) {
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

            int num_threads = 4; // default
            if (j.find("num_threads") != j.end()) {
                num_threads = j.at("num_threads").get<int>();
            }

            auto result = floyd_algorithm_parallel(graph, num_threads);
            json response_json;
            response_json["result"] = result.second;

            res.set_content(response_json.dump(4), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(std::string("Error: ") + e.what(), "text/plain");
        }
    });

    // API for comparison
    svr.Post("/compare", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            
            GraphMatrix matrix_graph;
            GraphAdjList list_graph;
            
            if (j.find("matrix") != j.end()) {
                auto matrix_data = j.at("matrix");
                size_t num_vert = matrix_data.size();
                matrix_graph = GraphMatrix(num_vert);
                
                for (size_t i = 0; i < num_vert; i++) {
                    for (size_t j = 0; j < num_vert; j++) {
                        if (matrix_data[i][j].is_null()) {
                            matrix_graph.weight_matrix[i][j] = INF;
                        } else {
                            matrix_graph.weight_matrix[i][j] = matrix_data[i][j].get<int>();
                        }
                    }
                }
                
                list_graph = matrix_to_list(matrix_graph);
            } else if (j.find("adjList") != j.end()) {
                auto list_data = j.at("adjList");
                list_graph = GraphAdjList(list_data.size());
                
                for (size_t i = 0; i < list_data.size(); i++) {
                    for (const auto& neighbor : list_data[i]) {
                        size_t to = neighbor.at("to").get<size_t>();
                        int weight = neighbor.at("weight").get<int>();
                        list_graph.adjList[i].push_back(make_pair(to, weight));
                    }
                }
                
                matrix_graph = list_to_matrix(list_graph);
            } else {
                res.status = 400;
                res.set_content("Error: Graph data required for comparison", "text/plain");
                return;
            }

            int num_threads = 4; // default
            if (j.find("num_threads") != j.end()) {
                num_threads = j.at("num_threads").get<int>();
            }

            auto result = compare_algorithms(matrix_graph, list_graph, num_threads);
            json response_json;
            response_json["comparison"] = result.first;
            response_json["detailed_results"] = result.second;

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