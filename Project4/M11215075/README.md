# ML-RCS (Minimizing Latency under Resource Constraints Scheduling)

## Project Overview
This project implements two scheduling algorithms for minimizing latency under given resource constraints:
1. A heuristic algorithm (list scheduling)
2. An ILP-based exact algorithm using Gurobi solver

## Requirements
- C++ Compiler with C++17 support
- Gurobi Optimizer 12.0.0
- Linux/macOS environment

## Important Notice
Before compiling, please check and modify the Gurobi paths in Makefile according to your system:
```makefile
# Default paths (you may need to modify these):
# For Linux users:
GUROBI_DIR = /opt/gurobi1200/linux64
# For macOS users:
GUROBI_DIR = /Library/gurobi1200/macos_universal2

# If your Gurobi installation is in a different location, 
# please modify the GUROBI_DIR path in the Makefile
```

## Build Instructions
1. Make sure Gurobi is properly installed in your system
2. Modify GUROBI_DIR in Makefile if necessary
3. Run `make` to build the project

## Usage
```bash
./mlrcs -h/-e BLIF_FILE AND_CONSTRAINT OR_CONSTRAINT NOT_CONSTRAINT
```

## Test Case Results
```
>% ./mlrcs -h aoi_benchmark/m11215075.blif 2 1 1
Heuristic Scheduling Result
1: {z1 z2} {} {}
2: {z3 z5} {} {}
3: {w1 z4} {w2} {}
4: {v1} {w3} {}
5: {} {w4} {y1}
6: {} {v2} {}
7: {} {v3} {}
8: {} {y2} {}
LATENCY: 8
END
>% ./mlrcs -e aoi_benchmark/m11215075.blif 2 1 1
ILP-based Scheduling Result
1: {z3 z5} {} {}
2: {z1 z2} {w3} {}
3: {w1 z4} {w2} {}
4: {} {v2} {}
5: {v1} {w4} {}
6: {} {v3} {y1}
7: {} {y2} {}
LATENCY: 7
END
```

## Observations on List Scheduling Limitations

1. Local Decision Making:
   - Makes decisions based only on current time step
   - Cannot predict which choice leads to better global solution
   - Example: Different ordering of z1/z2 and z3/z5 affects final latency

2. Incomplete Priority Assessment:
   - Uses simple priority rules (e.g., ASAP, ALAP)
   - Cannot fully capture complex dependencies
   - Example: Parallel operations w2, w3, w4 need better scheduling strategy

3. Greedy Resource Allocation:
   - Always tries to maximize current resource usage
   - Sometimes holding resources for critical later operations is better
   - Example: ILP successfully combines w3 with z1/z2 in cycle 2

In contrast, ILP finds optimal solution through:
- Global optimization across all time steps
- Complete dependency consideration
- Better resource distribution planning

## Notes
- All Boolean operations (AND, OR, NOT) take 1-cycle latency
- PI nodes are not counted as operations
- Input files must be in BLIF format
- The program reads BLIF files and corresponding resource constraints
