// main.cpp - Part II: solve a drilling instance with tabu search.
//
// Usage: tsp_tabu <instance file> [--seed S] [--init nn|random] [--tenure-frac F]
//                 [--no-improve-frac F] [--kick K] [--max-iter N] [--time-limit S]
//                 [--tour FILE] [--header]
// Prints one CSV line (--header prints the column names).
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "instance.h"
#include "tabu_search.h"

static const char* HEADER =
    "instance,n,seed,init,tenure,max_no_improve,kick,time_limit,init_cost,best_cost,"
    "iterations,best_iter,perturbations,time_to_best,total_s";

static void usage(const char* prog) {
    std::cerr << "usage: " << prog
              << " <instance file> [--seed S] [--init nn|random] [--tenure-frac F]"
                 " [--no-improve-frac F] [--kick K] [--max-iter N] [--time-limit S]"
                 " [--tour FILE] [--header]\n";
}

int main(int argc, char** argv) {
    std::string instFile, tourFile;
    TabuParams p;
    bool printHeader = false;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--seed" && i + 1 < argc) p.seed = (unsigned)std::atol(argv[++i]);
        else if (a == "--init" && i + 1 < argc) p.init = argv[++i];
        else if (a == "--tenure-frac" && i + 1 < argc) p.tenureFrac = std::atof(argv[++i]);
        else if (a == "--no-improve-frac" && i + 1 < argc) p.noImproveFrac = std::atof(argv[++i]);
        else if (a == "--kick" && i + 1 < argc) p.kick = std::atoi(argv[++i]);
        else if (a == "--max-iter" && i + 1 < argc) p.maxIter = std::atol(argv[++i]);
        else if (a == "--time-limit" && i + 1 < argc) p.timeLimit = std::atof(argv[++i]);
        else if (a == "--tour" && i + 1 < argc) tourFile = argv[++i];
        else if (a == "--header") printHeader = true;
        else if (a[0] != '-' && instFile.empty()) instFile = a;
        else { usage(argv[0]); return 2; }
    }
    if (printHeader) std::cout << HEADER << "\n";
    if (instFile.empty()) { if (printHeader) return 0; usage(argv[0]); return 2; }
    if (p.init != "nn" && p.init != "random") { std::cerr << "unknown --init\n"; return 2; }

    try {
        Instance inst = Instance::read(instFile);
        TabuSearch ts(inst, p);
        TabuResult r = ts.run();
        // sanity check: the best tour is hamiltonian and its cost, recomputed from
        // scratch, equals the value tracked incrementally by the search
        double check = inst.tourCost(r.bestTour);
        if (!inst.isHamiltonian(r.bestTour) || std::fabs(check - r.bestCost) > 1e-6 * check) {
            std::cerr << "ERROR: invalid best tour (cost " << r.bestCost << ", recomputed " << check << ")\n";
            return 3;
        }
        std::cout << inst.name << "," << inst.n() << "," << p.seed << "," << p.init << ","
                  << r.tenure << "," << r.maxNoImprove << "," << p.kick << "," << p.timeLimit << ","
                  << r.initCost << "," << r.bestCost << "," << r.iterations << "," << r.bestIter << ","
                  << r.perturbations << "," << r.timeToBest << "," << r.totalTime << "\n";
        if (!tourFile.empty()) {
            std::ofstream out(tourFile.c_str());
            out << inst.n() << " " << r.bestCost << "\n";
            for (size_t k = 0; k < r.bestTour.size(); ++k) out << r.bestTour[k] << "\n";
        }
    } catch (std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
