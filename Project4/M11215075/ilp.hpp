#pragma once
#include "parser.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include "gurobi_c++.h"

// Memory monitoring structure
struct MemoryStatus {
    double total_memory;
    double used_memory;
    double memory_usage_ratio;
};

class ILP {
private:
    std::vector<Node> nodes;
    std::unordered_map<std::string, Node> nodes_map;
    std::vector<std::string> primary_inputs;
    std::vector<std::string> primary_outputs;
    
    // New helper methods
    std::vector<int> calculate_asap();
    std::vector<int> calculate_alap(int upper_bound);
    MemoryStatus check_memory_usage();
    bool is_memory_critical();
    
    // New methods for handling large circuits
    std::vector<std::vector<std::vector<std::string>>> 
    solve_with_partitioning(int and_limit, int or_limit, int not_limit);
    
    std::vector<std::vector<Node>> partition_circuit();
    void configure_gurobi_for_size(GRBEnv& env, size_t node_count);
    
    // Helper method to validate resource constraints
    bool validate_constraints(int and_limit, int or_limit, int not_limit);

public:
    void parse(const std::vector<Node>& nodes, 
              const std::vector<std::string>& inputs,
              const std::vector<std::string>& outputs);
              
    std::vector<std::vector<std::vector<std::string>>> 
    run(int and_limit, int or_limit, int not_limit);
    
    // Constructor with optional parameters
    ILP() = default;
};

// Gurobi configuration helper
void configure_gurobi(GRBEnv& env, size_t node_count, int upper_bound);