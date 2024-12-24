#include "parser.hpp"
#include "list_scheduling.hpp"
#include "ilp.hpp"
#include <iostream>
#include <string>
#include <memory>

void printSchedulingResult(const std::vector<std::vector<std::vector<std::string>>>& schedule, bool is_ilp) {
    std::cout << (is_ilp ? "ILP-based Scheduling Result\n" : "Heuristic Scheduling Result\n");
    
    for (size_t t = 0; t < schedule.size(); t++) {
        bool hasOperations = false;
        for (const auto& type_ops : schedule[t]) {
            if (!type_ops.empty()) {
                hasOperations = true;
                break;
            }
        }
        
        if (!hasOperations) continue;
        
        std::cout << t + 1 << ": ";
        for (size_t type = 0; type < 3; type++) {
            std::cout << "{";
            if (!schedule[t][type].empty()) {
                std::cout << schedule[t][type][0];
                for (size_t i = 1; i < schedule[t][type].size(); i++) {
                    std::cout << " " << schedule[t][type][i];
                }
            }
            std::cout << "}" << (type < 2 ? " " : "\n");
        }
    }
    
    std::cout << "LATENCY: " << schedule.size() << "\nEND\n";
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " -h/-e BLIF_FILE AND_CONSTRAINT OR_CONSTRAINT NOT_CONSTRAINT\n";
        return 1;
    }
    
    try {
        std::string mode = argv[1];
        std::string blif_file = argv[2];
        int and_limit = std::stoi(argv[3]);
        int or_limit = std::stoi(argv[4]);
        int not_limit = std::stoi(argv[5]);
        
        BlifReader reader;
        reader.parse(blif_file);
        
        if (mode == "-h") {
            auto result = ListScheduling::schedule(
                reader.get_nodes(),
                and_limit, or_limit, not_limit
            );
            printSchedulingResult(result, false);
            
        } else if (mode == "-e") {
            ILP ilp;
            ilp.parse(reader.get_nodes(), reader.get_inputs(), reader.get_outputs());
            auto result = ilp.run(and_limit, or_limit, not_limit);
            
            if (!result.empty()) {
                printSchedulingResult(result, true);
            } else {
                std::cerr << "ILP solver failed to find a solution\n";
                return 1;
            }
            
        } else {
            std::cerr << "Invalid mode. Use -h for heuristic or -e for ILP.\n";
            return 1;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
