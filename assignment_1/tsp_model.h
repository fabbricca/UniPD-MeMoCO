// tsp_model.h - flow-based TSP model of lab exercise z01 (Gavish-Graves,
// single commodity), built and solved with the CPLEX Callable Library.
#ifndef TSP_MODEL_H
#define TSP_MODEL_H
#include <string>
#include <vector>

#include "cpxmacro.h"
#include "instance.h"

struct SolveOptions {
    double timeLimit;   // seconds given to CPXmipopt
    int threads;        // 1 = deterministic, fair comparison with the single-threaded heuristic
    double mipGap;      // relative MIP gap tolerance (CPLEX default 1e-4 is too loose)
    bool verbose;
    SolveOptions() : timeLimit(60.0), threads(1), mipGap(1e-6), verbose(false) {}
};

struct SolveResult {
    int status;               // CPXgetstat code
    std::string statusName;
    bool hasSolution;         // an incumbent exists
    bool optimal;             // CPXMIP_OPTIMAL or CPXMIP_OPTIMAL_TOL
    double objective;         // incumbent value (if hasSolution)
    double bestBound;         // best lower bound proved by CPLEX
    double relGap;            // relative gap (incumbent - bound) / incumbent
    long long nodes;          // branch-and-bound nodes explored
    double buildTime;         // seconds to create the model
    double solveTime;         // seconds inside CPXmipopt
    int numCols, numRows;
    std::vector<int> tour;    // hamiltonian cycle read from the y variables, starting at 0
    bool tourValid;           // tour is a single cycle through all holes with cost == objective
    SolveResult()
        : status(0), hasSolution(false), optimal(false), objective(0), bestBound(0), relGap(0),
          nodes(0), buildTime(0), solveTime(0), numCols(0), numRows(0), tourValid(false) {}
};

class TSPModel {
public:
    TSPModel(const Instance& inst, const SolveOptions& opt);
    ~TSPModel();
    SolveResult solve();

private:
    const Instance& inst_;
    SolveOptions opt_;
    int n_;
    Env env_;
    Prob lp_;
    // CPLEX identifies a variable by its column position: xIdx_[i][j] and
    // yIdx_[i][j] store the position of x_ij and y_ij (-1 if the variable
    // does not exist: i == j, or j == 0 for x).
    std::vector<std::vector<int> > xIdx_, yIdx_;

    void addVariables();
    void addFlowConstraints();      // (10)
    void addDegreeConstraints();    // (11) and (12)
    void addLinkingConstraints();   // (13)
    void setParameters();
    void extractTour(SolveResult& r);
};
#endif
