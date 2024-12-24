#pragma once
#include "parser.hpp"
#include <vector>
#include <string>

class ListScheduling {
public:
    // Schedule nodes with resource constraints
    // Return value: [time_step][gate_type][nodes]
    // gate_type: 0 for AND, 1 for OR, 2 for NOT
    static std::vector<std::vector<std::vector<std::string>>> schedule(
        const std::vector<Node>& nodes,
        int and_limit,
        int or_limit,
        int not_limit);
    
    // Print the scheduling result
    static void printResult(const std::vector<std::vector<std::vector<std::string>>>& schedule);
};