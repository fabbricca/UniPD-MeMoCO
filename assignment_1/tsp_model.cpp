#include "tsp_model.h"

#include <cmath>
#include <cstdio>
#include <iostream>

#include "timer.h"

// status code and message buffer used by the macros in cpxmacro.h
int status;
char errmsg[BUF_SIZE];

TSPModel::TSPModel(const Instance& inst, const SolveOptions& opt)
    : inst_(inst), opt_(opt), n_(inst.n()), env_(NULL), lp_(NULL) {
    DECL_ENV(env);
    env_ = env;
    DECL_PROB(env_, lp);
    lp_ = lp;
    CHECKED_CPX_CALL(CPXchgobjsen, env_, lp_, CPX_MIN);
}

TSPModel::~TSPModel() {
    if (lp_) CPXfreeprob(env_, &lp_);
    if (env_) CPXcloseCPLEX(&env_);
}

// Variables are created in bulk, one CPXnewcols call per family. The column
// position of every variable is recorded so that constraints can refer to it.
void TSPModel::addVariables() {
    xIdx_.assign(n_, std::vector<int>(n_, -1));
    yIdx_.assign(n_, std::vector<int>(n_, -1));
    char name[32];

    // x_ij: flow on arc (i,j), continuous, 0 <= x_ij <= n-1, no cost.
    // Variables x_i0 do not exist: no flow needs to go back to node 0.
    {
        std::vector<double> obj, lb, ub;
        std::vector<char> type;
        std::vector<std::string> names;
        int pos = CPXgetnumcols(env_, lp_);
        for (int i = 0; i < n_; ++i)
            for (int j = 1; j < n_; ++j) {
                if (i == j) continue;
                xIdx_[i][j] = pos++;
                obj.push_back(0.0);
                lb.push_back(0.0);
                ub.push_back(n_ - 1.0);
                type.push_back('C');
                std::snprintf(name, sizeof name, "x_%d_%d", i, j);
                names.push_back(name);
            }
        std::vector<char*> cnames;
        for (size_t k = 0; k < names.size(); ++k) cnames.push_back(&names[k][0]);
        CHECKED_CPX_CALL(CPXnewcols, env_, lp_, (int)obj.size(), &obj[0], &lb[0], &ub[0],
                         &type[0], &cnames[0]);
    }
    // y_ij: 1 if the drill moves from i to j; objective coefficient c_ij.
    {
        std::vector<double> obj, lb, ub;
        std::vector<char> type;
        std::vector<std::string> names;
        int pos = CPXgetnumcols(env_, lp_);
        for (int i = 0; i < n_; ++i)
            for (int j = 0; j < n_; ++j) {
                if (i == j) continue;
                yIdx_[i][j] = pos++;
                obj.push_back(inst_.cost[i][j]);
                lb.push_back(0.0);
                ub.push_back(1.0);
                type.push_back('B');
                std::snprintf(name, sizeof name, "y_%d_%d", i, j);
                names.push_back(name);
            }
        std::vector<char*> cnames;
        for (size_t k = 0; k < names.size(); ++k) cnames.push_back(&names[k][0]);
        CHECKED_CPX_CALL(CPXnewcols, env_, lp_, (int)obj.size(), &obj[0], &lb[0], &ub[0],
                         &type[0], &cnames[0]);
    }
}

// Constraints are added in bulk with one CPXaddrows call per family, using the
// sparse row format: rmatbeg[r] is the position in rmatind/rmatval where row r
// starts, rmatind the column indices and rmatval the coefficients.

// (10)  sum_i x_ik - sum_{j != 0} x_kj = 1   for every k != 0
//       (node k keeps one unit of flow and forwards the rest)
void TSPModel::addFlowConstraints() {
    std::vector<int> rmatbeg, rmatind;
    std::vector<double> rmatval, rhs;
    std::vector<char> sense;
    for (int k = 1; k < n_; ++k) {
        rmatbeg.push_back((int)rmatind.size());
        for (int i = 0; i < n_; ++i)
            if (i != k) { rmatind.push_back(xIdx_[i][k]); rmatval.push_back(1.0); }
        for (int j = 1; j < n_; ++j)
            if (j != k) { rmatind.push_back(xIdx_[k][j]); rmatval.push_back(-1.0); }
        rhs.push_back(1.0);
        sense.push_back('E');
    }
    CHECKED_CPX_CALL(CPXaddrows, env_, lp_, 0, (int)rhs.size(), (int)rmatind.size(), &rhs[0],
                     &sense[0], &rmatbeg[0], &rmatind[0], &rmatval[0], NULL, NULL);
}

// (11)  sum_j y_ij = 1  for every i   (one arc leaves each hole)
// (12)  sum_i y_ij = 1  for every j   (one arc enters each hole)
void TSPModel::addDegreeConstraints() {
    std::vector<int> rmatbeg, rmatind;
    std::vector<double> rmatval, rhs;
    std::vector<char> sense;
    for (int i = 0; i < n_; ++i) {
        rmatbeg.push_back((int)rmatind.size());
        for (int j = 0; j < n_; ++j)
            if (j != i) { rmatind.push_back(yIdx_[i][j]); rmatval.push_back(1.0); }
        rhs.push_back(1.0);
        sense.push_back('E');
    }
    for (int j = 0; j < n_; ++j) {
        rmatbeg.push_back((int)rmatind.size());
        for (int i = 0; i < n_; ++i)
            if (i != j) { rmatind.push_back(yIdx_[i][j]); rmatval.push_back(1.0); }
        rhs.push_back(1.0);
        sense.push_back('E');
    }
    CHECKED_CPX_CALL(CPXaddrows, env_, lp_, 0, (int)rhs.size(), (int)rmatind.size(), &rhs[0],
                     &sense[0], &rmatbeg[0], &rmatind[0], &rmatval[0], NULL, NULL);
}

// (13)  x_ij <= (n-1) y_ij  for every arc (i,j), j != 0,
//       written as x_ij - (n-1) y_ij <= 0 because the right-hand side must be a constant.
void TSPModel::addLinkingConstraints() {
    std::vector<int> rmatbeg, rmatind;
    std::vector<double> rmatval, rhs;
    std::vector<char> sense;
    for (int i = 0; i < n_; ++i)
        for (int j = 1; j < n_; ++j) {
            if (i == j) continue;
            rmatbeg.push_back((int)rmatind.size());
            rmatind.push_back(xIdx_[i][j]); rmatval.push_back(1.0);
            rmatind.push_back(yIdx_[i][j]); rmatval.push_back(-(n_ - 1.0));
            rhs.push_back(0.0);
            sense.push_back('L');
        }
    CHECKED_CPX_CALL(CPXaddrows, env_, lp_, 0, (int)rhs.size(), (int)rmatind.size(), &rhs[0],
                     &sense[0], &rmatbeg[0], &rmatind[0], &rmatval[0], NULL, NULL);
}

void TSPModel::setParameters() {
    CHECKED_CPX_CALL(CPXsetdblparam, env_, CPXPARAM_TimeLimit, opt_.timeLimit);
    CHECKED_CPX_CALL(CPXsetintparam, env_, CPXPARAM_Threads, opt_.threads);
    CHECKED_CPX_CALL(CPXsetdblparam, env_, CPXPARAM_MIP_Tolerances_MIPGap, opt_.mipGap);
    CHECKED_CPX_CALL(CPXsetintparam, env_, CPXPARAM_ScreenOutput, opt_.verbose ? CPX_ON : CPX_OFF);
}

// Follow the arcs with y_ij = 1 starting from hole 0. The result is valid if it
// is one cycle through all the holes and its cost equals the objective value.
void TSPModel::extractTour(SolveResult& r) {
    int ncols = CPXgetnumcols(env_, lp_);
    std::vector<double> vals(ncols);
    CHECKED_CPX_CALL(CPXgetx, env_, lp_, &vals[0], 0, ncols - 1);
    r.tour.clear();
    int cur = 0;
    for (int step = 0; step < n_; ++step) {
        r.tour.push_back(cur);
        int next = -1;
        for (int j = 0; j < n_; ++j)
            if (j != cur && vals[yIdx_[cur][j]] > 0.5) { next = j; break; }
        if (next < 0) return;
        cur = next;
    }
    if (cur != 0) return;                    // walk did not close at hole 0
    if (!inst_.isHamiltonian(r.tour)) return; // repeated hole -> subtour
    double c = inst_.tourCost(r.tour);
    r.tourValid = std::fabs(c - r.objective) <= 1e-6 * std::max(1.0, std::fabs(r.objective));
}

SolveResult TSPModel::solve() {
    SolveResult r;
    Timer t;
    addVariables();
    addFlowConstraints();
    addDegreeConstraints();
    addLinkingConstraints();
    r.buildTime = t.elapsed();
    r.numCols = CPXgetnumcols(env_, lp_);
    r.numRows = CPXgetnumrows(env_, lp_);

    setParameters();
    t.reset();
    CHECKED_CPX_CALL(CPXmipopt, env_, lp_);
    r.solveTime = t.elapsed();

    r.status = CPXgetstat(env_, lp_);
    char buf[CPXMESSAGEBUFSIZE];
    r.statusName = CPXgetstatstring(env_, r.status, buf) ? buf : "unknown";
    r.optimal = (r.status == CPXMIP_OPTIMAL || r.status == CPXMIP_OPTIMAL_TOL);
    int solnMethod, solnType;
    CHECKED_CPX_CALL(CPXsolninfo, env_, lp_, &solnMethod, &solnType, NULL, NULL);
    r.hasSolution = (solnType != CPX_NO_SOLN);
    CHECKED_CPX_CALL(CPXgetbestobjval, env_, lp_, &r.bestBound);
    r.nodes = CPXgetnodecnt(env_, lp_);
    if (r.hasSolution) {
        CHECKED_CPX_CALL(CPXgetobjval, env_, lp_, &r.objective);
        CHECKED_CPX_CALL(CPXgetmiprelgap, env_, lp_, &r.relGap);
        extractTour(r);
    }
    return r;
}
