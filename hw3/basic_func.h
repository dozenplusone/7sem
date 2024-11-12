#ifndef _HW3_BASIC_FUNC_H
#define _HW3_BASIC_FUNC_H

#include <cmath>
#include <vector>

class TFunction {
public:
    virtual ~TFunction() = default;

    virtual double operator()(double) const = 0;
};

class TIdent: public TFunction {
public:
    double operator()(double x) const override { return x; }
};

class TConst: public TFunction {
public:
    TConst(double param): ans(param) {}

    double operator()(double = 0.) const override { return ans; }

private:
    double ans;
};

class TExp: public TFunction {
public:
    double operator()(double x) const override { return std::exp(x); }
};

class TPower: public TFunction {
public:
    TPower(double param): pow(param) {}

    double operator()(double x) const override { return std::pow(x, pow); }

private:
    double pow;
};

class TPolynomial: public TFunction {
public:
    TPolynomial(const std::vector<double>& params): coef(params) {}

    double operator()(double x) const override {
        double ans = 0.;

        for (auto it = coef.crbegin(); it != coef.crend(); ++it) {
            ans *= x;
            ans += *it;
        }

        return ans;
    }

private:
    std::vector<double> coef;
};

#endif // _HW3_BASIC_FUNC_H
