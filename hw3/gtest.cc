#include "libfunc.h"

#include "gtest/gtest.h"

#include <unordered_set>

static TFunctionFactory factory;

static std::unordered_set<double> TestNumbers{
    42, -1, 0, 2.7182, -13, 3.1416, 100, 0.5, 1.618
};

/*
 * Tests for basic functions.
*/

TEST(Basics, Ident) {
    auto testee = factory.Create("ident");
    EXPECT_EQ(testee->ToString(), "x");

    for (const auto& x : TestNumbers) {
        EXPECT_EQ((*testee)(x), x);
    }
}

TEST(Basics, Const) {
    double constant = *TestNumbers.begin();
    std::string const_repr = (std::stringstream{} << constant).str();

    auto testee = factory.Create("const", constant);
    EXPECT_EQ(testee->ToString(), const_repr);

    for (const auto& x : TestNumbers) {
        EXPECT_EQ((*testee)(x), constant);
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
        EXPECT_DOUBLE_EQ((*inv)(x), 1. / x);

        if (x >= 0.) {
            EXPECT_DOUBLE_EQ((*sqrt)(x), std::sqrt(x));
        } else {
            EXPECT_TRUE(std::isnan((*sqrt)(x)));
        }
    }
}

TEST(Basics, Exp) {
    auto testee = factory.Create("exp");
    EXPECT_EQ(testee->ToString(), "e^x");

    for (const auto& x : TestNumbers) {
        EXPECT_EQ((*testee)(x), std::exp(x));
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
        EXPECT_NEAR((*poly2)(x), (1. - x) * (1. - x) * (1. - x), 1e-6);
        EXPECT_NEAR((*poly3)(x), 0.5 * x * (3.*x - 1.), 1e-6);
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

int main(int argc, char *argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
