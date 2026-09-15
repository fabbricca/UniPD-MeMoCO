// main.cpp - Part I: solve a drilling instance with the flow-based TSP model.
//
// Usage: tsp_cplex <instance file> [--time-limit S] [--threads K] [--gap G]
//                  [--tour FILE] [--verbose] [--header]
// Prints one CSV line with the results (--header prints the column names).
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "instance.h"
#include "tsp_model.h"

static const char* HEADER =
    "instance,n,time_limit,status,optimal,has_sol,obj,bound,gap,nodes,cols,rows,build_s,solve_s,tour_ok";

static void usage(const char* prog) {
    std::cerr << "usage: " << prog
              << " <instance file> [--time-limit S] [--threads K] [--gap G]"
                 " [--tour FILE] [--verbose] [--header]\n";
}

int main(int argc, char** argv) {
    std::string instFile, tourFile;
    SolveOptions opt;
    bool printHeader = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--time-limit" && i + 1 < argc) opt.timeLimit = std::atof(argv[++i]);
        else if (a == "--threads" && i + 1 < argc) opt.threads = std::atoi(argv[++i]);
        else if (a == "--gap" && i + 1 < argc) opt.mipGap = std::atof(argv[++i]);
        else if (a == "--tour" && i + 1 < argc) tourFile = argv[++i];
        else if (a == "--verbose") opt.verbose = true;
        else if (a == "--header") printHeader = true;
        else if (a[0] != '-' && instFile.empty()) instFile = a;
        else { usage(argv[0]); return 2; }
    }
    if (printHeader) std::cout << HEADER << "\n";
    if (instFile.empty()) { if (printHeader) return 0; usage(argv[0]); return 2; }

    try {
        Instance inst = Instance::read(instFile);
        TSPModel model(inst, opt);
        SolveResult r = model.solve();

        std::cout << inst.name << "," << inst.n() << "," << opt.timeLimit << ",\"" << r.statusName
                  << "\"," << (r.optimal ? 1 : 0) << "," << (r.hasSolution ? 1 : 0) << ",";
        if (r.hasSolution) std::cout << r.objective;
        std::cout << "," << r.bestBound << ",";
        if (r.hasSolution) std::cout << r.relGap;
        std::cout << "," << r.nodes << "," << r.numCols << "," << r.numRows << "," << r.buildTime
                  << "," << r.solveTime << "," << (r.tourValid ? 1 : 0) << "\n";

        if (!tourFile.empty() && r.hasSolution) {
            std::ofstream out(tourFile.c_str());
            out << inst.n() << " " << r.objective << "\n";
            for (size_t k = 0; k < r.tour.size(); ++k) out << r.tour[k] << "\n";
        }
        if (r.hasSolution && !r.tourValid) {
            std::cerr << "WARNING: the extracted tour is not a valid hamiltonian cycle\n";
            return 3;
        }
    } catch (std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
