# Clock Tree Synthesis (CTS) Project

This project implements a rectilinear clock tree algorithm with minimal skew ratio and wire length ratio.

## Requirements

- Linux environment (e.g., WSL)
- G++ compiler with C++17 support
- GNU Make
- Gnuplot (for visualization)

## Compilation

To compile the project, use the provided Makefile:

```
make
```

This will generate an executable named `cts`.

## Execution

To run the program, use the following command:

```
./cts <input_file> <output_file>
```

Example:
```
./cts case1.cts outputcase1.cts
```

## Visualization

To visualize the clock tree, use the provided Gnuplot script:

```
gnuplot plot_script.plt
```

Make sure you have Gnuplot installed on your system.

## Cleaning up

To remove the compiled executable, run:

```
make clean
```

## File Structure

- `M11215075.cpp`: Main source code file
- `Makefile`: For easy compilation
- `plot_script.plt`: Gnuplot script for visualization
- `README.md`: This file

## Notes

- The chip dimension is from (0, 0) to (dimX, dimY), and all coordinates are integers.
- The output clock tree is rectilinear (horizontal and vertical line segments) without cycles.
- The program calculates and outputs the skew ratio and wire length ratio.

For more detailed information about the project requirements and file formats, please refer to the project description.
