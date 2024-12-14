#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <cmath>
#include <limits>
#include <iomanip>
#include <filesystem>
#include <algorithm>
#include <random>
#include <chrono>

namespace fs = std::filesystem;

// Node structure definition
struct Node
{
    std::string name;
    double width;
    double height;
    bool is_terminal;
    double original_x;
    double original_y;
    double new_x;
    double new_y;
    std::string orientation;
    bool is_fixed;

    // Force vectors for diffusion
    double force_x;
    double force_y;
    // Velocity vectors for momentum
    double velocity_x;
    double velocity_y;
};

// Die area structure definition
struct DieArea
{
    double min_x;
    double max_x;
    double min_y;
    double max_y;
};

class CircuitLegalizer
{
private:
    fs::path input_dir;
    fs::path output_dir;
    std::string input_name;
    std::string output_name;
    std::vector<Node> nodes;
    std::map<std::string, std::vector<std::string>> file_headers;
    DieArea die_area;
    double total_displacement;
    double max_displacement;
    double row_height;

    struct Row
    {
        double y_coordinate;
        double right_edge;
        double width;
        std::vector<Node *> cells;

        Row(double y, double w) : y_coordinate(y), right_edge(0), width(w) {}
    };

    double calculateDisplacement(const Node &node, double new_x, double new_y)
    {
        return std::abs(new_x - node.original_x) + std::abs(new_y - node.original_y);
    }

    // File Processing Methods
    void processAuxFile()
    {
        std::string input_file = (input_dir / (input_name + ".aux")).string();
        std::string output_file = (output_dir / (output_name + ".aux")).string();

        std::ifstream in(input_file);
        if (!in.is_open())
        {
            throw std::runtime_error("Cannot open input .aux file: " + input_file);
        }

        std::vector<std::string> lines;
        std::string line;
        while (std::getline(in, line))
        {
            lines.push_back(line);
        }
        in.close();

        std::ofstream out(output_file);
        if (!out.is_open())
        {
            throw std::runtime_error("Cannot create output .aux file: " + output_file);
        }

        for (const auto &line : lines)
        {
            if (line.find("RowBasedPlacement") != std::string::npos)
            {
                // Replace input filenames with output filenames
                out << "RowBasedPlacement : "
                    << output_name << ".nodes "
                    << output_name << ".nets "
                    << output_name << ".wts "
                    << output_name << ".pl "
                    << output_name << ".scl" << std::endl;
            }
            else
            {
                out << line << std::endl;
            }
        }
        out.close();
    }
    void processNodesFile()
    {
        std::string input_file = (input_dir / (input_name + ".nodes")).string();
        std::string output_file = (output_dir / (output_name + ".nodes")).string();

        std::ifstream in(input_file);
        if (!in.is_open())
        {
            throw std::runtime_error("Cannot open input .nodes file: " + input_file);
        }

        std::vector<std::string> headers;
        std::string line;
        int node_count = 0;
        int terminal_count = 0;

        // First pass: read headers and count nodes
        while (std::getline(in, line))
        {
            if (line.find("NumNodes") != std::string::npos ||
                line.find("NumTerminals") != std::string::npos ||
                line.empty() || line[0] == '#' ||
                line.find("UCLA") != std::string::npos)
            {
                headers.push_back(line);
                continue;
            }

            std::istringstream iss(line);
            std::string name, terminal;
            double width, height;

            if (!(iss >> name >> width >> height))
            {
                headers.push_back(line);
                continue;
            }

            node_count++;
            if (iss >> terminal && terminal == "terminal")
            {
                terminal_count++;
            }
        }

        // Reset file pointer and read node data
        in.clear();
        in.seekg(0);
        nodes.clear();

        bool reading_data = false;
        while (std::getline(in, line))
        {
            if (!reading_data)
            {
                if (line.find("NumNodes") != std::string::npos ||
                    line.find("NumTerminals") != std::string::npos ||
                    line.empty() || line[0] == '#' ||
                    line.find("UCLA") != std::string::npos)
                {
                    continue;
                }
                reading_data = true;
            }

            if (reading_data)
            {
                std::istringstream iss(line);
                Node node;
                std::string terminal;

                if (!(iss >> node.name >> node.width >> node.height))
                {
                    continue;
                }

                node.is_terminal = iss >> terminal && terminal == "terminal";
                node.is_fixed = false;
                node.force_x = 0.0;
                node.force_y = 0.0;
                node.velocity_x = 0.0;
                node.velocity_y = 0.0;
                nodes.push_back(node);
            }
        }

        in.close();

        // Update file headers
        for (auto &header : headers)
        {
            if (header.find("NumNodes") != std::string::npos)
            {
                header = "NumNodes : " + std::to_string(node_count);
            }
            else if (header.find("NumTerminals") != std::string::npos)
            {
                header = "NumTerminals : " + std::to_string(terminal_count);
            }
        }
        file_headers[".nodes"] = headers;

        // Write output file
        std::ofstream out(output_file);
        if (!out.is_open())
        {
            throw std::runtime_error("Cannot create output .nodes file: " + output_file);
        }

        for (const auto &header : headers)
        {
            out << header << std::endl;
        }

        for (const auto &node : nodes)
        {
            out << node.name << " "
                << std::fixed << std::setprecision(1) << node.width << " "
                << node.height;
            if (node.is_terminal)
            {
                out << " terminal";
            }
            out << std::endl;
        }
        out.close();
    }

    void processPlFile()
    {
        std::string input_file = (input_dir / (input_name + ".pl")).string();
        std::string output_file = (output_dir / (output_name + ".pl")).string();

        std::ifstream in(input_file);
        if (!in.is_open())
        {
            throw std::runtime_error("Cannot open input .pl file: " + input_file);
        }

        std::vector<std::string> headers;
        std::string line;

        while (std::getline(in, line))
        {
            if (line.find("UCLA pl 1.0") != std::string::npos)
            {
                headers.push_back(line);
                break;
            }
            headers.push_back(line);
        }

        while (std::getline(in, line))
        {
            if (line.empty() || line[0] == '#')
                continue;

            std::istringstream iss(line);
            std::string name, orientation, fixed;
            double x, y;
            char colon;

            if (!(iss >> name >> x >> y >> colon >> orientation))
                continue;

            iss >> fixed;
            bool is_fixed = (fixed == "/FIXED");

            auto node_it = std::find_if(nodes.begin(), nodes.end(),
                                        [&name](const Node &node)
                                        {
                                            return node.name == name;
                                        });

            if (node_it != nodes.end())
            {
                node_it->original_x = x;
                node_it->original_y = y;
                node_it->new_x = x;
                node_it->new_y = y;
                node_it->orientation = orientation;
                node_it->is_fixed = is_fixed;
            }
        }

        in.close();
        file_headers[".pl"] = headers;

        std::ofstream out(output_file);
        if (!out.is_open())
        {
            throw std::runtime_error("Cannot create output .pl file: " + output_file);
        }

        for (const auto &header : headers)
        {
            out << header << std::endl;
        }
        out << std::endl;

        for (const auto &node : nodes)
        {
            // Snap to row height
            double snapped_y = std::round(node.new_y / row_height) * row_height;

            out << std::left << std::setw(10) << node.name
                << std::fixed << std::setprecision(1)
                << std::right << std::setw(8) << node.new_x << "  "
                << std::setw(8) << snapped_y << " : "
                << node.orientation;
            if (node.is_fixed)
            {
                out << " /FIXED";
            }
            out << std::endl;
        }
        out.close();
    }

    void processSclFile()
    {
        std::string input_file = (input_dir / (input_name + ".scl")).string();
        std::ifstream in(input_file);
        if (!in.is_open())
        {
            throw std::runtime_error("Cannot open input .scl file: " + input_file);
        }

        die_area = {
            .min_x = std::numeric_limits<double>::max(),
            .max_x = std::numeric_limits<double>::lowest(),
            .min_y = std::numeric_limits<double>::max(),
            .max_y = std::numeric_limits<double>::lowest()};

        row_height = 0.0; // Initialize row_height
        std::string line;
        int num_rows = 0;

        while (std::getline(in, line))
        {
            if (line.find("NumRows") != std::string::npos)
            {
                std::istringstream iss(line);
                std::string dummy;
                iss >> dummy >> dummy >> num_rows;
            }
            else if (line.find("CoreRow Horizontal") != std::string::npos)
            {
                double coord = 0.0;
                double height = 0.0;
                double subrow_origin = 0.0;
                int num_sites = 0;

                while (std::getline(in, line))
                {
                    if (line.find("End") != std::string::npos)
                        break;

                    std::istringstream iss(line);
                    std::string token;

                    if (line.find("Coordinate") != std::string::npos)
                    {
                        iss >> token >> token >> coord;
                        die_area.min_y = std::min(die_area.min_y, coord);
                    }
                    else if (line.find("Height") != std::string::npos)
                    {
                        iss >> token >> token >> height;
                        die_area.max_y = std::max(die_area.max_y, coord + height);
                        row_height = height; // Set the row height
                    }
                    else if (line.find("SubrowOrigin") != std::string::npos)
                    {
                        iss >> token >> token >> subrow_origin >> token >> token >> num_sites;
                        die_area.min_x = std::min(die_area.min_x, subrow_origin);
                        die_area.max_x = std::max(die_area.max_x, subrow_origin + num_sites * row_height);
                    }
                }
            }
        }

        if (row_height <= 0.0)
        {
            throw std::runtime_error("Invalid row height in SCL file");
        }

        in.close();
    }

    void detailedPlacement()
    {
        std::cout << "Starting greedy legalization process...\n";

        // Calculate number of rows
        int num_rows = static_cast<int>((die_area.max_y - die_area.min_y) / row_height);
        std::vector<Row> rows;

        // Initialize rows
        for (int i = 0; i < num_rows; ++i)
        {
            double y_coord = die_area.min_y + i * row_height;
            rows.emplace_back(y_coord, die_area.max_x - die_area.min_x);
            rows.back().right_edge = die_area.min_x;
        }

        // Sort cells by x-coordinate
        std::vector<Node *> sortedNodes;
        for (auto &node : nodes)
        {
            if (!node.is_terminal && !node.is_fixed)
            {
                sortedNodes.push_back(&node);
            }
        }
        std::sort(sortedNodes.begin(), sortedNodes.end(),
                  [](const Node *a, const Node *b)
                  {
                      return a->original_x < b->original_x;
                  });

        // Place each cell
        for (Node *node : sortedNodes)
        {
            // Find best row for this cell
            int best_row = -1;
            double min_displacement = std::numeric_limits<double>::max();
            double best_x = 0;

            // Try each row
            for (int i = 0; i < num_rows; ++i)
            {
                // Calculate potential position in this row
                double potential_x = std::max(die_area.min_x,
                                              std::max(node->original_x, rows[i].right_edge));

                // Check if cell fits in row width
                if (potential_x + node->width <= die_area.max_x)
                {
                    double displacement = calculateDisplacement(*node, potential_x, rows[i].y_coordinate);
                    if (displacement < min_displacement)
                    {
                        min_displacement = displacement;
                        best_row = i;
                        best_x = potential_x;
                    }
                }
            }

            // If no valid position found, try to place in row with least utilization
            if (best_row == -1)
            {
                double min_utilization = std::numeric_limits<double>::max();
                for (int i = 0; i < num_rows; ++i)
                {
                    double utilization = (rows[i].right_edge - die_area.min_x) / rows[i].width;
                    if (utilization < min_utilization &&
                        rows[i].right_edge + node->width <= die_area.max_x)
                    {
                        min_utilization = utilization;
                        best_row = i;
                        best_x = rows[i].right_edge;
                    }
                }
            }

            // Place the cell
            if (best_row != -1)
            {
                node->new_x = best_x;
                node->new_y = rows[best_row].y_coordinate;
                rows[best_row].right_edge = best_x + node->width;
                rows[best_row].cells.push_back(node);
            }
            else
            {
                std::cerr << "Warning: Could not place cell " << node->name << std::endl;
            }
        }

        // Calculate final displacement metrics
        total_displacement = 0;
        max_displacement = 0;

        for (const Node *node : sortedNodes)
        {
            double displacement = calculateDisplacement(*node, node->new_x, node->new_y);
            total_displacement += displacement;
            max_displacement = std::max(max_displacement, displacement);
        }
    }

    void calculateDisplacement()
    {
        total_displacement = 0.0;
        max_displacement = 0.0;

        for (const auto &node : nodes)
        {
            if (node.is_terminal || node.is_fixed)
                continue;

            double dx = node.new_x - node.original_x;
            double dy = node.new_y - node.original_y;
            double displacement = std::abs(dx) + std::abs(dy); // Manhattan distance

            total_displacement += displacement;
            max_displacement = std::max(max_displacement, displacement);
        }
    }

    double calculateOverlap(const Node &n1, const Node &n2)
    {
        double x_overlap = std::min(n1.new_x + n1.width, n2.new_x + n2.width) -
                           std::max(n1.new_x, n2.new_x);
        double y_overlap = std::min(n1.new_y + n1.height, n2.new_y + n2.height) -
                           std::max(n1.new_y, n2.new_y);

        if (x_overlap > 0 && y_overlap > 0)
        {
            return x_overlap * y_overlap;
        }
        return 0.0;
    }

    double calculateTotalOverlap()
    {
        double total_overlap = 0.0;
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            if (nodes[i].is_terminal || nodes[i].is_fixed)
                continue;

            for (size_t j = i + 1; j < nodes.size(); ++j)
            {
                if (nodes[j].is_terminal || nodes[j].is_fixed)
                    continue;
                total_overlap += calculateOverlap(nodes[i], nodes[j]);
            }
        }
        return total_overlap;
    }

    void generateVisualization(bool use_new_coordinates = false)
    {
        std::string case_name = input_dir.stem().string();
        std::string plot_file = (output_dir / (case_name + (use_new_coordinates ? "_output_plot.gp" : "_input_plot.gp"))).string();
        std::ofstream out(plot_file);

        // Set canvas size and white background
        out << "set terminal png enhanced size 800,800 background rgb 'white'\n";
        out << "set output '" << case_name
            << (use_new_coordinates ? "_output_placement.png" : "_input_placement.png") << "'\n";

        // Clean up the plot
        out << "unset title\n";
        out << "unset key\n";
        out << "set border 1\n";
        out << "unset xtics\n";
        out << "unset ytics\n";

        // Calculate bounds
        double min_x = std::numeric_limits<double>::max();
        double max_x = std::numeric_limits<double>::lowest();
        double min_y = std::numeric_limits<double>::max();
        double max_y = std::numeric_limits<double>::lowest();

        for (const auto &node : nodes)
        {
            double x = use_new_coordinates ? node.new_x : node.original_x;
            double y = use_new_coordinates ? node.new_y : node.original_y;

            min_x = std::min(min_x, x);
            max_x = std::max(max_x, x + node.width);
            min_y = std::min(min_y, y);
            max_y = std::max(max_y, y + node.height);
        }

        // Calculate ranges with smaller padding
        double width = max_x - min_x;
        double height = max_y - min_y;
        double max_dim = std::max(width, height);
        double padding = max_dim * 0.3; // Reduce padding even more

        // Center the plot
        double center_x = (min_x + max_x) / 2.0;
        double center_y = (min_y + max_y) / 2.0;

        // Set ranges
        double half_range = (max_dim + padding) / 2.0;
        out << "set xrange [" << (center_x - half_range) << ":" << (center_x + half_range) << "]\n";
        out << "set yrange [" << (center_y - half_range) << ":" << (center_y + half_range) << "]\n";

        // Ensure square aspect ratio
        out << "set size square\n";

        // Draw nodes with empty fill
        int obj_count = 1;
        for (const auto &node : nodes)
        {
            double x = use_new_coordinates ? node.new_x : node.original_x;
            double y = use_new_coordinates ? node.new_y : node.original_y;

            out << "set object " << obj_count++ << " rectangle from "
                << x << "," << y << " to "
                << (x + node.width) << "," << (y + node.height)
                << " fc rgb '#FFFFFF' fs empty border rgb '#800080' lw 1\n";
        }

        // Minimal margins
        out << "set lmargin 0\n";
        out << "set rmargin 0\n";
        out << "set tmargin 0\n";
        out << "set bmargin 0\n";

        out << "plot NaN notitle\n";
        out.close();

        // Execute gnuplot
        std::string gnuplot_cmd = "gnuplot \"" + plot_file + "\"";
        int result = system(gnuplot_cmd.c_str());
        if (result != 0)
        {
            std::cerr << "Warning: Gnuplot command failed for " << case_name << std::endl;
        }
    }

public:
    CircuitLegalizer(const std::string &input, const std::string &output)
        : input_dir(input), output_dir(output)
    {
        input_name = input_dir.stem().string();
        output_name = output_dir.stem().string();

        if (!fs::exists(input_dir))
        {
            throw std::runtime_error("Input directory does not exist: " + input);
        }

        fs::create_directories(output_dir);
    }

    void process()
    {
        try
        {
            std::cout << "Processing input files..." << std::endl;
            processNodesFile();
            processPlFile();
            processSclFile();

            std::cout << "\nGenerating initial visualization..." << std::endl;
            generateVisualization(false);

            std::cout << "\nPerforming detailed placement..." << std::endl;
            detailedPlacement();

            calculateDisplacement();
            std::cout << "\nPlacement results:" << std::endl;
            std::cout << "Total displacement: " << total_displacement << std::endl;
            std::cout << "Maximum displacement: " << max_displacement << std::endl;
            std::cout << "Final overlap: " << calculateTotalOverlap() << std::endl;

            std::cout << "\nGenerating final visualization..." << std::endl;
            generateVisualization(true);

            std::cout << "\nWriting output files..." << std::endl;
            processPlFile(); // Write final positions
            processAuxFile();

            // Copy unchanged files
            fs::copy_file(
                input_dir / (input_name + ".nets"),
                output_dir / (output_name + ".nets"),
                fs::copy_options::overwrite_existing);
            fs::copy_file(
                input_dir / (input_name + ".wts"),
                output_dir / (output_name + ".wts"),
                fs::copy_options::overwrite_existing);
            fs::copy_file(
                input_dir / (input_name + ".scl"),
                output_dir / (output_name + ".scl"),
                fs::copy_options::overwrite_existing);

            std::cout << "All processing completed successfully!" << std::endl;
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error("Error in processing: " + std::string(e.what()));
        }
    }
};

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " INPUT_DIR OUTPUT_DIR" << std::endl;
        return 1;
    }

    try
    {
        CircuitLegalizer legalizer(argv[1], argv[2]);
        legalizer.process();
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}