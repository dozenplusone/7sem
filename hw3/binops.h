#ifndef _HW3_BINOPS_H
#define _HW3_BINOPS_H

#include "basic_func.h"

#include <stdexcept>

class TFuncBinOper: public TFunction {
public:
    TFuncBinOper(TFunctionPtr left, TFunctionPtr right)
        : lhs(left)
        , rhs(right)
    {
        if (!lhs || !rhs) {
            throw std::logic_error("at least one operand is invalid");
        }
    }

protected:
    TFunctionPtr lhs;
    TFunctionPtr rhs;
};

class TFuncSum: public TFuncBinOper {
public:
    TFuncSum(TFunctionPtr left, TFunctionPtr right)
        : TFuncBinOper(left, right)
    {}

    double operator()(double x) const override {
        return (*lhs)(x) + (*rhs)(x);
    }

    std::string ToString() const override {
        std::stringstream out;
        out << lhs->ToString() << " + " << rhs->ToString();
        return out.str();
    }

    double GetDeriv(double x) const override {
        return lhs->GetDeriv(x) + rhs->GetDeriv(x);
    }
};

class TFuncDiff: public TFuncBinOper {
public:
    TFuncDiff(TFunctionPtr left, TFunctionPtr right)
        : TFuncBinOper(left, right)
    {}

    double operator()(double x) const override {
        return (*lhs)(x) - (*rhs)(x);
    }

    std::string ToString() const override {
        std::stringstream out;
        out << lhs->ToString() << " - (" << rhs->ToString() << ')';
        return out.str();
    }

    double GetDeriv(double x) const override {
        return lhs->GetDeriv(x) - rhs->GetDeriv(x);
    }
};

class TFuncMul: public TFuncBinOper {
public:
    TFuncMul(TFunctionPtr left, TFunctionPtr right)
        : TFuncBinOper(left, right)
    {}

    double operator()(double x) const override {
        return (*lhs)(x) * (*rhs)(x);
    }

    std::string ToString() const override {
        std::stringstream out;
        out << '(' << lhs->ToString() << ") * (" << rhs->ToString() << ')';
        return out.str();
    }

    double GetDeriv(double x) const override {
        return lhs->GetDeriv(x) * (*rhs)(x) + (*lhs)(x) * rhs->GetDeriv(x);
    }
};

class TFuncDiv: public TFuncBinOper {
public:
    TFuncDiv(TFunctionPtr left, TFunctionPtr right)
        : TFuncBinOper(left, right)
    {}

    double operator()(double x) const override {
        return (*lhs)(x) / (*rhs)(x);
    }

    std::string ToString() const override {
        std::stringstream out;
        out << '(' << lhs->ToString() << ") / (" << rhs->ToString() << ')';
        return out.str();
    }

    double GetDeriv(double x) const override {
        double div = (*rhs)(x);
        return (
            lhs->GetDeriv(x) * div - (*lhs)(x) * rhs->GetDeriv(x)
        ) / (div * div);
    }
};

TFunctionPtr operator+(TFunctionPtr lhs, TFunctionPtr rhs) {
    return std::make_shared<TFuncSum>(lhs, rhs);
}

TFunctionPtr operator-(TFunctionPtr lhs, TFunctionPtr rhs) {
    return std::make_shared<TFuncDiff>(lhs, rhs);
}

TFunctionPtr operator*(TFunctionPtr lhs, TFunctionPtr rhs) {
    return std::make_shared<TFuncMul>(lhs, rhs);
}

TFunctionPtr operator/(TFunctionPtr lhs, TFunctionPtr rhs) {
    return std::make_shared<TFuncDiv>(lhs, rhs);
}

#endif // _HW3_BINOPS_H
