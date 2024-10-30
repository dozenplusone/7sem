#ifndef _HW2_ANNEALING_H
#define _HW2_ANNEALING_H

#include <cmath>
#include <random>

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
} // namespace hw2

class hw2::Annealing {
    Solution *best;
    Mutation *mutation;
    Cooldown *cooldown;

public:
    Annealing(Mutation *mut, Cooldown *cd)
        : best(nullptr)
        , mutation(mut)
        , cooldown(cd)
    {}

    Solution *run(Solution*);
};

class hw2::Solution {
public:
    virtual ~Solution() = default;

    virtual double criterion(void) const = 0;
    virtual Solution *copy(void) const = 0;
};

class hw2::Mutation {
public:
    virtual ~Mutation() = default;

    virtual Solution *mutate(Solution*) = 0;
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

hw2::Solution *hw2::Annealing::run(Solution *init) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution dist(0., 1.);

    best = init->copy();
    Solution *sol_cur = init->copy();
    double crit_cur = sol_cur->criterion();
    unsigned not_improved = 0u;

    for (unsigned it = 0u; not_improved < 100u; ++it) {
        Solution *sol_new = mutation->mutate(sol_cur);
        double crit_new = sol_new->criterion();

        double temp = cooldown->get_temp(it);
        double _diff = crit_cur - crit_new;

        if (_diff >= 0. || dist(rng) < std::exp(_diff / temp)) {
            std::swap(sol_cur, sol_new);
            crit_cur = crit_new;
        }

        delete sol_new;

        if (crit_cur < best->criterion()) {
            delete best;
            best = sol_cur->copy();
        } else {
            ++not_improved;
        }
    }

    delete sol_cur;
    return best;
}

#endif // _HW2_ANNEALING_H
