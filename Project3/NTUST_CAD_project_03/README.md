# Standard Cell Placement Legalization

This project implements a placement legalization algorithm that minimizes both total displacement and maximum displacement.

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

This will generate an executable named `legalizer`.

## Execution

To run the program, use the following command:

```
./legalizer <input_dir> <output_dir>
```

Example:
```
./legalizer toy output
```

## Visualization

The program automatically generates visualization plots:
- `input_placement.png`: Initial placement visualization
- `output_placement.png`: Legalized placement visualization

## Cleaning up

To remove the compiled executable, run:

```
make clean
```

## File Structure

- `src/legalizer.cpp`: Main source code file
- `Makefile`: For easy compilation
- `README.md`: This file

## Notes

- Input and output use GSRC Bookshelf format
- All standard cells have the same row height
- The program reports total displacement and maximum displacement

For more detailed information about the project requirements and file formats, please refer to the project description.
