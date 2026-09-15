// tabu_search.h - Tabu search for the symmetric TSP (lab exercise z02).
//
// Design
//  * Solution representation: the sequence of the n holes (path representation)
//    plus the inverse map pos[hole] -> position, so a hole is located in O(1).
//  * Initial solution: nearest neighbour from a random start (or a random tour).
//  * Neighbourhood: 2-opt, i.e. reverse the sub-path between two positions.
//    The cost change of a move is computed in O(1) from the four arcs involved;
//    the whole neighbourhood is scanned in O(n^2) and the best admissible
//    move is applied (steepest descent).
//  * Tabu attribute: the two arcs removed by a move may not be re-inserted for
//    `tenure` iterations. tabuUntil[u][v] stores the iteration until which the
//    arc {u,v} is tabu, so the check and the update are O(1).
//  * Aspiration: a tabu move is allowed if it improves the best known tour.
//  * Diversification: after maxNoImprove non-improving iterations the best tour
//    is perturbed with `kick` random double-bridge moves and the search
//    restarts from there with an empty tabu memory.
//  * Stopping: iteration limit or time limit.
#ifndef TABU_SEARCH_H
#define TABU_SEARCH_H
#include <random>
#include <string>
#include <vector>

#include "instance.h"

struct TabuParams {
    std::string init;       // "nn" (nearest neighbour) or "random"
    double tenureFrac;      // tenure = max(tenureMin, tenureFrac * n)
    int tenureMin;
    double noImproveFrac;   // maxNoImprove = noImproveFrac * n
    int kick;               // double-bridge moves per perturbation (0 = no diversification)
    long maxIter;           // iteration cap
    double timeLimit;       // seconds
    unsigned seed;
    TabuParams()
        : init("nn"), tenureFrac(0.125), tenureMin(5), noImproveFrac(0.5), kick(6),
          maxIter(1000000000L), timeLimit(10.0), seed(1) {}
};

struct TabuResult {
    double initCost;
    double bestCost;
    long iterations;
    long bestIter;          // iteration at which the best tour was found
    double timeToBest;
    double totalTime;
    int perturbations;
    int tenure;
    long maxNoImprove;
    std::vector<int> bestTour;
    TabuResult()
        : initCost(0), bestCost(0), iterations(0), bestIter(0), timeToBest(0), totalTime(0),
          perturbations(0), tenure(0), maxNoImprove(0) {}
};

class TabuSearch {
public:
    TabuSearch(const Instance& inst, const TabuParams& p);
    TabuResult run();

private:
    const Instance& inst_;
    TabuParams p_;
    int n_;
    std::vector<double> c_;         // flattened cost matrix, c_[i*n+j]
    std::mt19937 rng_;
    std::vector<int> tour_, pos_;   // current tour and inverse map
    double cost_;
    std::vector<long> tabuUntil_;   // flattened n x n, symmetric

    double c(int i, int j) const { return c_[i * n_ + j]; }
    bool isTabu(int u, int v, long iter) const { return tabuUntil_[u * n_ + v] > iter; }
    void makeTabu(int u, int v, long until) { tabuUntil_[u * n_ + v] = until; tabuUntil_[v * n_ + u] = until; }

    void nearestNeighbour(int start);
    void randomTour();
    void setTour(const std::vector<int>& t);
    double evaluate(const std::vector<int>& t) const;
    void apply2opt(int i, int j);   // reverse tour_[i+1 .. j]
    void doubleBridge(std::vector<int>& t);
};
#endif
