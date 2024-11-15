#include "libfunc.h"

#include "gtest/gtest.h"

#include <unordered_set>

static TFunctionFactory factory;

static std::unordered_set<double> TestNumbers{
    42, -1, 0, 2.7182, -13, 3.1416, 100, 0.5, 1.618
};

/*
 * Tests for basic functions and their derivatives.
*/

TEST(Basics, Ident) {
    auto testee = factory.Create("ident");
    EXPECT_EQ(testee->ToString(), "x");

    for (const auto& x : TestNumbers) {
        EXPECT_EQ((*testee)(x), x);
        EXPECT_EQ(testee->GetDeriv(x), 1.);
    }
}

TEST(Basics, Const) {
    for (const auto& c : TestNumbers) {
        auto testee = factory.Create("const", c);
        EXPECT_EQ(testee->ToString(), (std::stringstream{} << c).str());

        for (const auto& x : TestNumbers) {
            EXPECT_EQ((*testee)(x), c);
            EXPECT_EQ(testee->GetDeriv(x), 0.);
        }
    }
}

TEST(Basics, Power) {
    auto sqr = factory.Create("power", 2);
    auto inv = factory.Create("power", -1);
    auto sqrt = factory.Create("power", 0.5);
    EXPECT_EQ(sqr->ToString(), "x^2");
    EXPECT_EQ(inv->ToString(), "x^-1");
    EXPECT_EQ(sqrt->ToString(), "x^0.5");

    for (const auto& x : TestNumbers) {
        EXPECT_DOUBLE_EQ((*sqr)(x), x * x);
        EXPECT_DOUBLE_EQ(sqr->GetDeriv(x), x + x);

        EXPECT_DOUBLE_EQ((*inv)(x), 1. / x);

        if (x != 0.) {
            EXPECT_DOUBLE_EQ(inv->GetDeriv(x), -1. / (x * x));
        } else {
            EXPECT_FALSE(std::isfinite(inv->GetDeriv(x)));
        }

        if (x >= 0.) {
            EXPECT_DOUBLE_EQ((*sqrt)(x), std::sqrt(x));
            EXPECT_DOUBLE_EQ(sqrt->GetDeriv(x), 0.5 / std::sqrt(x));
        } else {
            EXPECT_TRUE(std::isnan((*sqrt)(x)));
            EXPECT_TRUE(std::isnan(sqrt->GetDeriv(x)));
        }
    }
}

TEST(Basics, Exp) {
    auto testee = factory.Create("exp");
    EXPECT_EQ(testee->ToString(), "e^x");

    for (const auto& x : TestNumbers) {
        EXPECT_EQ((*testee)(x), std::exp(x));
        EXPECT_EQ(testee->GetDeriv(x), std::exp(x));
    }
}

TEST(Basics, Polynomial) {
    auto poly1 = factory.Create("polynomial", {-1, 0, 4});
    auto poly2 = factory.Create("polynomial", {1, -3, 3, -1});
    auto poly3 = factory.Create("polynomial", {0, -0.5, 1.5});
    EXPECT_EQ(poly1->ToString(), "-1 + 4*x^2");
    EXPECT_EQ(poly2->ToString(), "1 - 3*x + 3*x^2 - x^3");
    EXPECT_EQ(poly3->ToString(), "-0.5*x + 1.5*x^2");

    for (const auto& x : TestNumbers) {
        EXPECT_NEAR((*poly1)(x), (2.*x + 1.) * (2.*x - 1.), 1e-6);
        EXPECT_DOUBLE_EQ(poly1->GetDeriv(x), 8. * x);

        EXPECT_NEAR((*poly2)(x), (1. - x) * (1. - x) * (1. - x), 1e-6);
        EXPECT_NEAR(poly2->GetDeriv(x), -3. * (1. - x) * (1. - x), 1e-6);

        EXPECT_NEAR((*poly3)(x), 0.5 * x * (3.*x - 1.), 1e-6);
        EXPECT_DOUBLE_EQ(poly3->GetDeriv(x), 3. * x - 0.5);
    }
}

TEST(Basics, InvalidFunc) {
    EXPECT_EQ(factory.Create("abcd"), nullptr);
    EXPECT_EQ(factory.Create("idnet"), nullptr);
    EXPECT_EQ(factory.Create("cosnt", 42), nullptr);
    EXPECT_EQ(factory.Create("exp", 13), nullptr);
    EXPECT_EQ(factory.Create("polynomial", 5), nullptr);
    EXPECT_EQ(factory.Create("const", {1, 2, 3, 4}), nullptr);
}

/*
 * Tests for binary operations.
*/

TEST(BinOps, Add) {
    auto func1 = factory.Create("ident") + factory.Create("exp");
    auto func2 = factory.Create("const", -1) + factory.Create("power", 3);
    EXPECT_EQ(func1->ToString(), "x + e^x");
    EXPECT_EQ(func2->ToString(), "-1 + x^3");

    for (const auto& x : TestNumbers) {
        EXPECT_EQ((*func1)(x), x + std::exp(x));
        EXPECT_EQ(func1->GetDeriv(x), 1. + std::exp(x));

        EXPECT_NEAR((*func2)(x), x*x*x - 1., 1e-6);
        EXPECT_NEAR(func2->GetDeriv(x), 3.*x*x, 1e-6);
    }
}

TEST(BinOps, Subtract) {
    auto func1 = factory.Create("exp") - factory.Create("ident");
    auto func2
        = factory.Create("power", 2)
        - factory.Create("polynomial", {1, 0, 2, 1});
    EXPECT_EQ(func1->ToString(), "e^x - (x)");
    EXPECT_EQ(func2->ToString(), "x^2 - (1 + 2*x^2 + x^3)");

    for (const auto& x : TestNumbers) {
        EXPECT_EQ((*func1)(x), std::exp(x) - x);
        EXPECT_EQ(func1->GetDeriv(x), std::exp(x) - 1.);

        EXPECT_NEAR((*func2)(x), -x*x*(1. + x) - 1., 1e-6);
        EXPECT_NEAR(func2->GetDeriv(x), -x * (2. + 3.*x), 1e-6);
    }
}

TEST(BinOps, Multiply) {
    auto func1
        = factory.Create("polynomial", {0, 1, 3, 1, 2})
        * factory.Create("polynomial", {3, 0, 7, 15});
    auto func1_actual
        = factory.Create("polynomial", {0, 3, 9, 10, 42, 52, 29, 30});
    auto func2 = factory.Create("power", 2) * factory.Create("exp");
    EXPECT_EQ(
        func1->ToString(),
        "(x + 3*x^2 + x^3 + 2*x^4) * (3 + 7*x^2 + 15*x^3)"
    );
    EXPECT_EQ(func2->ToString(), "(x^2) * (e^x)");

    for (const auto& x : TestNumbers) {
        EXPECT_NEAR((*func1)(x), (*func1_actual)(x), 1e-6);
        EXPECT_NEAR(func1->GetDeriv(x), func1_actual->GetDeriv(x), 1e-6);

        EXPECT_DOUBLE_EQ((*func2)(x), x * x * std::exp(x));
        EXPECT_DOUBLE_EQ(func2->GetDeriv(x), x * (2. + x) * std::exp(x));
    }
}

TEST(BinOps, Divide) {
    auto func1
        = factory.Create("const", -4)
        / factory.Create("polynomial", {1, 2, 1});
    auto func2 = factory.Create("exp") / factory.Create("exp");
    EXPECT_EQ(func1->ToString(), "(-4) / (1 + 2*x + x^2)");
    EXPECT_EQ(func2->ToString(), "(e^x) / (e^x)");

    for (const auto& x : TestNumbers) {
        EXPECT_DOUBLE_EQ((*func2)(x), 1.);
        EXPECT_DOUBLE_EQ(func2->GetDeriv(x), 0.);

        if (x != -1.) {
            EXPECT_NEAR((*func1)(x), -4. * std::pow(x + 1, -2), 1e-6);
            EXPECT_NEAR(func1->GetDeriv(x), 8. * std::pow(x + 1, -3), 1e-6);
        } else {
            EXPECT_FALSE(std::isfinite((*func1)(x)));
            EXPECT_FALSE(std::isfinite(func1->GetDeriv(x)));
        }
    }
}

TEST(BinOps, Compose) {
    auto func1
        = factory.Create("const", 1.)
        / (factory.Create("ident") + factory.Create("exp"));
    auto func2
        = (factory.Create("power", 2)
            - factory.Create("ident")
            + factory.Create("const", 1)
        ) * (factory.Create("exp")
            - factory.Create("polynomial", {0, 0.5, 1}));
    auto func3
        = factory.Create("power", 0.5)
        + factory.Create("const", 4) * factory.Create("exp");
    auto func4
        = (factory.Create("const", 2) * factory.Create("power", 3)
            + factory.Create("const", 6) * factory.Create("power", 2)
            + factory.Create("const", -4) * factory.Create("ident")
            + factory.Create("const", 1))
        - factory.Create("polynomial", {1, -4, 6, 2});
    EXPECT_EQ(func1->ToString(), "(1) / (x + e^x)");
    EXPECT_EQ(func2->ToString(), "(x^2 - (x) + 1) * (e^x - (0.5*x + x^2))");
    EXPECT_EQ(func3->ToString(), "x^0.5 + (4) * (e^x)");
    EXPECT_EQ(
        func4->ToString(),
        "(2) * (x^3) + (6) * (x^2) + (-4) * (x) + 1"
        " - (1 - 4*x + 6*x^2 + 2*x^3)"
    );

    for (const auto& x : TestNumbers) {
        EXPECT_DOUBLE_EQ((*func1)(x), 1. / (x + std::exp(x)));
        EXPECT_DOUBLE_EQ(
            func1->GetDeriv(x),
            -(1. + std::exp(x)) / std::pow(x + std::exp(x), 2)
        );

        EXPECT_NEAR(
            (*func2)(x), (x*x - x + 1.)*(std::exp(x) - 0.5*x - x*x), 1e-6
        );
        EXPECT_NEAR(
            func2->GetDeriv(x),
            -0.5 + x*(-4.*x*x - 1 + 1.5*x + (1 + x)*std::exp(x)),
            1e-6
        );

        if (x >= 0.) {
            EXPECT_DOUBLE_EQ((*func3)(x), std::sqrt(x) + 4.*std::exp(x));
            EXPECT_DOUBLE_EQ(
                func3->GetDeriv(x),
                0.5 / std::sqrt(x) + 4.*std::exp(x)
            );
        } else {
            EXPECT_TRUE(std::isnan((*func3)(x)));
            EXPECT_TRUE(std::isnan(func3->GetDeriv(x)));
        }

        EXPECT_NEAR((*func4)(x), 0., 1e-6);
        EXPECT_NEAR(func4->GetDeriv(x), 0., 1e-6);
    }
}

TEST(BinOps, LogicError) {
    EXPECT_THROW(
        factory.Create("") + factory.Create("ident"),
        std::logic_error
    );
    EXPECT_THROW(
        factory.Create("power", 2) - factory.Create("abcd"),
        std::logic_error
    );
    EXPECT_THROW(
        factory.Create("exp") * factory.Create("power", std::vector{3.}),
        std::logic_error
    );
    EXPECT_THROW(
        factory.Create("polynomial", 2) / factory.Create("exp"),
        std::logic_error
    );
}

int main(int argc, char *argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
