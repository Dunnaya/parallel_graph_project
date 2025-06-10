# Makefile for Parallel Graph Algorithms Project
CXX = g++-15
CXXFLAGS = -std=c++17 -pthread -O2 -Wall -Wextra -fopenmp
INCLUDES = -I./src -I./include -I./third_party/httplib -I./third_party/json/include
SRCDIR = ./src
SOURCES = $(SRCDIR)/parallel_graph.cpp $(SRCDIR)/graph.cpp
TARGET = parallel_graph
WEBDIR = web

# Test configuration
TESTDIR = ./tests
TESTSOURCE = $(TESTDIR)/graph_tests.cpp
TESTTARGET = graph_tests
GTEST_DIR = third_party/googletest

# Default target
all: $(TARGET)

# Main target
$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) -o $(TARGET)

# Debug build
debug: CXXFLAGS += -g -DDEBUG -O0
debug: $(TARGET)
	@echo "Debug build completed"

# Release build
release: CXXFLAGS += -O3 -DNDEBUG -march=native
release: $(TARGET)
	@echo "Release build completed"

# Build and run tests
test: $(TESTTARGET)
	./$(TESTTARGET) --gtest_color=yes

# Build test executable
$(TESTTARGET): $(TESTSOURCE) $(SOURCES)
	@echo "Building tests..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(TESTSOURCE) $(SOURCES) -o $(TESTTARGET) -lgtest -lgtest_main -pthread
	@echo "Test build complete"

# Download and setup Google Test
setup-gtest:
	@echo "Setting up Google Test..."
	@mkdir -p $(GTEST_DIR)
	@git clone https://github.com/google/googletest.git $(GTEST_DIR) || \
	 (cd $(GTEST_DIR) && git pull)
	@echo "Google Test setup complete"

# Install dependencies (requires internet connection)
deps: setup-gtest
	@echo "Installing dependencies..."
	@mkdir -p third_party
	@if [ ! -d "third_party/httplib" ]; then \
		echo "Downloading httplib..."; \
		git clone https://github.com/yhirose/cpp-httplib.git third_party/httplib || \
		(mkdir -p third_party/httplib && \
		 curl -L https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h \
		 -o third_party/httplib/httplib.h); \
	fi
	@if [ ! -d "third_party/json" ]; then \
		echo "Downloading nlohmann/json..."; \
		git clone https://github.com/nlohmann/json.git third_party/json || \
		(mkdir -p third_party/json/include/nlohmann && \
		 curl -L https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp \
		 -o third_party/json/include/nlohmann/json.hpp); \
	fi
	@echo "Dependencies installed"

# Create web directory structure
web-setup:
	@echo "Setting up web directory..."
	@mkdir -p $(WEBDIR)
	@if [ ! -f "$(WEBDIR)/index.html" ]; then \
		echo "Creating basic index.html..."; \
		echo '<!DOCTYPE html><html><head><title>Graph Algorithms</title></head><body><h1>Graph Algorithms Server</h1><p>Server is running!</p></body></html>' > $(WEBDIR)/index.html; \
	fi

# Run the server (builds if necessary)
run: $(TARGET) web-setup
	@echo "Starting server on http://localhost:8080"
	./$(TARGET)

# Check for OpenMP support
check-openmp:
	@echo "Checking OpenMP support..."
	@$(CXX) -fopenmp -dM -E - < /dev/null | grep -i openmp > /dev/null && \
		echo "✓ OpenMP is supported" || echo "✗ OpenMP not found"

# Build without OpenMP (fallback)
no-openmp: CXXFLAGS := $(filter-out -fopenmp,$(CXXFLAGS))
no-openmp: $(TARGET)
	@echo "Built without OpenMP support"

# Test build (quick compile check)
test-build:
	@echo "Testing build configuration..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -fsyntax-only $(SOURCES)
	@echo "✓ Build configuration is valid"

# Clean build artifacts
clean:
	rm -f $(TARGET) $(TESTTARGET)
	@echo "Cleaned build artifacts"

# Clean everything including dependencies
clean-all: clean
	rm -rf third_party
	@echo "Cleaned everything"

# Setup complete project
setup: deps web-setup
	@echo "Project setup complete"

# Install system dependencies (Ubuntu/Debian)
install-deps-ubuntu:
	@echo "Installing system dependencies for Ubuntu/Debian..."
	sudo apt-get update
	sudo apt-get install -y build-essential g++ libomp-dev git curl libgtest-dev cmake

# Install system dependencies (CentOS/RHEL/Fedora)
install-deps-centos:
	@echo "Installing system dependencies for CentOS/RHEL/Fedora..."
	sudo yum install -y gcc-c++ libomp-devel git curl gtest-devel cmake || \
	sudo dnf install -y gcc-c++ libomp-devel git curl gtest-devel cmake

# Install system dependencies (macOS)
install-deps-macos:
	@echo "Installing system dependencies for macOS..."
	brew update
	brew install gcc googletest cmake

# Show help
help:
	@echo "Available targets:"
	@echo "  all              - Build the project (default)"
	@echo "  debug            - Build with debug flags"
	@echo "  release          - Build with optimization flags"
	@echo "  test             - Build and run tests"
	@echo "  deps             - Download required dependencies"
	@echo "  setup-gtest      - Setup Google Test framework"
	@echo "  web-setup        - Create web directory structure"
	@echo "  run              - Build and run the server"
	@echo "  setup            - Full project setup (deps + web-setup)"
	@echo "  check-openmp     - Check if OpenMP is available"
	@echo "  no-openmp        - Build without OpenMP support"
	@echo "  test-build       - Test build configuration"
	@echo "  clean            - Remove build artifacts"
	@echo "  clean-all        - Remove build artifacts and dependencies"
	@echo "  install-deps-*   - Install system dependencies"
	@echo "  help             - Show this help message"

# Declare phony targets
.PHONY: all debug release test deps setup-gtest web-setup run check-openmp no-openmp test-build clean clean-all setup install-deps-ubuntu install-deps-centos install-deps-macos help