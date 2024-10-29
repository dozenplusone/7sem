#ifndef _HW2_ANNEALING_H
#define _HW2_ANNEALING_H

#include <cmath>
#include <ctime>
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
    Annealing(Solution *init, Mutation *mut, Cooldown *cd)
        : best(init)
        , mutation(mut)
        , cooldown(cd)
    {}

    Solution *run(void);
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

class hw2::BasicCD::Boltzmann final: protected hw2::Cooldown {
public:
    Boltzmann(double init): Cooldown(init) {}
    ~Boltzmann() = default;

    double get_temp(unsigned it) const override {
        return it ? temp0 / std::log(1. + it) : temp0;
    }
};

class hw2::BasicCD::Cauchy final: protected hw2::Cooldown {
public:
    Cauchy(double init): Cooldown(init) {}
    ~Cauchy() = default;

    double get_temp(unsigned it) const override {
        return it ? temp0 / (1. + it) : temp0;
    }
};

class hw2::BasicCD::LogCauchy final: protected hw2::Cooldown {
public:
    LogCauchy(double init): Cooldown(init) {}
    ~LogCauchy() = default;

    double get_temp(unsigned it) const override {
        double itp1 = 1. + it;
        return it ? temp0 * std::log(itp1) / itp1 : temp0;
    }
};

hw2::Solution *hw2::Annealing::run(void) {
    std::mt19937 gen(std::time(nullptr));
    std::uniform_real_distribution<> rand(0., 1.);

    Solution *sol_cur = best->copy();
    double crit_cur = sol_cur->criterion();
    unsigned not_improved = 0u;

    for (unsigned it = 0u; not_improved < 100u; ++it) {
        Solution *sol_new = mutation->mutate(sol_cur);
        double crit_new = sol_new->criterion();

        double temp = cooldown->get_temp(it);
        double _diff = crit_cur - crit_new;

        if (_diff >= 0 || rand(gen) < std::exp(_diff / temp)) {
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