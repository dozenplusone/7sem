#ifndef _HW2_SCHEDULING_H
#define _HW2_SCHEDULING_H

#include "annealing.h"

#include <algorithm>
#include <limits>
#include <vector>

namespace Scheduling {
    class Solution;
    class Mutation;
}

class Scheduling::Solution: public hw2::Solution {
    std::vector<std::vector<bool>> schedule;
    std::vector<unsigned> times;

public:
    Solution(unsigned, const std::vector<unsigned>&);

    double criterion(void) const override;
    hw2::Solution *copy(void) const override { return new Solution(*this); }
};

class Scheduling::Mutation: protected hw2::Mutation {
public:
    hw2::Solution *mutate(hw2::Solution*) override;
};

Scheduling::Solution::Solution(
    unsigned n_proc,
    const std::vector<unsigned> &work_times
)
    : schedule(n_proc, std::vector<bool>(work_times.size(), false))
    , times(work_times)
{
    std::vector<unsigned> idx(times.size());
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(
        idx.begin(), idx.end(),
        [this](auto i, auto j) { return times[i] < times[j]; }
    );

    unsigned proc = 0u;
    bool desc = false;

    while (!idx.empty()) {
        schedule[proc - desc][idx.back()] = true;
        idx.pop_back();
        if (!desc && ++proc == n_proc || desc && --proc == 0u) {
            desc = !desc;
        }
    }
}

double Scheduling::Solution::criterion(void) const {
    double min = std::numeric_limits<double>::max();
    double max = 0.;
    for (const auto &proc: schedule) {
        double cur_min = 0.;
        double cur_max = 0.;
        for (unsigned i{}; i < proc.size(); ++i) {
            if (!proc[i]) {
                continue;
            }
            if (times[i] > cur_min) {
                cur_min = times[i];
            }
            cur_max += times[i];
        }
        if (cur_min < min) {
            min = cur_min;
        }
        if (cur_max > max) {
            max = cur_max;
        }
    }
    return max - min;
}

#endif // _HW2_SCHEDULING_H
