#ifndef _HW2_SCHEDULING_H
#define _HW2_SCHEDULING_H

#include "annealing.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <vector>

namespace Scheduling {
    class Solution;
    class Mutation;

    using SolutionPtr = std::shared_ptr<Solution>;
    using MutationPtr = std::shared_ptr<Mutation>;
}

class Scheduling::Solution: public hw2::Solution {
    friend class Mutation;

    std::vector<std::vector<bool>> schedule;
    std::vector<unsigned> times;

public:
    Solution(unsigned, const std::vector<unsigned>&);
    Solution(const std::filesystem::path&);

    double criterion(void) const override;
    std::vector<std::vector<unsigned>> get_schedule(void) const;
};

class Scheduling::Mutation: public hw2::Mutation {
public:
    hw2::SolutionPtr mutate(hw2::SolutionPtr) override;
};

Scheduling::Solution::Solution(
    unsigned n_proc,
    const std::vector<unsigned> &work_times
)
    : schedule(n_proc, std::vector<bool>(work_times.size(), false))
    , times(work_times)
{
    std::mt19937 rng(std::random_device{}());
    unsigned proc = std::uniform_int_distribution(0u, n_proc - 1u)(rng);

    for (unsigned work = 0u; work < times.size(); ++work) {
        schedule[proc][work] = true;
    }
}

Scheduling::Solution::Solution(const std::filesystem::path &path) {
    std::ifstream file(path);

    if (!file.is_open()) {
        throw std::invalid_argument("can't open file");
    }

    std::mt19937 rng(std::random_device{}());
    unsigned n_proc;
    unsigned time;

    file >> n_proc;
    file.get();
    schedule.resize(n_proc);
    unsigned proc = std::uniform_int_distribution(0u, n_proc - 1u)(rng);

    while (file >> time) {
        times.push_back(time);
        schedule[proc].push_back(true);
        file.get();
    }

    for (auto &proc: schedule) {
        if (proc.empty()) {
            proc.resize(times.size());
        }
    }

    file.close();
}

double Scheduling::Solution::criterion(void) const {
    double min = std::numeric_limits<double>::max();
    double max = 0.;

    for (const auto &proc: schedule) {
        double cur_min = 0.;
        double cur_max = 0.;

        for (unsigned work = 0u; work < proc.size(); ++work) {
            if (!proc[work]) {
                continue;
            }

            if (times[work] > cur_min) {
                cur_min = times[work];
            }

            cur_max += times[work];
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

std::vector<std::vector<unsigned>> Scheduling::Solution::get_schedule(void)
const
{
    std::vector<std::vector<unsigned>> sched;

    for (const auto &proc: schedule) {
        sched.emplace_back();

        for (unsigned work = 0u; work < proc.size(); ++work) {
            if (proc[work]) {
                sched.back().push_back(work);
            }
        }

        std::sort(
            sched.back().begin(), sched.back().end(),
            [this](auto i, auto j) { return times[i] > times[j]; }
        );
    }

    return sched;
}

hw2::SolutionPtr Scheduling::Mutation::mutate(hw2::SolutionPtr sol) {
    SolutionPtr ans = std::make_shared<Solution>(
        *std::dynamic_pointer_cast<Solution>(sol)
    );

    if (ans->schedule.size() <= 1u) {
        return ans;
    }

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<unsigned> dist_work(
        0u, ans->times.size() - 1u
    );
    std::uniform_int_distribution<unsigned> dist_proc(
        0u, ans->schedule.size() - 1u
    );

    unsigned work = dist_work(rng);
    unsigned src = ans->schedule.size();
    unsigned dst;

    for (unsigned proc = 0u; proc < src; ++proc) {
        if (ans->schedule[proc][work]) {
            src = proc;
        }
    }

    do {
        dst = dist_proc(rng);
    } while (dst == src);

    ans->schedule[src][work] = false;
    ans->schedule[dst][work] = true;

    return ans;
}

#endif // _HW2_SCHEDULING_H
