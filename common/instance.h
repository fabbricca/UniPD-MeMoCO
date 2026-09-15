// instance.h - a drilling instance: hole coordinates and the cost matrix.
//
// File format (plain text):
//   n
//   x_1 y_1
//   ...
//   x_n y_n
// Coordinates are in millimetres. The cost of moving from hole i to hole j is
// the Euclidean distance between them (the drill head moves in a straight
// line at constant speed, so time is proportional to distance).
#ifndef INSTANCE_H
#define INSTANCE_H
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

struct Instance {
    std::string name;
    std::vector<double> x, y;
    std::vector<std::vector<double> > cost;   // cost[i][j], symmetric, cost[i][i] = 0

    int n() const { return (int)x.size(); }

    static Instance read(const std::string& path) {
        std::ifstream in(path.c_str());
        if (!in) throw std::runtime_error("cannot open instance file: " + path);
        Instance inst;
        int n = 0;
        if (!(in >> n) || n < 3) throw std::runtime_error("bad instance file: " + path);
        inst.x.resize(n);
        inst.y.resize(n);
        for (int i = 0; i < n; ++i)
            if (!(in >> inst.x[i] >> inst.y[i]))
                throw std::runtime_error("bad coordinate line in " + path);
        // instance name = file name without directory and extension
        std::string base = path;
        size_t slash = base.find_last_of('/');
        if (slash != std::string::npos) base = base.substr(slash + 1);
        size_t dot = base.find_last_of('.');
        if (dot != std::string::npos) base = base.substr(0, dot);
        inst.name = base;
        inst.computeCosts();
        return inst;
    }

    void computeCosts() {
        int N = n();
        cost.assign(N, std::vector<double>(N, 0.0));
        for (int i = 0; i < N; ++i)
            for (int j = 0; j < N; ++j)
                if (i != j) {
                    double dx = x[i] - x[j], dy = y[i] - y[j];
                    cost[i][j] = std::sqrt(dx * dx + dy * dy);
                }
    }

    // Cost of a closed tour given as a permutation of 0..n-1 (return arc included).
    double tourCost(const std::vector<int>& tour) const {
        double c = 0.0;
        for (size_t k = 0; k < tour.size(); ++k)
            c += cost[tour[k]][tour[(k + 1) % tour.size()]];
        return c;
    }

    // True if `tour` visits every hole exactly once.
    bool isHamiltonian(const std::vector<int>& tour) const {
        if ((int)tour.size() != n()) return false;
        std::vector<char> seen(n(), 0);
        for (size_t k = 0; k < tour.size(); ++k) {
            int v = tour[k];
            if (v < 0 || v >= n() || seen[v]) return false;
            seen[v] = 1;
        }
        return true;
    }
};
#endif
