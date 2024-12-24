#include "parser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
using namespace std;

vector<string> BlifReader::tokenize(const string& line) {
    vector<string> tokens;
    istringstream ss(line);
    string token;
    
    while (ss >> token) {
        if (token != "\\") {  // Ignore line continuation
            tokens.push_back(token);
        }
    }
    return tokens;
}

GateType BlifReader::determine_gate_type(size_t input_count, const string& pattern) const {
    // For NOT gate, only check input count
    if (input_count == 1) {
        return GateType::NOT;
    }
    
    // If pattern contains '-', it's OR gate
    if (pattern.find('-') != string::npos) {
        return GateType::OR;
    }
    
    // Otherwise, it's AND gate
    return GateType::AND;
}

void BlifReader::parse(const string& filename) {
    ifstream file(filename);
    if (!file) {
        throw runtime_error("Cannot open file: " + filename);
    }

    string line;
    Node current_node;
    string current_pattern;
    bool reading_patterns = false;
    
    while (getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;
        
        auto tokens = tokenize(line);
        if (tokens.empty()) continue;

        if (tokens[0] == ".model") {
            model_name = tokens[1];
        }
        else if (tokens[0] == ".inputs") {
            // Add all inputs after .inputs keyword
            inputs.insert(inputs.end(), tokens.begin() + 1, tokens.end());
        }
        else if (tokens[0] == ".outputs") {
            // Add all outputs after .outputs keyword
            outputs.insert(outputs.end(), tokens.begin() + 1, tokens.end());
        }
        else if (tokens[0] == ".names") {
            if (reading_patterns && !current_pattern.empty()) {
                // Handle previous node before starting new one
                current_node.type = determine_gate_type(current_node.inputs.size(), current_pattern);
                nodes.push_back(current_node);
            }
            
            reading_patterns = true;
            current_node = Node();
            current_pattern = "";
            current_node.output = tokens.back();  // Last token is output
            // Remaining tokens (excluding .names and output) are inputs
            current_node.inputs = vector<string>(tokens.begin() + 1, tokens.end() - 1);
        }
        else if (tokens[0] == ".end") {
            if (reading_patterns && !current_pattern.empty()) {
                // Handle last node
                current_node.type = determine_gate_type(current_node.inputs.size(), current_pattern);
                nodes.push_back(current_node);
            }
            break;
        }
        else if (line[0] == '0' || line[0] == '1' || line[0] == '-') {
            // Store the first pattern for gate type determination
            if (current_pattern.empty()) {
                current_pattern = line;
            }
        }
    }
}

void Node::print() const {
    cout << "Gate Type: ";
    switch(type) {
        case GateType::AND: cout << "AND"; break;
        case GateType::OR: cout << "OR"; break;
        case GateType::NOT: cout << "NOT"; break;
    }
    cout << "\nInputs: ";
    for (const auto& input : inputs) cout << input << " ";
    cout << "\nOutput: " << output << "\n";
}

void BlifReader::print() const {
    cout << "Model: " << model_name << "\n";
    cout << "Primary Inputs: ";
    for (const auto& input : inputs) cout << input << " ";
    cout << "\nPrimary Outputs: ";
    for (const auto& output : outputs) cout << output << " ";
    cout << "\nNodes:\n";
    for (const auto& node : nodes) {
        node.print();
        cout << "-------------------\n";
    }
}