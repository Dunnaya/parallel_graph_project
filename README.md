# Graph Algorithms Demo

A web-based application that demonstrates sequential and parallel implementations of graph algorithms with performance benchmarking.

## Features

### Graph Generation
- Create random graphs with configurable parameters
- Build custom graphs with specific edges
- Support for both adjacency matrix and adjacency list representations

### Algorithms
- **Floyd-Warshall** (all pairs shortest paths) - sequential and parallel versions
- **Connected Components** - sequential and parallel versions
- Performance comparison between sequential and parallel implementations

### Visualization
- Interactive web interface
- Detailed algorithm output
- Performance benchmarks and charts

## Technologies

- **Backend**: C++ with OpenMP for parallel processing
- **Frontend**: HTML, CSS, JavaScript
- **Web Server**: httplib (HTTP server library)
- **Data Format**: JSON

## Getting Started

1. Clone the repository
2. Compile the C++ code (requires OpenMP support)
3. Run the server executable
4. Open `http://localhost:8080` in your browser