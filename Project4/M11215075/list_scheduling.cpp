#include "list_scheduling.hpp"
#include <queue>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

class ListScheduler {
private:
    struct NodeState {
        bool is_scheduled = false;           
        int scheduled_time = 0;              
        std::unordered_set<std::string> predecessors;
        std::unordered_set<std::string> successors;
        int critical_path_length = 0;        // Length of longest path to sink nodes
    };
    
    std::unordered_map<std::string, NodeState> node_states;
    std::unordered_map<std::string, Node> nodes_map;
    std::unordered_map<GateType, int> resource_limits;
    int current_time;
    std::vector<std::vector<std::string>> current_step;
    
    // Calculate the longest path from this node to any sink node
    int calculateCriticalPath(const std::string& node_name) {
        NodeState& state = node_states[node_name];
        if (state.critical_path_length > 0) {
            return state.critical_path_length;
        }
        
        // Base case: if node has no successors, path length is 1
        if (state.successors.empty()) {
            state.critical_path_length = 1;
            return 1;
        }
        
        // Find the maximum path length through all successors
        int max_length = 0;
        for (const auto& succ : state.successors) {
            max_length = std::max(max_length, calculateCriticalPath(succ));
        }
        
        state.critical_path_length = max_length + 1;
        return state.critical_path_length;
    }
    
    std::vector<std::string> getReadyNodes(GateType type) {
        std::vector<std::string> ready_nodes;
        
        // Get nodes scheduled in current cycle
        std::unordered_set<std::string> current_scheduled;
        for (const auto& type_ops : current_step) {
            for (const auto& node : type_ops) {
                current_scheduled.insert(node);
            }
        }
        
        // Find nodes that are ready to be executed
        for (const auto& pair : nodes_map) {
            const std::string& node_name = pair.first;
            const Node& node = pair.second;
            
            if (node_states[node_name].is_scheduled || node.type != type) {
                continue;
            }
            
            bool all_inputs_ready = true;
            for (const std::string& input : node.inputs) {
                if (nodes_map.find(input) != nodes_map.end()) {
                    if (!node_states[input].is_scheduled || 
                        current_scheduled.find(input) != current_scheduled.end()) {
                        all_inputs_ready = false;
                        break;
                    }
                }
            }
            
            if (all_inputs_ready) {
                ready_nodes.push_back(node_name);
            }
        }
        
        // Sort by critical path length and then alphabetically
        std::sort(ready_nodes.begin(), ready_nodes.end(),
            [this](const std::string& a, const std::string& b) {
                if (node_states[a].critical_path_length == node_states[b].critical_path_length) {
                    return a < b;  // When critical paths are equal, sort alphabetically
                }
                return node_states[a].critical_path_length > node_states[b].critical_path_length;
            });
        
        return ready_nodes;
    }
    
    bool hasUnscheduledNodes() {
        for (const auto& pair : node_states) {
            if (!pair.second.is_scheduled) {
                return true;
            }
        }
        return false;
    }

public:
    ListScheduler(const std::vector<Node>& nodes, 
                 int and_limit, int or_limit, int not_limit)
        : current_step(3) {
        resource_limits[GateType::AND] = and_limit;
        resource_limits[GateType::OR] = or_limit;
        resource_limits[GateType::NOT] = not_limit;
        current_time = 1;
        
        // Build nodes and dependency relationships
        for (const Node& node : nodes) {
            nodes_map[node.output] = node;
            
            for (const std::string& input : node.inputs) {
                if (nodes_map.find(input) != nodes_map.end()) {
                    node_states[node.output].predecessors.insert(input);
                    node_states[input].successors.insert(node.output);
                }
            }
        }
        
        // Calculate critical path lengths for all nodes
        for (const auto& pair : nodes_map) {
            calculateCriticalPath(pair.first);
        }
    }
    
    std::vector<std::vector<std::vector<std::string>>> run() {
        std::vector<std::vector<std::vector<std::string>>> schedule_result;
        
        while (hasUnscheduledNodes()) {
            current_step = std::vector<std::vector<std::string>>(3);
            bool scheduled_any = false;
            
            // Try scheduling each type of operation
            for (GateType type : {GateType::AND, GateType::OR, GateType::NOT}) {
                size_t type_index = static_cast<size_t>(type);
                auto ready_nodes = getReadyNodes(type);
                
                // Schedule nodes within resource constraints
                for (const auto& node : ready_nodes) {
                    if (static_cast<int>(current_step[type_index].size()) >= resource_limits[type]) {
                        break;
                    }
                    current_step[type_index].push_back(node);
                    node_states[node].is_scheduled = true;
                    node_states[node].scheduled_time = current_time;
                    scheduled_any = true;
                }

                // Sort nodes within each step alphabetically
                std::sort(current_step[type_index].begin(), current_step[type_index].end());
            }
            
            if (scheduled_any) {
                schedule_result.push_back(current_step);
                current_time++;
            }
        }
        
        return schedule_result;
    }
};

std::vector<std::vector<std::vector<std::string>>> ListScheduling::schedule(
    const std::vector<Node>& nodes,
    int and_limit,
    int or_limit,
    int not_limit) {
    
    ListScheduler scheduler(nodes, and_limit, or_limit, not_limit);
    return scheduler.run();
}

void ListScheduling::printResult(const std::vector<std::vector<std::vector<std::string>>>& schedule) {
    std::cout << "Heuristic Scheduling Result\n";
    for (size_t i = 0; i < schedule.size(); ++i) {
        std::cout << i + 1 << ": ";
        std::cout << "{";
        for (size_t j = 0; j < schedule[i][0].size(); ++j) {
            if (j > 0) std::cout << " ";
            std::cout << schedule[i][0][j];
        }
        std::cout << "} {";
        for (size_t j = 0; j < schedule[i][1].size(); ++j) {
            if (j > 0) std::cout << " ";
            std::cout << schedule[i][1][j];
        }
        std::cout << "} {";
        for (size_t j = 0; j < schedule[i][2].size(); ++j) {
            if (j > 0) std::cout << " ";
            std::cout << schedule[i][2][j];
        }
        std::cout << "}\n";
    }
    std::cout << "LATENCY: " << schedule.size() << "\nEND\n";
}