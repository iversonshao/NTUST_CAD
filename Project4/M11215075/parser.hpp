#pragma once
#include <string>
#include <vector>
#include <stdexcept>

// Only three types of Boolean operations as specified in project
enum class GateType { AND, OR, NOT };

class Node {
public:
    std::string output;               // Output signal name
    std::vector<std::string> inputs;  // Input signal names
    GateType type;                    // Gate type (AND/OR/NOT)
    void print() const;               // Debug print function
};

class BlifReader {
private:
    std::string model_name;           // Model name from .model
    std::vector<std::string> inputs;  // Primary inputs from .inputs
    std::vector<std::string> outputs; // Primary outputs from .outputs
    std::vector<Node> nodes;          // All logic gates

    // Helper function to split a line into tokens
    static std::vector<std::string> tokenize(const std::string& line);
    
    // Helper function to determine gate type based on:
    // 1. NOT gate: single input
    // 2. OR gate: pattern contains "-"
    // 3. AND gate: otherwise
    GateType determine_gate_type(size_t input_count, const std::string& pattern) const; 

public:
    // Parse BLIF file and build internal data structures
    void parse(const std::string& filename);
    
    // Debug print function
    void print() const;

    // Getters for ML-RCS algorithm
    const std::vector<Node>& get_nodes() const { return nodes; }
    const std::vector<std::string>& get_inputs() const { return inputs; }
    const std::vector<std::string>& get_outputs() const { return outputs; }
};