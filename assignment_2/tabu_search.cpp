#include "tabu_search.h"

#include <algorithm>
#include <limits>
#include <numeric>

#include "timer.h"

TabuSearch::TabuSearch(const Instance& inst, const TabuParams& p)
    : inst_(inst), p_(p), n_(inst.n()), rng_(p.seed), cost_(0) {
    c_.resize((size_t)n_ * n_);
    for (int i = 0; i < n_; ++i)
        for (int j = 0; j < n_; ++j) c_[i * n_ + j] = inst.cost[i][j];
}

double TabuSearch::evaluate(const std::vector<int>& t) const {
    double s = 0.0;
    for (int k = 0; k < n_; ++k) s += c(t[k], t[(k + 1) % n_]);
    return s;
}

void TabuSearch::setTour(const std::vector<int>& t) {
    tour_ = t;
    pos_.assign(n_, -1);
    for (int k = 0; k < n_; ++k) pos_[tour_[k]] = k;
    cost_ = evaluate(tour_);
}

// Nearest neighbour constructive heuristic, O(n^2).
void TabuSearch::nearestNeighbour(int start) {
    std::vector<char> used(n_, 0);
    std::vector<int> t;
    t.reserve(n_);
    int cur = start;
    t.push_back(cur);
    used[cur] = 1;
    for (int k = 1; k < n_; ++k) {
        int best = -1;
        double bestC = std::numeric_limits<double>::max();
        for (int j = 0; j < n_; ++j)
            if (!used[j] && c(cur, j) < bestC) { bestC = c(cur, j); best = j; }
        used[best] = 1;
        t.push_back(best);
        cur = best;
    }
    setTour(t);
}

void TabuSearch::randomTour() {
    std::vector<int> t(n_);
    std::iota(t.begin(), t.end(), 0);
    std::shuffle(t.begin(), t.end(), rng_);
    setTour(t);
}

// Reverse the segment tour_[i+1..j] (i < j) and keep pos_ up to date.
// Removed arcs: (t[i],t[i+1]) and (t[j],t[j+1]); added: (t[i],t[j]) and (t[i+1],t[j+1]).
void TabuSearch::apply2opt(int i, int j) {
    int a = i + 1, b = j;
    while (a < b) {
        std::swap(tour_[a], tour_[b]);
        pos_[tour_[a]] = a;
        pos_[tour_[b]] = b;
        ++a; --b;
    }
    if (a == b) pos_[tour_[a]] = a;
}

// Double-bridge move: cut the tour into four segments A B C D and reconnect
// them as A C B D. A single 2-opt move cannot undo it, so it moves the search
// to a different region without destroying the tour completely.
void TabuSearch::doubleBridge(std::vector<int>& t) {
    if (n_ < 8) { std::shuffle(t.begin(), t.end(), rng_); return; }
    std::uniform_int_distribution<int> d(1, n_ - 1);
    int cut[3] = {d(rng_), d(rng_), d(rng_)};
    std::sort(cut, cut + 3);
    if (cut[0] == cut[1] || cut[1] == cut[2]) return;  // degenerate cut, skip
    std::vector<int> r;
    r.reserve(n_);
    r.insert(r.end(), t.begin(), t.begin() + cut[0]);            // A
    r.insert(r.end(), t.begin() + cut[1], t.begin() + cut[2]);   // C
    r.insert(r.end(), t.begin() + cut[0], t.begin() + cut[1]);   // B
    r.insert(r.end(), t.begin() + cut[2], t.end());              // D
    t.swap(r);
}

TabuResult TabuSearch::run() {
    Timer timer;
    TabuResult res;
    const int tenure = std::max(p_.tenureMin, (int)(p_.tenureFrac * n_));
    const long maxNoImprove = std::max(1L, (long)(p_.noImproveFrac * n_));
    res.tenure = tenure;
    res.maxNoImprove = maxNoImprove;

    if (p_.init == "random") randomTour();
    else nearestNeighbour(std::uniform_int_distribution<int>(0, n_ - 1)(rng_));
    res.initCost = cost_;
    std::vector<int> bestTour = tour_;
    double bestCost = cost_;
    res.timeToBest = timer.elapsed();

    tabuUntil_.assign((size_t)n_ * n_, 0);
    long iter = 0, noImprove = 0;
    while (iter < p_.maxIter && timer.elapsed() < p_.timeLimit) {
        ++iter;
        // --- scan the 2-opt neighbourhood for the best admissible move ---
        int bi = -1, bj = -1;
        double bestDelta = std::numeric_limits<double>::max();
        for (int i = 0; i < n_ - 2; ++i) {
            int a = tour_[i], b = tour_[i + 1];
            double cab = c(a, b);
            int jmax = (i == 0) ? n_ - 2 : n_ - 1;   // the two removed arcs must not be adjacent
            for (int j = i + 2; j <= jmax; ++j) {
                int cc = tour_[j], d = tour_[j + 1 == n_ ? 0 : j + 1];
                double delta = c(a, cc) + c(b, d) - cab - c(cc, d);
                if (delta >= bestDelta) continue;
                bool tabuMove = isTabu(a, cc, iter) || isTabu(b, d, iter);
                if (tabuMove && !(cost_ + delta < bestCost - 1e-9)) continue;   // aspiration
                bestDelta = delta;
                bi = i; bj = j;
            }
        }
        if (bi < 0) {   // every move is tabu: forget the memory and retry
            tabuUntil_.assign((size_t)n_ * n_, 0);
            continue;
        }
        // --- apply the move; the removed arcs become tabu ---
        int a = tour_[bi], b = tour_[bi + 1], cc = tour_[bj], d = tour_[bj + 1 == n_ ? 0 : bj + 1];
        makeTabu(a, b, iter + tenure);
        makeTabu(cc, d, iter + tenure);
        apply2opt(bi, bj);
        cost_ += bestDelta;

        if (cost_ < bestCost - 1e-9) {
            cost_ = evaluate(tour_);   // recompute from scratch: no accumulated rounding error
            bestCost = cost_;
            bestTour = tour_;
            res.bestIter = iter;
            res.timeToBest = timer.elapsed();
            noImprove = 0;
        } else {
            ++noImprove;
        }
        // --- diversification: perturb the best tour and restart from it ---
        if (noImprove >= maxNoImprove) {
            if (p_.kick <= 0) break;    // no diversification: the search is over
            std::vector<int> t = bestTour;
            for (int k = 0; k < p_.kick; ++k) doubleBridge(t);
            setTour(t);
            tabuUntil_.assign((size_t)n_ * n_, 0);
            noImprove = 0;
            ++res.perturbations;
        }
    }
    res.iterations = iter;
    res.bestCost = bestCost;
    res.bestTour = bestTour;
    res.totalTime = timer.elapsed();
    return res;
}
