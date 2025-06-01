#include "../include/httplib.h"
#include "../include/json.hpp"

int main() {
    httplib::Server svr;

    // Корневая страница
    svr.Get("/", [](const httplib::Request &req, httplib::Response &res) {
        std::string html = R"(
            <!DOCTYPE html>
            <html>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <head>
                <title>Parallel Graph Server</title>
            </head>
            <body>
                <h1>IT WORKS OMG!!!</h1>
                <p>Endpoint: <a href="/parallel_graph">/parallel_graph</a></p>
            </body>
            </html>
        )";
        res.set_content(html, "text/html");
    });

    // Эндпоинт с JSON
    svr.Get("/parallel_graph", [](const httplib::Request &req, httplib::Response &res) {
        nlohmann::json response_json;
        response_json["message"] = "This is a parallel graph endpoint.";
        res.set_content(response_json.dump(), "application/json");
    });

    std::cout << "Server is running at http://localhost:8080" << std::endl;

    svr.listen("localhost", 8080);
    return 0;
}
