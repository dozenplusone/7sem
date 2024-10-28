#ifndef _HW2_ANNEALING_H
#define _HW2_ANNEALING_H

#include <cmath>

namespace hw2 {
    class Solution;
    class Mutation;
    class Cooldown;

    namespace BasicCD {
        class Boltzmann;
        class Cauchy;
        class LogCauchy;
    }
} // namespace hw2

class hw2::Solution {
public:
    virtual ~Solution() = default;

    virtual double criterion(void) const = 0;
};

class hw2::Mutation {
public:
    virtual ~Mutation() = default;

    virtual void mutate(Solution&) = 0;
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

#endif // _HW2_ANNEALING_H
