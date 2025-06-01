#include "../include/httplib.h"
#include "../include/json.hpp"

int main() {
    httplib::Server svr;

    svr.Get("/parallel_graph", [](const httplib::Request &req, httplib::Response &res) {
        nlohmann::json response_json;
        response_json["message"] = "This is a parallel graph endpoint.";
        res.set_content(response_json.dump(), "application/json");
    });

    svr.listen("localhost", 8080);
    return 0;
}