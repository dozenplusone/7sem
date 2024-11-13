#ifndef _HW3_BASIC_FUNC_H
#define _HW3_BASIC_FUNC_H

#include <cmath>
#include <sstream>
#include <vector>

class TFunction {
public:
    virtual ~TFunction() = default;

    virtual double operator()(double) const = 0;

    virtual std::string ToString() const = 0;
};

class TIdent: public TFunction {
public:
    explicit TIdent() = default;

    double operator()(double x) const override { return x; }

    std::string ToString() const override { return "x"; }
};

class TConst: public TFunction {
public:
    explicit TConst(double param)
        : ans(param)
    {}

    double operator()(double) const override { return ans; }

    std::string ToString() const override {
        std::stringstream out;
        out << ans;
        return out.str();
    }

private:
    double ans;
};

class TExp: public TFunction {
public:
    explicit TExp() = default;

    double operator()(double x) const override { return std::exp(x); }

    std::string ToString() const override { return "e^x"; }
};

class TPower: public TFunction {
public:
    explicit TPower(double param)
        : pow(param)
    {}

    double operator()(double x) const override { return std::pow(x, pow); }

    std::string ToString() const override {
        std::stringstream out;
        out << "x^" << pow;
        return out.str();
    }

private:
    double pow;
};

class TPolynomial: public TFunction {
public:
    explicit TPolynomial(const std::vector<double>& params)
        : coef(params)
    {}

    double operator()(double x) const override {
        double ans = 0.;

        for (auto it = coef.crbegin(); it != coef.crend(); ++it) {
            ans *= x;
            ans += *it;
        }

        return ans;
    }

    std::string ToString() const override {
        std::stringstream out;
        std::size_t i = 0;

        while (i < coef.size() && coef[i] == 0.) {
            ++i;
        }

        if (i == coef.size()) {
            return "0";
        }

        if (i == 0) {
            out << coef[i];
        } else {
            if (coef[i] == -1.) {
                out << '-';
            } else if (coef[i] != 1.) {
                out << coef[i] << '*';
            }

            out << 'x';

            if (i != 1) {
                out << '^' << i;
            }
        }

        for (++i; i < coef.size(); ++i) {
            if (coef[i] == 0.) {
                continue;
            }

            if (coef[i] < 0.) {
                out << " - ";

                if (coef[i] != -1.) {
                    out << -coef[i] << '*';
                }
            } else {
                out << " + ";

                if (coef[i] != 1.) {
                    out << coef[i] << '*';
                }
            }

            out << 'x';

            if (i != 1) {
                out << '^' << i;
            }
        }

        return out.str();
    }

private:
    std::vector<double> coef;
};

#endif // _HW3_BASIC_FUNC_H
