#include "ilp.hpp"
#include "list_scheduling.hpp"
#include <queue>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <limits>
#include <chrono>
#include <iomanip>

// Configure Gurobi solver based on problem size
void configure_gurobi(GRBEnv& env, size_t node_count, int upper_bound) {
    // Basic settings
    env.set(GRB_IntParam_OutputFlag, 0);  // Disable solver output
    env.set(GRB_IntParam_Threads, 4);     // Limit thread count to reduce memory usage
    env.set(GRB_DoubleParam_MIPGap, 0.05); // Set 5% optimality gap
    
    // Adjust settings based on circuit size
    if (node_count > 10000) {
        // Settings for very large circuits
        env.set(GRB_DoubleParam_TimeLimit, 1800.0);   // 30 minutes time limit
        env.set(GRB_IntParam_MIPFocus, 1);           // Focus on finding feasible solutions
        env.set(GRB_DoubleParam_NodefileStart, 0.5);  // Start using disk at 50% memory
        env.set(GRB_IntParam_PreSparsify, 1);         // Use sparsification preprocessing
        env.set(GRB_IntParam_Cuts, 2);                // Aggressive cut generation
    } else if (node_count > 5000) {
        // Settings for large circuits
        env.set(GRB_DoubleParam_TimeLimit, 1200.0);   // 20 minutes time limit
        env.set(GRB_IntParam_MIPFocus, 1);
        env.set(GRB_DoubleParam_NodefileStart, 0.8);
    } else {
        // Settings for medium and small circuits
        env.set(GRB_DoubleParam_TimeLimit, 600.0);    // 10 minutes time limit
    }

    // Advanced preprocessing options
    env.set(GRB_IntParam_Presolve, 1);      // Enable presolve
    env.set(GRB_IntParam_PrePasses, 8);     // Increase presolve passes
    env.set(GRB_IntParam_CutPasses, 5);     // Increase cut passes
}

// Calculate As Soon As Possible (ASAP) schedule
std::vector<int> ILP::calculate_asap() {
    std::vector<int> asap(nodes.size(), 0);
    std::vector<int> in_degree(nodes.size(), 0);
    
    // Calculate in-degree for each node
    for (size_t i = 0; i < nodes.size(); i++) {
        for (const auto& input : nodes[i].inputs) {
            if (std::find(primary_inputs.begin(), primary_inputs.end(), input) == primary_inputs.end()) {
                auto it = std::find_if(nodes.begin(), nodes.end(),
                    [&input](const Node& n) { return n.output == input; });
                if (it != nodes.end()) {
                    in_degree[i]++;
                }
            }
        }
    }
    
    // Use topological sort to determine ASAP times
    std::queue<int> q;
    for (size_t i = 0; i < nodes.size(); i++) {
        if (in_degree[i] == 0) {
            q.push(i);
            asap[i] = 0;
        }
    }
    
    while (!q.empty()) {
        int curr = q.front();
        q.pop();
        
        // Update successors
        for (size_t i = 0; i < nodes.size(); i++) {
            for (const auto& input : nodes[i].inputs) {
                if (input == nodes[curr].output) {
                    asap[i] = std::max(asap[i], asap[curr] + 1);
                    in_degree[i]--;
                    if (in_degree[i] == 0) {
                        q.push(i);
                    }
                }
            }
        }
    }
    
    return asap;
}

// Calculate As Late As Possible (ALAP) schedule
std::vector<int> ILP::calculate_alap(int upper_bound) {
    std::vector<int> alap(nodes.size(), upper_bound - 1);
    
    // Build reverse dependency graph
    std::unordered_map<std::string, std::vector<int>> rev_deps;
    for (size_t i = 0; i < nodes.size(); i++) {
        for (const auto& input : nodes[i].inputs) {
            rev_deps[input].push_back(i);
        }
    }
    
    // Start with output nodes
    std::queue<int> q;
    for (size_t i = 0; i < nodes.size(); i++) {
        if (std::find(primary_outputs.begin(), primary_outputs.end(), nodes[i].output) != primary_outputs.end() ||
            rev_deps[nodes[i].output].empty()) {
            q.push(i);
            alap[i] = upper_bound - 1;
        }
    }
    
    // Process nodes in reverse topological order
    while (!q.empty()) {
        int curr = q.front();
        q.pop();
        
        // Update predecessors
        for (const auto& input : nodes[curr].inputs) {
            auto pred_it = std::find_if(nodes.begin(), nodes.end(),
                [&input](const Node& n) { return n.output == input; });
            
            if (pred_it != nodes.end()) {
                int pred_idx = pred_it - nodes.begin();
                alap[pred_idx] = std::min(alap[pred_idx], alap[curr] - 1);
                q.push(pred_idx);
            }
        }
    }
    
    return alap;
}

// Parse input data
void ILP::parse(const std::vector<Node>& input_nodes,
               const std::vector<std::string>& inputs,
               const std::vector<std::string>& outputs) {
    nodes = input_nodes;
    primary_inputs = inputs;
    primary_outputs = outputs;
    
    for (const auto& node : nodes) {
        nodes_map[node.output] = node;
    }
}

// Main ILP scheduling algorithm
std::vector<std::vector<std::vector<std::string>>>
ILP::run(int and_limit, int or_limit, int not_limit) {
    try {
        // Get initial solution using list scheduling
        auto list_result = ListScheduling::schedule(nodes, and_limit, or_limit, not_limit);
        int upper_bound = list_result.size();

        // Initialize Gurobi environment and model
        GRBEnv env = GRBEnv(true);
        configure_gurobi(env, nodes.size(), upper_bound);
        env.start();
        
        GRBModel model = GRBModel(env);
        
        // Calculate scheduling bounds
        auto asap = calculate_asap();
        auto alap = calculate_alap(upper_bound);
        
        // Create binary variables x[i][t] for each node i and time t
        std::vector<std::vector<GRBVar>> x(nodes.size());
        for (size_t i = 0; i < nodes.size(); i++) {
            for (size_t t = asap[i]; t <= static_cast<size_t>(alap[i]); t++) {
                x[i].push_back(model.addVar(0.0, 1.0, 0.0, GRB_BINARY));
            }
        }

        // Add constraints: each operation must be scheduled exactly once
        for (size_t i = 0; i < nodes.size(); i++) {
            GRBLinExpr sum = 0;
            for (size_t t = 0; t < x[i].size(); t++) {
                sum += x[i][t];
            }
            model.addConstr(sum == 1);
        }

        // Add precedence constraints
        for (size_t i = 0; i < nodes.size(); i++) {
            for (const auto& input : nodes[i].inputs) {
                auto it = std::find_if(nodes.begin(), nodes.end(),
                    [&input](const Node& n) { return n.output == input; });
                if (it == nodes.end()) continue;
                
                size_t j = it - nodes.begin();
                GRBLinExpr ti = 0, tj = 0;
                for (size_t t = 0; t < x[i].size(); t++) {
                    ti += (asap[i] + t) * x[i][t];
                }
                for (size_t t = 0; t < x[j].size(); t++) {
                    tj += (asap[j] + t) * x[j][t];
                }
                model.addConstr(ti >= tj + 1);
            }
        }

        // Add resource constraints for each time step
        for (int t = 0; t < upper_bound; t++) {
            GRBLinExpr and_sum = 0, or_sum = 0, not_sum = 0;
            for (size_t i = 0; i < nodes.size(); i++) {
                int time_offset = t - asap[i];
                if (time_offset >= 0 && time_offset < static_cast<int>(x[i].size())) {
                    switch (nodes[i].type) {
                        case GateType::AND: and_sum += x[i][time_offset]; break;
                        case GateType::OR:  or_sum += x[i][time_offset]; break;
                        case GateType::NOT: not_sum += x[i][time_offset]; break;
                    }
                }
            }
            model.addConstr(and_sum <= and_limit);
            model.addConstr(or_sum <= or_limit);
            model.addConstr(not_sum <= not_limit);
        }

        // Create and constrain makespan variable
        GRBVar makespan = model.addVar(0.0, upper_bound, 0.0, GRB_INTEGER);
        for (size_t i = 0; i < nodes.size(); i++) {
            GRBLinExpr completion_time = 0;
            for (size_t t = 0; t < x[i].size(); t++) {
                completion_time += (asap[i] + t) * x[i][t];
            }
            model.addConstr(makespan >= completion_time + 1);
        }

        // Set objective to minimize makespan
        GRBLinExpr obj = makespan;
        model.setObjective(obj, GRB_MINIMIZE);

        // Initialize solution from list scheduling result
        std::unordered_map<std::string, int> node_times;
        for (size_t t = 0; t < list_result.size(); t++) {
            for (int type = 0; type < 3; type++) {
                for (const auto& node : list_result[t][type]) {
                    node_times[node] = t;
                }
            }
        }

        // Set initial values for variables
        for (size_t i = 0; i < nodes.size(); i++) {
            auto it = node_times.find(nodes[i].output);
            if (it != node_times.end()) {
                int time_offset = it->second - asap[i];
                if (time_offset >= 0 && time_offset < static_cast<int>(x[i].size())) {
                    x[i][time_offset].set(GRB_DoubleAttr_Start, 1.0);
                }
            }
        }
        makespan.set(GRB_DoubleAttr_Start, upper_bound);

        // Run optimization
        model.optimize();

        // Extract solution if found
        if (model.get(GRB_IntAttr_SolCount) > 0) {
            int final_makespan = std::ceil(makespan.get(GRB_DoubleAttr_X));
            std::vector<std::vector<std::vector<std::string>>> result(
                final_makespan, std::vector<std::vector<std::string>>(3));

            // Construct schedule from solution
            for (size_t i = 0; i < nodes.size(); i++) {
                for (size_t t = 0; t < x[i].size(); t++) {
                    if (x[i][t].get(GRB_DoubleAttr_X) > 0.5) {
                        int scheduled_time = asap[i] + t;
                        result[scheduled_time][static_cast<int>(nodes[i].type)].push_back(nodes[i].output);
                    }
                }
            }

            // Sort nodes within each time step and type
            for (auto& step : result) {
                for (auto& type_nodes : step) {
                    std::sort(type_nodes.begin(), type_nodes.end());
                }
            }

            return result;
        }

        throw GRBException("No solution found", -1);

    } catch (GRBException& e) {
        std::cerr << "Gurobi error " << e.getErrorCode() << ": " << e.getMessage() << std::endl;
        throw;
    }
}