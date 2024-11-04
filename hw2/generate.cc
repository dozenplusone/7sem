#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>

namespace hw2 {
    class DataGenerator;
}

class hw2::DataGenerator {
    std::mt19937 rng;
    std::uniform_int_distribution<unsigned> dist_work;
    unsigned n_proc;
    unsigned n_works;

public:
    DataGenerator(unsigned, unsigned, unsigned, unsigned);

    void generate(const std::filesystem::path&);
};

hw2::DataGenerator::DataGenerator(
    unsigned procs, unsigned works, unsigned time_min, unsigned time_max
)
    : rng(std::random_device{}())
    , dist_work(time_min, time_max)
    , n_proc(procs)
    , n_works(works)
{}

void hw2::DataGenerator::generate(const std::filesystem::path &path) {
    std::ofstream file(path);
    file << n_proc;

    for (unsigned i = 0; i < n_works; ++i) {
        file << ',' << dist_work(rng);
    }

    file << '\n';
    file.close();
}

int main(int argc, char *argv[]) {
    if (argc < 6) {
        std::cerr << "usage: " << argv[0]
                  << " N_PROC N_WORKS TIME_MIN TIME_MAX PATH" << std::endl;
        return 1;
    }
    hw2::DataGenerator(
        std::strtoul(argv[1], nullptr, 10),
        std::strtoul(argv[2], nullptr, 10),
        std::strtoul(argv[3], nullptr, 10),
        std::strtoul(argv[4], nullptr, 10)
    ).generate(argv[5]);
    return 0;
}
