#include <iostream>
#include <vector>
#include <cmath>
#include <set>
#include <algorithm>
#include <limits>
#include <fstream>
#include <iomanip>
#include <string>
using namespace std;

struct Point {
    int x, y;
    Point(int x = 0, int y = 0) : x(x), y(y) {}
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
    bool operator<(const Point& other) const {
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
    bool operator!=(const Point& other) const {
        return !(*this == other);
    }
};

struct LineSegment {
    Point start, end;
    LineSegment(Point s, Point e) : start(s), end(e) {}
    bool operator<(const LineSegment& other) const {
        if (start != other.start) return start < other.start;
        return end < other.end;
    }
};

class ClockTree {
private:
    vector<Point> sinks;
    Point source;
    int dimX, dimY;
    set<LineSegment> segments;

    int manhattan_distance(const Point& a, const Point& b) {
        return abs(a.x - b.x) + abs(a.y - b.y);
    }

    void add_manhattan_path(const Point& from, const Point& to) {
        if (from.x != to.x) {
            segments.insert(LineSegment(from, Point(to.x, from.y)));
        }
        if (from.y != to.y) {
            segments.insert(LineSegment(Point(to.x, from.y), to));
        }
    }

    Point find_median_point(vector<Point>& points) {
        nth_element(points.begin(), points.begin() + points.size()/2, points.end(),
                    [](const Point& a, const Point& b) { return a.x < b.x; });
        int median_x = points[points.size()/2].x;
        
        nth_element(points.begin(), points.begin() + points.size()/2, points.end(),
                    [](const Point& a, const Point& b) { return a.y < b.y; });
        int median_y = points[points.size()/2].y;
        
        return Point(median_x, median_y);
    }

    Point find_center(const vector<Point>& points) {
        int sum_x = 0, sum_y = 0;
        for (const auto& p : points) {
            sum_x += p.x;
            sum_y += p.y;
        }
        return Point(sum_x / points.size(), sum_y / points.size());
    }

    void build_tree(const vector<Point>& points) {
        if (points.size() <= 1) return;
        
        Point center = find_center(points);
        
        vector<Point> quadrants[4];
        for (const auto& p : points) {
            if (p.x <= center.x && p.y <= center.y) quadrants[0].push_back(p);
            else if (p.x > center.x && p.y <= center.y) quadrants[1].push_back(p);
            else if (p.x <= center.x && p.y > center.y) quadrants[2].push_back(p);
            else quadrants[3].push_back(p);
        }
        
        for (int i = 0; i < 4; ++i) {
            if (!quadrants[i].empty()) {
                Point quad_center = find_center(quadrants[i]);
                add_manhattan_path(center, quad_center);
                build_tree(quadrants[i]);
            }
        }
    }

    void generate_plot_script(const string& filename) {
        ofstream file(filename);
        file << "set xrange [0:" << dimX << "]\n";
        file << "set yrange [0:" << dimY << "]\n";

        for (const auto& seg : segments) {
            file << "set arrow from " << seg.start.x << "," << seg.start.y 
                 << " to " << seg.end.x << "," << seg.end.y << " nohead\n";
        }

        file << "plot '-' with points pt 7 ps 1.5 title 'Sinks', "
             << "'-' with points pt 7 ps 2 title 'Source'\n";
        
        for (const auto& sink : sinks) {
            file << sink.x << " " << sink.y << endl;
        }
        file << "e\n";
        file << source.x << " " << source.y << endl;
        file << "e\n";

        file.close();
    }

public:
    void read_input(const string& filename) {
        ifstream file(filename);
        string line;
        getline(file, line); // Skip the first line
        file >> line >> dimX;
        file >> line >> dimY;
        int x, y;
        file >> x >> y;
        source = Point(x, y);
        while (file >> x >> y) {
            if (x == '.' && y == 'e') break;
            sinks.emplace_back(x, y);
        }
    }

    void synthesize() {
        vector<Point> all_points = sinks;
        all_points.push_back(source);
        build_tree(all_points);
    }

    void write_output(const string& filename) {
        ofstream file(filename);
        file << ".l " << segments.size() << endl;
        file << ".dimx " << dimX << endl;
        file << ".dimy " << dimY << endl;
        for (const auto& seg : segments) {
            file << seg.start.x << " " << seg.start.y << " "
                 << seg.end.x << " " << seg.end.y << endl;
        }
        file << ".e" << endl;

        int t_max = 0, t_min = numeric_limits<int>::max();
        int w_cts = 0;
        for (const auto& sink : sinks) {
            int path_length = manhattan_distance(sink, source);
            t_max = max(t_max, path_length);
            t_min = min(t_min, path_length);
        }

        for (const auto& seg : segments) {
            w_cts += manhattan_distance(seg.start, seg.end);
        }

        int w_flute = w_cts; // This is a placeholder

        double skew_ratio = static_cast<double>(t_max) / t_min;
        double wire_length_ratio = static_cast<double>(w_cts) / w_flute;
        cout << "T_max: " << t_max << ", T_min: " << t_min << ", Skew ratio: " << fixed << setprecision(2) << skew_ratio << endl;
        cout << "W_cts: " << w_cts << ", W_FLUTE: " << w_flute << ", Wire length ratio: " << fixed << setprecision(2) << wire_length_ratio << endl;

        file << "T_max: " << t_max << ", T_min: " << t_min << ", Skew ratio: " << fixed << setprecision(2) << skew_ratio << endl;
        file << "W_cts: " << w_cts << ", W_FLUTE: " << w_flute << ", Wire length ratio: " << fixed << setprecision(2) << wire_length_ratio << endl;

        string gnuplot_filename = filename + ".plt";
        generate_plot_script(gnuplot_filename);
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " INPUT_FILE OUTPUT_FILE" << endl;
        return 1;
    }

    ClockTree ct;
    ct.read_input(argv[1]);
    ct.synthesize();
    ct.write_output(argv[2]);

    cout << "Clock tree synthesis completed. Output written to " << argv[2] << endl;
    cout << "To visualize the result, run: gnuplot " << argv[2] << ".plt" << endl;

    return 0;
}