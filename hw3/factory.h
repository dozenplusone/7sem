#ifndef _HW3_FACTORY_H
#define _HW3_FACTORY_H

#include "basic_func.h"

class TFunctionFactory {
public:
    static TFunctionPtr Create(const std::string& name) {
        if (name == "ident") {
            return std::make_shared<TIdent>();
        }
        if (name == "exp") {
            return std::make_shared<TExp>();
        }
        return nullptr;
    }

    static TFunctionPtr Create(const std::string& name, double param) {
        if (name == "const") {
            return std::make_shared<TConst>(param);
        }
        if (name == "power") {
            return std::make_shared<TPower>(param);
        }
        return nullptr;
    }

    static TFunctionPtr Create(
        const std::string& name,
        const std::vector<double>& params
    ) {
        if (name == "polynomial") {
            return std::make_shared<TPolynomial>(params);
        }
        return nullptr;
    }
};

#endif // _HW3_FACTORY_H
