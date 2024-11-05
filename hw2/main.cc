#include "scheduling.h"

#include <chrono>
#include <iostream>

void print_sched(const std::vector<std::vector<unsigned>> &vec) {
    for (unsigned proc = 0; proc < vec.size(); ++proc) {
        std::cout << "proc" << proc << ": ";
        for (const auto &work: vec[proc]) {
            std::cout << work << ' ';
        }
        std::cout << "\b\n";
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "usage: " << argv[0] << " N_PROC INPUT_PATH" << std::endl;
        return 1;
    }

    unsigned n_proc = std::strtoul(argv[1], nullptr, 10);
    auto mutation = std::make_shared<Scheduling::Mutation>();
    auto sol = std::make_shared<Scheduling::Solution>(argv[2]);
    std::chrono::duration<double> time;

    std::cout << "Boltzmann law: ";
    hw2::Annealing algo_boltzmann(
        n_proc, mutation, std::make_shared<hw2::BasicCD::Boltzmann>(1000.)
    );

    auto time_start = std::chrono::high_resolution_clock::now();
    auto best = std::dynamic_pointer_cast<Scheduling::Solution>(
        algo_boltzmann.run(sol)
    );
    auto time_stop = std::chrono::high_resolution_clock::now();
    time = time_stop - time_start;

    std::cout << "criterion = " << best->criterion() << ' ';
    std::cout << "time = " << time.count() << std::endl;

    std::cout << "Cauchy law: ";
    hw2::Annealing algo_cauchy(
        n_proc, mutation, std::make_shared<hw2::BasicCD::Cauchy>(1000.)
    );

    time_start = std::chrono::high_resolution_clock::now();
    best = std::dynamic_pointer_cast<Scheduling::Solution>(
        algo_cauchy.run(sol)
    );
    time_stop = std::chrono::high_resolution_clock::now();
    time = time_stop - time_start;

    std::cout << "criterion = " << best->criterion() << ' ';
    std::cout << "time = " << time.count() << std::endl;

    std::cout << "Log Cauchy law: ";
    hw2::Annealing algo_logcauchy(
        n_proc, mutation, std::make_shared<hw2::BasicCD::LogCauchy>(1000.)
    );

    time_start = std::chrono::high_resolution_clock::now();
    best = std::dynamic_pointer_cast<Scheduling::Solution>(
        algo_logcauchy.run(sol)
    );
    time_stop = std::chrono::high_resolution_clock::now();
    time = time_stop - time_start;

    std::cout << "criterion = " << best->criterion() << ' ';
    std::cout << "time = " << time.count() << std::endl;
    return 0;
}
