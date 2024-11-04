#ifndef _HW2_ANNEALING_H
#define _HW2_ANNEALING_H

#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

namespace hw2 {
    class Annealing;

    class Solution;
    class Mutation;
    class Cooldown;

    namespace BasicCD {
        class Boltzmann;
        class Cauchy;
        class LogCauchy;
    }

    using SolutionPtr = std::shared_ptr<Solution>;
    using MutationPtr = std::shared_ptr<Mutation>;
    using CooldownPtr = std::shared_ptr<Cooldown>;
} // namespace hw2

class hw2::Annealing {
    std::vector<std::thread> threads;
    std::vector<SolutionPtr> locals;
    SolutionPtr best;
    MutationPtr mutation;
    CooldownPtr cooldown;
    mutable std::mutex mutex;

    void thread_payload(void);

public:
    Annealing(unsigned n_proc, MutationPtr mut, CooldownPtr cd)
        : threads(n_proc)
        , best(nullptr)
        , mutation(mut)
        , cooldown(cd)
    {}

    SolutionPtr run(SolutionPtr);
};

class hw2::Solution {
public:
    virtual ~Solution() = default;

    virtual double criterion(void) const = 0;
};

class hw2::Mutation {
public:
    virtual ~Mutation() = default;

    virtual SolutionPtr mutate(SolutionPtr) = 0;
};

class hw2::Cooldown {
protected:
    const double temp0;

public:
    Cooldown(double init): temp0(init) {}
    virtual ~Cooldown() = default;

    virtual double get_temp(unsigned) const = 0;
};

class hw2::BasicCD::Boltzmann final: public hw2::Cooldown {
public:
    Boltzmann(double init): Cooldown(init) {}
    ~Boltzmann() = default;

    double get_temp(unsigned it) const override {
        if (it) {
            return temp0 / std::log(1. + it);
        }
        return temp0;
    }
};

class hw2::BasicCD::Cauchy final: public hw2::Cooldown {
public:
    Cauchy(double init): Cooldown(init) {}
    ~Cauchy() = default;

    double get_temp(unsigned it) const override {
        if (it) {
            return temp0 / (1. + it);
        }
        return temp0;
    }
};

class hw2::BasicCD::LogCauchy final: public hw2::Cooldown {
public:
    LogCauchy(double init): Cooldown(init) {}
    ~LogCauchy() = default;

    double get_temp(unsigned it) const override {
        if (it) {
            double itp1 = 1. + it;
            return temp0 * std::log(itp1) / itp1;
        }
        return temp0;
    }
};

void hw2::Annealing::thread_payload(void) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution dist(0., 1.);

    SolutionPtr sol_best = best;
    double crit_best = best->criterion();

    SolutionPtr sol_cur = best;
    double crit_cur = crit_best;

    unsigned not_improved = 0u;

    for (unsigned it = 0u; not_improved < 10u; ++it) {
        SolutionPtr sol_new = mutation->mutate(sol_cur);
        double crit_new = sol_new->criterion();

        double temp = cooldown->get_temp(it);
        double _diff = crit_cur - crit_new;

        if (_diff >= 0. || dist(rng) < std::exp(_diff / temp)) {
            sol_cur = sol_new;
            crit_cur = crit_new;
        }

        if (crit_cur < crit_best) {
            sol_best = sol_cur;
            crit_best = crit_cur;
            not_improved = 0u;
        } else {
            ++not_improved;
        }
    }

    std::lock_guard guard(mutex);
    locals.emplace_back(std::move(sol_best));
}

hw2::SolutionPtr hw2::Annealing::run(SolutionPtr init) {
    best = init;
    double crit_best = best->criterion();

    SolutionPtr cur = best;
    double crit_cur = crit_best;

    unsigned not_improved = 0u;

    do {
        for (auto &thr: threads) {
            thr = std::thread(&Annealing::thread_payload, this);
        }

        for (auto &thr: threads) {
            thr.join();
        }

        cur = *std::min_element(
            locals.begin(), locals.end(),
            [](auto s1, auto s2) { return s1->criterion() < s2->criterion(); }
        );
        crit_cur = cur->criterion();

        if (crit_cur < crit_best) {
            best = cur;
            crit_best = crit_cur;
            not_improved = 0u;
        } else {
            ++not_improved;
        }

        locals.clear();
    } while (not_improved < 10u);

    return best;
}

#endif // _HW2_ANNEALING_H
