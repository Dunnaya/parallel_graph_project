#include "httplib.h"
#include "json.hpp"
#include "graph.h"

#include <iostream>
#include <fstream>

using json = nlohmann::json;

int main() {
    httplib::Server svr;

    // Обслуживание статики — папка web с index.html и скриптами
    svr.set_mount_point("/", "./web");

    // Добавляем CORS заголовки
    svr.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        return httplib::Server::HandlerResponse::Unhandled;
    });

    // Обработка OPTIONS запросов для CORS
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        return;
    });

    // API для генерации графа
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
                    for (const auto& [to, weight] : adj) {
                        adj_json.push_back({{"to", to}, {"weight", weight}});
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

    std::cout << "Server started at http://localhost:8080\n";
    std::cout << "Open http://localhost:8080 in your browser\n";
    svr.listen("0.0.0.0", 8080);

    return 0;
}