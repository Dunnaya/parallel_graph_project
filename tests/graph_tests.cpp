#include "graph.h"
#include "parallel_graph.h"
#include "googletest/googletest/include/gtest/gtest.h"
#include <vector>
#include <limits>

const int INF = std::numeric_limits<int>::max() / 2;

class GraphTest : public ::testing::Test {
protected:
    void SetUp() override {
        // small undirected graph (adjacency list)
        small_undirected_list = GraphAdjList(4);
        add_edge_adjList(small_undirected_list, 0, 1, 2);
        add_edge_adjList(small_undirected_list, 0, 2, 4);
        add_edge_adjList(small_undirected_list, 1, 2, 1);
        add_edge_adjList(small_undirected_list, 2, 3, 3);

        // small directed graph (matrix)
        small_directed_matrix = GraphMatrix(4);
        add_edge_matrix(small_directed_matrix, 0, 1, 1, 0, true);
        add_edge_matrix(small_directed_matrix, 0, 2, 4, 0, true);
        add_edge_matrix(small_directed_matrix, 1, 2, 2, 0, true);
        add_edge_matrix(small_directed_matrix, 2, 0, 1, 0, true);
        add_edge_matrix(small_directed_matrix, 2, 3, 5, 0, true);
    }

    GraphAdjList small_undirected_list;
    GraphMatrix small_directed_matrix;
};

TEST_F(GraphTest, GraphGeneration) {
    // test matrix generation
    GraphMatrix matrix = generate_random_graph_matrix(10, 100, 15, true);
    EXPECT_EQ(matrix.num_vert, 10);
    EXPECT_TRUE(matrix.valid);

    // test list generation
    GraphAdjList list = generate_random_graph_list(10, 100, 15, false);
    EXPECT_EQ(list.num_vert, 10);
    EXPECT_TRUE(list.valid);

    // test invalid generation (too many edges)
    GraphMatrix invalid_matrix = generate_random_graph_matrix(3, 10, 10, false);
    EXPECT_FALSE(invalid_matrix.valid);
}

TEST_F(GraphTest, GraphConversion) {
    //tTest matrix to list conversion
    GraphAdjList converted_list = matrix_to_list(small_directed_matrix);
    EXPECT_EQ(converted_list.num_vert, small_directed_matrix.num_vert);
    
    // check some edges
    EXPECT_EQ(converted_list.adjList[0].size(), 2); // 0->1 and 0->2
    EXPECT_EQ(converted_list.adjList[2].size(), 2); // 2->0 and 2->3

    // test list to matrix conversion
    GraphMatrix converted_matrix = list_to_matrix(small_undirected_list);
    EXPECT_EQ(converted_matrix.num_vert, small_undirected_list.num_vert);

    // check some edges
    EXPECT_EQ(converted_matrix.weight_matrix[0][1], 2);
    EXPECT_EQ(converted_matrix.weight_matrix[1][0], 2); // undirected
    EXPECT_EQ(converted_matrix.weight_matrix[2][3], 3);
}

TEST_F(GraphTest, FloydWarshallSequential) {
    auto [dist, report] = floyd_algorithm(small_directed_matrix);
    
    // check some shortest paths
    EXPECT_EQ(dist[0][0], 0);
    EXPECT_EQ(dist[0][2], 3); // 0->1->2
    EXPECT_EQ(dist[2][1], 3); // 2->0->1
    EXPECT_EQ(dist[1][3], 7); // 1->2->3
    
    // verify INF for unreachable nodes (none in this graph)
    EXPECT_EQ(dist[3][0], INF);
}

TEST_F(GraphTest, FloydWarshallParallel) {
    auto [dist, report] = floyd_algorithm_parallel(small_directed_matrix, 2);
    
    // check some shortest paths (should match sequential)
    EXPECT_EQ(dist[0][0], 0);
    EXPECT_EQ(dist[0][2], 3);
    EXPECT_EQ(dist[2][1], 3);
    EXPECT_EQ(dist[1][3], 7);
    EXPECT_EQ(dist[3][0], INF);
}

TEST_F(GraphTest, ConnectedComponentsSequential) {
    // add a disconnected vertex
    GraphAdjList test_graph = small_undirected_list;
    test_graph.num_vert = 5;
    test_graph.adjList.resize(5);
    
    auto [components, report] = connected_components_algorithm(test_graph);
    
    // should have 2 components (0-1-2-3 and 4)
    EXPECT_EQ(components.size(), 2);

    // check component sizes
    EXPECT_TRUE((components[0].size() == 4 && components[1].size() == 1) ||
                (components[0].size() == 1 && components[1].size() == 4));
}

TEST_F(GraphTest, ConnectedComponentsParallel) {
    // add a disconnected vertex
    GraphAdjList test_graph = small_undirected_list;
    test_graph.num_vert = 5;
    test_graph.adjList.resize(5);
    
    auto [components, report] = connected_components_algorithm_parallel(test_graph, 2);
    
    // should have 2 components (0-1-2-3 and 4)
    EXPECT_EQ(components.size(), 2);

    // check component sizes (should match sequential)
    EXPECT_TRUE((components[0].size() == 4 && components[1].size() == 1) ||
                (components[0].size() == 1 && components[1].size() == 4));
}

TEST_F(GraphTest, AlgorithmComparison) {
    auto [comparison, details] = compare_algorithms(small_directed_matrix, small_undirected_list, 2);
    
    // just verify the comparison string isn't empty
    EXPECT_FALSE(comparison.empty());
    EXPECT_FALSE(details.empty());

    // should contain both algorithm names
    EXPECT_TRUE(comparison.find("FLOYD-WARSHALL") != std::string::npos);
    EXPECT_TRUE(comparison.find("CONNECTED COMPONENTS") != std::string::npos);
}

TEST_F(GraphTest, EdgeOperations) {
    GraphMatrix matrix(5);
    add_edge_matrix(matrix, 0, 1, 5, 0, true);
    add_edge_matrix(matrix, 1, 2, 3, 0, false); // undirected
    
    EXPECT_EQ(matrix.weight_matrix[0][1], 5);
    EXPECT_EQ(matrix.weight_matrix[1][0], INF); // directed
    EXPECT_EQ(matrix.weight_matrix[1][2], 3);
    EXPECT_EQ(matrix.weight_matrix[2][1], 3); // undirected
    
    GraphAdjList list(5);
    add_edge_adjList(list, 0, 1, 5, 0, true);
    add_edge_adjList(list, 1, 2, 3, 0, false); // undirected
    
    EXPECT_EQ(list.adjList[0].front().first, 1);
    EXPECT_EQ(list.adjList[0].front().second, 5);
    EXPECT_TRUE(list.adjList[1].size() == 2); // 1->2 and 2->1 (undirected)
}

TEST_F(GraphTest, LargeGraphPerformance) {
    // generate a larger graph for performance testing
    GraphMatrix large_matrix = generate_random_graph_matrix(100, 100, 2000, true);
    GraphAdjList large_list = matrix_to_list(large_matrix);
    
    // just verify they run without errors
    EXPECT_NO_THROW(floyd_algorithm(large_matrix));
    EXPECT_NO_THROW(floyd_algorithm_parallel(large_matrix, 4));
    EXPECT_NO_THROW(connected_components_algorithm(large_list));
    EXPECT_NO_THROW(connected_components_algorithm_parallel(large_list, 4));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}